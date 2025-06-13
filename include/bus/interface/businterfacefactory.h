/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <memory>
#include <cstdint>

#include "bus/ibusmessagebroker.h"

namespace bus {

enum class BrokerType : int {
  SimulateBrokerType,
  SharedMemoryBrokerType,
  TcpBrokerType,
};

class BusInterfaceFactory {
public:
  static std::unique_ptr<IBusMessageBroker> CreateBroker(BrokerType type);
};

} // bus::interface


