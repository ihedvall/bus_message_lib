/*
* Copyright 2023 Ingemar Hedvall
* SPDX-License-Identifier: MIT
 */
#include <array>

#include "bus/candataframe.h"

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

} // end namespace

namespace bus {

CanDataFrame::CanDataFrame() :
IBusMessage(BusMessageType::CAN_DataFrame) {

}

void CanDataFrame::MessageId(uint32_t msg_id) {
  message_id_ = msg_id;
  if (msg_id > k11BitMask) {
    // more than 11 bit means extended ID
    message_id_ |= kExtendedBit;
  }
}

uint32_t CanDataFrame::MessageId() const { return message_id_; }

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
  size_t record_size = 0;
  /*
    case MessageType::CAN_DataFrame:
      record_size = mandatory_only ? 23 - 8 : 28 - 8;
      record_size += save_index ? 8 : max_data_length;
      if (record.size() != record_size) {
        record.resize(record_size);
      }
      record[8] = BusChannel();
      MdfHelper::UnsignedToRaw(true, 0, 32, MessageId(), record.data() + 9);
      record[13] =  Dlc();
      record[14] = static_cast<uint8_t>(DataLength());
      if (mandatory_only) {
        if (save_index) {
          // The data index have in reality not been updated at this point, but
          // it will be updated when the sample buffer is written to the disc.
          // We need to save the data bytes to a temp buffer (VLSD data).
          sample.vlsd_data = true;
          sample.vlsd_buffer = data_bytes_;
          MdfHelper::UnsignedToRaw(true, 0, 64, data_index, record.data() + 15);
        } else {
          sample.vlsd_data = false;
          sample.vlsd_buffer.clear();
          sample.vlsd_buffer.shrink_to_fit();
          for (size_t index = 0; index < max_data_length; ++index) {
            record[15 + index] =
                index < data_bytes_.size() ? data_bytes_[index] : 0xFF;
          }
        }
        break;
      }
      record[13] |= (Dir() ? 0x01 : 0x00) << 4;
      record[13] |= (Srr() ? 0x01 : 0x00) << 5;
      record[13] |= (Edl() ? 0x01 : 0x00) << 6;
      record[13] |= (Brs() ? 0x01 : 0x00) << 7;
      record[15] = Esi() ? 0x01 : 0x00;
      record[15] |= (WakeUp() ? 0x01 : 0x00) << 1;
      record[15] |= (SingleWire() ? 0x01 : 0x00) << 2;
      record[15] |= (R0() ? 0x01 : 0x00) << 3;
      record[15] |= (R1() ? 0x01 : 0x00) << 4;
      MdfHelper::UnsignedToRaw(true, 0, 32, FrameDuration(), record.data() + 16);

      if (save_index) {
        // The data index have in reality not been updated at this point, but
        // it will be updated when the sample buffer is written to the disc.
        // We need to save the data bytes to a temp buffer (VLSD data).
        sample.vlsd_data = true;
        sample.vlsd_buffer = data_bytes_;
        MdfHelper::UnsignedToRaw(true, 0, 64, data_index, record.data() + 20);
      } else {
        sample.vlsd_data = false;
        sample.vlsd_buffer.clear();
        sample.vlsd_buffer.shrink_to_fit();
        for (size_t index = 0; index < max_data_length; ++index) {
          record[20 + index] =
              index < data_bytes_.size() ? data_bytes_[index] : 0xFF;
        }
      }
      break;

    case MessageType::CAN_RemoteFrame:
      record_size = mandatory_only ? 15 : 20 ;
      if (record.size() != record_size) {
        record.resize(record_size);
      }
      record[8] = BusChannel();
      MdfHelper::UnsignedToRaw(true, 0, 32, MessageId(), record.data() + 9);
      record[13] = Dlc() & 0x0F;
      record[14] = static_cast<uint8_t>(DataLength());
      if (mandatory_only) {
        break;
      }
      record[13] |= (Dir() ? 0x01 : 0x00) << 4;
      record[13] |= (Srr() ? 0x01 : 0x00) << 5;
      record[13] |= (WakeUp() ? 0x01 : 0x00) << 6;
      record[13] |= (SingleWire() ? 0x01 : 0x00) << 7;
      record[15] = R0() ? 0x01 : 0x00;
      record[15] |= (R1() ? 0x01 : 0x00) << 1;
      MdfHelper::UnsignedToRaw(true, 0, 32, FrameDuration(), record.data() + 16);
      break;

    case MessageType::CAN_ErrorFrame:
      record_size = 30 - 8;
      record_size += save_index ? 8 : max_data_length;
      if (record.size() < record_size) {
        record.resize(record_size);
      }
      record[8] = BusChannel();
      MdfHelper::UnsignedToRaw(true, 0, 32, MessageId(), record.data() + 9);
      record[13] = Dlc() & 0x0F;
      record[14] = DataLength();
      record[13] |= (Dir() ? 0x01 : 0x00) << 4;
      record[13] |= (Srr() ? 0x01 : 0x00) << 5;
      record[13] |= (Edl() ? 0x01 : 0x00) << 6;
      record[13] |= (Brs() ? 0x01 : 0x00) << 7;
      record[15] = Esi() ? 0x01 : 0x00;
      record[15] |= (WakeUp() ? 0x01 : 0x00) << 1;
      record[15] |= (SingleWire() ? 0x01 : 0x00) << 2;
      record[15] |= (R0() ? 0x01 : 0x00) << 3;
      record[15] |= (R1() ? 0x01 : 0x00) << 4;
      record[15] |= (static_cast<uint8_t>(ErrorType()) & 0x07) << 5;
      MdfHelper::UnsignedToRaw(true, 0, 32, FrameDuration(), record.data() + 16);
      MdfHelper::UnsignedToRaw(true, 0, 16, BitPosition(), record.data() + 20);

      if (save_index) {
        // The data index have in reality not been updated at this point, but
        // it will be updated when the sample buffer is written to the disc.
        // We need to save the data bytes to a temp buffer (VLSD data).
        sample.vlsd_data = true;
        sample.vlsd_buffer = data_bytes_;
        MdfHelper::UnsignedToRaw(true, 0, 64, data_index, record.data() + 22);
      } else {
        sample.vlsd_data = false;
        sample.vlsd_buffer.clear();
        sample.vlsd_buffer.shrink_to_fit();
        for (size_t index = 0; index < max_data_length; ++index) {
          record[22 + index] =
              index < data_bytes_.size() ? data_bytes_[index] : 0xFF;
        }
      }
      break;


    case MessageType::CAN_OverloadFrame:
      record_size = 10;
      if (record.size() != record_size) {
        record.resize(record_size);
      }
      record[8] = BusChannel();
      record[9] = Dir() ? 0x01 : 0x00;
      break;

    default:
      break;
  }
  */
}


}  // namespace mdf