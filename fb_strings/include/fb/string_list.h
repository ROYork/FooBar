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

/**
 * @brief A container for lists of strings with powerful manipulation methods
 *
 * string_list wraps std::vector<std::string> and provides convenience methods
 * inspired by Python and modern C++ idioms. It is fully compatible with STL
 * algorithms and range-based for loops.
 */
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

  /// Default constructor - creates empty list
  string_list() = default;

  /// Construct from initializer list
  string_list(std::initializer_list<std::string> init);

  /// Construct from vector
  explicit string_list(const std::vector<std::string>& vec);

  /// Construct from vector (move)
  explicit string_list(std::vector<std::string>&& vec);

  /// Construct from iterator range
  template<typename InputIt>
  string_list(InputIt first, InputIt last);

  /// Copy constructor
  string_list(const string_list& other) = default;

  /// Move constructor
  string_list(string_list&& other) noexcept = default;

  /// Copy assignment
  string_list& operator=(const string_list& other) = default;

  /// Move assignment
  string_list& operator=(string_list&& other) noexcept = default;

  /// Destructor
  ~string_list() = default;

  // ========================================================================
  // Factory Methods
  // ========================================================================

  /**
   * @brief Create string_list by splitting a string
   * @param str String to split
   * @param delimiter Character to split on
   * @param keep_empty Keep empty parts (default: true)
   * @return New string_list
   */
  static string_list from_split(std::string_view str,
                                char delimiter,
                                bool keep_empty = true);

  /**
   * @brief Create string_list by splitting a string
   * @param str String to split
   * @param delimiter String to split on
   * @param keep_empty Keep empty parts (default: true)
   * @return New string_list
   */
  static string_list from_split(std::string_view str,
                                std::string_view delimiter,
                                bool keep_empty = true);

  /**
   * @brief Create string_list from lines of text
   * @param str Multi-line string
   * @param keep_empty Keep empty lines (default: true)
   * @return New string_list
   */
  static string_list from_lines(std::string_view str, bool keep_empty = true);

  // ========================================================================
  // Element Access
  // ========================================================================

  /// Access element at index (bounds-checked)
  reference at(size_type index);
  const_reference at(size_type index) const;

  /// Access element at index (unchecked)
  reference operator[](size_type index);
  const_reference operator[](size_type index) const;

  /// Access first element
  reference front();
  const_reference front() const;

  /// Access last element
  reference back();
  const_reference back() const;

  /// Get underlying data pointer
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

  /// Check if list is empty
  bool empty() const noexcept;

  /// Get number of elements
  size_type size() const noexcept;

  /// Get maximum possible size
  size_type max_size() const noexcept;

  /// Reserve capacity
  void reserve(size_type new_cap);

  /// Get current capacity
  size_type capacity() const noexcept;

  /// Reduce capacity to fit size
  void shrink_to_fit();

  // ========================================================================
  // Modifiers
  // ========================================================================

  /// Clear all elements
  void clear() noexcept;

  /// Insert element at position
  iterator insert(const_iterator pos, const std::string& value);
  iterator insert(const_iterator pos, std::string&& value);

  /// Erase element at position
  iterator erase(const_iterator pos);
  iterator erase(const_iterator first, const_iterator last);

  /// Add element to end
  void push_back(const std::string& value);
  void push_back(std::string&& value);

  /// Construct element in place at end
  template<typename... Args>
  reference emplace_back(Args&&... args);

  /// Remove last element
  void pop_back();

  /// Resize list
  void resize(size_type count);
  void resize(size_type count, const std::string& value);

  /// Swap contents with another list
  void swap(string_list& other) noexcept;

  // ========================================================================
  // String List Operations
  // ========================================================================

  /**
   * @brief Join all strings with a delimiter
   * @param delimiter String to insert between elements
   * @return Joined string
   */
  std::string join(std::string_view delimiter) const;

  /**
   * @brief Join all strings with a character delimiter
   * @param delimiter Character to insert between elements
   * @return Joined string
   */
  std::string join(char delimiter) const;

  /**
   * @brief Join all strings with no delimiter
   * @return Concatenated string
   */
  std::string join() const;

  /**
   * @brief Filter elements by predicate
   * @param predicate Function returning true for elements to keep
   * @return New filtered string_list
   */
  string_list filter(std::function<bool(const std::string&)> predicate) const;

  /**
   * @brief Transform elements
   * @param transform Function to apply to each element
   * @return New transformed string_list
   */
  string_list map(std::function<std::string(const std::string&)> transform) const;

  /**
   * @brief Filter elements containing a substring
   * @param substr Substring to search for
   * @return New filtered string_list
   */
  string_list filter_containing(std::string_view substr) const;

  /**
   * @brief Filter elements starting with a prefix
   * @param prefix Prefix to check
   * @return New filtered string_list
   */
  string_list filter_starts_with(std::string_view prefix) const;

  /**
   * @brief Filter elements ending with a suffix
   * @param suffix Suffix to check
   * @return New filtered string_list
   */
  string_list filter_ends_with(std::string_view suffix) const;

  /**
   * @brief Filter elements matching a regex pattern
   * @param pattern Regex pattern
   * @return New filtered string_list
   */
  string_list filter_matching(const std::string& pattern) const;

  /**
   * @brief Filter elements matching a regex
   * @param regex Compiled regex
   * @return New filtered string_list
   */
  string_list filter_matching(const std::regex& regex) const;

  // ========================================================================
  // Search Operations
  // ========================================================================

  /**
   * @brief Check if list contains a string
   * @param str String to search for
   * @return true if found
   */
  bool contains(std::string_view str) const;

  /**
   * @brief Check if list contains a string (case-insensitive)
   * @param str String to search for
   * @return true if found
   */
  bool contains_ignore_case(std::string_view str) const;

  /**
   * @brief Find index of a string
   * @param str String to search for
   * @return Index or std::nullopt if not found
   */
  std::optional<size_type> index_of(std::string_view str) const;

  /**
   * @brief Find last index of a string
   * @param str String to search for
   * @return Index or std::nullopt if not found
   */
  std::optional<size_type> last_index_of(std::string_view str) const;

  /**
   * @brief Count occurrences of a string
   * @param str String to count
   * @return Number of occurrences
   */
  size_type count(std::string_view str) const;

  // ========================================================================
  // Sorting and Ordering
  // ========================================================================

  /**
   * @brief Sort list in ascending order
   * @return Reference to this
   */
  string_list& sort();

  /**
   * @brief Sort list with custom comparator
   * @param comp Comparison function
   * @return Reference to this
   */
  string_list& sort(std::function<bool(const std::string&,
                                       const std::string&)> comp);

  /**
   * @brief Sort list in ascending order (case-insensitive)
   * @return Reference to this
   */
  string_list& sort_ignore_case();

  /**
   * @brief Sort list using natural ordering (handles numbers)
   * @return Reference to this
   */
  string_list& sort_natural();

  /**
   * @brief Reverse the order of elements
   * @return Reference to this
   */
  string_list& reverse();

  /**
   * @brief Remove consecutive duplicate elements
   * @return Reference to this
   */
  string_list& unique();

  /**
   * @brief Shuffle elements randomly
   * @return Reference to this
   */
  string_list& shuffle();

  // ========================================================================
  // Utility Operations
  // ========================================================================

  /**
   * @brief Remove empty strings from list
   * @return Reference to this
   */
  string_list& remove_empty();

  /**
   * @brief Remove blank strings (empty or whitespace-only)
   * @return Reference to this
   */
  string_list& remove_blank();

  /**
   * @brief Trim whitespace from all elements
   * @return Reference to this
   */
  string_list& trim_all();

  /**
   * @brief Convert all elements to lowercase
   * @return Reference to this
   */
  string_list& to_lower_all();

  /**
   * @brief Convert all elements to uppercase
   * @return Reference to this
   */
  string_list& to_upper_all();

  /**
   * @brief Remove duplicate strings (preserves first occurrence)
   * @return Reference to this
   */
  string_list& remove_duplicates();

  /**
   * @brief Take first n elements
   * @param n Number of elements
   * @return New string_list with first n elements
   */
  string_list take(size_type n) const;

  /**
   * @brief Skip first n elements
   * @param n Number of elements to skip
   * @return New string_list without first n elements
   */
  string_list skip(size_type n) const;

  /**
   * @brief Take last n elements
   * @param n Number of elements
   * @return New string_list with last n elements
   */
  string_list take_last(size_type n) const;

  /**
   * @brief Get slice of list
   * @param start Start index
   * @param end End index (exclusive)
   * @return New string_list with elements [start, end)
   */
  string_list slice(size_type start, size_type end) const;

  // ========================================================================
  // Conversion
  // ========================================================================

  /**
   * @brief Get underlying vector
   * @return Reference to internal vector
   */
  std::vector<std::string>& to_vector() noexcept;
  const std::vector<std::string>& to_vector() const noexcept;

  /**
   * @brief Create a copy as vector
   * @return Copy of internal vector
   */
  std::vector<std::string> to_vector_copy() const;

  // ========================================================================
  // Operators
  // ========================================================================

  /// Equality comparison
  bool operator==(const string_list& other) const;
  bool operator!=(const string_list& other) const;

  /// Concatenate lists
  string_list operator+(const string_list& other) const;
  string_list& operator+=(const string_list& other);

  /// Append single string
  string_list& operator+=(const std::string& str);
  string_list& operator+=(std::string&& str);

  /// Append operator for chaining
  string_list& operator<<(const std::string& str);
  string_list& operator<<(std::string&& str);

private:
  std::vector<std::string> m_data;
};

// ============================================================================
// Template Implementations
// ============================================================================

template<typename InputIt>
string_list::string_list(InputIt first, InputIt last)
  : m_data(first, last)
{
}

template<typename... Args>
string_list::reference string_list::emplace_back(Args&&... args)
{
  return m_data.emplace_back(std::forward<Args>(args)...);
}

} // namespace fb
