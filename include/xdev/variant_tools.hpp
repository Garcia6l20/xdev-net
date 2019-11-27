#pragma once

#include <type_traits>

namespace xdev {

template<class T>
struct always_false : std::false_type {};

template<class T>
inline constexpr bool always_false_v = always_false<T>::value;

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace xdev
