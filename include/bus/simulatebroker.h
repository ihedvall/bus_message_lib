/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <cstdint>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <array>
#include <condition_variable>

#include "bus/ibusmessagebroker.h"

namespace bus {

class SimulateQueue;

class SimulateBroker : public IBusMessageBroker {
public:
  SimulateBroker();
   ~SimulateBroker() override;

  [[nodiscard]] std::shared_ptr<IBusMessageQueue> CreatePublisher() override;
  [[nodiscard]] std::shared_ptr<IBusMessageQueue> CreateSubscriber() override;

  void Start() override;
  void Stop() override;

  void PublisherPoll(SimulateQueue& queue);
  bool SubscriberPoll(SimulateQueue& queue);
  void GetChannel(SimulateQueue& queue);
  bool BufferFull() { return buffer_full_; }
private:
  std::atomic<bool> stop_master_task_ = true;
  std::condition_variable buffer_full_condition_;
  std::mutex event_mutex_;
  std::thread master_task_;
  time_t timeout_ = 0;

  struct Channel {
    bool used = false;
    uint32_t queue_index = 0;
  };
  std::mutex buffer_mutex_;
  std::atomic<bool> buffer_full_ = false;

  std::vector<uint8_t> buffer_;

  /** \brief Internal queue index
   *
   *  The channel array holds indexes for all publishers and subscribers.
   *  The index 0 is used for all publishers,
   */
  std::array<Channel, 256> channels_;

  void BrokerMasterTask();
  void HandleBufferFull();
  void ResetChannels();

};

} // bus


