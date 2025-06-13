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

TEST(SharedMemoryBroker, TestProperties) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();

  auto broker = BusInterfaceFactory::CreateBroker(
    BrokerType::SharedMemoryBrokerType);
  ASSERT_TRUE(broker);
  broker->Name("BusMemTest");
  // Note that this starts the master shared memory.
  // Clients doesn't do this. Instead they are creating
  // subscriber or publisher to this memory name.
  // This unit test have the master and its client in the
  // same process.
  broker->Start();


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

  std::this_thread::sleep_for(1000ms);
  publisher->Stop();
  subscriber->Stop();
  broker->Stop();

  publisher.reset();
  subscriber.reset();
  broker.reset();

  EXPECT_EQ(BusLogStream::ErrorCount(), 0);
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

TEST(SharedMemoryBroker, TestOneInOneOut) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();

  constexpr size_t max_messages = 10'000;

  auto broker = BusInterfaceFactory::CreateBroker(
    BrokerType::SharedMemoryBrokerType);
  ASSERT_TRUE(broker);
  broker->Name("BusMemTest");
  broker->Start();

  auto publisher = broker->CreatePublisher();
  ASSERT_TRUE(publisher);
  publisher->Start();

  auto subscriber = broker->CreateSubscriber();
  ASSERT_TRUE(subscriber);
  subscriber->Start();

  for (size_t index = 0; index < max_messages; ++index) {
    auto msg = std::make_shared<CanDataFrame>();
    ASSERT_TRUE(msg);
    msg->MessageId(123);
    std::vector<uint8_t> data;
    for (size_t data_index = 0; data_index < 8; ++data_index) {
      data.push_back(static_cast<uint8_t>(index));
    }
    msg->DataBytes(data);
    publisher->Push(msg);
    std::this_thread::yield();
  }

  size_t timeout = 0;
  while (subscriber->Size() < max_messages && timeout < 100) {
    std::this_thread::sleep_for(100ms);
    ++timeout;
  }

  std::cout << "Time:[ms]" << timeout * 100 << std::endl;

  EXPECT_EQ(publisher->Size(), 0);
  EXPECT_EQ(subscriber->Size(), max_messages);

  EXPECT_EQ(BusLogStream::ErrorCount(), 0);
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

TEST(SharedMemoryBroker, TestTenInTenOut) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();

  constexpr size_t max_messages = 1'000;

  auto broker = BusInterfaceFactory::CreateBroker(
    BrokerType::SharedMemoryBrokerType);
  ASSERT_TRUE(broker);
  broker->Name("BusMemTest");
  broker->Start();

  std::array<std::shared_ptr<IBusMessageQueue>, 10> publishers;
  for (auto& publisher : publishers) {
    publisher = broker->CreatePublisher();
    ASSERT_TRUE(publisher);
    publisher->Start();
  }

  std::array<std::shared_ptr<IBusMessageQueue>, 10> subscribers;
  for (auto& subscriber : subscribers) {
    subscriber = broker->CreateSubscriber();
    ASSERT_TRUE(subscriber);
    subscriber->Start();
  }

  for (auto& publisher : publishers) {
    for (size_t index = 0; index < max_messages; ++index) {
      auto msg = std::make_shared<CanDataFrame>();
      ASSERT_TRUE(msg);
      msg->MessageId(123);
      std::vector<uint8_t> data;
      for (size_t data_index = 0; data_index < 8; ++data_index) {
        data.push_back(static_cast<uint8_t>(index));
      }
      msg->DataBytes(data);
      publisher->Push(msg);
    }
  }

  size_t timeout = 0;
  bool ready = false;
  while (!ready && timeout < 100) {
    ready = std::ranges::all_of(subscribers,
      [&](const auto& subscriber)->bool  {
      return subscriber->Size() == max_messages * publishers.size();
    });
    std::this_thread::sleep_for(100ms);
    ++timeout;
  }

  std::cout << "Time:[ms]" << timeout * 100 << std::endl;

  EXPECT_TRUE(std::ranges::all_of(publishers,
    [] (const auto& publisher) ->bool {
    return publisher->Empty();
  }));
  EXPECT_TRUE(ready);

  EXPECT_EQ(BusLogStream::ErrorCount(), 0);
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

TEST(SharedMemoryBroker, TestErrorHandling) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();
  constexpr size_t max_messages = 10'000;

  auto broker = BusInterfaceFactory::CreateBroker(
    BrokerType::SharedMemoryBrokerType);
  ASSERT_TRUE(broker);
  broker->Name("BusMemTest");
  // broker->Start();

  auto publisher = broker->CreatePublisher();
  ASSERT_TRUE(publisher);
  publisher->Start();

  auto subscriber = broker->CreateSubscriber();
  ASSERT_TRUE(subscriber);
  subscriber->Start();

  for (size_t index = 0; index < max_messages; ++index) {
    auto msg = std::make_shared<CanDataFrame>();
    ASSERT_TRUE(msg);
    msg->MessageId(123);
    std::vector<uint8_t> data;
    for (size_t data_index = 0; data_index < 8; ++data_index) {
      data.push_back(static_cast<uint8_t>(index));
    }
    msg->DataBytes(data);
    publisher->Push(msg);
  }

  broker->Start();

  size_t timeout = 0;
  while (subscriber->Size() < max_messages && timeout < 200) {
    std::this_thread::sleep_for(100ms);
    ++timeout;
  }

  std::cout << "Time:[ms]" << timeout * 100 << std::endl;

  broker->Stop();
  publisher->Stop();
  subscriber->Stop();

  EXPECT_EQ(publisher->Size(), 0);
  EXPECT_EQ(subscriber->Size(), max_messages);

  broker.reset();
  publisher.reset();
  subscriber.reset();

  EXPECT_GT(BusLogStream::ErrorCount(), 0);
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

} // namespace bus
