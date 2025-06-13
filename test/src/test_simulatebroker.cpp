/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */
#include <chrono>
#include <algorithm>

#include <gtest/gtest.h>

#include "bus/simulatebroker.h"
#include "bus/buslogstream.h"
#include "bus/candataframe.h"

using namespace std::chrono_literals;

namespace bus {

TEST(SimulateBroker, TestProperties) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  SimulateBroker broker;
  broker.MemorySize(16'000);

  EXPECT_EQ(broker.NofPublishers(), 0);
  EXPECT_EQ(broker.NofSubscribers(), 0);

  auto publisher = broker.CreatePublisher();
  EXPECT_TRUE(publisher);
  EXPECT_TRUE(publisher->Empty());
  EXPECT_EQ(broker.NofPublishers(), 1);

  broker.Start();
  broker.Stop();

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
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

TEST(SimulateBroker, TestOneInOneOut) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();
  constexpr size_t max_messages = 100'000;

  SimulateBroker broker;
  broker.MemorySize(16'000);

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
  broker.Stop();

  std::cout << "Time:[ms]" << timeout * 100 << std::endl;

  EXPECT_EQ(publisher->Size(), 0);
  EXPECT_EQ(subscriber->Size(), max_messages);

  EXPECT_EQ(BusLogStream::ErrorCount(), 0);
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

TEST(SimulateBroker, TestOneInTenOut) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();
  constexpr size_t max_messages = 100'000;

  SimulateBroker broker;
  broker.MemorySize(16'000);

  auto publisher = broker.CreatePublisher();
  std::array<std::shared_ptr<IBusMessageQueue>, 10> subscribers;
  for (auto& subscriber : subscribers) {
     subscriber = broker.CreateSubscriber();
  }
  broker.Start();

  for (size_t index = 0; index < max_messages; ++index) {
    auto msg = std::make_shared<IBusMessage>();
    publisher->Push(msg);
  }

  size_t timeout = 0;
  bool ready = false;
  while (timeout < 200 && !ready) {
    ready = std::ranges::all_of(subscribers, [&] (auto& subscriber) -> bool {
      return subscriber && subscriber->Size() == max_messages;
    });
    std::this_thread::sleep_for(100ms);
    ++timeout;
  }
  broker.Stop();
  std::cout << "Time:[ms]" << timeout * 100 << std::endl;
  EXPECT_TRUE(ready);
  EXPECT_EQ(publisher->Size(), 0);
  int channel_index = 1;
  for (auto& subscriber : subscribers) {
    EXPECT_EQ(subscriber->Size(), max_messages) << channel_index++;
  }

}
TEST(SimulateBroker, TestTenInTenOut) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();
  constexpr size_t max_messages = 10'000;

  SimulateBroker broker;
  broker.MemorySize(16'000);

  std::array<std::shared_ptr<IBusMessageQueue>, 10> publishers;
  for (auto& publisher : publishers) {
    publisher = broker.CreatePublisher();
  }

  std::array<std::shared_ptr<IBusMessageQueue>, 10> subscribers;
  for (auto& subscriber : subscribers) {
     subscriber = broker.CreateSubscriber();
  }
  broker.Start();

  for (auto& publisher : publishers) {
    for (size_t index = 0; index < max_messages; ++index) {
      auto msg = std::make_shared<IBusMessage>();
      publisher->Push(msg);
    }
  }

  size_t timeout = 0;
  bool ready = false;
  while (timeout < 200 && !ready) {
    ready = std::ranges::all_of(subscribers, [&] (auto& subscriber) -> bool {
      return subscriber && subscriber->Size() == max_messages * publishers.size();
    });
    std::this_thread::sleep_for(100ms);
    ++timeout;
  }
  broker.Stop();
  std::cout << "Time:[ms]" << timeout * 100 << std::endl;
  EXPECT_TRUE(ready);

  for (auto& publisher : publishers) {
    EXPECT_EQ(publisher->Size(), 0);
  }

  int channel_index = 1;
  for (auto& subscriber : subscribers) {
    EXPECT_EQ(subscriber->Size(), max_messages * publishers.size()) << channel_index++;
  }
}

TEST(SimulateBroker, TestDiffrentMessages) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();

  SimulateBroker broker;
  broker.MemorySize(16'000);

  auto publisher = broker.CreatePublisher();
  auto subscriber = broker.CreateSubscriber();

  broker.Start();
  constexpr size_t max_messages = 2;
  {
    auto msg = std::make_shared<IBusMessage>();
    publisher->Push(msg);
  }
  {
    auto msg = std::make_shared<CanDataFrame>();
    publisher->Push(msg);
  }

  size_t timeout = 0;
  while (subscriber->Size() < max_messages && timeout < 100) {
    std::this_thread::sleep_for(100ms);
    ++timeout;
  }
  broker.Stop();

  std::cout << "Time:[ms]" << timeout * 100 << std::endl;

  EXPECT_EQ(publisher->Size(), 0);
  EXPECT_EQ(subscriber->Size(), max_messages);

  for (size_t index = 0; index < max_messages; ++index) {
    const auto msg = subscriber->Pop();
    ASSERT_TRUE(msg);
    EXPECT_TRUE(msg->Valid());

    switch (index) {
      case 1: {
        CanDataFrame msg1(msg);
        EXPECT_TRUE(msg1.Valid());
        break;
        }

      case 0:
      default:
        EXPECT_EQ(msg->Type(), BusMessageType::Unknown);
        break;
    }
  }

  EXPECT_EQ(BusLogStream::ErrorCount(), 0);
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

}