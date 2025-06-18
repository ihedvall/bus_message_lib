/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include <thread>

#include "bus/ibusmessagequeue.h"

#include <queue>

#include "bus/buslogstream.h"

#include "bus/littlebuffer.h"

namespace bus {

IBusMessageQueue::~IBusMessageQueue() {
  std::lock_guard<std::mutex> queue_lock(queue_mutex_);
  queue_.clear();
}

void IBusMessageQueue::Push(const std::shared_ptr<IBusMessage>& message) {
  {
    std::lock_guard<std::mutex> queue_lock(queue_mutex_);
    queue_.emplace_back(message);
    queue_size_ = queue_.size();
  }
  queue_not_empty_.notify_one();
}
void IBusMessageQueue::PushFront(const std::shared_ptr<IBusMessage>& message) {
  {
    std::lock_guard<std::mutex> queue_lock(queue_mutex_);
    queue_.emplace_front(message);
    queue_size_ = queue_.size();
  }
  queue_not_empty_.notify_one();
}

void IBusMessageQueue::Push(const std::vector<uint8_t>& message_buffer) {

  // Convert to byte array to message
  IBusMessage header;
  header.FromRaw(message_buffer);
  auto message = IBusMessage::Create(header.Type());
  if (!message) {
    BUS_ERROR() << "Unknown IBusMessage header type "
        << static_cast<int>(header.Type());
    return;
  }
  message->FromRaw(message_buffer);
  Push(message);
}

std::shared_ptr<IBusMessage> IBusMessageQueue::Pop() {
  std::shared_ptr<IBusMessage> message;
  {
    std::lock_guard<std::mutex> queue_lock(queue_mutex_);
    if (!queue_.empty()) {
      message = std::move(queue_.front());
      queue_.pop_front();
      queue_size_ = queue_.size();
    } else {
      queue_size_ = 0;
    }

  }
  return message;
}

size_t IBusMessageQueue::MessageSize() const {
  std::lock_guard<std::mutex> queue_lock(queue_mutex_);
  if (queue_.empty()) {
    return 0;
  }
  const auto& msg = queue_.front();
  return msg ? msg->Size() : 0;
}

size_t IBusMessageQueue::Size() const {
  return queue_size_;
}

bool IBusMessageQueue::Empty() const {
  return queue_size_ == 0;
}

void IBusMessageQueue::Start() {
  std::lock_guard<std::mutex> queue_lock(queue_mutex_);
  queue_.clear();
  queue_size_ = 0;
}

void IBusMessageQueue::Stop() {
  queue_not_empty_.notify_all(); // Just releases any waiting call
}

void IBusMessageQueue::Clear() {
  std::lock_guard<std::mutex> queue_lock(queue_mutex_);
  queue_.clear();
  queue_size_ = 0;
}

} // bus