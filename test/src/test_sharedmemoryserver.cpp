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

TEST(SharedMemoryServer, TestProperties) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();

  auto server = BusInterfaceFactory::CreateBroker(
    BrokerType::SharedMemoryServerType);
  ASSERT_TRUE(server);
  server->Name("BusMemTest");
  // Note that this starts the master shared memory.
  // Clients doesn't do this. Instead they are creating
  // subscriber or publisher to this memory name.
  // This unit test have the master and its client in the
  // same process.
  server->Start();


  auto server_publisher = server->CreatePublisher();
  ASSERT_TRUE(server_publisher);
  EXPECT_TRUE(server_publisher->Empty());
  EXPECT_EQ(server->NofPublishers(), 0);
  server_publisher->Start();

  auto server_subscriber = server->CreateSubscriber();
  EXPECT_TRUE(server_subscriber);
  EXPECT_TRUE(server_subscriber->Empty());
  EXPECT_EQ(server->NofSubscribers(), 0);
  server_subscriber->Start();

  auto client = BusInterfaceFactory::CreateBroker(
    BrokerType::SharedMemoryClientType);
  ASSERT_TRUE(client);
  client->Name("BusMemTest");

  auto client_publisher = client->CreatePublisher();
  ASSERT_TRUE(client_publisher);
  EXPECT_TRUE(client_publisher->Empty());
  EXPECT_EQ(client->NofPublishers(), 0);
  client_publisher->Start();

  auto client_subscriber = client->CreateSubscriber();
  EXPECT_TRUE(client_subscriber);
  EXPECT_TRUE(client_subscriber->Empty());
  EXPECT_EQ(client->NofSubscribers(), 0);
  client_subscriber->Start();

  auto msg = std::make_shared<CanDataFrame>();
  server_publisher->Push(msg);
  client_publisher->Push(msg);

  for (size_t timeout = 0; timeout < 100; ++timeout ) {
    if (server_subscriber->Size() == 1 && client_subscriber->Size() == 1) {
      break;
    }
    std::this_thread::sleep_for(100ms);
  }
  EXPECT_EQ(server_subscriber->Size(), 1);
  EXPECT_EQ(client_subscriber->Size(), 1);
  EXPECT_EQ(server_publisher->Size(), 0);
  EXPECT_EQ(client_publisher->Size(), 0);

  server_publisher->Stop();
  server_subscriber->Stop();
  server->Stop();

  server_publisher.reset();
  server_subscriber.reset();
  server.reset();

  EXPECT_EQ(BusLogStream::ErrorCount(), 0);
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

TEST(SharedMemoryServer, TestOneClient) {
  constexpr size_t max_messages = 10'000;
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();

  auto server = BusInterfaceFactory::CreateBroker(
    BrokerType::SharedMemoryServerType);
  ASSERT_TRUE(server);
  server->Name("BusMemTest");
  // Note that this starts the master shared memory.
  // Clients doesn't do this. Instead they are creating
  // subscriber or publisher to this memory name.
  // This unit test have the master and its client in the
  // same process.
  server->Start();

  auto server_publisher = server->CreatePublisher();
  ASSERT_TRUE(server_publisher);
  EXPECT_TRUE(server_publisher->Empty());
  EXPECT_EQ(server->NofPublishers(), 0);
  server_publisher->Start();

  auto server_subscriber = server->CreateSubscriber();
  EXPECT_TRUE(server_subscriber);
  EXPECT_TRUE(server_subscriber->Empty());
  EXPECT_EQ(server->NofSubscribers(), 0);
  server_subscriber->Start();

  auto client = BusInterfaceFactory::CreateBroker(
    BrokerType::SharedMemoryClientType);
  ASSERT_TRUE(client);
  client->Name("BusMemTest");
  client->Start();

  auto client_publisher = client->CreatePublisher();
  ASSERT_TRUE(client_publisher);
  EXPECT_TRUE(client_publisher->Empty());
  EXPECT_EQ(client->NofPublishers(), 0);
  client_publisher->Start();

  auto client_subscriber = client->CreateSubscriber();
  EXPECT_TRUE(client_subscriber);
  EXPECT_TRUE(client_subscriber->Empty());
  EXPECT_EQ(client->NofSubscribers(), 0);
  client_subscriber->Start();

  auto msg = std::make_shared<CanDataFrame>();
  for (size_t index = 0; index < max_messages; ++index) {
    server_publisher->Push(msg);
    client_publisher->Push(msg);
  }

  for (size_t timeout = 0; timeout < 100; ++timeout ) {
    if (server_subscriber->Size() == max_messages &&
      client_subscriber->Size() == max_messages) {
      break;
    }
    std::this_thread::sleep_for(100ms);
  }
  EXPECT_EQ(server_subscriber->Size(), max_messages);
  EXPECT_EQ(client_subscriber->Size(), max_messages);
  EXPECT_EQ(server_publisher->Size(), 0);
  EXPECT_EQ(client_publisher->Size(), 0);

  server_publisher->Stop();
  server_subscriber->Stop();
  server->Stop();

  server_publisher.reset();
  server_subscriber.reset();
  server.reset();

  EXPECT_EQ(BusLogStream::ErrorCount(), 0);
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

}