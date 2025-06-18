/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <atomic>
#include <thread>

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include "bus/ibusmessagequeue.h"

namespace bus {

class SharedMemoryBroker;
struct SharedMemoryObjects;

class SharedMemoryQueue : public IBusMessageQueue {
public:
  SharedMemoryQueue() = delete;
  explicit SharedMemoryQueue(const std::string& shared_memory_name,
    bool publisher);
  ~SharedMemoryQueue() override;

  void Start() override;
  void Stop() override;

private:
  bool publisher_ = false;
  std::string shared_memory_name_;
  uint8_t channel_ = 0;
  std::atomic<bool> stop_thread_ = true;
  std::thread thread_;
  mutable std::atomic<bool> operable_ = false; ///<  Supress of log messages
  enum class SharedMemoryState : int {
    WaitOnSharedMemory =  0,
    HandleMessages
  };
  SharedMemoryState state_ = SharedMemoryState::WaitOnSharedMemory;
  std::unique_ptr<boost::interprocess::shared_memory_object> shared_memory_;
  std::unique_ptr<boost::interprocess::mapped_region> region_;
  SharedMemoryObjects* shm_ = nullptr;

  void PublisherTask();
  void SubscriberTask();
  void GetChannel();
  static bool PublisherPoll(SharedMemoryObjects& shm,
                            const IBusMessage& message);
  bool SubscriberPoll(SharedMemoryObjects& shm,
    std::vector<uint8_t>& msg_buffer
    ) const;

  void ConnectToSharedMemory();
};

} // bus


