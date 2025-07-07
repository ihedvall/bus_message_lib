/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include <gtest/gtest.h>

#include "../../src/simulatebroker.h"
#include "bus/buslogstream.h"
#include "bus/candataframe.h"
#include "bus/interface/businterfacefactory.h"

namespace bus {

TEST(BusInterfaceFactory, TestCreateBroker) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();

  {
    auto simulate_broker =
      BusInterfaceFactory::CreateBroker(BrokerType::SimulateBrokerType);
    ASSERT_TRUE(simulate_broker);
    simulate_broker->Start();
    simulate_broker->Stop();
  }

  {
    auto shared_broker =
      BusInterfaceFactory::CreateBroker(BrokerType::SharedMemoryBrokerType);
    ASSERT_TRUE(shared_broker);
    shared_broker->Name("Olle");
    shared_broker->Start();
    shared_broker->Stop();
  }
  EXPECT_EQ(BusLogStream::ErrorCount(), 0);
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

} // bus