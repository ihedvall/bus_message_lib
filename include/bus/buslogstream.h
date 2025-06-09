/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <cstdint>
#include <string_view>
#include <string>
#include <sstream>
#include <source_location>
#include <functional>


namespace bus {

/** \brief Defines the log severity level.
 *
 * Log severity level. Each log messaage havea severity attached.
 * Note that the severity number/level is identical with the
 * standard syslog severity.
 */
enum class BusLogSeverity : uint8_t {
  kTrace = 0,      ///< Trace or listen message
  kDebug = 1,      ///< Debug message
  kInfo = 2,       ///< Informational message
  kNotice = 3,     ///< Notice message. Notify the user.
  kWarning = 4,    ///< Warning message
  kError = 5,      ///< Error message
  kCritical = 6,   ///< Critical message (device error)
  kAlert = 7,      ///< Alert or alarm message
  kEmergency = 8   ///< Fatal error message
};

static std::string_view BusLogServerityToText(BusLogSeverity severity);

#define BUS_TRACE() BusLogStream(std::source_location::current(), BusLogSeverity::kTrace)
#define BUS_DEBUG() BusLogStream(std::source_location::current(), BusLogSeverity::kDebug)
#define BUS_INFO() BusLogStream(std::source_location::current(), BusLogSeverity::kInfo)
#define BUS_NOTICE() BusLogStream(std::source_location::current(), BusLogSeverity::kNotice)
#define BUS_WARNING() BusLogStream(std::source_location::current(), BusLogSeverity::kWarning)
#define BUS_ERROR() BusLogStream(std::source_location::current(), BusLogSeverity::kError)
#define BUS_CRITICAL() BusLogStream(std::source_location::current(), BusLogSeverity::kCritical)
#define BUS_ALERT() BusLogStream(std::source_location::current(), BusLogSeverity::kAlert)
#define BUS_EMERGENCY() BusLogStream(std::source_location::current(), BusLogSeverity::kEmergency)

class BusLogStream : public std::ostringstream {
public:
  BusLogStream() = delete;
  BusLogStream(std::source_location location, BusLogSeverity severity);
  ~BusLogStream() override;

  static std::function<void(const std::source_location& location,
                         BusLogSeverity severity,
                         const std::string& text)> UserLogFunction;

  static uint64_t ErrorCount() { return error_count_;}
  static void ResetErrorCount() { error_count_ = 0;}

  static void BusConsoleLogFunction(const std::source_location& location,
    BusLogSeverity severity, const std::string& text);

  static void BusNoLogFunction(const std::source_location& location,
    BusLogSeverity severity, const std::string& text);
private:
  BusLogSeverity severity_;
  std::source_location location_;
  static std::atomic<uint64_t> error_count_;

  static void LogString(const std::source_location& location,
                        BusLogSeverity severity,
                        const std::string& text);
};



} // bus


