#ifndef SERIAL_FACTORY_INTERNAL_SFINAE_HELPERS_H
#define SERIAL_FACTORY_INTERNAL_SFINAE_HELPERS_H

#include <cstddef>
#include <type_traits>

namespace serial_factory
{
namespace internal
{

// get max size of a list of types
template <typename T>
static constexpr size_t maxSizeOf()
{
  return sizeof(T);
}

template <typename T1, typename T2, typename... Ts>
static constexpr size_t maxSizeOf()
{
  return sizeof(T1) > maxSizeOf<T2, Ts...>() ? sizeof(T1) : maxSizeOf<T2, Ts...>();
}

// enable/disable function if types match
template <typename T1, typename T2, typename ReturnType>
using IfSameType = std::enable_if_t<std::is_same<T1, T2>::value, ReturnType>;

template <typename T1, typename T2, typename ReturnType>
using IfNotSameType = std::enable_if_t<!std::is_same<T1, T2>::value, ReturnType>;

// get index of type in a list of types
namespace implementation
{
  template <typename Target, size_t index, typename Current, typename... Tail>
  static constexpr IfSameType<Target, Current, size_t> indexOfTypeInList()
  {
    return index;
  }

  template <typename Target, size_t index, typename Current, typename... Tail>
  static constexpr IfNotSameType<Target, Current, size_t> indexOfTypeInList()
  {
    return indexOfTypeInList<Target, index + 1, Tail...>();
  }
} // namespace implementation

template <typename Target, typename... TypeList>
static constexpr size_t indexOfTypeInList()
{
  return implementation::indexOfTypeInList<Target, 0, TypeList...>();
}

// check if type is in a list of types
namespace implementation
{
  template <typename Target>
  static constexpr bool is_in_list()
  {
    return false;
  }

  template <typename Target, typename Current, typename... Tail>
  static constexpr IfSameType<Target, Current, bool> is_in_list()
  {
    return true;
  }

  template <typename Target, typename Current, typename... Tail>
  static constexpr IfNotSameType<Target, Current, bool> is_in_list()
  {
    return is_in_list<Target, Tail...>();
  }
} // namespace implementation

// enable function if type is in list of types
template <typename Target, typename ReturnType, typename... TypeList>
using IfIsTypeInList = typename std::enable_if_t<implementation::is_in_list<Target, TypeList...>(), ReturnType>;

} // namespace internal
} // namespace serial_factory

#endif // SERIAL_FACTORY_INTERNAL_SFINAE_HELPERS_H
