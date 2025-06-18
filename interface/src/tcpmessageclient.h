/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <atomic>
#include <thread>
#include <memory>
#include <array>
#include <vector>

#include <boost/asio.hpp>

#include "bus/ibusmessagebroker.h"

namespace bus {

class TcpMessageClient : public IBusMessageBroker {
public:
  TcpMessageClient();
  ~TcpMessageClient() override;

  void Start() override;
  void Stop() override;

private:
  /** The stop server task boolean is not used to stop the server thread.
   *  Instead does it actually suppress error message when the ASIO context
   *  is stopped.
   */
  std::atomic<bool> stop_client_thread_;

  std::thread client_thread_;
  boost::asio::io_context context_;
  boost::asio::ip::tcp::resolver lookup_;
  boost::asio::steady_timer retry_timer_;
  std::unique_ptr<boost::asio::ip::tcp::socket> socket_;
  boost::asio::ip::tcp::resolver::results_type endpoints_;

  std::array<uint8_t, 4> size_data_ = {0};
  std::vector<uint8_t> message_data_;

  boost::asio::steady_timer send_timer_;
  std::vector<uint8_t> send_data_;
  void ClientThread();

  void DoLookup();
  void DoRetryWait();
  void Close();
  void DoConnect();
  void DoReadSize();
  void DoReadMessage();
  void DoSendMessage();
  void DoSendWait();
};

} // bus

