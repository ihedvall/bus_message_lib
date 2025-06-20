/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include "bus/interface/businterfacefactory.h"

#include "bus/simulatebroker.h"
#include "bus/buslogstream.h"
#include "sharedmemorybroker.h"
#include "sharedmemoryserver.h"
#include "sharedmemoryclient.h"
#include "tcpmessagebroker.h"
#include "tcpmessageserver.h"
#include "tcpmessageclient.h"

namespace bus {

std::unique_ptr<IBusMessageBroker> BusInterfaceFactory::CreateBroker(
    BrokerType type) {
  std::unique_ptr<IBusMessageBroker> broker;

  switch (type) {
    case BrokerType::TcpClientType: {
      auto tcp_client = std::make_unique<TcpMessageClient>();
      broker = std::move(tcp_client);
      break;
    }

    case BrokerType::TcpServerType: {
      auto tcp_server = std::make_unique<TcpMessageServer>();
      broker = std::move(tcp_server);
      break;
    }

    case BrokerType::TcpBrokerType: {
      auto tcp_broker = std::make_unique<TcpMessageBroker>();
      broker = std::move(tcp_broker);
      break;
    }

    case BrokerType::SimulateBrokerType: {
      auto simulate_broker = std::make_unique<SimulateBroker>();
      broker = std::move(simulate_broker);
      break;
    }

    case BrokerType::SharedMemoryClientType: {
      auto shared_client = std::make_unique<SharedMemoryClient>();
      broker = std::move(shared_client);
      break;
    }

    case BrokerType::SharedMemoryServerType: {
      auto shared_server = std::make_unique<SharedMemoryServer>();
      broker = std::move(shared_server);
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