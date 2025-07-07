/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

/** \file ibusmessage.h
 * \brief Defines an interface against a bus message.
 *
 * Defines a virtual interface against all bus messages.
 * All messages inherit this class.
 */

#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include <string>


namespace bus {

/** \brief Defines all message types. */
enum class BusMessageType : uint16_t {
  Unknown = 0,
  Ctrl_BusChannel = 1,
  CAN_DataFrame = 10,
  CAN_RemoteFrame = 11,
  CAN_ErrorFrame = 12,
  CAN_OverloadFrame = 13,

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
   /** Contructor that creates a type of message.
   *
   * This constructor should not be used by end-user but
   * it is used for testing purpose.
   * @param type Type of message
   */
  explicit IBusMessage(BusMessageType type);

  virtual ~IBusMessage() = default;

  /** \brief Creates a message by its type.
   *
   * Creates a message by its type.
   * This fucntion is used by subscriber when deserialize a message.
   *
   * @param type Type of message.
   * @return Smart pointer to IBussMessage object.
   */
  static std::shared_ptr<IBusMessage> Create(BusMessageType type);

  /** \brief Serialize the message.
   *
   * The function serialize a message to a byte array.
   * This array is later used by the communication interfaces for
   * transfer the message in shared ememroy or TCP/IP.
   * The destination array is sized by the function.
   * @param dest Destination buffer.
   */
  virtual void ToRaw(std::vector<uint8_t>& dest) const;

  /** \brief Deserialize the message.
   *
   * This function desrialize a message from an byte array.
   * @param source Source buffer.
   */
  virtual void FromRaw(const std::vector<uint8_t>& source);


  virtual std::string ToString(uint64_t loglevel)  const;


  /** \brief Returns type of message.
   *
   * @return Type of message.
   */

  [[nodiscard]] BusMessageType Type() const { return type_; }

  /** \brief Sets the vewrsion number for a message.
   *
   * This function should rarely be used but defines the
   * version of the message.
   * This is used by the serialization and deserialization functions.
   * The version is changed when properties are added.
   * @param version Version number.
   */
  void Version(uint16_t version) { version_ = version; }
  /** \brief Returns the version number of the message.
   *
   * Rarely used function but used by the serialization and
   * deserialization functions.
   * @return Version number.
   */
  [[nodiscard]] uint16_t Version() const { return version_; }

  /** \brief Returns the total size of the message.
   *
   * The function return number of total message bytes,
   * not the paylod data size.
   * @return Number of total message bytes.
   */
  [[nodiscard]] uint32_t Size() const { return size_;};

  /** \brief Returns true if the message is valid.
   *
   * The message can be set invalid if the serialization
   * or deserialization fails. olle
   * @return True if the message is valid.
   */
  [[nodiscard]] bool Valid() const { return valid_;};

  /**
   * @brief Sets the absolute time.
   *
   * Sets the timestamp for the message.
   * The time is number of nanoseconds from midnight 1970-01-01.
   * The timezone is always UTC.
   * @param timestamp Nanoseconds sinncs 1970.
   */
  void Timestamp(uint64_t timestamp) { timestamp_ = timestamp; }

  /**
   * @brief Retuns the timestampe.
   *
   * The timestamp is defined as nanoseconds since midnight 1970-01-01.
   * The timezone is always UTC.
   * @return Time since 1970.
   */
  [[nodiscard]] uint64_t Timestamp() const { return timestamp_; }

  /**
   * @brief Sets the source channel.
   *
   * The bus channel is a number that defines the source of the message.
   * The source is typical a physical driver channel and not defining
   * any ECU device.
   *
   * Note that many manufactures of CAN communication card,
   * name the first channel as 'Channel 1' but uses channel 0 in its
   * software.
   * This causes a lot of problems and it doesn't help that the channel
   * number randomly changes between computer restarts.
   * It is recommended to start the channel number at 1 and
   * assign the first channel, the 'Channel 1' name.
   * The channel name should be assoiated with the device driver
   * serial number instead of its internal channel number.
   * @param channel Bus channel number.
   */
  void BusChannel(uint16_t channel) { bus_channel_ = channel; }

  /**
   * @brief Returns the bus channel number.
   *
   * The bus channel is a number that defines the source of the message.
   * The source is typical a physical driver channel and not defining
   * any ECU device.
   *
   * Note that many manufactures of CAN communication cards,
   * name the first channel as 'Channel 1' but uses channel 0 in its
   * software.
   * This causes a lot of problems and it doesn't help that the channel
   * number randomly changes between computer restarts.
   * It is recommended to start the channel number at 1 and
   * assign the first channel, the 'Channel 1' name.
   * The channel name should be assoiated with the device driver
   * serial number instead of its internal channel number.
   * @return Bus channel number.
   */
  [[nodiscard]] uint16_t BusChannel() const { return bus_channel_; }

protected:
  /**
   * @brief Sets the total size of the message.
   *
   * This function set the messsage size when serialize and
   * deserialize the message.
   *
   * @param size Message size (butes).
   */
  void Size(uint32_t size) const {size_ = size; }

 /**
  * @brief Sets the message valid or invalid.
  *
  * Sets the message valid or invalid.
  * By default the message is set valid (true).
  * @param valid Valid is true while invalid is false.
  */
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

