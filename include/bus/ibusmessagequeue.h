/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <cstdint>
#include <deque>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>

#include "bus/ibusmessage.h"

namespace bus {

class IBusMessageQueue {
public:
  IBusMessageQueue() = default;
  virtual ~IBusMessageQueue();

  void Push(const std::shared_ptr<IBusMessage>& message);
  void Push(const std::vector<uint8_t>& message_buffer);
  void PushFront(const std::shared_ptr<IBusMessage>& message);
  std::shared_ptr<IBusMessage> Pop();

  template< class Rep, class Period >
  std::shared_ptr<IBusMessage> PopWait(const std::chrono::duration<Rep,
    Period>& rel_time);

  [[nodiscard]] size_t MessageSize() const;

  [[nodiscard]] size_t Size() const;

  [[nodiscard]] bool Empty() const;

  template< class Rep, class Period >
  void EmptyWait(const std::chrono::duration<Rep, Period>& rel_time);

  virtual void Start();
  virtual void Stop();

  void Clear();

private:
  std::deque<std::shared_ptr<IBusMessage>> queue_;
  mutable std::mutex queue_mutex_;
  std::atomic<size_t> queue_size_ = 0;
  std::condition_variable queue_not_empty_;
};

template< class Rep, class Period >
std::shared_ptr<IBusMessage> IBusMessageQueue::PopWait(const std::chrono::duration<Rep,
                                    Period>& rel_time) {
  std::unique_lock lock(queue_mutex_);
  queue_not_empty_.wait_for(lock, rel_time, [&] () ->bool {
      return queue_size_.load() > 0;
    });

  std::shared_ptr<IBusMessage> message;
  if (!queue_.empty()) {
    message = std::move(queue_.front());
    queue_.pop_front();
    queue_size_ = queue_.size();
  } else {
    queue_size_ = 0;
  }

  return message;
}

template< class Rep, class Period >
void IBusMessageQueue::EmptyWait(const std::chrono::duration<Rep, Period>& rel_time) {
  std::unique_lock lock(queue_mutex_);
  queue_not_empty_.wait_for(lock, rel_time, [&] () ->bool {
      return queue_size_.load() > 0;
    });
}
} // bus


