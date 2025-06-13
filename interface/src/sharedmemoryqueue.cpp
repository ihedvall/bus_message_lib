/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include <chrono>
#include <sstream>

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "sharedmemoryqueue.h"
#include "sharedmemorybroker.h"
#include "bus/buslogstream.h"
#include "bus/littlebuffer.h"
using namespace std::chrono_literals;
using namespace boost::interprocess;

namespace bus {

SharedMemoryQueue::SharedMemoryQueue(const std::string& shared_memory_name,
  bool publisher )
  : shared_memory_name_(shared_memory_name),
    publisher_(publisher) {
}

SharedMemoryQueue::~SharedMemoryQueue() {
  SharedMemoryQueue::Stop();
}

void SharedMemoryQueue::Start() {
  Stop();
  stop_thread_ = false;
  if (publisher_) {
    operable_ = false;
    thread_ = std::thread(&SharedMemoryQueue::PublisherTask, this);
  } else {
    GetChannel();
    operable_ = true;
    thread_ = std::thread(&SharedMemoryQueue::SubscriberTask, this);
  }
}

void SharedMemoryQueue::Stop() {
  stop_thread_ = true;
  if (thread_.joinable()) {
    thread_.join();
  }
  stop_thread_ = false;
}

void SharedMemoryQueue::PublisherTask() {
  while (!stop_thread_ ) {
    if (!Empty()) {
      // Connect to shared memory
      // Transfer messages
      // Disconnect
      try {
        shared_memory_object shared_mem(open_only, shared_memory_name_.c_str(),
          read_write);
        mapped_region region(shared_mem, read_write);
        if (region.get_address() == nullptr) {
          throw std::runtime_error("No shared memory found");
        }
        auto *shm = static_cast<SharedMemoryObjects *>(region.get_address());
        if (!shm->initialized) {
          std::ostringstream err;
          err << "Shared memory not initialized. Name: " << shared_memory_name_;
          throw std::runtime_error(err.str());
        }
        if (!operable_) {
          // The connection is back again.
          BUS_INFO() << "The shared memory is connected. Name: " << shared_memory_name_;
          operable_ = true;
          // Create a delay so subscribers can allocate its channels.
          std::this_thread::sleep_for(1000ms);
        }
        while (!stop_thread_ && !Empty() && !shm->buffer_full) {
          auto msg = Pop();
          if (!msg) {
            continue;
          }
          bool message_sent;
          {
            scoped_lock lock(shm->memory_mutex);
            message_sent = PublisherPoll(*shm, *msg);
          }
          if (!message_sent) {
            PushFront(msg);
          }
        }
        if (shm->buffer_full) {
          shm->buffer_full_condition.notify_all();
        }
      } catch (const std::exception &err) {
        if (operable_) {
          BUS_ERROR() << "Shared memory failure. Error: " << err.what();
          operable_ = false;
        }
      }
    }

    std::this_thread::sleep_for(10ms);
  }
}

void SharedMemoryQueue::SubscriberTask() {
  while (!stop_thread_ ) {
    if (channel_ == 0) {
      GetChannel();
      std::this_thread::sleep_for(1000ms);
      continue;
    }

    try {
      shared_memory_object shared_mem(open_only, shared_memory_name_.c_str(),
  read_write);
      mapped_region region(shared_mem, read_write);
      if (region.get_address() == nullptr) {
        throw std::runtime_error("No shared memory found");
      }
      auto *shm = static_cast<SharedMemoryObjects *>(region.get_address());
      if (!shm->initialized) {
        std::ostringstream err;
        err << "Shared memory not initialized. Name: " << shared_memory_name_;
        throw std::runtime_error(err.str());
      }
      size_t no_activity = 0;
      while(!stop_thread_ && no_activity < 100) {
        bool more = true;
        std::vector<uint8_t> message_buffer;
        while (more && !stop_thread_) {
          {
            scoped_lock lock(shm->memory_mutex);
            more = SubscriberPoll(*shm, message_buffer);
          }
          if (more && !message_buffer.empty()) {
            Push(message_buffer);
            no_activity = 0;
          }
        }
        ++no_activity;        ;
        std::this_thread::sleep_for(10ms);
      }
    } catch (const std::exception &err) {
      if (operable_) {
        BUS_ERROR() << "Shared memory failure. Error: " << err.what();
      }
      operable_ = false;
    }
    std::this_thread::sleep_for(operable_ ? 10ms : 1000ms);
  }
}

void SharedMemoryQueue::GetChannel() {
  try {
    shared_memory_object shared_mem(open_only, shared_memory_name_.c_str(),
      read_write);
    mapped_region region(shared_mem, read_write);
    if (region.get_address() == nullptr) {
      std::ostringstream err;
      err << "No shared memory found. Name: " << shared_memory_name_;
      throw std::runtime_error(err.str());
    }
    auto *shm = static_cast<SharedMemoryObjects *>(region.get_address());
    if (!shm->initialized) {
      std::ostringstream err;
      err << "Shared memory not initialized. Name: " << shared_memory_name_;
      throw std::runtime_error(err.str());
    }
    if (!operable_) {
      BUS_INFO() << "Shared memory connected. Name: " << shared_memory_name_;
      operable_ = true;
    }

    scoped_lock lock(shm->memory_mutex);
    for (size_t index = 1; index < shm->channels.size(); ++index) {
      if (shm->channels[index].used) {
        continue;
      }
      channel_ = index;
      shm->channels[index].used = true;
      break;
    }
  } catch (const std::exception &err) {
    if (operable_) {
      BUS_ERROR() << "Shared memory error. Name: " << shared_memory_name_
        << ", Error: " << err.what();
      operable_ = false;
    }
  }
}

bool SharedMemoryQueue::PublisherPoll(SharedMemoryObjects& shm,
  const IBusMessage& message) {
  auto& channel = shm.channels[0];
  const uint32_t message_size = message.Size();

  auto bytes_left = static_cast<int64_t>(shm.buffer.size());
  bytes_left -= channel.queue_index;
  bytes_left -= message_size;
  bytes_left -= 4;

  if (bytes_left < 0 ) {
    shm.buffer_full = true;
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
  shm.buffer.begin() + channel.queue_index );
  channel.queue_index += length.size();

  std::copy_n(msg_buffer.begin(), msg_buffer.size(),
  shm.buffer.begin() + channel.queue_index );
  channel.queue_index += msg_buffer.size();
  return true;
}

bool SharedMemoryQueue::SubscriberPoll(SharedMemoryObjects& shm,
    std::vector<uint8_t>& msg_buffer ) const {
  msg_buffer.clear();
  uint8_t out_index = channel_;
  if (out_index == 0) {
    BUS_ERROR() << "Invalid subscriber channel index. Index: " <<
      static_cast<int>(out_index);
    return false;
  }

  auto& out_channel = shm.channels[out_index];
  const auto& in_channel = shm.channels[0];
  if (!out_channel.used) {
    // Probably reconnected to the shared memory.
    out_index = 0; // Trigger a new channel
    operable_ = false;
    BUS_ERROR() << "Channel suddennly unused. Channel: "
      << static_cast<int>(out_index);
    return false;
  }

  if (in_channel.queue_index < out_channel.queue_index) {
    BUS_ERROR() << "Invalid channel indexes. Channel: "
      << static_cast<int>(out_index)
      << ", Index: " << in_channel.queue_index
      << "/" << out_channel.queue_index;
    out_channel.queue_index = in_channel.queue_index;
    return false;
  }

  if (out_channel.queue_index == in_channel.queue_index) {
    return false;
  }

  if (out_channel.queue_index + 4 > shm.buffer.size()) {
    BUS_ERROR() << "Length out-of-boound. Index: "
      << static_cast<int>(out_channel.queue_index)
      << "/" << shm.buffer.size();
    out_channel.queue_index = in_channel.queue_index;
    return false;
  }

  LittleBuffer<uint32_t> length(shm.buffer.data(),out_channel.queue_index);
  out_channel.queue_index += length.size();
  const uint32_t message_length = length.value();

  if (out_channel.queue_index + message_length > shm.buffer.size()) {
    BUS_ERROR() << "Data out-of-boound. Index: "
        << static_cast<int>(out_channel.queue_index)
        << ", Length: " << message_length
        << ", Size: " << shm.buffer.size();
    out_channel.queue_index = in_channel.queue_index;
    return false;
  }

  try {
    msg_buffer.resize(message_length);
    std::copy_n(shm.buffer.cbegin() + out_channel.queue_index,
      message_length, msg_buffer.begin());
    out_channel.queue_index += message_length;


  } catch (const std::exception& err) {
    BUS_ERROR() << "Message copy failure. Error: " << err.what();
    out_channel.queue_index = in_channel.queue_index;
    return false;
  }

  return true;
}

} // bus