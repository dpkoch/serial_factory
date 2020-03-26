#ifndef SERIAL_FACTORY_SERIAL_FACTORY_H
#define SERIAL_FACTORY_SERIAL_FACTORY_H

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <limits>
#include <type_traits>

#include "internal/checksum.h"
#include "internal/sfinae_helpers.h"
#include "internal/variadic_union.h"

#define SERIAL_MESSAGE __attribute__((packed))

namespace serial_factory
{

template <typename... Ts>
class SerialFactory
{
private:
  template <typename TargetType, typename ReturnType>
  using IfSupportedType = typename internal::IfIsTypeInList<TargetType, ReturnType, Ts...>;

public:

  static constexpr size_t MAX_PAYLOAD_SIZE = internal::maxSizeOf<Ts...>();

  // union Checksum
  // {
  //   struct __attribute__((packed))
  //   {
  //     uint8_t c1;
  //     uint8_t c2;
  //   };
  //   uint16_t checksum;
  // };

  struct __attribute__((packed)) GenericMessage
  {
    const uint8_t start_byte = START_BYTE;
    uint8_t id;
    uint8_t payload_size;
    union
    {
      uint8_t buffer[MAX_PAYLOAD_SIZE];
      internal::VariadicUnion<Ts...> msgs;
    } payload;
    uint16_t checksum;

    template <typename T>
    IfSupportedType<T, T> unpack()
    {
      return payload.msgs.template get<T>();
    }
  };

  static_assert(sizeof...(Ts) <= std::numeric_limits<decltype(GenericMessage::id)>::max()+1,
    "The number of message types must not be greater than can be expressed by the type of GenericMessage::id");

  static_assert(MAX_PAYLOAD_SIZE <= std::numeric_limits<decltype(GenericMessage::payload_size)>::max()+1,
    "The max payload size must not be larger than can be expressed by the type of GenericMessage::payload_size");

  static constexpr size_t BUFFER_SIZE = sizeof(GenericMessage);

  template <typename Target>
  static constexpr size_t id()
  {
    return internal::indexOfTypeInList<Target, Ts...>();
  }

  template <typename T>
  static IfSupportedType<T, size_t> send_to_buffer(uint8_t *dst, const T &msg)
  {
    dst[0] = START_BYTE;
    dst[1] = id<T>();
    dst[2] = sizeof(msg);
    std::memcpy(reinterpret_cast<void*>(dst+3), reinterpret_cast<const void*>(&msg), sizeof(msg));

    uint16_t checksum = compute_checksum(dst, 3+sizeof(msg));
    dst[3+sizeof(msg)] = 0; //! @todo actual checksum
    dst[4+sizeof(msg)] = 0; //! @todo actual checksum
  }

  bool parse_byte(uint8_t byte, GenericMessage &msg)
  {
    bool got_message = false;

    switch (parse_state)
    {
    case ParseState::IDLE:
      if (byte == START_BYTE)
      {
        parse_state = ParseState::GOT_START_BYTE;
      }
      break;
    case ParseState::GOT_START_BYTE:
      msg_buffer.id = byte;
      parse_state = ParseState::GOT_ID;
      break;
    case ParseState::GOT_ID:
      msg_buffer.payload_size = byte;
      if (msg_buffer.payload_size > 0)
      {
        parse_state = ParseState::GOT_LENGTH;
        payload_bytes_received = 0;
      }
      else
      {
        parse_state = ParseState::GOT_PAYLOAD;
      }
      break;
    case ParseState::GOT_LENGTH:
      msg_buffer.payload.buffer[payload_bytes_received++] = byte;
      if (payload_bytes_received >= msg_buffer.payload_size)
      {
        parse_state = ParseState::GOT_PAYLOAD;
      }
      break;
    case ParseState::GOT_PAYLOAD:
      checksum = 0; //! @todo Actual checksum
      parse_state = ParseState::GOT_CHECKSUM_1;
      break;
    case ParseState::GOT_CHECKSUM_1:
      checksum = 0; //! @todo Actual checksum
      if (checksum == compute_checksum(reinterpret_cast<const uint8_t*>(&msg_buffer), 3 + msg_buffer.payload_size))
      {
        got_message = true;
        std::memcpy(reinterpret_cast<void*>(&msg), reinterpret_cast<const void*>(&msg_buffer), sizeof(msg));
      }
      parse_state = ParseState::IDLE;
    }

    return got_message;
  }

private:
  static constexpr uint8_t START_BYTE = 0xBD;

  enum class ParseState
  {
    IDLE,
    GOT_START_BYTE,
    GOT_ID,
    GOT_LENGTH,
    GOT_PAYLOAD,
    GOT_CHECKSUM_1
  };

  static uint16_t compute_checksum(const uint8_t *src, size_t len)
  {
    return 0;
  }

  ParseState parse_state = ParseState::IDLE;
  size_t payload_bytes_received = 0;
  uint16_t checksum;
  GenericMessage msg_buffer;
};

} // namespace serial_factory

#endif // SERIAL_FACTORY_SERIAL_FACTORY_H
