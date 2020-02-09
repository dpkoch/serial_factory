#ifndef SERIAL_FACTORY_INTERNAL_VARIADIC_UNION_H
#define SERIAL_FACTORY_INTERNAL_VARIADIC_UNION_H

#include "sfinae_helpers.h"

namespace serial_factory
{
namespace internal
{

template <typename... Tail>
union VariadicUnion {};

template <typename Head, typename... Tail>
union VariadicUnion<Head, Tail...>
{
  Head head;
  VariadicUnion<Tail...> tail;

  template <typename Target>
  IfSameType<Target, Head, Target> get()
  {
    return head;
  }

  template <typename Target>
  IfNotSameType<Target, Head, Target> get()
  {
    return tail.template get<Target>();
  }
};

} // namespace internal
} // namespace serial_factory

#endif // SERIAL_FACTORY_INTERNAL_VARIADIC_UNION_H
