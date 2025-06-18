/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include <chrono>

#include "tcpmessagebroker.h"
#include "bus/buslogstream.h"
#include "tcpmessageconnection.h"

using namespace boost::asio;
using namespace boost::system;
using namespace std::chrono_literals;

namespace bus {
TcpMessageBroker::TcpMessageBroker()
  : SharedMemoryBroker(),
    cleanup_timer_(context_) {

}

TcpMessageBroker::~TcpMessageBroker() {
  TcpMessageBroker::Stop();
}

void TcpMessageBroker::Start() {
  Stop();
  connected_ = false;
  if (Name().empty()) {
    return;
  }
  stop_server_thread_ = false;

    // Connect to the shared memory
  SharedMemoryBroker::Start();
  connected_ = false;
  if (context_.stopped()) {
    context_.restart();
  }
  // Start the TCP/IP server to accept connections
  try {
    if (Address().empty() || Address() == "0.0.0.0") {
      const auto address = ip::address_v4::any();
      const ip::tcp::endpoint endpoint(address, Port());
      acceptor_ = std::make_unique<ip::tcp::acceptor>(context_, endpoint);
    } else {
      const auto address = ip::make_address("127.0.0.1");
      const ip::tcp::endpoint endpoint(address, Port());
      acceptor_ = std::make_unique<ip::tcp::acceptor>(context_, endpoint);
    }
    DoAccept();
    DoCleanUp();
    server_thread_ = std::thread(&TcpMessageBroker::ServerThread, this);
    connected_ = true;
  } catch (const std::exception& error) {
    BUS_ERROR() << "Failed to start the server. Name: " << Name()
                << ", Error: " << error.what();
  }

}

void TcpMessageBroker::Stop() {
  connected_ = false;
  stop_server_thread_ = true;

  if (!context_.stopped()) {
    context_.stop();
  }
  if (server_thread_.joinable()) {
    server_thread_.join();
  }

  {
    std::lock_guard lock(connection_list_lock_);
    connection_list_.clear();
  }
  SharedMemoryBroker::Stop();
}

void TcpMessageBroker::DoAccept() {
  connection_socket_ = std::make_unique<ip::tcp::socket>(context_);
  acceptor_->async_accept(*connection_socket_,
    [&](const boost::system::error_code& err) {
        if (err) {
          connection_socket_.reset();

          BUS_ERROR() << "Accept error. Name: " << Name()
                      << ", Error: " << err.message();
        } else {
          {
            auto connection = std::make_unique<TcpMessageConnection>(
              *this, connection_socket_);
            std::lock_guard lock(connection_list_lock_);
            connection_list_.push_back(std::move(connection));
          }
          DoAccept();
        }
      });
}

void TcpMessageBroker::DoCleanUp() {
  cleanup_timer_.expires_after(2s);
  cleanup_timer_.async_wait([&](const error_code& error)-> void {
    if (error) {
      BUS_ERROR() << "Cleanup timer error. Name: " << Name()
                  << ", Error: " << error.message();
    } else {
      std::lock_guard lock(connection_list_lock_);
      std::erase_if(connection_list_, [] (auto& connection) -> bool {
        return !connection || connection->CleanUp();
      });
      DoCleanUp();
    }
  });
}

void TcpMessageBroker::ServerThread() {
  try {
    const auto& count = context_.run();
    BUS_TRACE() << "Stopped main worker thread. Name: " << Name()
                << ", Count: " << count;
  } catch (const std::exception& err) {
    if (!stop_server_thread_) {
      BUS_ERROR() << "Context error. Name: " << Name()
                  << ", Error: " << err.what();
    }
  }
}

} // bus