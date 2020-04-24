#ifndef SERIAL_FACTORY_INTERNAL_CHECKSUM_H
#define SERIAL_FACTORY_INTERNAL_CHECKSUM_H

#include <cstddef>
#include <cstdint>

namespace serial_factory
{

using checksum_t = uint8_t;

namespace internal
{

// source: http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html#gab27eaaef6d7fd096bd7d57bf3f9ba083
inline uint8_t update_checksum(checksum_t current, uint8_t byte)
{
  uint8_t i;
  uint8_t data;

  data = current ^ byte;

  for (i = 0; i < 8; i++)
  {
    if ((data & 0x80) != 0)
    {
      data <<= 1;
      data ^= 0x07;
    }
    else
    {
      data <<= 1;
    }
  }
  return data;
}

checksum_t compute_checksum(const uint8_t *src, size_t len)
{
  checksum_t checksum = 0;
  for (size_t i = 0; i < len; i++)
  {
    checksum = update_checksum(checksum, src[i]);
  }
  return checksum;
}

} // namespace internal
} // namespace serial_factory


#endif // SERIAL_FACTORY_INTERNAL_CHECKSUM_H