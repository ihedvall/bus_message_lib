/*
* Copyright 2025 Ingemar Hedvall
* SPDX-License-Identifier: MIT
*/

/** \file candataframe.h
 * \brief Simple wrapper around a CAN or CAN FD messages.
 *
 * The class is a simpler wrapper around a CAN message. It is used when
 * serializing CAN messages in a general way.
 */
#pragma once

 #include <bitset>
#include <cstdint>

#include "bus/ibusmessage.h"

namespace bus {
/** \brief Enumerate yhat defines type of CAN bus error. */
enum class CanErrorType : uint8_t {
  UNKNOWN_ERROR = 0, ///< Unspecified error.
  BIT_ERROR = 1,     ///< CAN bit error.
  FORM_ERROR = 2,    ///< CAN format error.
  BIT_STUFFING_ERROR = 3, ///< Bit stuffing error.
  CRC_ERROR = 4, ///< Checksum error.
  ACK_ERROR = 5 ///< Acknowledgement error.
};

/** \class CanDataFrame candataframe.h "bus/candataframe.h"
 * \brief Implements an interface against a CAN Data Frame message.
 *
 * The class implements an interface against a CAN Data Frame message.
 * This message is the normal CAN messages while the other messages
 * indicates some sort of exception or error.
 *
 * The serialization is according to table below and uses little endian
 * byte order.
 * <table>
 * <caption id="CanDataFrameLayout">CAN Data Frame Message Layout</caption>
 * <tr><th>Byte (Bits) Offset</th><th>Description</th><th>Size</th></tr>
 * <tr><td>0-17</td><td>Type of Message (enumerate)</td><td>18 bytes</td></tr>
 * <tr><td>18</td><td>Message ID (CAN Id + IDE)</td><td>uint32_t</td></tr>
 * <tr><td>22</td><td>DLC</td><td>uint8_t</td></tr>
 * <tr><td>23</td><td>Data Length</td><td>uint8_t</td></tr>
 * <tr><td>24</td><td>CRC</td><td>uint32_t</td></tr>
 * <tr><td>28:0</td><td>Direction (Rx=0, Tx=1)</td><td>1-bit</td></tr>
 * <tr><td>28:1</td><td>SRR</td><td>1-bit</td></tr>
 * <tr><td>28:2</td><td>EDL</td><td>1-bit</td></tr>
 * <tr><td>28:3</td><td>BRS</td><td>1-bit</td></tr>
 * <tr><td>28:4</td><td>ESI</td><td>1-bit</td></tr>
 * <tr><td>28:5</td><td>RTR</td><td>1-bit</td></tr>
 * <tr><td>28:6</td><td>R0</td><td>1-bit</td></tr>
 * <tr><td>28:7</td><td>R1</td><td>1-bit</td></tr>
 * <tr><td>29:0</td><td>Wake Up</td><td>1-bit</td></tr>
 * <tr><td>29:1</td><td>Single Wire</td><td>1-bit</td></tr>
 * <tr><td>30</td><td>Frame Duration (ns)/td><td>uint32_t</td></tr>
 * <tr><td>34</td><td>Data Bytes</td><td>Data Length bytes</td></tr>
 * </table>
 */
class CanDataFrame : public IBusMessage  {
 public:
    CanDataFrame();
    explicit CanDataFrame(CanErrorType type) = delete;
    explicit CanDataFrame(const std::shared_ptr<IBusMessage>& message);
  /** \brief DBC message ID. Note that bit 31 indicate extended ID.
   *
   * The message ID is the CAN ID + highest bit 31 set if the CAN ID
   * uses extended 29-bit ID. The message ID is actually used in a
   * DBC file and not sent on the CAN bus, well the extended bit is sent.
   * @param msg_id DBC Message ID
   */
  void MessageId(uint32_t msg_id);

  /** \brief DBC message ID. Note that bit 31 indicate extended ID.
   *
   * The message ID is the CAN ID with the highest bit 31 set if the
   * CAN ID uses a 29-bit address.
   * @return DBC message ID.
   */
  [[nodiscard]] uint32_t MessageId() const;

  void CanId(uint32_t can_id);

  /** \brief 29/11 bit CAN message ID. Note that bit 31 is not used.
   *
   *  The CAN message ID identifies the message on a CAN bus. Note that
   *  there is a DBC message ID which is the CAN ID + highest bit 31 set
   *  if the CAN ID uses 29-bit addressing. This is a confusing naming
   *  that causes invalid handling of messages.
   * @return 29-bit CAN ID.
   */
  [[nodiscard]] uint32_t CanId() const;

  /** \brief Set true if the CAN ID uses 20-bit addressing.
   *
   * @param extended True if CAN ID uses 29-bit addressing.
   */
  void ExtendedId(bool extended );

