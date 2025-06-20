/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once
#include <memory>

#include "sharedmemoryserver.h"

namespace bus {

class SharedMemoryClient : public IBusMessageBroker {
public:
  SharedMemoryClient() = default;
  ~SharedMemoryClient() override;

  void Start() override;
  void Stop() override;

  [[nodiscard]] std::shared_ptr<IBusMessageQueue> CreatePublisher() override;
  [[nodiscard]] std::shared_ptr<IBusMessageQueue> CreateSubscriber() override;
};

} // bus


