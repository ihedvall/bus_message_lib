/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include "bus/interface/businterfacefactory.h"

#include "bus/simulatebroker.h"
#include "bus/buslogstream.h"
#include "sharedmemorybroker.h"

namespace bus {

std::unique_ptr<IBusMessageBroker> BusInterfaceFactory::CreateBroker(
    BrokerType type) {
  std::unique_ptr<IBusMessageBroker> broker;

  switch (type) {
    case BrokerType::SimulateBrokerType: {
      auto simulate_broker = std::make_unique<SimulateBroker>();
      broker = std::move(simulate_broker);
      break;
    }

    case BrokerType::SharedMemoryBrokerType: {
      auto shared_broker = std::make_unique<SharedMemoryBroker>();
      broker = std::move(shared_broker);
      break;
    }

    default:
      BUS_ERROR() << "Unknown broker type. Type:" << static_cast<int>(type);
      break;
  }
  return broker;
}

} // bus