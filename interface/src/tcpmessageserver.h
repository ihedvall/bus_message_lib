/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>

#include <boost/asio.hpp>

#include "bus/ibusmessagebroker.h"
#include "tcpmessageconnection.h"

namespace bus {

class TcpMessageServer : public IBusMessageBroker {
public:
  TcpMessageServer();
  ~TcpMessageServer() override;
  [[nodiscard]] virtual std::shared_ptr<IBusMessageQueue> CreatePublisher();
  [[nodiscard]] virtual std::shared_ptr<IBusMessageQueue> CreateSubscriber();
  void Start() override;
  void Stop() override;
private:
  /** The stop server task boolean is not used to stop the server thread.
   *  Instead does it actually suppress error message when the ASIO constext
   *  is stopped.
   */
  std::atomic<bool> stop_server_thread_;
  std::thread server_thread_;
  std::thread message_thread_;

  boost::asio::io_context context_;

  std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
  std::unique_ptr<boost::asio::ip::tcp::socket> connection_socket_;

  mutable std::mutex connection_list_lock_;
  std::vector<std::unique_ptr<TcpMessageConnection>> connection_list_;

  boost::asio::steady_timer cleanup_timer_;
    // Common subscribers
  std::shared_ptr<IBusMessageQueue> tx_queue_;
  std::shared_ptr<IBusMessageQueue> rx_queue_;

  void DoAccept();
  void DoCleanUp();

  void ServerThread();
  void MessageThread() const;
};
;

} // bus


