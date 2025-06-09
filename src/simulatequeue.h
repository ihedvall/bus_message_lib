/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once
#include "bus/ibusmessagequeue.h"

namespace bus {

class SimulateBroker;

class SimulateQueue : public IBusMessageQueue {
public:
  SimulateQueue() = delete;
  explicit SimulateQueue(SimulateBroker& broker, bool publisher);
  ~SimulateQueue() override;

  void Start() override;
  void Stop() override;

  void Channel(uint8_t channel) { channel_ = channel; }
  [[nodiscard]] uint8_t Channel() const { return channel_; }

private:
  bool publisher_ = false;
  uint8_t channel_ = 0;

  SimulateBroker& broker_;
  std::atomic<bool> stop_thread_ = true;
  std::thread thread_;

  void PublisherTask();
  void SubscriberTask();
};

} // bus


