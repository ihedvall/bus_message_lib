/*
 * Copyright 2025 Ingemar Hedvall
 * SPDX-License-Identifier: MIT
 */

/** \file littlebuffer.h
 * \brief Support class to handle byte order of various numeric values.
 */
#pragma once

#include <cstring>
#include <cstdint>
#include <vector>
#include <array>
#include <bit>

namespace bus {

/**
 * @brief Support class to handle byte order problems with numeric values.
 * @tparam T Type of numeric
 */
template <typename T>
class LittleBuffer {
 public:
  LittleBuffer() = default;

  /**
   * @brief Constructor that converts the input values.
   * @param value Value and its type.
   */
  explicit LittleBuffer(const T& value);

  /**
   * @brief Reads in a byte array and at an offset in that array
   * @param buffer Byte array.
   * @param offset Offset in that array.
   */
  LittleBuffer(const std::vector<uint8_t>& buffer, size_t offset);

  /**
   * @brief Reads in a value from a byte array from an offset.
   * @param buffer Pointer to the byte array.
   * @param offset Offset in the array.
   */
  LittleBuffer(const uint8_t* buffer, size_t offset);

  /**
   * @brief Returns an iterator to the first byte.
   * @return Constant iterator to the first byte.
   */
  [[nodiscard]] auto cbegin() const {
    return buffer_.cbegin();
  }

  /**
   * @brief Returns the an iterator to the last byte.
   * @return Const iterator to the last byte.
   */
  [[nodiscard]] auto cend() const {
    return buffer_.cend();
  }

  /**
   * @brief Returns a constant pointer to the internal byte array.
   * @return Constant pointer to the internal buffer.
   */
  [[nodiscard]] const uint8_t* data() const;

  /**
   * @brief Returns a pointer to the internal byte array.
   * @return Pointer to the internal byte array.
   */
  [[nodiscard]] uint8_t* data();

  /**
   * @brief Returns size of the value type.
   * @return Size of the value type.
   */
  [[nodiscard]] size_t size() const;

  /**
   * @brief Returns the value
   * @return Returns the value.
   */
  T value() const;

 private:
  std::array<uint8_t, sizeof(T)> buffer_;
};

template <typename T>
LittleBuffer<T>::LittleBuffer(const T& value) {
  if (constexpr bool big_endian = std::endian::native == std::endian::big;
    big_endian) { // Computer uses big endian
    std::array<uint8_t, sizeof(T)> temp = {0};
    memcpy(temp.data(), &value, sizeof(T));
    for (size_t index = sizeof(T); index > 0; --index) {
      buffer_[sizeof(T) - index] = temp[index - 1];
    }
  } else {
    std::memcpy(buffer_.data(), &value, sizeof(T));
  }
}

template <typename T>
LittleBuffer<T>::LittleBuffer(const std::vector<uint8_t>& buffer,
                              size_t offset) {
  std::memcpy(buffer_.data(), buffer.data() + offset, sizeof(T));
}

template <typename T>
LittleBuffer<T>::LittleBuffer(const uint8_t* buffer,
                              size_t offset) {
  if (buffer != nullptr) {
      std::memcpy(buffer_.data(), buffer + offset, sizeof(T));
  }
}

template <typename T>
const uint8_t* LittleBuffer<T>::data() const {
  return buffer_.data();
}

template <typename T>
uint8_t* LittleBuffer<T>::data() {
  return buffer_.data();
}
template <typename T>
size_t LittleBuffer<T>::size() const {
  return buffer_.size();
}

template <typename T>
T LittleBuffer<T>::value() const {

  std::array<uint8_t, sizeof(T)> temp = {0};
  if (  constexpr bool big_endian = std::endian::native == std::endian::big;
    big_endian) {
    for (size_t index = sizeof(T); index > 0; --index) {
      temp[sizeof(T) - index] = buffer_[index - 1];
    }
  } else {
    std::memcpy(temp.data(), buffer_.data(), sizeof(T));
  }
  const T* val = reinterpret_cast<const T*>(temp.data());
  return *val;
}

}  // namespace mdf
