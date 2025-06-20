/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <memory>
#include <thread>
#include <atomic>

#include <boost/asio.hpp>
#include "bus/ibusmessagequeue.h"

namespace bus {

class TcpMessageBroker;
class TcpMessageServer;

class TcpMessageConnection {
 public:
  TcpMessageConnection() = delete;
  TcpMessageConnection(TcpMessageBroker& broker,
                         std::unique_ptr<boost::asio::ip::tcp::socket>& socket);
  TcpMessageConnection(TcpMessageServer& server,
                         std::unique_ptr<boost::asio::ip::tcp::socket>& socket);
  virtual ~TcpMessageConnection();

  bool CleanUp() const;

 private:

  // TcpMessageBroker& broker_;
  std::unique_ptr<boost::asio::ip::tcp::socket> socket_;

  std::atomic<bool> stop_connection_thread_ = true;
  std::thread connection_thread_;

  std::shared_ptr<IBusMessageQueue> publisher_;
  std::shared_ptr<IBusMessageQueue> subscriber_;

  std::array<uint8_t, 4> size_data_;
  std::vector<uint8_t> message_data_;
  std::vector<uint8_t> send_data_;
  void DoReadSize();
  void DoReadMessage();
  void Close() const;

  void ConnectionThread();



};

} // bus


