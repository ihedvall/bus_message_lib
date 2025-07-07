/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */
#include "bus/candataframe.h"

#include <array>
#include <iomanip>
#include <stdexcept>

#include "../include/bus/littlebuffer.h"
#include "bus/buslogstream.h"

namespace {

constexpr uint32_t kExtendedBit    = 0x80000000;
constexpr uint32_t k11BitMask      = 0x7FF;
constexpr size_t kDirBit         = 0;
constexpr size_t kSrrBit         = 1;
constexpr size_t kEdlBit         = 2;
constexpr size_t kBrsBit         = 3;
constexpr size_t kEsiBit         = 4;
constexpr size_t kWakeUpBit      = 5;
constexpr size_t kSingleWireBit  = 6;
constexpr size_t kRtrBit         = 7;
constexpr size_t kR0Bit          = 8;
constexpr size_t kR1Bit          = 9;

constexpr std::array<size_t,16> kDataLengthCode =
    {0,1,2,3,4,5,6,7,8,12,16,20,24,32,48,64};

constexpr uint32_t kCanDataFrameSize = 34;

} // end namespace

namespace bus {

CanDataFrame::CanDataFrame() : IBusMessage(BusMessageType::CAN_DataFrame) {
  Size(kCanDataFrameSize);
}

CanDataFrame::CanDataFrame(const std::shared_ptr<IBusMessage>& message)
  : CanDataFrame() {
  if (!message) {
    BUS_ERROR() << "NULL message pointer. Invalid use of function.";
    Valid(false);
    return;
  }

  const auto* msg = dynamic_cast<const CanDataFrame*>(message.get());
  if (msg == nullptr) {
    BUS_ERROR() << "Invalid message pointer. Invalid use of function.";
    Valid(false);
    return;
  }
  if (this != msg) {
    *this = *msg;;
  }
}

void CanDataFrame::MessageId(uint32_t msg_id) {
  message_id_ = msg_id;
  if (msg_id > k11BitMask) {
    // more than 11 bit means extended ID
    message_id_ |= kExtendedBit;
  }
}

uint32_t CanDataFrame::MessageId() const { return message_id_; }

void CanDataFrame::CanId(uint32_t can_id) {
  can_id &= ~kExtendedBit;
  message_id_ &= kExtendedBit;
  message_id_ |= can_id;
  if (can_id > k11BitMask) {
    message_id_ |= kExtendedBit;
  }
}

uint32_t CanDataFrame::CanId() const { return message_id_ & ~kExtendedBit; }

void CanDataFrame::ExtendedId(bool extended) {
  if (extended) {
    message_id_ |= kExtendedBit;
  } else {
    message_id_ &= ~kExtendedBit;
  }
}

bool CanDataFrame::ExtendedId() const {
  return (message_id_ & kExtendedBit) != 0;
}

void CanDataFrame::DataLength(uint8_t data_length) {
  if (data_length != data_bytes_.size()) {
    data_bytes_.resize(data_length);
  }

  uint8_t dlc = 0;
  for (const auto data_size : kDataLengthCode) {
    if (data_length <= data_size) {
      break;
    }
    ++dlc;
  }
  Dlc(dlc);
}

uint8_t CanDataFrame::DataLength() const {
  return static_cast<uint8_t>(data_bytes_.size());
}

void CanDataFrame::DataBytes(const std::vector<uint8_t>& data) {
  DataLength(static_cast<uint8_t>(data.size()));
  for (size_t index = 0; index < data.size() && index < data_bytes_.size(); ++index) {
    data_bytes_[index] = data[index];
  }
  Size(kCanDataFrameSize + data_bytes_.size());
}

const std::vector<uint8_t>& CanDataFrame::DataBytes() const {
  return data_bytes_;
}

void CanDataFrame::Dir(bool transmit) {
  flags_.set(kDirBit, transmit);
}

bool CanDataFrame::Dir() const {
  return flags_.test(kDirBit);
}

void CanDataFrame::Srr(bool srr) {
  flags_.set(kSrrBit, srr);
}

bool CanDataFrame::Srr() const {
  return flags_.test(kSrrBit);
}

void CanDataFrame::Edl(bool edl) {
  flags_.set(kEdlBit, edl);
}

bool CanDataFrame::Edl() const {
  return flags_.test(kEdlBit);
}

void CanDataFrame::Brs(bool brs) {
  flags_.set(kBrsBit, brs);
}

bool CanDataFrame::Brs() const {
  return flags_.test(kBrsBit);
}

void CanDataFrame::Esi(bool esi) {
  flags_.set(kEsiBit, esi);
}

bool CanDataFrame::Esi() const {
  return flags_.test(kEsiBit);
}

void CanDataFrame::Rtr(bool rtr) {
  flags_.set(kRtrBit, rtr);
}

bool CanDataFrame::Rtr() const {
  return flags_.test(kRtrBit);
}

void CanDataFrame::WakeUp(bool wake_up) {
  flags_.set(kWakeUpBit, wake_up);
}

bool CanDataFrame::WakeUp() const {
  return flags_.test( kWakeUpBit);
}

void CanDataFrame::SingleWire(bool single_wire) {
  flags_.set(kSingleWireBit, single_wire);
}

bool CanDataFrame::SingleWire() const {
  return flags_.test(kSingleWireBit);
}

void CanDataFrame::R0(bool flag) {
  flags_.set(kR0Bit, flag);
}

bool CanDataFrame::R0() const {
  return flags_.test(kR0Bit);
}

void CanDataFrame::R1(bool flag) {
  flags_.set(kR1Bit, flag);
}

bool CanDataFrame::R1() const {
  return flags_.test(kR1Bit);
}
/*
void CanDataFrame::BitPosition(uint16_t position) {
  bit_position_ = position;
}

uint16_t CanDataFrame::BitPosition() const {
  return bit_position_;
}

void CanDataFrame::ErrorType(CanErrorType error_type) {
  error_type_ =error_type;
}

CanErrorType CanDataFrame::ErrorType() const {
  return error_type_;
}
*/
void CanDataFrame::FrameDuration(uint32_t length) {
  frame_duration_ = length;
}

uint32_t CanDataFrame::FrameDuration() const {
  return frame_duration_;
}

size_t CanDataFrame::DlcToLength(uint8_t dlc) {
  return dlc < kDataLengthCode.size() ? kDataLengthCode[dlc] : 0;
}

void CanDataFrame::ToRaw(std::vector<uint8_t>& dest) const {
  Valid(true);
  Size(kCanDataFrameSize + DataLength());
  IBusMessage::ToRaw(dest);
  if (dest.size() != Size() || !Valid()) {
    BUS_ERROR() << "Allocation or size mismatch. Size: " << Size() << "/"
                << dest.size();
    Valid(false);
    return;
  }

  LittleBuffer<uint32_t> message_id(MessageId());
  std::copy_n(message_id.cbegin(), message_id.size(), dest.begin() + 18);

  dest[22] = Dlc();
  dest[23] = DataLength();

  LittleBuffer<uint32_t> crc(Crc());
  std::copy_n(crc.cbegin(), crc.size(), dest.begin() + 24);

  dest[28] = Dir() ? 0x01 : 0x00;
  dest[28] |= (Srr() ? 0x01 : 0x00) << 1;
  dest[28] |= (Edl() ? 0x01 : 0x00) << 2;
  dest[28] |= (Brs() ? 0x01 : 0x00) << 3;
  dest[28] |= (Esi() ? 0x01 : 0x00) << 4;
  dest[28] |= (Rtr() ? 0x01 : 0x00) << 5;
  dest[28] |= (R0() ? 0x01 : 0x00) << 6;
  dest[28] |= (R1() ? 0x01 : 0x00) << 7;

  dest[29] = WakeUp() ? 0x01 : 0x00;
  dest[29] |= (SingleWire() ? 0x01 : 0x00) << 8;

  LittleBuffer<uint32_t> frame_duration(FrameDuration());
  std::copy_n(frame_duration.cbegin(), frame_duration.size(),
  dest.begin() + 30);

  if (!data_bytes_.empty()) {
    std::copy_n(data_bytes_.cbegin(), data_bytes_.size(),
      dest.begin() + 34);
  }

}
void CanDataFrame::FromRaw(const std::vector<uint8_t>& source) {
  try {
    Size(source.size());
    if (Size() < kCanDataFrameSize) {
      std::ostringstream error;
      error << "CAN Data Frame message is to small. Size :" << kCanDataFrameSize
            << "/" << Size();
      throw std::runtime_error(error.str());
    }

    // Parse the header
    Valid(true);
    IBusMessage::FromRaw(source);

    if (!Valid()) {
      throw std::runtime_error("Message is not valid");
    }

    LittleBuffer<uint32_t> message_id(source, 18);
    MessageId(message_id.value());

    Dlc(source[22]);
    DataLength(source[23]);

    LittleBuffer<uint32_t> crc(source, 24);
    Crc(crc.value());

    Dir(source[28] & 0x01 != 0);
    Srr(source[28] & 0x02 != 0);
    Edl(source[28] & 0x04 != 0);
    Brs(source[28] & 0x08 != 0);
    Esi(source[28] & 0x10 != 0);
    Rtr(source[28] & 0x20 != 0);
    R0(source[28] & 0x40 != 0);
    R1(source[28] & 0x80 != 0);

    WakeUp(source[29] & 0x01 != 0);
    SingleWire(source[29] & 0x02 != 0);

    LittleBuffer<uint32_t> duration(source, 30);
    FrameDuration(duration.value());

    data_bytes_.resize(DataLength());
    std::copy_n(source.cbegin() + 34, DataLength(), data_bytes_.begin());
  } catch (const std::exception& err) {
    BUS_ERROR() << "Deserialization error. Error: " << err.what();
    Valid(false);
  }
}
std::string CanDataFrame::ToString(uint64_t loglevel) const {

  switch (loglevel) {
    case 0:
      break;;
    case 1:
      break;;
      default:
      return "";
  }
  std::ostringstream ss;
  ss << "Type:  CanDataFrame , ";
  ss << "CanId: " << CanId() << " ";

  std::ostringstream temp;
  for (unsigned char data_byte : data_bytes_) {
    temp  << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data_byte) << "  ";
  }
  ss << ", Data: " <<  temp.str() << " ";


  return ss.str();
}

}  // namespace mdf  LittleBuffer<uint32_t> crc(source, 24);
