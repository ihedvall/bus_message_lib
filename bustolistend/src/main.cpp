/*
 * Copyright 2022 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */
#include <util/ixmlfile.h>
#include <util/listenconfig.h>
#include <util/logconfig.h>
#include <util/logstream.h>
#include <util/utilfactory.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "bustolisten.h"
using namespace bus;
using namespace util::log;
using namespace util::xml;
using namespace std::chrono_literals;
namespace {
std::atomic<bool> kStopMain = false;

void StopMainHandler(int signal) {
  LOG_INFO() << "Stopping. Signal: " << signal;
  BusToListen::StopMessage();
}
}  // namespace

int main(int nof_arg, char* arg_list[]) {
  signal(SIGTERM, StopMainHandler);
  signal(SIGABRT, StopMainHandler);
#if (_MSC_VER)
  signal(SIGABRT_COMPAT, StopMainHandler);
  signal(SIGBREAK, StopMainHandler);
#endif

  BusToListen bus_message;
  std::vector<std::string> args;
  for (int i = 1; i < nof_arg; ++i) {
    args.push_back(arg_list[i]);
  }
  bus_message.MainFunc();
  kStopMain = false;
  return EXIT_SUCCESS;
}
