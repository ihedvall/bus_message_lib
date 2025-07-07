/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include "simulatebroker.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <thread>

#include "../include/bus/littlebuffer.h"
#include "bus/buslogstream.h"
#include "simulatequeue.h"

using namespace std::chrono_literals;

namespace bus {

SimulateBroker::SimulateBroker()
  : IBusMessageBroker() {

}

SimulateBroker::~SimulateBroker() {
  SimulateBroker::Stop();
}

std::shared_ptr<IBusMessageQueue> SimulateBroker::CreatePublisher() {
  auto queue = std::make_shared<SimulateQueue>(*this, true);
  publishers_.emplace_back(queue);
  return queue;
}

std::shared_ptr<IBusMessageQueue> SimulateBroker::CreateSubscriber() {
  auto queue = std::make_shared<SimulateQueue>(*this, false);
  subscribers_.emplace_back(queue);
  // Need to allocate a subsciber index
  GetChannel(*queue);
  return queue;
}

void SimulateBroker::Start() {
  Stop(); // Stop on-going threads

  // Reset the channels in/out indexes.
  std::ranges::for_each(channels_, [] (Channel& channel) ->void {
    channel.used = false;
    channel.queue_index = 0;
  });

  // Set up the simulated shared memory.
  if (MemorySize() < 1'000) {
    BUS_INFO() << "Very small memory allocated. Memory: " << MemorySize();
    MemorySize( 0x10000);
  }
  try {
    buffer_.resize(MemorySize());
  } catch (const std::exception& err) {
    BUS_ERROR() << "Allocation error. Error: " << err.what();
    return;
  }

  // Allocate the first array item for the publishers.
  // The other array items are used for the subscribers.
  // The subscriber tries to allocate a free channal
  // array at startup.
  channels_[0].used = true;

  stop_master_task_ = false;
  master_task_ = std::thread(&SimulateBroker::BrokerMasterTask, this);
  std::this_thread::yield();

  // Start all publishers
  std::ranges::for_each(publishers_, [] (auto& publisher) -> void {
    if (publisher) {
      publisher->Start();
      std::this_thread::yield();
    }
  });

  // Start all subscribers
  std::ranges::for_each(subscribers_, [] (auto& subscriber) -> void {
    if (subscriber) {
      subscriber->Start();
      std::this_thread::yield();
    }
  });
  connected_ = true;
}

void SimulateBroker::Stop() {
  connected_ = false;
  // Stop all publishers
  std::ranges::for_each(publishers_, [] (auto& publisher) -> void {
    if (publisher) {
      publisher->Stop();
    }
  });

  // Stop all subscribers
  std::ranges::for_each(subscribers_, [] (auto& subscriber) -> void {
    if (subscriber) {
      subscriber->Stop();
    }
  });

  stop_master_task_ = true;
  buffer_full_condition_.notify_all(); // Speed up the stop

  if (master_task_.joinable()) {
    master_task_.join();
  }
}

void SimulateBroker::PublisherPoll(SimulateQueue& queue) {
  if (buffer_full_) {
    return;
  }

  std::lock_guard lock_memory(buffer_mutex_);

  Channel& channel = channels_[0];
  const uint32_t message_size = queue.MessageSize();
  auto bytes_left = static_cast<int64_t>(buffer_.size());
  bytes_left -= channel.queue_index;
  bytes_left -= message_size;
  bytes_left -= 4;

  if (bytes_left < 0 ) {
    buffer_full_ = true;
    timeout_ = 0;
    buffer_full_condition_.notify_all();
    return;
  }

  auto msg = queue.Pop();
  if (!msg) {
    BUS_ERROR() << "Poped an empty message. Internal error";
    return;
  }

  std::vector<uint8_t> msg_buffer;
  msg->ToRaw(msg_buffer);
  if (msg_buffer.size() != message_size) {
    BUS_ERROR() << "Mismatching message sizes ("
      << msg_buffer.size() << "/" << message_size
      << ". Internal error";
    return;
  }

  const LittleBuffer length(message_size);
    std::copy_n(length.cbegin(), length.size(),
    buffer_.begin() + channel.queue_index );
  channel.queue_index += length.size();

  std::copy_n(msg_buffer.begin(), msg_buffer.size(),
    buffer_.begin() + channel.queue_index );
  channel.queue_index += msg_buffer.size();
}

bool SimulateBroker::SubscriberPoll(SimulateQueue& queue) {
  uint8_t out_index = queue.Channel();


  if (out_index == 0) {
    BUS_ERROR() << "Invalid subscriber channel index. Index: " <<
      static_cast<int>(out_index);
    return false;
  }
  std::vector<uint8_t> msg_buffer;

  {
    std::lock_guard lock_memory(buffer_mutex_);
    auto& out_channel = channels_[out_index];
    const auto& in_channel = channels_[0];
    if (!out_channel.used ||
          in_channel.queue_index < out_channel.queue_index) {
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

    if (out_channel.queue_index + 4 > buffer_.size()) {
      BUS_ERROR() << "Length out-of-boound. Index: "
        << static_cast<int>(out_channel.queue_index)
        << "/" << buffer_.size();
      out_channel.queue_index = buffer_.size();
      return false;
    }

    LittleBuffer<uint32_t> length(buffer_,out_channel.queue_index);
    out_channel.queue_index += length.size();
    const uint32_t message_length = length.value();

    if (out_channel.queue_index + message_length > buffer_.size()) {
      BUS_ERROR() << "Data out-of-boound. Index: "
          << static_cast<int>(out_channel.queue_index)
          << ", Length: " << message_length
          << ", Size: " << buffer_.size();
      out_channel.queue_index = in_channel.queue_index;
      return false;
    }

    try {
      msg_buffer.resize(message_length);
      std::copy_n(buffer_.cbegin() + out_channel.queue_index,
        message_length, msg_buffer.begin());
      out_channel.queue_index += message_length;

    } catch (const std::exception& err) {
      BUS_ERROR() << "Message copy failure. Error: " << err.what();
      out_channel.queue_index = in_channel.queue_index;
      return false;
    }
  }

  if (!msg_buffer.empty()) {
    queue.Push(msg_buffer);
  }
  return true;
}


void SimulateBroker::GetChannel(SimulateQueue& queue) {
  for (size_t index = 1; index < channels_.size(); ++index) {
    if (channels_[index].used) {
      continue;
    }
    queue.Channel(index);
    channels_[index].used = true;
    return;
  }
}

void SimulateBroker::BrokerMasterTask() {
  while (!stop_master_task_) {
    std::unique_lock<std::mutex> lock(event_mutex_);
    buffer_full_condition_.wait_for(lock, 1000ms, [&] () -> bool {
      return stop_master_task_.load() || buffer_full_.load();
    } );
    if (!stop_master_task_) {
      HandleBufferFull();
    }
  }
}

void SimulateBroker::HandleBufferFull() {
  std::lock_guard lock(buffer_mutex_);

  const uint32_t ref_index  = channels_[0].queue_index;
  bool all_index_ok = std::ranges::all_of( channels_,
    [&] (const Channel& channel)-> bool {
      if (!channel.used) {
        return true;
      }
      return channel.queue_index == ref_index;
    });;
  if (all_index_ok) {
    ResetChannels();
  } else if (buffer_full_) {
    std::time_t now = std::time(nullptr);
    if (timeout_ == 0) {
      timeout_ = now + 10;;
    } else if (now > timeout_) {
      BUS_ERROR() << "Buffer full (10s) timeout occurred. Resetting";
      ResetChannels();
    }
  }
}

void SimulateBroker::ResetChannels() {
  std::ranges::for_each(channels_,
  [] (Channel& channel) -> void {
    channel.queue_index = 0;
  });
  buffer_full_= false;
  timeout_ = 0;
}

} // bus