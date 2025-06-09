/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include <string_view>
#include <string>
#include <array>
#include <iostream>
#include <filesystem>
#include <exception>

#include "../include/bus/buslogstream.h"
namespace {

constexpr std::array<std::string_view, 9> kSeverityList = {
  "Trace", "Debug", "Info", "Notice",
  "Warning", "Error", "Critical", "Alert",
  "Emergency"
};

}

namespace bus {

std::function<void(const std::source_location& location,
                         BusLogSeverity severity,
                         const std::string& text)>
  BusLogStream::UserLogFunction = BusLogStream::BusNoLogFunction;

std::atomic<uint64_t> BusLogStream::error_count_ = 0;

std::string_view BusLogServerityToText(BusLogSeverity severity) {
  const auto index = static_cast<uint8_t>(severity);
  return index < kSeverityList.size() ? kSeverityList[index] : "Unknown";
}

BusLogStream::BusLogStream(std::source_location location,
                           BusLogSeverity severity)
: severity_(severity),
  location_(location) {
}

BusLogStream::~BusLogStream() {
  BusLogStream::LogString(location_, severity_, str());
}

void BusLogStream::LogString(const std::source_location& location,
      BusLogSeverity severity,
      const std::string &text) {
  if (severity >= BusLogSeverity::kError) {
    ++error_count_;
  }
  if (UserLogFunction) {
    UserLogFunction(location, severity, text);
  }
}

void BusLogStream::BusConsoleLogFunction(const std::source_location& location,
    BusLogSeverity severity, const std::string& text) {
  std::string file;
  try {
    std::filesystem::path fullname(location.file_name());
    file = fullname.filename().string();
  } catch (const std::exception& ) {
    return;
  }
  std::cout << "[" << BusLogServerityToText(severity) << "] "
    << text << " "
    << "(" << file << "/" << location.function_name() << ":"
    << location.line() << ")" << std::endl;
}

void BusLogStream::BusNoLogFunction(const std::source_location& location,
    BusLogSeverity severity, const std::string& text) {
}

} // bus