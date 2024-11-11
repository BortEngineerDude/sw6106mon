// By gh/BortEngineerDude
#pragma once

#include <bit>
#include <cstring>
#include <ios>
#include <stdexcept>
#include <stdint.h>
#include <type_traits>
#include <vector>

namespace bytes {
using byte = uint8_t; // std::byte sucks!
using std::endian;
using vect = std::vector<byte>;

template <typename T>
concept arithmetic = std::is_arithmetic<T>::value;

// C++23 std::byteswap can't swap anything but integers. But this can swap
// anything!
template <size_t size> inline void ___bswap(char *data);

template <> inline void ___bswap<1>(char *data) {}

template <> inline void ___bswap<2>(char *data) {
  auto *ptr = reinterpret_cast<uint16_t *>(data);
  *ptr = (*ptr >> 8) | (*ptr << 8);
}

template <> inline void ___bswap<4>(char *data) {
  // clang-format off
  // Otherwise this nice arrangement would be ruined
  auto *ptr = reinterpret_cast<uint32_t *>(data);
  *ptr = (*ptr >> 24) |
         ((*ptr & 0x00ff0000) >> 8) |
         ((*ptr & 0x0000ff00) << 8) |
         (*ptr << 24);
}

template <> inline void ___bswap<8>(char *data) {
  auto *ptr = reinterpret_cast<uint64_t *>(data);
  *ptr = (*ptr >> 56) |
         ((0x000000000000ff00ULL & *ptr) << 40) |
         ((0x0000000000ff0000ULL & *ptr) << 24) |
         ((0x00000000ff000000ULL & *ptr) << 8)  |
         ((0x000000ff00000000ULL & *ptr) >> 8 ) |
         ((0x0000ff0000000000ULL & *ptr) >> 24) |
         ((0x00ff000000000000ULL & *ptr) >> 40) |
         (*ptr << 56);
  // clang-format on
}

template <typename T> void byteswap(T &value) {
  ___bswap<sizeof(T)>(reinterpret_cast<char *>(&value));
}

template <arithmetic T> vect serialize(T val) {
  vect result(sizeof(T), 0);
  std::memcpy(result.data(), &val, sizeof(T));
  return result;
}

template <arithmetic T> vect le(T val) {
  if (endian::little != endian::native)
    byteswap(val);

  return serialize(val);
}

template <arithmetic T> vect be(T val) {
  if (endian::big != endian::native)
    byteswap(val);

  return serialize(val);
}

template <typename T>
concept ByteVect = std::is_same<std::remove_const_t<T>, vect>::value;

template <ByteVect Vect> class buffer {
  Vect &m_buffer;

  std::conditional<std::is_const<Vect>::value, typename Vect::const_iterator,
                   typename Vect::iterator>::type mutable m_current_point;
  mutable endian m_endianness;

  template <typename V>
  using disable_if_const =
      std::enable_if_t<std::is_const<Vect>::value == false, V>;

public:
  buffer(Vect &buf, endian e = endian::native)
      : m_buffer(buf), m_current_point(m_buffer.begin()), m_endianness(e) {}

  void seek(int distance,
            std::ios_base::seekdir direction = std::ios_base::cur) const {
    switch (direction) {
    case std::ios_base::beg:
      m_current_point = m_buffer.begin();
      break;
    case std::ios_base::end:
      m_current_point = m_buffer.end();
      break;
    case std::ios_base::cur:
    default:
      break;
    }

    m_current_point += distance;
  }

  uint tell() { return std::distance(m_buffer.begin(), m_current_point); }

  void set_endianness(endian e) const { m_endianness = e; }
  endian endianness() const { return m_endianness; }

  template <arithmetic T> buffer &operator>>(T &out) {
    if (std::distance(m_current_point, m_buffer.end()) < sizeof(T))
      throw std::out_of_range(
          "Trying to read a value past the end of the buffer.");

    std::memcpy(&out, &*m_current_point, sizeof(T));

    if (m_endianness != endian::native)
      byteswap(out);

    m_current_point += sizeof(T);

    return *this;
  }

  template <arithmetic T> const buffer &operator>>(T &out) const {
    if (std::distance(m_current_point, m_buffer.end()) < sizeof(T))
      throw std::out_of_range(
          "Trying to read a value past the end of the buffer.");

    std::memcpy(&out, &*m_current_point, sizeof(T));

    if (m_endianness != endian::native)
      byteswap(out);

    m_current_point += sizeof(T);

    return *this;
  }

  template <arithmetic T> disable_if_const<T> &operator<<(T in) {
    auto distance = std::distance(m_current_point, m_buffer.end());
    if (distance < sizeof(T)) {
      // preserve iterator...
      auto iterator_pos = std::distance(m_buffer.begin(), m_current_point);

      auto target_size = m_buffer.size() + sizeof(T) - distance;
      // ...otherwise iterator can be lost on resize.
      m_buffer.resize(target_size);

      m_current_point = m_buffer.begin() + iterator_pos;
    }

    if (m_endianness != endian::native)
      byteswap(in);

    memcpy(&*m_current_point, &in, sizeof(T));

    m_current_point += sizeof(T);

    return *this;
  }
};

} // namespace bytes
