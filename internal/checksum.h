#ifndef SERIAL_FACTORY_CHECKSUM_H
#define SERIAL_FACTORY_CHECKSUM_H

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace serial_factory
{
namespace internal
{

/*
 * If S is the maximum value that SummandType can represent and A is the maximum
 * value that AccumulatorType can represent, then this constant is obtained by
 * solving the following equation for n (choosing the solution for which n > 0):
 *
 * n * (n + 1) * S / 2 < A
 */
template <typename SummandType, typename AccumulatorType>
constexpr size_t fletcherSummationOverflowLimit()
{
  static_assert(std::is_integral<SummandType>::value && std::is_integral<AccumulatorType>::value,
                "SummandType and AccumulatorType must both be integral types");

  long unsigned int S = std::numeric_limits<SummandType>::max();
  long unsigned int A = std::numeric_limits<AccumulatorType>::max();

  return static_cast<size_t>((std::sqrt(1 + 8 * A / S) - 1) / 2) - 1; // subtract one to ensure strict inequality
}


class __attribute__((packed)) Checksum
{
public:
  Checksum() {}
  Checksum(const void *src, size_t len) {}

  void compute(const uint8_t *src, size_t len)
  {
    uint32_t c1_buffer;
    uint32_t c2_buffer;

    for (size_t i = 0; i < len; i++)
    {
      c1_buffer += src[i];
      c2_buffer += c1_buffer;
    }

    c1 = c1_buffer % 255;
    c2 = c2_buffer % 255;

    // reset();
    // for (size_t i = 0; i < len; i++)
    // {
    //   update(src[i]);
    // }
    // finalize();
  }

  // void reset() {}
  // void update(uint8_t byte) {}
  // void finalize() {}

  bool operator==(const Checksum &other)
  {
    return c1 = other.c1 && c2 == other.c2;
  }

  static constexpr bool canHandleLength(size_t len)
  {
    return len <= fletcherSummationOverflowLimit<uint8_t, uint32_t>();
  }

private:
  uint8_t c1;
  uint8_t c2;
};

} // namespace internal
} // namespace serial_factory

#endif // SERIAL_FACTORY_CHECKSUM_H