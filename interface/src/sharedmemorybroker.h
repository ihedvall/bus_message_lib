/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <atomic>
#include <thread>
#include <ctime>
#include <array>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

#include "bus/ibusmessagebroker.h"

namespace bus {

struct Channel {
  bool used = false;
  uint32_t queue_index = 0;
};

struct SharedMemoryObjects {
  std::atomic<bool> initialized = false; // Indicate that shared memory ready
  boost::interprocess::interprocess_mutex memory_mutex;
  boost::interprocess::interprocess_condition buffer_full_condition;
  std::atomic<bool> buffer_full = false;
  std::array<Channel, 256> channels;
  std::array<uint8_t, 16'000> buffer;
};

class SharedMemoryBroker : public IBusMessageBroker {
public:
  SharedMemoryBroker();
  ~SharedMemoryBroker() override;
  void Start() override;
  void Stop() override;

  [[nodiscard]] std::shared_ptr<IBusMessageQueue> CreatePublisher() override;
  [[nodiscard]] std::shared_ptr<IBusMessageQueue> CreateSubscriber() override;

private:
  std::atomic<bool> stop_master_task_ = true;
  std::thread master_task_;
  std::time_t timeout_ = 0;

  // Defines the shared memory objects

  SharedMemoryObjects* shm_ = nullptr;
  std::unique_ptr<boost::interprocess::shared_memory_object>  shared_memory_;
  std::unique_ptr<boost::interprocess::mapped_region>  region_;
  void BrokerMasterTask();
  void HandleBufferFull();
  void ResetChannels();
};


} // bus


