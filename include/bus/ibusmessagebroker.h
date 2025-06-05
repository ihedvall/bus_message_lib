/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once
#include <memory>
#include <vector>
#include <atomic>
#include <thread>

#include "ibusmessagequeue.h"

namespace bus {

class IBusMessageBroker {
public:
  IBusMessageBroker() = default;
  virtual ~IBusMessageBroker() = default;

  [[nodiscard]] std::shared_ptr<IBusMessageQueue> CreatePublisher();
  [[nodiscard]] std::shared_ptr<IBusMessageQueue> CreateSubscriber();

  void DetachPublisher(const std::shared_ptr<IBusMessageQueue>& publisher);
  void DetachSubscriber(const std::shared_ptr<IBusMessageQueue>& subscriber);

  [[nodiscard]] size_t NofPublishers() const;
  [[nodiscard]] size_t NofSubscribers() const;

  void Start();
  void Stop();

protected:
  std::vector<std::shared_ptr<IBusMessageQueue>> publishers_;
  std::vector<std::shared_ptr<IBusMessageQueue>> subscribers_;

  std::atomic<bool> stop_thread_ = false;
  std::thread thread_;

  void Poll(IBusMessageQueue& queue) const;
private:
  void InprocessThread();
};

} // bus


