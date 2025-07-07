/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <string>

namespace bus {

enum class BusMessageType : uint16_t {
  Unknown = 0,
  CAN_DataFrame = 1,
  CAN_RemoteFrame = 2,
  CAN_ErrorFrame = 3,
  CAN_OverloadFrame = 4,
  CAN_BusWakeUp = 5,
  CAN_SingleWire = 6,
};

/**
 * \class IBusMessage ibusmessage.h "bus/ibusmessage.h"
 * \brief Abstract base class representing a generic bus message.
 *
 * This abstract base class is used to implement generic bus messages.
 * A message must have a specific type and a version.
 * The type and version, defines how the message is serialized.
 * The serialization are done with the ToRaw() and FromRaw() functions.
 * These functions are abstract and have to be implemented by the derived
 * class.
 *
 * The serialization of a bus message have a header which has
 * the same length.
 * The header is according to table below and uses little endian
 * byte order.
 * <table>
 * <caption id="IBusMessageLayout">Bus Message Header</caption>
 * <tr><th>Byte (Bits) Offset</th><th>Description</th><th>Size</th></tr>
 * <tr><td>0</td><td>Type of Message (enumerate)</td><td>uint16_t</td></tr>
 * <tr><td>2</td><td>Version Number</td><td>uint16_t</td></tr>
 * <tr><td>4</td><td>Length of the message</td><td>uint32_t</td></tr>
 * <tr><td>8</td><td>Timestamp ns since 1970</td><td>uint64_t</td></tr>
 * <tr><td>16</td><td>Bus Channel</td><td>uint16_t</td></tr>
 * </table>
 */
class IBusMessage {
public:
  IBusMessage() = default;
  explicit IBusMessage(BusMessageType type);
  virtual ~IBusMessage() = default;

  static std::shared_ptr<IBusMessage> Create(BusMessageType type);

  virtual void ToRaw(std::vector<uint8_t>& dest) const;
  virtual void FromRaw(const std::vector<uint8_t>& source);

  virtual std::string ToString(uint64_t loglevel)  const;

  [[nodiscard]] BusMessageType Type() const { return type_; }

  void Version(uint16_t version) { version_ = version; }
  [[nodiscard]] uint16_t Version() const { return version_; }

  [[nodiscard]] uint32_t Size() const { return size_;};
  [[nodiscard]] bool Valid() const { return valid_;};

  void Timestamp(uint64_t timestamp) { timestamp_ = timestamp; }
  [[nodiscard]] uint64_t Timestamp() const { return timestamp_; }

  void BusChannel(uint16_t channel) { bus_channel_ = channel; }
  [[nodiscard]] uint16_t BusChannel() const { return bus_channel_; }

protected:
  void Size(uint32_t size) const {size_ = size;}
  void Valid(bool valid) const { valid_ = valid;}

private:
  uint64_t timestamp_ = 0;
  BusMessageType type_ = BusMessageType::Unknown;
  uint16_t version_ = 0;
  uint8_t bus_channel_ = 0;

  mutable uint32_t size_ = 18;
  mutable bool valid_ = true;

};

} // bus

