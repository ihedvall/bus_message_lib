/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <string>

#include "ibusmessagequeue.h"

namespace bus {

class IBusMessageBroker {
public:
  IBusMessageBroker() = default;
  virtual ~IBusMessageBroker() = default;

  void Name(std::string name);
  [[nodiscard]] const std::string& Name() const;

  void MemorySize(uint32_t size) {memory_size_ = size;}
  [[nodiscard]] uint32_t MemorySize() const { return memory_size_;}

  [[nodiscard]] virtual std::shared_ptr<IBusMessageQueue> CreatePublisher();
  [[nodiscard]] virtual std::shared_ptr<IBusMessageQueue> CreateSubscriber();

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
  std::string name_;
  uint32_t memory_size_ = 16'000;
  void InprocessThread();
};

} // bus


