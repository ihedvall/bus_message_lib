/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include <chrono>

#include "tcpmessageconnection.h"
#include "tcpmessagebroker.h"
#include "bus/buslogstream.h"
#include "bus/littlebuffer.h"

using namespace std::chrono_literals;
using namespace boost::asio;
using namespace boost::system;

namespace bus {

TcpMessageConnection::TcpMessageConnection(TcpMessageBroker& broker_,
    std::unique_ptr<boost::asio::ip::tcp::socket>& socket)
: broker_(broker_),
  socket_(std::move(socket)) {
  publisher_ = std::move(broker_.CreatePublisher());
  if (publisher_) {
    publisher_->Start();
  }

  subscriber_ = std::move(broker_.CreateSubscriber());
  if (subscriber_) {
    subscriber_->Start();
  }

  DoReadSize();
  stop_connection_thread_ = false;
  connection_thread_ = std::thread(&TcpMessageConnection::ConnectionThread, this);
}

TcpMessageConnection::~TcpMessageConnection() {
  if (publisher_) {
    publisher_->Stop();
  }
  if (subscriber_) {
    subscriber_->Stop();
  }
  stop_connection_thread_ = true;
  if (connection_thread_.joinable()) {
    connection_thread_.join();
  }

  publisher_.reset();
  subscriber_.reset();

}

bool TcpMessageConnection::CleanUp() const {
  return !socket_ || !socket_->is_open();
}

void TcpMessageConnection::DoReadSize() {  // NOLINT
  if (CleanUp()) {
    return;
  }

  async_read(*socket_, buffer(size_data_),
      [&](const boost::system::error_code& error, size_t bytes) {  // NOLINT
        if (error && error == error::eof) {
          BUS_INFO() << "Connection closed by remote";
          Close();
        } else if (error) {
          BUS_ERROR() << "Message size error. Error: " << error.message();
          Close();
        } else if (bytes != size_data_.size()) {
          BUS_ERROR() << "Message size length error. Error: "
                      << error.message();
          Close();
        } else {
          LittleBuffer<uint32_t> length(size_data_.data(), 0);
          if (length.value() > 0) {
            try {
              message_data_.resize(length.value(), 0);
              DoReadMessage();
            } catch (const std::exception& erre) {
              BUS_ERROR() << "Message allocation  error. Error: " << erre.what();
              Close();
            }
          } else {
            DoReadSize();
          }
        }
      });
}

void TcpMessageConnection::DoReadMessage() {  // NOLINT
  if (!socket_ || !socket_->is_open()) {
    return;
  }
  async_read(*socket_, boost::asio::buffer(message_data_),
      [&](const error_code& error, size_t bytes) {  // NOLINT
        if (error) {
          BUS_ERROR() << "Read message error. Error: " << error.message();
          Close();
        } else if (bytes != message_data_.size()) {
          BUS_ERROR() << "Message length error. Error: " << error.message();
          Close();
        } else {
          if (publisher_) {
            publisher_->Push(message_data_);
          }
          DoReadSize();
        }
      });
}

void TcpMessageConnection::Close() const {
  boost::system::error_code dummy;
  socket_->shutdown(ip::tcp::socket::shutdown_both, dummy);
  socket_->close(dummy);
}

void TcpMessageConnection::ConnectionThread() {
  while (!stop_connection_thread_ && subscriber_) {
    auto msg = subscriber_->PopWait(100ms);
    if (msg && socket_ && socket_->is_open()) {
      LittleBuffer length(msg->Size());
      try {
        std::vector<uint8_t> data;
        msg->ToRaw(data);
        send_data_.resize(msg->Size() + 4);
        std::copy_n(length.cbegin(), length.size(), send_data_.data());
        std::copy_n(data.cbegin(), data.size(),
          send_data_.data() + length.size());
        error_code error;
        socket_->send(buffer(send_data_));

      } catch (const std::exception& err) {
        BUS_ERROR() << "Send message error. Error: " << err.what();
      }
    }

  }
}

} // bus