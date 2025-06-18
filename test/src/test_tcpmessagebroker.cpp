/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */
#include <chrono>
#include <algorithm>
#include <array>
#include <memory>

#include <gtest/gtest.h>

#include "bus/interface/businterfacefactory.h"
#include "bus/buslogstream.h"
#include "bus/candataframe.h"

using namespace std::chrono_literals;

namespace bus {

TEST(TcpMessageBroker, TestProperties) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();

  constexpr size_t max_messages = 10;

  auto broker = BusInterfaceFactory::CreateBroker(
    BrokerType::TcpBrokerType);
  ASSERT_TRUE(broker);
  broker->Name("BusMemTest");
  broker->Address("127.0.0.1"); // Only accessible locally
  broker->Port(42611);

  // Note that this starts the master shared memory.
  // Clients doesn't do this. Instead they are creating
  // subscriber or publisher to this memory name.
  // This unit test have the master and its client in the
  // same process.
  broker->Start();

  // The publisher should connect to the shared memory direct
  // withoout adding itself to any publisher queue
  auto publisher = broker->CreatePublisher();
  ASSERT_TRUE(publisher);
  EXPECT_TRUE(publisher->Empty());
  EXPECT_EQ(broker->NofPublishers(), 0);
  publisher->Start();

  auto subscriber = broker->CreateSubscriber();
  EXPECT_TRUE(subscriber);
  EXPECT_TRUE(subscriber->Empty());
  EXPECT_EQ(broker->NofSubscribers(), 0);
  subscriber->Start();

  for (size_t index = 0; index < max_messages; ++index) {
    auto msg = std::make_shared<CanDataFrame>();
    publisher->Push(msg);
  }

  for (size_t timeout = 0;timeout < 100 && subscriber->Size() != max_messages;
    ++timeout) {
    std::this_thread::sleep_for(100ms);
  }
  EXPECT_EQ(publisher->Size(), 0);
  EXPECT_EQ(subscriber->Size(), max_messages);

  publisher->Stop();
  subscriber->Stop();
  broker->Stop();

  publisher.reset();
  subscriber.reset();
  broker.reset();

  EXPECT_EQ(BusLogStream::ErrorCount(), 0);
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

TEST(TcpMessageBroker, TestOneClient) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();

  constexpr size_t max_messages = 100'000;

  auto broker = BusInterfaceFactory::CreateBroker(
    BrokerType::TcpBrokerType);
  ASSERT_TRUE(broker);
  broker->Name("BusMemTest");
  broker->Address("127.0.0.1"); // Only accessible locally
  broker->Port(42611);

  // Note that this starts the master shared memory.
  // Clients doesn't do this. Instead they are creating
  // subscriber or publisher to this memory name.
  // This unit test have the master and its client in the
  // same process.
  EXPECT_FALSE(broker->IsConnected());
  broker->Start();
  EXPECT_TRUE(broker->IsConnected());


  auto client = BusInterfaceFactory::CreateBroker(
    BrokerType::TcpClientType);
  ASSERT_TRUE(client);
  client->Name("TcpClient");
  client->Address("127.0.0.1"); // Only accessible locally
  client->Port(42611);

  client->Start();

  // The publisher should connect to the shared memory direct
  // withoout adding itself to any publisher queue
  auto publisher = client->CreatePublisher();
  ASSERT_TRUE(publisher);
  EXPECT_TRUE(publisher->Empty());
  EXPECT_EQ(client->NofPublishers(), 1);
  publisher->Start();

  auto subscriber = client->CreateSubscriber();
  EXPECT_TRUE(subscriber);
  EXPECT_TRUE(subscriber->Empty());
  EXPECT_EQ(client->NofSubscribers(), 1);
  subscriber->Start();

  for (size_t sample = 0; sample < max_messages; ++sample) {
    auto msg = std::make_shared<CanDataFrame>();
    publisher->Push(msg);
  }

  for (size_t timeout = 0;
       timeout < 100 && subscriber->Size() != max_messages;
       ++timeout) {
    std::this_thread::sleep_for(100ms);
  }
  EXPECT_TRUE(client->IsConnected());
  EXPECT_EQ(publisher->Size(),0);
  EXPECT_EQ(subscriber->Size(),max_messages);
  client->Stop();

  publisher->Stop();
  subscriber->Stop();
  broker->Stop();

  publisher.reset();
  subscriber.reset();
  broker.reset();

  EXPECT_EQ(BusLogStream::ErrorCount(), 0);
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

} // namespace bus