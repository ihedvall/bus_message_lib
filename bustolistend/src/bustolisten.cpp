/*
 * Copyright 2025 Manoj Kumar
 * SPDX-License-Identifier: MIT
 */

#include "bustolisten.h"

#include <util/ixmlfile.h>
#include <util/listenconfig.h>
#include <util/logconfig.h>
#include <util/logstream.h>
#include <util/utilfactory.h>

#include <chrono>
#include <iostream>

#include "bus/interface/businterfacefactory.h"

using namespace util::log;
using namespace util::xml;
using namespace std::chrono_literals;

namespace bus {
bool BusToListen::stop_ = false;

void BusToListen::MainFunc() {
  // Set the log file name to the service name
  auto& log_config = LogConfig::Instance();
  log_config.Type(LogType::LogToFile);
  log_config.SubDir("utillib/log");
  log_config.BaseName("buslistend");
  log_config.CreateDefaultLogger();
  LOG_DEBUG() << "Log File created. Path: " << log_config.GetLogFile();
  auto broker =
      BusInterfaceFactory::CreateBroker(BrokerType::SharedMemoryBrokerType);
  broker->Name("SharedMemoryBroker");
  auto queue = broker->CreateSubscriber();
  queue->Start();
  auto listen_proxy = util::UtilFactory::CreateListen("ListenProxy", "LISBUS");
  listen_proxy->PreText(" BUS >");
  while (!stop_) {
    if (auto message = queue->PopWait(100ms); message) {
      std::string msgStr = message->ToString(listen_proxy->LogLevel());
      if (!msgStr.empty())
        listen_proxy->ListenTextEx(message->Timestamp(),
                                   listen_proxy->PreText(), "%s",
                                   msgStr.c_str());
    }
  }

  log_config.DeleteLogChain();
  // return EXIT_SUCCESS;
}
}  // namespace bus