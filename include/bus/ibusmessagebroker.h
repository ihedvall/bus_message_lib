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

  virtual [[nodiscard]] std::shared_ptr<IBusMessageQueue> CreatePublisher();
  virtual [[nodiscard]] std::shared_ptr<IBusMessageQueue> CreateSubscriber();

  void DetachPublisher(const std::shared_ptr<IBusMessageQueue>& publisher);
  void DetachSubscriber(const std::shared_ptr<IBusMessageQueue>& subscriber);

  [[nodiscard]] size_t NofPublishers() const;
  [[nodiscard]] size_t NofSubscribers() const;

  virtual void Start();
  virtual void Stop();

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


