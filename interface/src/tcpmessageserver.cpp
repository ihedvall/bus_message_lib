/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#include <chrono>

#include "tcpmessageserver.h"
#include "bus/buslogstream.h"
#include "tcpmessageconnection.h"

using namespace boost::asio;
using namespace boost::system;
using namespace std::chrono_literals;

namespace bus {
TcpMessageServer::TcpMessageServer()
  : IBusMessageBroker(),
     cleanup_timer_(context_) {
  tx_queue_ = std::make_shared<IBusMessageQueue>();
  tx_queue_->Start();

  rx_queue_ = std::make_shared<IBusMessageQueue>();
  rx_queue_->Start();
}

TcpMessageServer::~TcpMessageServer() {
  if (tx_queue_) {
    tx_queue_->Stop();
  }
  if (rx_queue_) {
    rx_queue_->Stop();
  }
  TcpMessageServer::Stop();
  tx_queue_.reset();
  rx_queue_.reset();
}

std::shared_ptr<IBusMessageQueue> TcpMessageServer::CreatePublisher() {
  return tx_queue_;
}

std::shared_ptr<IBusMessageQueue> TcpMessageServer::CreateSubscriber() {
  return rx_queue_;
}

void TcpMessageServer::Start() {
  Stop();
  connected_ = false;
  stop_server_thread_ = false;

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
    server_thread_ = std::thread(&TcpMessageServer::ServerThread, this);
    message_thread_ = std::thread(&TcpMessageServer::MessageThread, this);
    connected_ = true;
  } catch (const std::exception& error) {
    BUS_ERROR() << "Failed to start the server. Name: " << Name()
                << ", Error: " << error.what();
  }

}

void TcpMessageServer::Stop() {
  connected_ = false;
  stop_server_thread_ = true;

  if (!context_.stopped()) {
    context_.stop();
  }
  if (server_thread_.joinable()) {
    server_thread_.join();
  }
  if (message_thread_.joinable()) {
    message_thread_.join();
  }
  {
    std::lock_guard lock(connection_list_lock_);
    connection_list_.clear();
  }
}

void TcpMessageServer::DoAccept() {
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

void TcpMessageServer::DoCleanUp() {
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

void TcpMessageServer::ServerThread() {
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

void TcpMessageServer::MessageThread() const {
  while (!stop_server_thread_ && tx_queue_ && rx_queue_) {
    tx_queue_->EmptyWait(10ms);
    while (!tx_queue_->Empty()) {
      auto msg = tx_queue_->Pop();
      std::lock_guard lock(queue_mutex_);
      for (auto& subscriber : subscribers_) {
        if (subscriber) {
          subscriber->Push(msg);
        }
      }
    }
    {
      std::lock_guard lock(queue_mutex_);
      for (auto& publisher : publishers_) {
        while (publisher && !publisher->Empty()) {
          auto msg = publisher->Pop();
          rx_queue_->Push(msg);
        }
      }
    }
  }
}
} // bus