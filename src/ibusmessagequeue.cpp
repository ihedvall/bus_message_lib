/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include <thread>

#include "bus/ibusmessagequeue.h"
#include "bus/buslogstream.h"

#include "littlebuffer.h"

namespace bus {

IBusMessageQueue::~IBusMessageQueue() {
  std::lock_guard<std::mutex> queue_lock(queue_mutex_);
  while (!queue_.empty() ) {
    queue_.pop();
  }
}

void IBusMessageQueue::Push(const std::shared_ptr<IBusMessage>& message) {
  {
    std::lock_guard<std::mutex> queue_lock(queue_mutex_);
    queue_.push(message);
  }
  std::this_thread::yield();
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
  std::lock_guard<std::mutex> queue_lock(queue_mutex_);
  if (queue_.empty()) {
    return {};
  }

  auto message_ptr = std::move(queue_.front());
  queue_.pop();
  return message_ptr;
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
  std::lock_guard<std::mutex> queue_lock(queue_mutex_);
  return queue_.size();
}

bool IBusMessageQueue::Empty() const {
  std::lock_guard<std::mutex> queue_lock(queue_mutex_);
  return queue_.empty();
}

void IBusMessageQueue::Start() {}
void IBusMessageQueue::Stop() {}

void IBusMessageQueue::Clear() {
  while (!Empty()) {
    Pop();
  }
}

} // bus