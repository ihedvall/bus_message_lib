/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <atomic>
#include <thread>
#include <array>
#include <memory>
#include <ctime>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include "bus/ibusmessagebroker.h"


namespace bus {



struct SharedServerObjects {
  std::atomic<bool> initialized = false; // Indicate that shared memory ready
  boost::interprocess::interprocess_mutex memory_mutex;

  boost::interprocess::interprocess_condition tx_full_condition;
  std::atomic<bool> tx_full = false;
  std::array<Channel, 256> tx_channels;
  std::array<uint8_t, 16'000> tx_buffer;

  boost::interprocess::interprocess_condition rx_full_condition;
  std::atomic<bool> rx_full = false;
  std::array<Channel, 256> rx_channels;
  std::array<uint8_t, 16'000> rx_buffer;
};

class SharedMemoryServer : public IBusMessageBroker {
public:
  SharedMemoryServer() = default;
  ~SharedMemoryServer() override;
  void Start() override;
  void Stop() override;

  [[nodiscard]] std::shared_ptr<IBusMessageQueue> CreatePublisher() override;
  [[nodiscard]] std::shared_ptr<IBusMessageQueue> CreateSubscriber() override;
protected:
  void ConnectToSharedMemory();
private:
  std::atomic<bool> stop_server_threads_ = true;
  std::thread tx_thread_;
  std::thread rx_thread_;
  std::time_t tx_timeout_ = 0;
  std::time_t rx_timeout_ = 0;

  SharedServerObjects* shm_ = nullptr;
  std::unique_ptr<boost::interprocess::shared_memory_object>  shared_memory_;
  std::unique_ptr<boost::interprocess::mapped_region>  region_;

  void TxThread();
  void HandleTxFull();
  void ResetTxChannels();

  void RxThread();
  void HandleRxFull();
  void ResetRxChannels();
};

} // bus

