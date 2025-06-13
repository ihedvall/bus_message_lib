/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>

#include "bus/ibusmessage.h"
#include "bus/buslogstream.h"

namespace bus {

TEST(IBusMessage, TestProperties) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();

  IBusMessage msg(BusMessageType::Unknown);
  EXPECT_EQ(msg.Type(), BusMessageType::Unknown);

  msg.Version(33);
  EXPECT_EQ(msg.Version(), 33);

  EXPECT_EQ(msg.Size(), 18);

  msg.Timestamp(1234567);
  EXPECT_EQ(msg.Timestamp(), 1234567);

  msg.BusChannel(23);
  EXPECT_EQ(msg.BusChannel(), 23);

  EXPECT_EQ(BusLogStream::ErrorCount(), 0);
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

TEST(IBusMessage, TestSerialize) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();

  std::vector<uint8_t> buffer;
  {
    IBusMessage msg(BusMessageType::Unknown);
    msg.Version(33);
    msg.Timestamp(1234567);
    msg.BusChannel(23);

    msg.ToRaw(buffer);
  }

  EXPECT_EQ(buffer.size(), 18);
  {
    IBusMessage msg1;
    msg1.FromRaw(buffer);

    EXPECT_EQ(msg1.Type(), BusMessageType::Unknown);
    EXPECT_EQ(msg1.Version(), 33);
    EXPECT_EQ(msg1.Size(), 18);
    EXPECT_EQ(msg1.Timestamp(), 1234567);
    EXPECT_EQ(msg1.BusChannel(), 23);
  }

  EXPECT_EQ(BusLogStream::ErrorCount(), 0);
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

TEST(IBusMessage, TestValid) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();

  std::vector<uint8_t> buffer;
  {
    IBusMessage msg(BusMessageType::Unknown);
    msg.Version(33);
    msg.Timestamp(1234567);
    msg.BusChannel(23);

    msg.ToRaw(buffer);
  }

  buffer.resize(17); // Generate an invalid length

  {
    IBusMessage msg1;
    msg1.FromRaw(buffer);

    EXPECT_FALSE(msg1.Valid());
  }

  EXPECT_EQ(BusLogStream::ErrorCount(), 1);
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

}
