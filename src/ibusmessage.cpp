/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/
#include <algorithm>
#include <stdexcept>

#include "bus/ibusmessage.h"
#include "bus/buslogstream.h"

#include "littlebuffer.h"

namespace bus {

IBusMessage::IBusMessage(BusMessageType type) : type_(type) {}

std::shared_ptr<IBusMessage> IBusMessage::Create(BusMessageType type) {
  std::shared_ptr<IBusMessage> message;
  switch (type) {

    default:
      message = std::make_shared<IBusMessage>(type);
      break;
  }
  return message;
}

void IBusMessage::ToRaw(std::vector<uint8_t>& dest) const {
  try {
    if (Size() < 18) {
      throw std::runtime_error(
          "IBusMessage::ToRaw() called with invalid length");
    }
    dest.resize(Size());

    LittleBuffer type(static_cast<uint16_t>(type_));
    LittleBuffer version(version_);
    LittleBuffer length(Size());
    LittleBuffer timestamp(timestamp_);
    LittleBuffer channel(bus_channel_);

    std::copy_n(type.cbegin(), type.size(), dest.begin());
    std::copy_n(version.cbegin(), version.size(), dest.begin() + 2);
    std::copy_n(length.cbegin(), length.size(), dest.begin() + 4);
    std::copy_n(timestamp.cbegin(), timestamp.size(), dest.begin() + 8);
    std::copy_n(channel.cbegin(), channel.size(), dest.begin() + 16);
  } catch (const std::exception& err) {
    BUS_ERROR() << "Message serialization errror. Error: " << err.what();
  }
}

void IBusMessage::FromRaw(const std::vector<uint8_t>& source) {
  try {
    if (source.size() < 18) {
      throw std::runtime_error("The input array is to small");
    }

    LittleBuffer<uint16_t> type(source, 0);
    LittleBuffer<uint16_t> version(source, 2);
    LittleBuffer<uint32_t> length(source, 4);
    LittleBuffer<uint64_t> timestamp(source, 8);
    LittleBuffer<uint16_t> channel(source, 16);

    type_ = static_cast<BusMessageType>(type.value());
    version_ = version.value();
    size_ = length.value();
    timestamp_ = timestamp.value();
    bus_channel_ = channel.value();

  } catch (const std::exception& err) {
    BUS_ERROR() << "Message deserialization errror. Error: " << err.what();
  }
}


} // bus