/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include <chrono>

#include <gtest/gtest.h>

#include "bus/ibusmessagebroker.h"
using namespace std::chrono_literals;

namespace bus {

TEST(IBusMessageBroker, TestProperties) {
  IBusMessageBroker broker;
  broker.MemorySize(31'000);
  EXPECT_EQ(broker.MemorySize(), 31'000);

  broker.Name("Olle");
  EXPECT_EQ(broker.Name(), "Olle");

  EXPECT_EQ(broker.NofPublishers(), 0);
  EXPECT_EQ(broker.NofSubscribers(), 0);

  auto publisher = broker.CreatePublisher();
  EXPECT_TRUE(publisher);
  EXPECT_TRUE(publisher->Empty());
  EXPECT_EQ(broker.NofPublishers(), 1);

  broker.DetachPublisher(publisher);
  EXPECT_TRUE(publisher);
  EXPECT_TRUE(publisher->Empty());
  EXPECT_EQ(broker.NofPublishers(), 0);

  auto subscriber = broker.CreateSubscriber();
  EXPECT_TRUE(subscriber);
  EXPECT_TRUE(subscriber->Empty());
  EXPECT_EQ(broker.NofSubscribers(), 1);

  broker.DetachSubscriber(subscriber);
  EXPECT_TRUE(subscriber);
  EXPECT_TRUE(subscriber->Empty());
  EXPECT_EQ(broker.NofSubscribers(), 0);
}

TEST(IBusMessageBroker, TestOneInOneOut) {
  constexpr size_t max_messages = 100'000;

  IBusMessageBroker broker;

  auto publisher = broker.CreatePublisher();
  auto subscriber = broker.CreateSubscriber();

  broker.Start();


  for (size_t index = 0; index < max_messages; ++index) {
    auto msg = std::make_shared<IBusMessage>();
    publisher->Push(msg);
  }
  size_t timeout = 0;
  while (subscriber->Size() < max_messages && timeout < 100) {
    std::this_thread::sleep_for(100ms);
    ++timeout;
  }
  EXPECT_TRUE(broker.IsConnected());

  broker.Stop();

  std::cout << "Time:[ms]" << timeout * 100 << std::endl;

  EXPECT_EQ(publisher->Size(), 0);
  EXPECT_EQ(subscriber->Size(), max_messages);

}

}