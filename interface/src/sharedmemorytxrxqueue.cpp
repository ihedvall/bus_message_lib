/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include <cstdint>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>

#include <boost/interprocess/sync/scoped_lock.hpp>

#include "sharedmemorytxrxqueue.h"
#include "sharedmemoryserver.h"
#include "bus/buslogstream.h"
#include "bus/littlebuffer.h"

using namespace std::chrono_literals;
using namespace boost::interprocess;

namespace bus {

SharedMemoryTxRxQueue::SharedMemoryTxRxQueue(std::string shared_memory_name,
  bool tx_queue, bool publisher )
  : tx_queue_(tx_queue),
    publisher_(publisher),
    shared_memory_name_(std::move(shared_memory_name)) {
}

SharedMemoryTxRxQueue::~SharedMemoryTxRxQueue() {
  SharedMemoryTxRxQueue::Stop();
}

void SharedMemoryTxRxQueue::Start() {
  Stop();
  state_ = SharedMemoryState::WaitOnSharedMemory;
  operable_ = true;
  if (shared_memory_name_.empty()) {
    BUS_ERROR() << "the share memory has no name. Invalid use of function.";
    return;
  }

  IBusMessageQueue::Start();
  stop_thread_ = false;
  if (publisher_) {
    thread_ = std::thread(&SharedMemoryTxRxQueue::PublisherThread, this);
  } else {
    thread_ = std::thread(&SharedMemoryTxRxQueue::SubscriberThread, this);
  }
}

void SharedMemoryTxRxQueue::Stop() {
  stop_thread_ = true;
  if (thread_.joinable()) {
    thread_.join();
  }
  shm_ = nullptr;
  region_.reset();
  shared_memory_.reset();
  state_ = SharedMemoryState::WaitOnSharedMemory;
  operable_ = false;

  IBusMessageQueue::Stop();
  stop_thread_ = false;
}

void SharedMemoryTxRxQueue::PublisherThread() {
  while (!stop_thread_ ) {
    switch (state_) {
      case SharedMemoryState::HandleMessages:
        if (shm_ == nullptr) {
          state_ = SharedMemoryState::WaitOnSharedMemory;
        }
        break;

      case SharedMemoryState::WaitOnSharedMemory:
      default:
        ConnectToSharedMemory();
        break;
    }
    if (state_ == SharedMemoryState::WaitOnSharedMemory) {
      std::this_thread::sleep_for(1000ms);
      continue;
    }


    if (const bool buffer_full = tx_queue_ ? shm_->tx_full : shm_->rx_full;
        buffer_full) {
      std::this_thread::sleep_for(10ms);
      continue;
    }
    EmptyWait(10ms); // Signalled if a message been added
    auto msg = Pop();
    if (!msg) {
      continue;
    }
    bool message_sent;
    {
      scoped_lock lock(shm_->memory_mutex);
      message_sent = PublisherPoll(*shm_, *msg);
    }

    if (!message_sent) {
      // The buffer should be flagged full
      PushFront(msg);
    }
  }
}

void SharedMemoryTxRxQueue::SubscriberThread() {
  while (!stop_thread_ ) {
    switch (state_) {
      case SharedMemoryState::HandleMessages:
        if (shm_ == nullptr) {
          state_ = SharedMemoryState::WaitOnSharedMemory;
        }
        break;

      case SharedMemoryState::WaitOnSharedMemory:
      default:
        ConnectToSharedMemory();
        break;
    }
    if (state_ == SharedMemoryState::WaitOnSharedMemory) {
      std::this_thread::sleep_for(1000ms);
      continue;
    }

    if (channel_ == 0) {
      GetChannel();
    }
    if (channel_ == 0) {
      std::this_thread::sleep_for(1000ms);
      continue;
    }

    std::vector<uint8_t> message_buffer;
    bool more = true;
    while ( more && !stop_thread_) {
      {
        scoped_lock lock(shm_->memory_mutex);
        more = SubscriberPoll(*shm_, message_buffer);
      }
      if (more && !message_buffer.empty()) {
        Push(message_buffer);
      }
    }
      // Trig a reset of channels as the in/out indexes should point on the
      // same indexies.
    auto& condition = tx_queue_ ? shm_->tx_full_condition : shm_->rx_full_condition;
    condition.notify_all();
    std::this_thread::sleep_for(10ms);
  }
}

void SharedMemoryTxRxQueue::GetChannel() {
  if (shm_ == nullptr) {
    return;
  }
  scoped_lock lock(shm_->memory_mutex);
  auto& channels = tx_queue_ ? shm_->tx_channels : shm_->rx_channels;
  for (size_t index = 1; index < channels.size(); ++index) {
    if (channels[index].used) {
      continue;
    }
    channel_ = index;
    channels[index].used = true;
    break;
  }
}

bool SharedMemoryTxRxQueue::PublisherPoll(SharedServerObjects& shm,
  const IBusMessage& message) const {
  auto& channel = tx_queue_ ? shm.tx_channels[0] : shm.rx_channels[0];
  const uint32_t message_size = message.Size();
  auto& buffer = tx_queue_ ? shm.tx_buffer : shm.rx_buffer;
  auto bytes_left = static_cast<int64_t>(buffer.size());
  bytes_left -= channel.queue_index;
  bytes_left -= message_size;
  bytes_left -= 4;

  if (bytes_left < 0 ) {
    if (tx_queue_) {
      shm.tx_full = true;
    } else {
      shm.rx_full = true;
    }
    return false;
  }

  std::vector<uint8_t> msg_buffer;
  message.ToRaw(msg_buffer);
  if (msg_buffer.size() != message_size) {
    BUS_ERROR() << "Mismatching message sizes ("
      << msg_buffer.size() << "/" << message_size
      << ". Internal error";
    return false;
  }

  const LittleBuffer length(message_size);
  std::copy_n(length.cbegin(), length.size(),
  buffer.begin() + channel.queue_index );
  channel.queue_index += length.size();

  std::copy_n(msg_buffer.begin(), msg_buffer.size(),
  buffer.begin() + channel.queue_index );
  channel.queue_index += msg_buffer.size();
  return true;
}

bool SharedMemoryTxRxQueue::SubscriberPoll(SharedServerObjects& shm,
    std::vector<uint8_t>& msg_buffer ) {
  msg_buffer.clear();
  if (channel_ == 0) {
    BUS_ERROR() << "Invalid subscriber channel index. Index: " <<
      static_cast<int>(channel_);
    return false;
  }

  const auto& buffer = tx_queue_ ? shm.tx_buffer : shm.rx_buffer;
  auto& out_channel = tx_queue_ ?
    shm.tx_channels[channel_] : shm.rx_channels[channel_];
  const auto& in_channel = tx_queue_ ? shm.tx_channels[0] : shm.rx_channels[0];

  if (!out_channel.used) {
    // Probably reconnected to the shared memory.
    BUS_ERROR() << "Channel suddenly unused. Channel: "
          << static_cast<int>(channel_);
    channel_ = 0; // Trigger a new channel
    operable_ = false;
    return false;
  }

  if (in_channel.queue_index < out_channel.queue_index) {
    BUS_ERROR() << "Invalid channel indexes. Channel: "
      << static_cast<int>(channel_)
      << ", Index: " << in_channel.queue_index
      << "/" << out_channel.queue_index;
    out_channel.queue_index = in_channel.queue_index;
    return false;
  }

  if (out_channel.queue_index == in_channel.queue_index) {
    // No message to send
    return false;
  }

  if (out_channel.queue_index + 4 > buffer.size()) {
    BUS_ERROR() << "Length out-of-boound. Index: "
      << static_cast<int>(out_channel.queue_index)
      << "/" << buffer.size();
    out_channel.queue_index = in_channel.queue_index;
    return false;
  }

  LittleBuffer<uint32_t> length(buffer.data(),out_channel.queue_index);
  out_channel.queue_index += length.size();
  const uint32_t message_length = length.value();

  if (out_channel.queue_index + message_length > buffer.size()) {
    BUS_ERROR() << "Data out-of-boound. Index: "
        << static_cast<int>(out_channel.queue_index)
        << ", Length: " << message_length
        << ", Size: " << buffer.size();
    out_channel.queue_index = in_channel.queue_index;
    return false;
  }

  try {
    msg_buffer.resize(message_length);
    std::copy_n(buffer.cbegin() + out_channel.queue_index,
      message_length, msg_buffer.begin());
    out_channel.queue_index += message_length;
  } catch (const std::exception& err) {
    BUS_ERROR() << "Message copy failure. Error: " << err.what();
    out_channel.queue_index = in_channel.queue_index;
    return false;
  }

  return true;
}

void SharedMemoryTxRxQueue::ConnectToSharedMemory() {
  try {
    shm_ = nullptr;
    region_.reset();
    shared_memory_.reset();

    shared_memory_ = std::make_unique<shared_memory_object>(
      open_only, shared_memory_name_.c_str(), read_write);
    if (!shared_memory_) {
      throw std::runtime_error("Shared memory allocation error (null)");
    }
    region_ = std::make_unique<mapped_region>(*shared_memory_, read_write);
    if (!shared_memory_) {
      throw std::runtime_error("Mapped region allocation error (null)");
    }
    if (region_->get_address() == nullptr) {
      throw std::runtime_error("No shared memory found");
    }
    shm_ = static_cast<SharedServerObjects *>(region_->get_address());
    if (!shm_->initialized) {
      std::ostringstream err;
      err << "Shared memory not initialized. Name: " << shared_memory_name_;
      throw std::runtime_error(err.str());
    }
    if (!operable_) {
      // The connection is back again.
      BUS_INFO() << "Shared memory connected. Name: " << shared_memory_name_;
      operable_ = true;
    }
    state_ = SharedMemoryState::HandleMessages;
  } catch (const std::exception& err) {
    if (operable_) {
      // The connection is back again.
      BUS_ERROR() << "Cannot connect to shared memory. Name: "
        << shared_memory_name_ << ", Error: " << err.what();
      operable_ = false;
      state_ = SharedMemoryState::WaitOnSharedMemory;
    }
  }
}

} // bus