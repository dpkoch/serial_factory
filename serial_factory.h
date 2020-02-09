#ifndef SERIAL_FACTORY_SERIAL_FACTORY_H
#define SERIAL_FACTORY_SERIAL_FACTORY_H

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <limits>
#include <type_traits>

#define SERIAL_MESSAGE __attribute__((packed))

namespace serial_factory
{

namespace internal
{
  // get max size of a list of types
  template <typename T>
  static constexpr size_t max_sizeof()
  {
    return sizeof(T);
  }

  template <typename T1, typename T2, typename... Ts>
  static constexpr size_t max_sizeof()
  {
    return sizeof(T1) > max_sizeof<T2, Ts...>() ? sizeof(T1) : max_sizeof<T2, Ts...>();
  }

  // get index of type in the list
  template <typename T1, typename T2, typename ReturnType = size_t>
  using IfSameType = std::enable_if_t<std::is_same<T1, T2>::value, ReturnType>;

  template <typename T1, typename T2, typename ReturnType = size_t>
  using IfNotSameType = std::enable_if_t<!std::is_same<T1, T2>::value, ReturnType>;

  template <typename Target, size_t index, typename Current, typename... Tail>
  // static constexpr std::enable_if_t<std::is_same<Target, Current>::value, size_t> get_index()
  static constexpr IfSameType<Target, Current> get_index()
  {
    return index;
  }

  template <typename Target, size_t index, typename Current, typename... Tail>
  // static constexpr std::enable_if_t<!std::is_same<Target, Current>::value, size_t> get_index()
  static constexpr IfNotSameType<Target, Current> get_index()
  {
    return get_index<Target, index + 1, Tail...>();
  }

  // check whether type is in a list of types
  template <typename Target>
  static constexpr bool is_in_list()
  {
    return false;
  }

  template <typename Target, typename Current, typename... Tail>
  static constexpr std::enable_if_t<std::is_same<Target, Current>::value, bool> is_in_list()
  {
    return true;
  }

  template <typename Target, typename Current, typename... Tail>
  static constexpr std::enable_if_t<!std::is_same<Target, Current>::value, bool> is_in_list()
  {
    return is_in_list<Target, Tail...>();
  }



  template <typename... Ts>
  union VariadicUnion {};

  template <typename T, typename... Ts>
  union VariadicUnion<T, Ts...> {
    T head;
    VariadicUnion<Ts...> tail;
  };

  template <typename... Ts>
  struct PayloadBuffer
  {
    union
    {
      uint8_t buffer[max_sizeof<Ts...>()];
      VariadicUnion<Ts...> msgs;
    } payload;

  private:
    template <typename TargetType, typename ReturnType>
    using IfSupportedType = typename std::enable_if_t<is_in_list<TargetType, Ts...>(), ReturnType>;

    template <typename Target, typename Head, typename... Tail>
    IfSameType<Target, Head, Target> get_msg(const VariadicUnion<Head, Tail...> &current) const
    {
      return current.head;
    }

    template <typename Target, typename Head, typename... Tail>
    IfNotSameType<Target, Head, Target> get_msg(const VariadicUnion<Head, Tail...> &current) const
    {
      return get_msg<Target, Tail...>(current.tail);
    }

  public:
    template <typename T>
    IfSupportedType<T, T> get() const
    {
      return get_msg<T, Ts...>(payload.msgs);
    }
  };
} // namespace internal





// //! @note Variadic union structure:
// struct FirstMessage {};
// struct SecondMessage {};
// struct ThirdMessage {};
// union X
// {
//   uint8_t buffer[20];
//   // VariadicUnion<Ts...> msgs;
//   union
//   {
//     FirstMessage msg;
//     union
//     {
//       SecondMessage msg;
//       union
//       {
//         ThirdMessage msg;
//       } tail;
//     } tail;
//   } tail;
// };
// then do something like `T msg = X.msgs.get<T>()`;





template <typename... Ts>
class SerialFactory
{
private:
  template <typename TargetType, typename ReturnType>
  using IfSupportedType = typename std::enable_if_t<internal::is_in_list<TargetType, Ts...>(), ReturnType>;

public:

  static constexpr size_t MAX_PAYLOAD_SIZE = internal::max_sizeof<Ts...>();

  struct __attribute__((packed)) GenericMessage
  {
    const uint8_t start_byte = START_BYTE;
    uint8_t id;
    uint8_t payload_size;
    // uint8_t payload[MAX_PAYLOAD_SIZE];
    internal::PayloadBuffer<Ts...> payload;
    uint16_t checksum;
  };

  static_assert(sizeof...(Ts) <= std::numeric_limits<decltype(GenericMessage::id)>::max()+1,
    "The number of message types must not be greater than can be expressed by the type of GenericMessage::id");

  static_assert(MAX_PAYLOAD_SIZE <= std::numeric_limits<decltype(GenericMessage::payload_size)>::max()+1,
    "The max payload size must not be larger than can be expressed by the type of GenericMessage::payload_size");

  static constexpr size_t BUFFER_SIZE = sizeof(GenericMessage);

  template <typename Target>
  static constexpr size_t id()
  {
    return internal::get_index<Target, 0, Ts...>();
  }

  template <typename T>
  // static std::enable_if_t<internal::is_in_list<T, Ts...>(), size_t> send_to_buffer(uint8_t *dst, const T &msg)
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

  template <typename T>
  // static std::enable_if_t<internal::is_in_list<T, Ts...>(), T> unpack(const GenericMessage &msg)
  static IfSupportedType<T, T> unpack(const GenericMessage &msg)
  {
    // union
    // {
    //   uint8_t (*buffer)[MAX_PAYLOAD_SIZE];
    //   T data;
    // } converter;

    // converter.buffer = msg.payload;
    // return converter.data;

    // return *reinterpret_cast<T*>(msg.payload);

    return msg.payload.template get<T>();
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
      buffer.id = byte;
      parse_state = ParseState::GOT_ID;
      break;
    case ParseState::GOT_ID:
      buffer.payload_size = byte;
      if (buffer.payload_size > 0)
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
      buffer.payload.payload.buffer[payload_bytes_received++] = byte;
      if (payload_bytes_received >= buffer.payload_size)
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
      if (checksum == compute_checksum(reinterpret_cast<const uint8_t*>(&buffer), 3 + buffer.payload_size))
      // if (true)
      {
        got_message = true;
        // msg = buffer; //! @todo should this be a memcpy?
        std::memcpy(reinterpret_cast<void*>(&msg), reinterpret_cast<const void*>(&buffer), sizeof(msg));
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
  GenericMessage buffer;
};

} // namespace serial_factory

#endif // SERIAL_FACTORY_SERIAL_FACTORY_H
