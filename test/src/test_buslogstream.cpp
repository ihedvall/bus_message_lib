/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include "bus/buslogstream.h"

namespace bus {

TEST(BusLogStream, TestConsole) {
  BUS_TRACE() << "Shall not be shown.";
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BUS_TRACE() << "Shall be shown.";
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

TEST(BusLogStream, TestSeverity) {
  BusLogStream::UserLogFunction = BusLogStream::BusConsoleLogFunction;
  BusLogStream::ResetErrorCount();

  BUS_TRACE() << "Trace message.";
  EXPECT_EQ(BusLogStream::ErrorCount(), 0);

  BUS_DEBUG() << "Debug message.";
  EXPECT_EQ(BusLogStream::ErrorCount(), 0);

  BUS_INFO() << "Info message.";
  EXPECT_EQ(BusLogStream::ErrorCount(), 0);

  BUS_NOTICE() << "Notice message.";
  EXPECT_EQ(BusLogStream::ErrorCount(), 0);

  BUS_WARNING() << "Warning message.";
  EXPECT_EQ(BusLogStream::ErrorCount(), 0);

  BUS_ERROR() << "Error message.";
  EXPECT_EQ(BusLogStream::ErrorCount(), 1);

  BUS_CRITICAL() << "Critical message.";
  EXPECT_EQ(BusLogStream::ErrorCount(), 2);

  BUS_ALERT() << "Alert message.";
  EXPECT_EQ(BusLogStream::ErrorCount(), 3);

  BUS_EMERGENCY() << "Emergency message.";
  EXPECT_EQ(BusLogStream::ErrorCount(), 4);

  BusLogStream::ResetErrorCount();
  EXPECT_EQ(BusLogStream::ErrorCount(), 0);

  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;
}

}
