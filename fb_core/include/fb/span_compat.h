#pragma once

/*
 * Lightweight C++17-compatible span implementation.
 *
 * Provides a subset of std::span functionality used across the FooBar codebase.
 * When compiling with C++20 or later, this header simply aliases fb::span to
 * std::span; otherwise it supplies a minimal drop-in replacement that supports:
 *   - construction from pointers, pointer ranges, std::array, and std::vector
 *   - element access, size queries, begin/end iterators
 *   - subspan(offset, count) with count defaulting to dynamic extent
 *
 * The intent is to keep public APIs consistent while allowing the project to
 * build on compilers that only support C++17.
 */

#include <array>
#include <cstddef>
#include <type_traits>
#include <vector>

#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
  #include <span>
#endif

namespace fb {

#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
using std::span;
inline constexpr std::size_t dynamic_extent = std::dynamic_extent;
#else

inline constexpr std::size_t dynamic_extent = static_cast<std::size_t>(-1);

template <typename T>
class span {
public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;
    using iterator = pointer;
    using const_iterator = pointer;

    static constexpr size_type extent = dynamic_extent;

    constexpr span() noexcept : m_ptr(nullptr), m_size(0) {}

    constexpr span(pointer ptr, size_type count) noexcept
        : m_ptr(ptr), m_size(count) {}

    constexpr span(pointer first_elem, pointer last_elem) noexcept
        : m_ptr(first_elem), m_size(static_cast<size_type>(last_elem - first_elem)) {}

    template <std::size_t N>
    constexpr span(element_type (&arr)[N]) noexcept
        : m_ptr(arr), m_size(N) {}

    template <typename U, std::size_t N,
              typename = std::enable_if_t<std::is_convertible<U (*)[], T (*)[]>::value>>
    constexpr span(std::array<U, N>& arr) noexcept
        : m_ptr(arr.data()), m_size(N) {}

    template <typename U, std::size_t N,
              typename = std::enable_if_t<std::is_convertible<const U (*)[], T (*)[]>::value>>
    constexpr span(const std::array<U, N>& arr) noexcept
        : m_ptr(arr.data()), m_size(N) {}

    template <typename U, typename Alloc,
              typename = std::enable_if_t<std::is_convertible<U (*)[], T (*)[]>::value>>
    span(std::vector<U, Alloc>& vec) noexcept
        : m_ptr(vec.data()), m_size(vec.size()) {}

    template <typename U, typename Alloc,
              typename = std::enable_if_t<std::is_convertible<const U (*)[], T (*)[]>::value>>
    span(const std::vector<U, Alloc>& vec) noexcept
        : m_ptr(vec.data()), m_size(vec.size()) {}

    template <typename U,
              typename = std::enable_if_t<std::is_convertible<U (*)[], T (*)[]>::value>>
    constexpr span(const span<U>& other) noexcept
        : m_ptr(other.data()), m_size(other.size()) {}

    constexpr iterator begin() const noexcept { return m_ptr; }
    constexpr iterator end() const noexcept { return m_ptr + m_size; }
    constexpr pointer data() const noexcept { return m_ptr; }
    constexpr size_type size() const noexcept { return m_size; }
    constexpr bool empty() const noexcept { return m_size == 0; }

    constexpr reference operator[](size_type index) const { return m_ptr[index]; }

    span subspan(size_type offset, size_type count = dynamic_extent) const noexcept {
        if (offset > m_size) {
            return span{};
        }

        const size_type remaining = m_size - offset;
        if (count == dynamic_extent || count > remaining) {
            count = remaining;
        }

        return span(m_ptr + offset, count);
    }

private:
    pointer m_ptr;
    size_type m_size;
};

// Deduction guides so fb::span(...) behaves like std::span under C++17
template <typename T>
span(T*, std::size_t) -> span<T>;

template <typename T>
span(T*, T*) -> span<T>;

template <typename T, std::size_t N>
span(T (&)[N]) -> span<T>;

template <typename T, std::size_t N>
span(std::array<T, N>&) -> span<T>;

template <typename T, std::size_t N>
span(const std::array<T, N>&) -> span<const T>;

template <typename T, typename Alloc>
span(std::vector<T, Alloc>&) -> span<T>;

template <typename T, typename Alloc>
span(const std::vector<T, Alloc>&) -> span<const T>;

#endif // std::span available

} // namespace fb