  /** \brief Returns true if the CAN ID uses 29-bit addressing,
   *
   * @return True if CAN ID uses extended addressing.
   */
  [[nodiscard]] bool ExtendedId() const; ///< Returns the extended CAN ID.

  /** \brief Sets the CAN message data length code.
   *
   * Sets the data length code (DLC). The DLC is the same as data length for
   * CAN but not for CAN FD. Note that the DataBytes() function fix both data
   * length and the DLC code so this function is normally not used.
   * @param dlc Data length code.
   */
  void Dlc(uint8_t dlc) { dlc_ = dlc;};

  /** \brief Returns the data length code (DLC).
   *
   * The DLC is equal to number of bytes for CAN messages but not for CAN FD
   * messages.
   *
   * @return Data length code for the message.
   */
  [[nodiscard]] uint8_t Dlc() const { return dlc_;};

  void Crc(uint32_t crc) { crc_ = crc;};
  [[nodiscard]] uint32_t Crc() const { return crc_;};

  /** \brief Sets number of data bytes.
   *
   * The data length is not sent on the bus. Instead is it calculated from
   * the DLC code. Note that the DataBytes() function fix both data
   * length and the DLC code so this function is normally not used.
   * @param data_length Number of payload data bytes.
   */
  void DataLength(uint8_t data_length);
  [[nodiscard]] uint8_t DataLength() const; ///< Returns number of data bytes.

  /** \brief Sets the payload data bytes.
   *
   * This function sets the payload data bytes in the message. Note that this
   * function also set the data length and DLC code.
   * @param data
   */
  void DataBytes(const std::vector<uint8_t>& data);

  /** \brief Returns a reference to the payload data bytes. */
  [[nodiscard]] const std::vector<uint8_t>& DataBytes() const;

  /** \brief If set true, the message was transmitted. */
  void Dir(bool transmit );
  /** \brief Returns true if the message was transmitted. */
  [[nodiscard]] bool Dir() const;

  void Srr(bool srr ); ///< Sets the SRR bit. */
  [[nodiscard]] bool Srr() const; ///< Returns the SRR bit

  void Edl(bool edl ); ///< Extended (CAN FD) data length.
  [[nodiscard]] bool Edl() const; ///< Extended (CAN FD) data length.

  void Brs(bool brs ); ///< Bit rate switch (CAN FD).
  [[nodiscard]] bool Brs() const; ///< Bit rate switch (CAN FD).

  void Esi(bool esi ); ///< Error state indicator (CAN FD).
  [[nodiscard]] bool Esi() const; ///< Error state indicator (CAN FD).

  void Rtr(bool rtr ); ///< Sets the RTR bit (remote frame).
  [[nodiscard]] bool Rtr() const; ///< Returns the RTR bit.

  void WakeUp(bool wake_up ); ///< Indicate a CAN bus wake up status
  [[nodiscard]] bool WakeUp() const; ///< Indicate a CAN bus wake up message

  void SingleWire(bool single_wire ); ///< Indicate a single wire CAN bus
  [[nodiscard]] bool SingleWire() const; ///< Indicate a single wire CAN bus

  void R0(bool flag); ///< Optional R0 flag.
  [[nodiscard]] bool R0() const; ///< Optional R0 flag

  void R1(bool flag); ///< Optional R1 flag.
  [[nodiscard]] bool R1() const; ///< Optional R1 flag
/*
  void BitPosition(uint16_t position); ///< Error bit position (error frame).
  [[nodiscard]] uint16_t BitPosition() const; ///< Error bit position.

  void ErrorType(CanErrorType error_type); ///< Type of error.
  [[nodiscard]] CanErrorType ErrorType() const; ///< Type of error.
*/
  void FrameDuration(uint32_t duration); ///< Frame duration in nano-seconds.
  [[nodiscard]] uint32_t FrameDuration() const; ///< Frame duration in nano-seconds.

  static size_t DlcToLength(uint8_t dlc); ///< Return the data length by DLC.

  /** \brief Creates an MDF sample record. Used primarily internally. */
  void ToRaw(std::vector<uint8_t>& dest) const override;
  void FromRaw(const std::vector<uint8_t>& source) override;
 private:
  uint32_t message_id_ = 0; ///< Message ID with bit 31 set if extended ID.
  uint8_t  dlc_ = 0; ///< Data length code.
  std::bitset<16> flags_;   ///< All CAN flags.
  std::vector<uint8_t> data_bytes_; ///< Payload data.
  uint16_t bit_position_ = 0; ///< Error bit position.
  CanErrorType error_type_ = CanErrorType::UNKNOWN_ERROR; ///< Error type.
  uint32_t frame_duration_ = 0;
  uint32_t crc_ = 0;
};

}  // namespace mdf
