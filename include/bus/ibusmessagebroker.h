/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

/** \file ibusmessagebroker.h
 * \brief Defines an interface against borkers, servers and clients.
 *
 * Defines a generic interface against brokers, servers and clients.
 * The purpose is to hide the implementation against the end-user.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <string>
#include <mutex>

#include "ibusmessagequeue.h"

namespace bus {

/**
 * @brief The channel structure define a shared memory connection.
 */
struct Channel {
  bool used = false; ///< Indicate if the connection is used.
  uint32_t queue_index = 0; ///< Queue index of the connection.
};

/**
 * @brief Defines an interface to a broker, server or client.
 *
 * The user should use the BusInterFaceFactory::CreateBroker()
 * function when creating an object.
 */
class IBusMessageBroker {
public:
  IBusMessageBroker() = default; ///< Default constructor
  virtual ~IBusMessageBroker() = default;  ///< Default destructor

  /**
   * @brief Name of the shared memory.
   *
   * Name of the brokers shared memory.
   * This is also used when referencing the broker in log messages.
   * @param name Shared memory name.
   */
  void Name(std::string name);

  /**
   * @brief Returns the shared memory name.
   * @return Shared memory name.
   */
  [[nodiscard]] const std::string& Name() const;

  /**
   * @brief Sets the internal memory size.
   *
   * This function is deprecated as the shared memory is fixed to 16kB.
   * @param size Size of internal memory.
   */
  void MemorySize(uint32_t size) {memory_size_ = size; }

  /**
   * @brief Returns the internal memory size.
   * @return Internal memory size;
   */
  [[nodiscard]] uint32_t MemorySize() const { return memory_size_; }

  /**
   * @brief Sets the TCP/IP host address.
   *
   * The function set the TCP/IP address.
   * TCP/IP server uses only 2 addresses.
   * The address '127.0.0.1' means that only applications on this computer
   * can coonects to this server.
   * The address '0.0.0.0.' means that any computer on the network can
   * connect to this server.
   * @param address TCP/IP host address.
   */
  void Address(std::string address);

  /**
   * @brief Returns the TCP/IP address.
   * @return TCP/IP address.
   */
  [[nodiscard]] const std::string& Address() const;

  /**
   * @brief Sets the TCP/IP port.
   *
   * The port number should be between 1024 and 49151.
   * The internal standard is to use port numbers 42511 - 42998.
   * The util library uses the port range 42511 - 42610
   * This library uses the port range 42611 - 42710
   * @param port TCP/IP port.
   */
  void Port(uint16_t port) {port_ = port; }

  /**
   * @brief Returns the TCP/IP port
   * @return TCP/IP port.
   */
  [[nodiscard]] uint16_t Port() const { return port_; }

  /**
   * @brief Return true if the client is connected.
   *
   * This function is normally only used for client objects and
   * indicate if the client is connected to the server.
   * A client should wait until it is connected to send messages.
   * @return True if it is connected.
   */
  [[nodiscard]] bool IsConnected() const;

  /**
   * @brief Creates a publisker queue.
   *
   * Create a publisher queue.
   * A publisher send (push) messages while a subscriber receive (pop) messages.
   * @return Smart pointer to a message queue.
   */
  [[nodiscard]] virtual std::shared_ptr<IBusMessageQueue> CreatePublisher();

  /**
   * @brief Creates a subscriber queue.
   *
   * Create a subscriber queue.
   * A publisher send (push) messages while a subscriber receive (pop) messages.
   * @return Smart pointer to a message queue.
   */
  [[nodiscard]] virtual std::shared_ptr<IBusMessageQueue> CreateSubscriber();

  /**
   * @brief Detach a publisher from its broker.
   *
   * Detach the publisher from its broker.
   * This function is rarely used when using smart pointers.
   * @param publisher Smart pointer to a publisher.
   */
  void DetachPublisher(const std::shared_ptr<IBusMessageQueue>& publisher);

  /**
  * @brief Detach a subscriber from its broker.
  *
  * Detach the subscriber from its broker.
  * This function is rarely used when using smart pointers.
  * @param subscriber Smart pointer to a publisher.
  */
  void DetachSubscriber(const std::shared_ptr<IBusMessageQueue>& subscriber);

  /**
   * @brief Returns number of attached publishers.
   * @return Number of publishers
   */
  [[nodiscard]] size_t NofPublishers() const;

  /**
   * @brief Returns number of attached subscribers.
   * @return Number of subscribers.
   */
  [[nodiscard]] size_t NofSubscribers() const;

  /**
   * @brief Starts the broker.
   *
   * This function is called to start the broker. This start any needed working
   * thread and creating shared memory.
   */
  virtual void Start();

  /**
   * @brief Stops the broker.
   *
   * The function stop any working thread and removes any shared memory.
   */
  virtual void Stop();

protected:
  std::atomic<bool> connected_ = false; ///< True if the broker is connected.
  mutable std::mutex queue_mutex_; ///< Queue (lock) mutex.

  /** \brief List of attached publishers. */
  std::vector<std::shared_ptr<IBusMessageQueue>> publishers_;
  /** \brief List of attached subscribers. */
  std::vector<std::shared_ptr<IBusMessageQueue>> subscribers_;

  std::atomic<bool> stop_thread_ = false; ///< True if the thread shall stop.
  std::thread thread_; ///< Working thread

private:
  std::string name_;
  uint32_t memory_size_ = 16'000;
  std::string address_;
  uint16_t port_ = 0;

  void Poll(IBusMessageQueue& queue) const;
  void InprocessThread() const;
};

} // bus


