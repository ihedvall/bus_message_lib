/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

/** \file buslogstream.h
 * Defines an log interface for this library.
 *
 * The file defines an interface against a generic log system.
 *
 */

#pragma once

#include <cstdint>
#include <string_view>
#include <string>
#include <atomic>
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

/** \brief Support function that converts a severity code to text string. */
static std::string_view BusLogServerityToText(BusLogSeverity severity);

/** \brief Generates a trace log message. */
#define BUS_TRACE() BusLogStream(std::source_location::current(), BusLogSeverity::kTrace)

/** \brief Generates a debug log message. */
#define BUS_DEBUG() BusLogStream(std::source_location::current(), BusLogSeverity::kDebug)

/** \brief Generates an information log message. */
#define BUS_INFO() BusLogStream(std::source_location::current(), BusLogSeverity::kInfo)

/** \brief Generates a notice log message.*/
#define BUS_NOTICE() BusLogStream(std::source_location::current(), BusLogSeverity::kNotice)

/** \brief Generates a warning log message.*/
#define BUS_WARNING() BusLogStream(std::source_location::current(), BusLogSeverity::kWarning)

/** \brief Generates an error log message.*/
#define BUS_ERROR() BusLogStream(std::source_location::current(), BusLogSeverity::kError)

/** \brief Generates a critical log message.*/
#define BUS_CRITICAL() BusLogStream(std::source_location::current(), BusLogSeverity::kCritical)

/** \brief Generates an alert log message.*/
#define BUS_ALERT() BusLogStream(std::source_location::current(), BusLogSeverity::kAlert)

/** \brief Generates an emergency log message.*/
#define BUS_EMERGENCY() BusLogStream(std::source_location::current(), BusLogSeverity::kEmergency)

/** \brief Simple interface against a logging system.
 *
 * The class defines an API against a text logging system.
 * It doesn't implement the logging system itself.
 * Instead the end-user need to write some adpater code that
 * redirect the messages to their logging system.
 * The 'ihedvall/utillib' GitHub repository implements a logging system.
 */
class BusLogStream : public std::ostringstream {
public:
  BusLogStream() = delete;
 /** \brief Constructor that is a simple wrapper around an outpout stream.
  *
  * The end-user doesn't use this class directly.
  * Instead he/she use the log macros.
  * @param location Set by the macros to the current location.
  * @param severity Sets the severity level (syslog severity levels).
  */
BusLogStream(std::source_location location, BusLogSeverity severity);
  ~BusLogStream() override;

/** \brief The end-user should supply a function that redirect the logs.
 *
 * This callback function need to be set by the end-user.
 * If the function is used, all log messages are redirected by this
 * function.
 * If no function is set, no log messages are reocrded.
 */
  static std::function<void(const std::source_location& location,
                         BusLogSeverity severity,
                         const std::string& text)> UserLogFunction;

  /** \brief Returns number of error messages.
   *
   * Returns number of error messages generated.
   * More correctly it counts all severity code large or equal
   * to error.
   * This is typical used in test situations.
   * @return Number of severe error logs.
   */
  static uint64_t ErrorCount() { return error_count_; }

  /** \brief Resets the error counter. */
  static void ResetErrorCount() { error_count_ = 0;}

  /** \brief Simple function that sends all logs to the std::cout*/
  static void BusConsoleLogFunction(const std::source_location& location,
    BusLogSeverity severity, const std::string& text);
  /** \brief Simple function that doesn't do anything. */
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


