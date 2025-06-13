/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <deque>
#include <memory>
#include <mutex>
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

  [[nodiscard]] size_t MessageSize() const;

  [[nodiscard]] size_t Size() const;

  [[nodiscard]] bool Empty() const;

  virtual void Start();
  virtual void Stop();

  void Clear();
private:
  std::deque<std::shared_ptr<IBusMessage>> queue_;
  mutable std::mutex queue_mutex_;
};

} // bus


