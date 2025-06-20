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
#include <mutex>

#include "ibusmessagequeue.h"

namespace bus {

struct Channel {
  bool used = false;
  uint32_t queue_index = 0;
};

class IBusMessageBroker {
public:
  IBusMessageBroker() = default;
  virtual ~IBusMessageBroker() = default;

  void Name(std::string name);
  [[nodiscard]] const std::string& Name() const;

  void MemorySize(uint32_t size) {memory_size_ = size;}
  [[nodiscard]] uint32_t MemorySize() const { return memory_size_;}

  void Address(std::string address);
  [[nodiscard]] const std::string& Address() const;

  void Port(uint16_t port) {port_ = port;}
  [[nodiscard]] uint16_t Port() const { return port_;}

  [[nodiscard]] bool IsConnected() const;

  [[nodiscard]] virtual std::shared_ptr<IBusMessageQueue> CreatePublisher();
  [[nodiscard]] virtual std::shared_ptr<IBusMessageQueue> CreateSubscriber();

  void DetachPublisher(const std::shared_ptr<IBusMessageQueue>& publisher);
  void DetachSubscriber(const std::shared_ptr<IBusMessageQueue>& subscriber);

  [[nodiscard]] size_t NofPublishers() const;
  [[nodiscard]] size_t NofSubscribers() const;

  virtual void Start();
  virtual void Stop();

protected:
  std::atomic<bool> connected_ = false;
  mutable std::mutex queue_mutex_;
  std::vector<std::shared_ptr<IBusMessageQueue>> publishers_;
  std::vector<std::shared_ptr<IBusMessageQueue>> subscribers_;

  std::atomic<bool> stop_thread_ = false;
  std::thread thread_;

  void Poll(IBusMessageQueue& queue) const;
private:
  std::string name_;
  uint32_t memory_size_ = 16'000;
  std::string address_;
  uint16_t port_ = 0;
  void InprocessThread() const;
};

} // bus


