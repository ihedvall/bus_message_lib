/*
 * Copyright 2025 Manoj Kumar
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <string>
#include <vector>

namespace bus {

class BusToListen {
 public:
  std::vector<std::string> args_;
  void MainFunc();
  static void StopMessage() { stop_ = true; };

 private:
  static bool stop_;
};

}  // namespace bus