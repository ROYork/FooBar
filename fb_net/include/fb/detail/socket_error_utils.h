#pragma once

#include <system_error>
#include <string>

namespace fb::detail
{

inline const std::error_category &native_socket_error_category() noexcept
{
#ifdef _WIN32
  return std::system_category();
#else
  return std::generic_category();
#endif
}

inline std::error_code make_native_error_code(int code) noexcept
{
  return std::error_code(code, native_socket_error_category());
}

[[noreturn]] inline void throw_system_error(int code, std::string message)
{
  throw std::system_error(make_native_error_code(code), std::move(message));
}

[[noreturn]] inline void throw_system_error(std::errc condition,
                                            std::string message)
{
  throw std::system_error(std::make_error_code(condition),
                          std::move(message));
}

} // namespace fb::detail

