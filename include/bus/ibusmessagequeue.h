/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
/** \file ibusmessagequeue.h
 * \brief Defines an interface against a message queue.
 *
 * The file defines a generic interface against a message queue.
 * The queue contains message objects.
 * All brokers have 2 types of queues, publishers or subscribers.
 * The publishers sends messages while the subscribers receive messages.
 * The user use one side of the queue while the broker uses the other end.
 * The implementation of the queue is dependent of the broker type.
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

/**
 * @brief Interface against a message queue.
 *
 * The class defines a generic interface against a message queue.
 * The queue stores messages in primary memory,
 * The queue is thread-safe.
 * The user application only use one end of the queue while
 * the broker uses the other end.
 * The actual implementation of the queue is dependent on the type
 * of broker.
 */
class IBusMessageQueue {
public:
  IBusMessageQueue() = default;
  virtual ~IBusMessageQueue(); ///< Destructor

  /**
   * @brief Adds a message to the end of the queue.
   * @param message Smart pointer to the message.
   */
  void Push(const std::shared_ptr<IBusMessage>& message);

  /**
   * @brief Adds a serialized message to the queue.
   *
   * Adds aserialized message to the end of the queue.
   * The function deserialize the message first and adds it to the queue.
   *
   * @param message_buffer Serialized byte array.
   */
  void Push(const std::vector<uint8_t>& message_buffer);

  /**
   * @brief Adds a message first in the queue.
   *
   * Adds a message to the front of the queue.
   * This happens in certain circumstances when the message cannot be
   * sent due to not enogh room in the shared memory.
   * @param message Smart pointer to a message.
   */
  void PushFront(const std::shared_ptr<IBusMessage>& message);

  /**
   * @brief Extract a message from the front of the queue.
   *
   * Returns the next message in the queue.
   * Note that if the queue is empty, the smart pointer may be empty.
   *
   * @return Smart pointer to a message.
   */
  std::shared_ptr<IBusMessage> Pop();

  /**
   * @brief Blocks a period if queue is empty otherwise returning a message.
   * @tparam Rep Type of clock
   * @tparam Period Duration
   * @param rel_time Time period to wait. Use std::chrono_utils.
   * @return Return the next message or an empty message.
   */
  template < class Rep, class Period >
  std::shared_ptr<IBusMessage> PopWait(const std::chrono::duration<Rep,
    Period>& rel_time);

  /**
   * @brief Retuns the size of next message.
   * @return The next message size.
   */
  [[nodiscard]] size_t MessageSize() const;

  /**
   * @brief Retuns the number messages in the queue.
   * @return Number of messages in the queue.
   */
  [[nodiscard]] size_t Size() const;

  /**
   * @brief Returns true if the queue is empty.
   * @return True if the queue doesn't have any messages.
   */
  [[nodiscard]] bool Empty() const;

  /**
   * @brief Waits a period or direct if the queue not is empty.
   * @tparam Rep See std::chrono_utils.
   * @tparam Period See std::chrono_utils.
   * @param rel_time Time to wait.
   */
  template < class Rep, class Period >
  void EmptyWait(const std::chrono::duration<Rep, Period>& rel_time);

  /**
   * @brief Initialize the queue.
   */
  virtual void Start();

  /**
   * @brief Stops the queue.
   */
  virtual void Stop();

  /**
   * @brief Removes all messages in the queue.
   */
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


