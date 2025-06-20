/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include "sharedmemoryclient.h"
#include "sharedmemorytxrxqueue.h"

namespace bus {
SharedMemoryClient::~SharedMemoryClient() {
}


void SharedMemoryClient::Start() {
  Stop(); // Stop on-going threads
  if (Name().empty()) {
    Name("BusMessageServer");
  }
  connected_ = true;
}

void SharedMemoryClient::Stop() {
  connected_ = false;
}

std::shared_ptr<IBusMessageQueue> SharedMemoryClient::CreatePublisher() {
  auto publisher = std::make_shared<SharedMemoryTxRxQueue>(Name(),
    false, true);
  return publisher;
}

std::shared_ptr<IBusMessageQueue> SharedMemoryClient::CreateSubscriber() {
  auto subscriber = std::make_shared<SharedMemoryTxRxQueue>(Name(),
    true, false);
  return subscriber;
}

} // bus
