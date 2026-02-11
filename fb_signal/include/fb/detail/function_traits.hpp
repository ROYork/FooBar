#pragma once

/// @file detail/function_traits.hpp
/// @brief Type traits for callable analysis

#include <cstddef>
#include <functional>
#include <type_traits>

namespace fb
{
namespace detail
{

/// @brief Extract function signature from callable types
template <typename T>
struct function_traits : function_traits<decltype(&T::operator())>
{
};

// Free function
template <typename Ret, typename... Args>
struct function_traits<Ret(Args...)>
{
  using return_type = Ret;
  using args_tuple = std::tuple<Args...>;
  static constexpr std::size_t arity = sizeof...(Args);

  template <std::size_t N>
  using arg = std::tuple_element_t<N, args_tuple>;
};

// Free function pointer
template <typename Ret, typename... Args>
struct function_traits<Ret (*)(Args...)> : function_traits<Ret(Args...)>
{
};

// Member function pointer
template <typename Class, typename Ret, typename... Args>
struct function_traits<Ret (Class::*)(Args...)>
{
  using return_type = Ret;
  using class_type = Class;
  using args_tuple = std::tuple<Args...>;
  static constexpr std::size_t arity = sizeof...(Args);

  template <std::size_t N>
  using arg = std::tuple_element_t<N, args_tuple>;
};

// Const member function pointer
template <typename Class, typename Ret, typename... Args>
struct function_traits<Ret (Class::*)(Args...) const>
{
  using return_type = Ret;
  using class_type = Class;
  using args_tuple = std::tuple<Args...>;
  static constexpr std::size_t arity = sizeof...(Args);

  template <std::size_t N>
  using arg = std::tuple_element_t<N, args_tuple>;
};

// Lambda and functor (via operator())
template <typename Class, typename Ret, typename... Args>
struct function_traits<Ret (Class::*)(Args...) const &>
    : function_traits<Ret(Args...)>
{
};

/// @brief Check if a type is callable with given arguments
template <typename F, typename... Args>
struct is_invocable_with
{
  static constexpr bool value = std::is_invocable_v<F, Args...>;
};

template <typename F, typename... Args>
inline constexpr bool is_invocable_with_v =
    is_invocable_with<F, Args...>::value;

/// @brief Check if type fits in small buffer optimization storage
template <typename T, std::size_t BufferSize>
struct fits_in_sbo
{
  static constexpr bool value = sizeof(T) <= BufferSize &&
                                alignof(T) <= alignof(std::max_align_t) &&
                                std::is_nothrow_move_constructible_v<T>;
};

template <typename T, std::size_t BufferSize>
inline constexpr bool fits_in_sbo_v = fits_in_sbo<T, BufferSize>::value;

} // namespace detail
} // namespace fb
