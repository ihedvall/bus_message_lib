/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

/** \file businterfacefactory.h
 * \brief The file implement a factory class for this library.
*/
#pragma once

#include <memory>

#include "bus/ibusmessagebroker.h"

namespace bus {

/** \brief Defines the types of brokers/servers and clients the
 * factory interface can create.
 *
 */
enum class BrokerType : int {
  SimulateBrokerType, ///< Only for internal test usage.
  SharedMemoryBrokerType, ///< Shared memory broker.
  SharedMemoryServerType, ///< Shared memory TX/RX server.
  SharedMemoryClientType, ///< Shared memory TX/RX client.
  TcpBrokerType, ///< Shared memory broker with a TCP/IP server.
  TcpServerType, ///< TCP/IP TX/RX server.
  TcpClientType, ///< TCP/IP TX/RX client.
};

/** \brief Factory class that create brokers/servers and clients.
 *
 * The bus interface factory creates brokers, servers and clients.
 *
 */
class BusInterfaceFactory {
public:
  /** \brief Create a broker/server or client.
   *
   * Creates a broker object and returns a generic
   * broker interface.
   *
   * @param type Type of broker/server or client to create.
   * @return Smart pointer (unique_ptr) to a IBusMessageBroker.
   */
  static std::unique_ptr<IBusMessageBroker> CreateBroker(BrokerType type);
};

} // bus::interface


