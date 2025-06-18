/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include <cstdint>
#include <chrono>
#include <thread>
#include <atomic>
#include <array>

#include <gtest/gtest.h>

#include "bus/candataframe.h"
#include "bus/ibusmessagequeue.h"

using namespace std::chrono_literals;

namespace {

bus::IBusMessageQueue kQueue;
constexpr size_t kMaxMessage = 100'000;
std::atomic<size_t> kNofMessages = 0;
std::atomic<bool> kStopTread = true;

void WriteMessage() {
  for (size_t index = 0; index < kMaxMessage; ++index) {
    auto msg = std::make_shared<bus::CanDataFrame>();
    kQueue.Push(msg);
    std::this_thread::yield();
  }
}

void ReadMessage() {
  while (!kStopTread) {
    auto msg = kQueue.PopWait(100ms);
    if (msg) {
      ++kNofMessages;
    }
  }
}


}
namespace bus {
TEST(IBusMessageQueue, TestProperties) {
  IBusMessageQueue queue;

  EXPECT_EQ(queue.Size(), 0);
  EXPECT_TRUE(queue.Empty());
  EXPECT_EQ(queue.MessageSize(), 0);

  auto msg = std::make_shared<IBusMessage>();
  queue.Push(msg);
  EXPECT_TRUE(msg);
  EXPECT_EQ(queue.Size(), 1);
  EXPECT_FALSE(queue.Empty());
  EXPECT_EQ(queue.MessageSize(), 18);

  auto msg1 = queue.Pop();
  EXPECT_TRUE(msg1);
  EXPECT_EQ(queue.Size(), 0);
  EXPECT_TRUE(queue.Empty());
  EXPECT_EQ(queue.MessageSize(), 0);

  queue.Push(msg);
  EXPECT_TRUE(msg);
  EXPECT_EQ(queue.Size(), 1);
  EXPECT_FALSE(queue.Empty());
  EXPECT_EQ(queue.MessageSize(), 18);

  queue.Clear();
  EXPECT_EQ(queue.Size(), 0);
  EXPECT_TRUE(queue.Empty());
  EXPECT_EQ(queue.MessageSize(), 0);
}

TEST(IBusMessageQueue, TestOneInOneOut) {

  kNofMessages= 0;
  kStopTread = false;

  auto subscriber = std::thread(&ReadMessage);
  auto publisher = std::thread(&WriteMessage);

  size_t timeout = 0;
  while (kNofMessages < kMaxMessage && timeout < 100) {
    std::this_thread::sleep_for(100ms);
    ++timeout;
  }
  kStopTread = true;
  publisher.join();
  subscriber.join();
  kQueue.Clear();
  EXPECT_EQ(kNofMessages, kMaxMessage);
}

TEST(IBusMessageQueue, TestOneInTenOut) {

  kNofMessages= 0;
  kStopTread = false;

  std::array<std::thread, 10> subscribers;
  for (auto& subscriber : subscribers) {
    subscriber = std::thread(&ReadMessage);
  }
  auto publisher = std::thread(&WriteMessage);

  size_t timeout = 0;
  while (kNofMessages < kMaxMessage && timeout < 100) {
    std::this_thread::sleep_for(100ms);
    ++timeout;
  }
  kStopTread = true;
  publisher.join();
  for (auto& subscriber : subscribers) {
    subscriber.join();;
  }
  kQueue.Clear();
  EXPECT_EQ(kNofMessages, kMaxMessage);
}

TEST(IBusMessageQueue, TestTenInTenOut) {
  kNofMessages= 0;
  kStopTread = false;

  std::array<std::thread, 10> subscribers;
  for (auto& subscriber : subscribers) {
    subscriber = std::thread(&ReadMessage);
  }
  std::array<std::thread, 10> publishers;
  for (auto& publisher : publishers) {
    publisher = std::thread(&WriteMessage);
  }

  size_t timeout = 0;
  while (kNofMessages < publishers.size() * kMaxMessage && timeout < 100) {
    std::this_thread::sleep_for(100ms);
    ++timeout;
  }
  kStopTread = true;

  for (auto& subscriber : subscribers) {
    subscriber.join();;
  }
  for (auto& publisher : publishers) {
    publisher.join();;
  }
  kQueue.Clear();
  EXPECT_EQ(kNofMessages, publishers.size() * kMaxMessage);
}

}
