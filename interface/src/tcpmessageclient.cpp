/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include <chrono>
#include <boost/system.hpp>

#include "tcpmessageclient.h"
#include "bus/buslogstream.h"
#include "bus/littlebuffer.h"

using namespace std::chrono_literals;
using namespace boost::asio;
using namespace boost::system;

namespace bus {
TcpMessageClient::TcpMessageClient()
  : lookup_(context_),
    retry_timer_(context_),
    send_timer_(context_) {

}

TcpMessageClient::~TcpMessageClient() {
  TcpMessageClient::Stop();
}

void TcpMessageClient::Start() {
  Stop();
  if (context_.stopped()) {
    context_.restart();
  }
  connected_ = false;
  stop_client_thread_ = false;

  DoLookup();
  DoSendMessage();
  client_thread_ = std::thread(&TcpMessageClient::ClientThread, this);

  for (size_t timeout = 0; timeout < 20; timeout++) {
    if (IsConnected()) {
      break;
    }
    std::this_thread::sleep_for(100ms);
  }
}

void TcpMessageClient::Stop() {
  connected_ = false;
  stop_client_thread_ = true;
  if (!context_.stopped()) {
    context_.stop();
  }
  if (client_thread_.joinable()) {
    client_thread_.join();
  }
}

void TcpMessageClient::ClientThread() {
  try {
    const auto& count = context_.run();
    BUS_TRACE() << "Stopped main worker thread. Name: " << Name()
                << ", Count: " << count;
  } catch (const std::exception& err) {
    if (!stop_client_thread_) {
      BUS_ERROR() << "Context error. Name: " << Name()
                  << ", Error: " << err.what();
    }
  }
}

void TcpMessageClient::DoLookup() {
  connected_ = false;
  lookup_.async_resolve(
      ip::tcp::v4(), Address(), std::to_string(Port()),
      [&](const error_code& error, ip::tcp::resolver::results_type result) -> void {
        if (error) {
          BUS_ERROR() << "Lookup error. Host: " << Address() << ":" << Port()
                      << ",Error: (" << error.value() << ") " << error.message();
          DoRetryWait();
        } else {
          socket_ = std::make_unique<ip::tcp::socket>(context_);
          endpoints_ = std::move(result);
          DoConnect();
        }
      });
}

void TcpMessageClient::DoRetryWait() {
  connected_ = false;
  retry_timer_.expires_after(5s);
  retry_timer_.async_wait([&](const error_code error) {
    if (error) {
      BUS_ERROR() << "Retry timer error. Error: " << error.message();
    }
    DoLookup();
  });
  Close();
}

void TcpMessageClient::Close() {
  if (socket_ && socket_->is_open() && connected_) {
    error_code shutdown_error;
    error_code sh_error = socket_->shutdown(ip::tcp::socket::shutdown_both, shutdown_error);

    error_code close_error;
    error_code cl_error = socket_->close(close_error);
  }
  connected_ = false;
}

void TcpMessageClient::DoConnect() {
  connected_ = false;
  for (const auto& endpoint : endpoints_) {
    if (connected_) {
      break;
    }
    socket_->async_connect(endpoint, [&](const error_code error) {
      if (error.failed() || !socket_->is_open()) {
        BUS_ERROR() << "Connect error. Error: " << error.message();
        connected_ = false;
        DoRetryWait();
      } else {
        connected_ = true;
        DoReadSize();
      }
    });
  }
}

void TcpMessageClient::DoReadSize() {
  if (!socket_ || !socket_->is_open()) {
    DoRetryWait();
    return;
  }
  connected_ = true;
  async_read(*socket_, buffer(size_data_),
             [&](const error_code& error, size_t bytes) {  // NOLINT
               if (error && error == error::eof) {
                 BUS_INFO() << "Connection closed by remote";
                 DoRetryWait();
               } else if (error) {
                 BUS_ERROR() << "Reading size error. Error: "
                   << error.message();
                 DoRetryWait();
               } else if (bytes != size_data_.size()) {
                 BUS_ERROR() << "Reading size length error. Error: "
                             << error.message();
                 DoRetryWait();
               } else {
                 LittleBuffer<uint32_t> length(size_data_.data(), 0);
                 if (length.value() > 0) {
                   try {
                     message_data_.resize(length.value(), 0);
                     DoReadMessage();
                   } catch (const std::exception& err) {
                     BUS_ERROR() << "Allocation error. Size: " << length.value()
                      << ", Error: " << err.what();
                     DoRetryWait();
                   }

                 } else {
                   DoReadSize();
                 }
               }
             });
}

void TcpMessageClient::DoReadMessage() {  // NOLINT
  if (!socket_ || !socket_->is_open()) {
    DoRetryWait();
    return;
  }
  async_read(*socket_, buffer(message_data_),
             [&](const error_code& error, size_t bytes) {  // NOLINT
               if (error) {
                 BUS_ERROR() << "Read message data error. Error: " << error.message();
                 DoRetryWait();
               } else if (bytes != message_data_.size()) {
                 BUS_ERROR() << "Read message length error. Error: " << error.message();
                 DoRetryWait();
               } else {
                 {
                   std::lock_guard lock(queue_mutex_);
                   for (auto& subscriber : subscribers_) {
                     if (subscriber) {
                       subscriber->Push(message_data_);
                     }
                   }
                 }
                 DoReadSize();
               }
             });
}

void TcpMessageClient::DoSendMessage() {
  if (!connected_ || !socket_ || !socket_->is_open()) {
    DoSendWait();
    return;
  }
  std::lock_guard lock(queue_mutex_);
  for (auto& publisher : publishers_) {
    if (!publisher || publisher->Empty()) {
      continue;
    }
    auto msg = publisher->Pop();
    if (!msg || msg->Size() <= 0) {
      continue;
    }
    LittleBuffer<uint32_t> length(msg->Size());
    std::vector<uint8_t> data;
    msg->ToRaw(data);

    try {
      send_data_.resize(msg->Size() + 4, 0);
      std::copy_n(length.cbegin(), length.size(),
        send_data_.begin());
      std::copy_n(data.cbegin(), msg->Size(),
        send_data_.begin() + length.size());
    } catch (const std::exception& err) {
      BUS_ERROR() << "Send message allocation data error. Error: " << err.what();
      DoSendWait();
      return;
    }
    async_write(*socket_, buffer(send_data_),
      [&](const error_code& error, size_t bytes) -> void {
        if (error) {
          BUS_ERROR() << "Send message data error. Error: " << error.message();
        }
        DoSendMessage();
      });
    return;
  }
  // Nothing to send
  DoSendWait();
}

void TcpMessageClient::DoSendWait() {
  send_timer_.expires_after(10ms);
  send_timer_.async_wait([&](const error_code error) ->void {
    if (error) {
      BUS_ERROR() << "Send timer error. Error: " << error.message();
    }
    DoSendMessage();
  });
}

} // bus