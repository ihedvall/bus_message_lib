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
#include <vector>

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include "bus/ibusmessagequeue.h"


namespace bus {

struct SharedServerObjects;

class SharedMemoryTxRxQueue : public IBusMessageQueue {
public:
  SharedMemoryTxRxQueue() = delete;
  explicit SharedMemoryTxRxQueue(std::string shared_memory_name,
    bool tx_queue, bool publisher );
  ~SharedMemoryTxRxQueue() override;

  void Start() override;
  void Stop() override;

private:
  bool tx_queue_ = false;
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
  SharedServerObjects* shm_ = nullptr;

  void PublisherThread();
  void SubscriberThread();
  void GetChannel();

  [[nodiscard]] bool PublisherPoll(SharedServerObjects& shm,
                            const IBusMessage& message) const;
  bool SubscriberPoll(SharedServerObjects& shm,
    std::vector<uint8_t>& msg_buffer) ;

  void ConnectToSharedMemory();

};

} // bus

