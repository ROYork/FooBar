/// @file string_list.h
/// @brief A container for lists of strings with powerful manipulation methods
///
/// string_list wraps std::vector<std::string> and provides convenience methods
/// inspired by Python and modern C++ idioms. It is fully compatible with STL
/// algorithms and range-based for loops.
///
/// Features:
/// - STL-compatible container interface
/// - Factory methods for splitting strings
/// - Functional-style filter, map, and transform operations
/// - Powerful search and sorting capabilities
/// - Utility methods for common string list operations
///
/// Thread Safety:
/// - string_list is NOT thread-safe
/// - Create separate instances for multi-threaded use
///
/// Example:
/// @code
/// fb::string_list lines = fb::string_list::from_lines(text);
/// auto filtered = lines
///     .trim_all()
///     .remove_empty()
///     .filter_starts_with("#");
/// std::string result = filtered.join("\n");
/// @endcode

#pragma once

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

namespace fb
{

class string_list
{
public:
  // STL-compatible type aliases
  using value_type      = std::string;
  using size_type       = std::vector<std::string>::size_type;
  using difference_type = std::vector<std::string>::difference_type;
  using reference       = std::vector<std::string>::reference;
  using const_reference = std::vector<std::string>::const_reference;
  using pointer         = std::vector<std::string>::pointer;
  using const_pointer   = std::vector<std::string>::const_pointer;
  using iterator        = std::vector<std::string>::iterator;
  using const_iterator  = std::vector<std::string>::const_iterator;
  using reverse_iterator       = std::vector<std::string>::reverse_iterator;
  using const_reverse_iterator = std::vector<std::string>::const_reverse_iterator;

  // ========================================================================
  // Construction
  // ========================================================================

  string_list() = default;
  string_list(std::initializer_list<std::string> init);
  explicit string_list(const std::vector<std::string>& vec);
  explicit string_list(std::vector<std::string>&& vec);

  template<typename InputIt>
  string_list(InputIt first, InputIt last);

  string_list(const string_list& other) = default;
  string_list(string_list&& other) noexcept = default;
  string_list& operator=(const string_list& other) = default;
  string_list& operator=(string_list&& other) noexcept = default;
  ~string_list() = default;

  // ========================================================================
  // Factory Methods
  // ========================================================================

  static string_list from_split(std::string_view str,
                                char delimiter,
                                bool keep_empty = true);

  static string_list from_split(std::string_view str,
                                std::string_view delimiter,
                                bool keep_empty = true);

  static string_list from_lines(std::string_view str, bool keep_empty = true);

  // ========================================================================
  // Element Access
  // ========================================================================

  reference at(size_type index);
  const_reference at(size_type index) const;

  reference operator[](size_type index);
  const_reference operator[](size_type index) const;

  reference front();
  const_reference front() const;

  reference back();
  const_reference back() const;

  std::string* data() noexcept;
  const std::string* data() const noexcept;

  // ========================================================================
  // Iterators
  // ========================================================================

  iterator begin() noexcept;
  const_iterator begin() const noexcept;
  const_iterator cbegin() const noexcept;

  iterator end() noexcept;
  const_iterator end() const noexcept;
  const_iterator cend() const noexcept;

  reverse_iterator rbegin() noexcept;
  const_reverse_iterator rbegin() const noexcept;
  const_reverse_iterator crbegin() const noexcept;

  reverse_iterator rend() noexcept;
  const_reverse_iterator rend() const noexcept;
  const_reverse_iterator crend() const noexcept;

  // ========================================================================
  // Capacity
  // ========================================================================

  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] size_type size() const noexcept;
  [[nodiscard]] size_type max_size() const noexcept;
  void reserve(size_type new_cap);
  [[nodiscard]] size_type capacity() const noexcept;
  void shrink_to_fit();

  // ========================================================================
  // Modifiers
  // ========================================================================

  void clear() noexcept;

  iterator insert(const_iterator pos, const std::string& value);
  iterator insert(const_iterator pos, std::string&& value);

  iterator erase(const_iterator pos);
  iterator erase(const_iterator first, const_iterator last);

  void push_back(const std::string& value);
  void push_back(std::string&& value);

  template<typename... Args>
  reference emplace_back(Args&&... args);

  void pop_back();

  void resize(size_type count);
  void resize(size_type count, const std::string& value);

  void swap(string_list& other) noexcept;

  // ========================================================================
  // String List Operations
  // ========================================================================

  [[nodiscard]] std::string join(std::string_view delimiter) const;
  [[nodiscard]] std::string join(char delimiter) const;
  [[nodiscard]] std::string join() const;

  [[nodiscard]] string_list filter(std::function<bool(const std::string&)> predicate) const;
  [[nodiscard]] string_list map(std::function<std::string(const std::string&)> transform) const;

  [[nodiscard]] string_list filter_containing(std::string_view substr) const;
  [[nodiscard]] string_list filter_starts_with(std::string_view prefix) const;
  [[nodiscard]] string_list filter_ends_with(std::string_view suffix) const;
  [[nodiscard]] string_list filter_matching(const std::string& pattern) const;
  [[nodiscard]] string_list filter_matching(const std::regex& regex) const;

  // ========================================================================
  // Search Operations
  // ========================================================================

  [[nodiscard]] bool contains(std::string_view str) const;
  [[nodiscard]] bool contains_ignore_case(std::string_view str) const;
  [[nodiscard]] std::optional<size_type> index_of(std::string_view str) const;
  [[nodiscard]] std::optional<size_type> last_index_of(std::string_view str) const;
  [[nodiscard]] size_type count(std::string_view str) const;

  // ========================================================================
  // Sorting and Ordering
  // ========================================================================

  string_list& sort();
  string_list& sort(std::function<bool(const std::string&,
                                       const std::string&)> comp);
  string_list& sort_ignore_case();
  string_list& sort_natural();
  string_list& reverse();
  string_list& unique();
  string_list& shuffle();

  // ========================================================================
  // Utility Operations
  // ========================================================================

  string_list& remove_empty();
  string_list& remove_blank();
  string_list& trim_all();
  string_list& to_lower_all();
  string_list& to_upper_all();
  string_list& remove_duplicates();

  [[nodiscard]] string_list take(size_type n) const;
  [[nodiscard]] string_list skip(size_type n) const;
  [[nodiscard]] string_list take_last(size_type n) const;
  [[nodiscard]] string_list slice(size_type start, size_type end) const;

  // ========================================================================
  // Conversion
  // ========================================================================

  std::vector<std::string>& to_vector() noexcept;
  const std::vector<std::string>& to_vector() const noexcept;
  [[nodiscard]] std::vector<std::string> to_vector_copy() const;

  // ========================================================================
  // Operators
  // ========================================================================

  bool operator==(const string_list& other) const;
  bool operator!=(const string_list& other) const;

  [[nodiscard]] string_list operator+(const string_list& other) const;
  string_list& operator+=(const string_list& other);
  string_list& operator+=(const std::string& str);
  string_list& operator+=(std::string&& str);

  string_list& operator<<(const std::string& str);
  string_list& operator<<(std::string&& str);

private:
  std::vector<std::string> m_data;
};

// ============================================================================
// Template Implementations
// ============================================================================

/// Construct from iterator range
template<typename InputIt>
string_list::string_list(InputIt first, InputIt last)
  : m_data(first, last)
{
}

/// Construct element in place at end
template<typename... Args>
string_list::reference string_list::emplace_back(Args&&... args)
{
  return m_data.emplace_back(std::forward<Args>(args)...);
}

} // namespace fb
