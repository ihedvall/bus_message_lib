/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */
#include <gtest/gtest.h>

#include "../../include/bus/littlebuffer.h"

namespace bus {
TEST(LittleBuffer, TEST_UNIT32) {
  std::vector<uint8_t> buffer(32, 0);

  for (uint32_t value = 0; value < 0x10000; ++value) {
    LittleBuffer value1(value);
    std::copy_n(value1.cbegin(), value1.size(), buffer.begin() + 5);
    LittleBuffer<uint32_t> value2(buffer, 5);
    ASSERT_EQ(value2.value(), value);
  }

}
}

