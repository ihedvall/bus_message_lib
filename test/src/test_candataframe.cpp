/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>

#include "bus/candataframe.h"
#include "bus/buslogstream.h"

namespace bus {

TEST(CanDataFrame, TestProperties) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();

  CanDataFrame msg;;
  EXPECT_EQ(msg.Type(), BusMessageType::CAN_DataFrame);
  constexpr uint32_t kExtendedBit    = 0x80000000;
  constexpr uint32_t kCanId1 = 1234;
  constexpr uint32_t kCanId2 = 234;
  constexpr uint32_t kMessageId = kCanId1 | kExtendedBit;

  msg.MessageId(kMessageId);
  EXPECT_EQ(msg.MessageId(), kMessageId);
  EXPECT_EQ(msg.CanId(), kCanId1);
  EXPECT_TRUE(msg.ExtendedId());

  msg.CanId(kCanId2);
  EXPECT_EQ(msg.CanId(), kCanId2);
  EXPECT_TRUE(msg.ExtendedId());

  msg.ExtendedId(false);
  EXPECT_EQ(msg.CanId(), kCanId2);
  EXPECT_FALSE(msg.ExtendedId());

  EXPECT_EQ(msg.Size(), 34);

  const std::vector<uint8_t> data = {1,2,3,4,5,6,7,8};
  msg.DataBytes(data);
  EXPECT_EQ(msg.Dlc(), 8);
  EXPECT_EQ(msg.DataLength(), 8);
  EXPECT_EQ(msg.Size(), 34 + 8);

  msg.Crc(0x12345);
  EXPECT_EQ(msg.Crc(), 0x12345);

  msg.Dir(true);
  EXPECT_TRUE(msg.Dir());
  msg.Dir(false);
  EXPECT_FALSE(msg.Dir());

  msg.Srr(true);
  EXPECT_TRUE(msg.Srr());
  msg.Srr(false);
  EXPECT_FALSE(msg.Srr());

  msg.Edl(true);
  EXPECT_TRUE(msg.Edl());
  msg.Edl(false);
  EXPECT_FALSE(msg.Edl());

  msg.Brs(true);
  EXPECT_TRUE(msg.Brs());
  msg.Brs(false);
  EXPECT_FALSE(msg.Brs());

  msg.Esi(true);
  EXPECT_TRUE(msg.Esi());
  msg.Esi(false);
  EXPECT_FALSE(msg.Esi());

  msg.Rtr(true);
  EXPECT_TRUE(msg.Rtr());
  msg.Rtr(false);
  EXPECT_FALSE(msg.Rtr());

  msg.WakeUp(true);
  EXPECT_TRUE(msg.WakeUp());
  msg.WakeUp(false);
  EXPECT_FALSE(msg.WakeUp());

  msg.SingleWire(true);
  EXPECT_TRUE(msg.SingleWire());
  msg.SingleWire(false);
  EXPECT_FALSE(msg.SingleWire());

  msg.R0(true);
  EXPECT_TRUE(msg.R0());
  msg.R0(false);
  EXPECT_FALSE(msg.R0());

  msg.R1(true);
  EXPECT_TRUE(msg.R1());
  msg.R1(false);
  EXPECT_FALSE(msg.R1());

  msg.FrameDuration(123);
  EXPECT_EQ(msg.FrameDuration(), 123);

  EXPECT_EQ(BusLogStream::ErrorCount(), 0);
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

TEST(CanDataFrame, TestSerialize) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();

  CanDataFrame msg;;
  EXPECT_EQ(msg.Type(), BusMessageType::CAN_DataFrame);
  constexpr uint32_t kExtendedBit    = 0x80000000;
  constexpr uint32_t kCanId1 = 1234;
  constexpr uint32_t kMessageId = kCanId1 | kExtendedBit;

  msg.MessageId(kMessageId);
  msg.CanId(kCanId1);

  const std::vector<uint8_t> data = {1,2,3,4,5,6,7,8};
  msg.DataBytes(data);
  msg.Crc(0x12345);
  msg.Dir(true);
  msg.Srr(true);
  msg.Edl(true);
  msg.Brs(true);
  msg.Esi(true);
  msg.Rtr(true);
  msg.WakeUp(true);
  msg.SingleWire(true);
  msg.R0(true);
  msg.R1(true);
  msg.FrameDuration(123);

  std::vector<uint8_t> buffer;
  msg.ToRaw(buffer);

  CanDataFrame msg1;
  msg1.FromRaw(buffer);
  EXPECT_TRUE(msg1.Valid());
  EXPECT_EQ(msg1.MessageId(), kMessageId);
  EXPECT_EQ(msg1.CanId(), kCanId1);
  EXPECT_TRUE(msg1.ExtendedId());
  EXPECT_EQ(msg1.Dlc(), 8);
  EXPECT_EQ(msg1.DataLength(), 8);
  EXPECT_EQ(msg1.Size(), 34 + 8);
  EXPECT_EQ(msg1.Crc(), 0x12345);
  EXPECT_EQ(msg1.Dir(), true);
  EXPECT_EQ(msg1.Srr(), true);
  EXPECT_EQ(msg1.Edl(), true);
  EXPECT_EQ(msg1.Brs(), true);
  EXPECT_EQ(msg1.Esi(), true);
  EXPECT_EQ(msg1.Rtr(), true);
  EXPECT_EQ(msg1.WakeUp(), true);
  EXPECT_EQ(msg1.SingleWire(), true);
  EXPECT_EQ(msg1.R0(), true);
  EXPECT_EQ(msg1.R1(), true);
  EXPECT_EQ(msg1.FrameDuration(), 123);

  EXPECT_EQ(BusLogStream::ErrorCount(), 0);
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

}