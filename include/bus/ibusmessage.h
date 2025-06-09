/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

#pragma once
#include <cstdint>
#include <memory>
#include <vector>


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
 * <tr><th>Byte (Bits) Offset</th><th>Size</th><th>Description</th></tr>
 * <tr><td>0</td><td>uint16_t</td><td>Type of Message (enumerate)</td></tr>
 * <tr><td>2</td><td>uint16_t</td><td>Version Number</td></tr>
 * <tr><td>4</td><td>uint32_t</td><td>Length of the message</td></tr>
 * <tr><td>8</td><td>uint64_t</td><td>Timestamp ns since 1970</td></tr>
 * <tr><td>16</td><td>uint16_t</td><td>Bus Channel</td></tr>
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

  [[nodiscard]] BusMessageType Type() const { return type_; }

  void Version(uint16_t version) { version_ = version; }
  [[nodiscard]] uint16_t Version() const { return version_; }

  [[nodiscard]] uint32_t Size() const { return size_;};

  void Timestamp(uint64_t timestamp) { timestamp_ = timestamp; }
  [[nodiscard]] uint64_t Timestamp() const { return timestamp_; }

  void BusChannel(uint16_t channel) { bus_channel_ = channel; }
  [[nodiscard]] uint16_t BusChannel() const { return bus_channel_; }

protected:
  void Size(uint32_t size) {size_ = size;}


private:
  uint64_t timestamp_ = 0;
  BusMessageType type_ = BusMessageType::Unknown;
  uint16_t version_ = 0;
  uint8_t bus_channel_ = 0;
  uint32_t size_ = 18;

};

} // bus

