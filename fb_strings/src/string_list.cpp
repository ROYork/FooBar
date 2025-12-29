#include <fb/string_list.h>
#include <fb/string_utils.h>

#include <algorithm>
#include <random>
#include <unordered_set>

namespace fb
{

// ============================================================================
// Construction
// ============================================================================

string_list::string_list(std::initializer_list<std::string> init)
  : m_data(init)
{
}

string_list::string_list(const std::vector<std::string>& vec)
  : m_data(vec)
{
}

string_list::string_list(std::vector<std::string>&& vec)
  : m_data(std::move(vec))
{
}

// ============================================================================
// Factory Methods
// ============================================================================

string_list string_list::from_split(std::string_view str,
                                    char delimiter,
                                    bool keep_empty)
{
  return string_list(fb::split(str, delimiter, keep_empty));
}

string_list string_list::from_split(std::string_view str,
                                    std::string_view delimiter,
                                    bool keep_empty)
{
  return string_list(fb::split(str, delimiter, keep_empty));
}

string_list string_list::from_lines(std::string_view str, bool keep_empty)
{
  return string_list(fb::split_lines(str, keep_empty));
}

// ============================================================================
// Element Access
// ============================================================================

string_list::reference string_list::at(size_type index)
{
  return m_data.at(index);
}

string_list::const_reference string_list::at(size_type index) const
{
  return m_data.at(index);
}

string_list::reference string_list::operator[](size_type index)
{
  return m_data[index];
}

string_list::const_reference string_list::operator[](size_type index) const
{
  return m_data[index];
}

string_list::reference string_list::front()
{
  return m_data.front();
}

string_list::const_reference string_list::front() const
{
  return m_data.front();
}

string_list::reference string_list::back()
{
  return m_data.back();
}

string_list::const_reference string_list::back() const
{
  return m_data.back();
}

std::string* string_list::data() noexcept
{
  return m_data.data();
}

const std::string* string_list::data() const noexcept
{
  return m_data.data();
}

// ============================================================================
// Iterators
// ============================================================================

string_list::iterator string_list::begin() noexcept
{
  return m_data.begin();
}

string_list::const_iterator string_list::begin() const noexcept
{
  return m_data.begin();
}

string_list::const_iterator string_list::cbegin() const noexcept
{
  return m_data.cbegin();
}

string_list::iterator string_list::end() noexcept
{
  return m_data.end();
}

string_list::const_iterator string_list::end() const noexcept
{
  return m_data.end();
}

string_list::const_iterator string_list::cend() const noexcept
{
  return m_data.cend();
}

string_list::reverse_iterator string_list::rbegin() noexcept
{
  return m_data.rbegin();
}

string_list::const_reverse_iterator string_list::rbegin() const noexcept
{
  return m_data.rbegin();
}

string_list::const_reverse_iterator string_list::crbegin() const noexcept
{
  return m_data.crbegin();
}

string_list::reverse_iterator string_list::rend() noexcept
{
  return m_data.rend();
}

string_list::const_reverse_iterator string_list::rend() const noexcept
{
  return m_data.rend();
}

string_list::const_reverse_iterator string_list::crend() const noexcept
{
  return m_data.crend();
}

// ============================================================================
// Capacity
// ============================================================================

bool string_list::empty() const noexcept
{
  return m_data.empty();
}

string_list::size_type string_list::size() const noexcept
{
  return m_data.size();
}

string_list::size_type string_list::max_size() const noexcept
{
  return m_data.max_size();
}

void string_list::reserve(size_type new_cap)
{
  m_data.reserve(new_cap);
}

string_list::size_type string_list::capacity() const noexcept
{
  return m_data.capacity();
}

void string_list::shrink_to_fit()
{
  m_data.shrink_to_fit();
}

// ============================================================================
// Modifiers
// ============================================================================

void string_list::clear() noexcept
{
  m_data.clear();
}

string_list::iterator string_list::insert(const_iterator pos,
                                          const std::string& value)
{
  return m_data.insert(pos, value);
}

string_list::iterator string_list::insert(const_iterator pos,
                                          std::string&& value)
{
  return m_data.insert(pos, std::move(value));
}

string_list::iterator string_list::erase(const_iterator pos)
{
  return m_data.erase(pos);
}

string_list::iterator string_list::erase(const_iterator first,
                                         const_iterator last)
{
  return m_data.erase(first, last);
}

void string_list::push_back(const std::string& value)
{
  m_data.push_back(value);
}

void string_list::push_back(std::string&& value)
{
  m_data.push_back(std::move(value));
}

void string_list::pop_back()
{
  m_data.pop_back();
}

void string_list::resize(size_type count)
{
  m_data.resize(count);
}

void string_list::resize(size_type count, const std::string& value)
{
  m_data.resize(count, value);
}

void string_list::swap(string_list& other) noexcept
{
  m_data.swap(other.m_data);
}

// ============================================================================
// String List Operations
// ============================================================================

std::string string_list::join(std::string_view delimiter) const
{
  return fb::join(m_data, delimiter);
}

std::string string_list::join(char delimiter) const
{
  return fb::join(m_data, delimiter);
}

std::string string_list::join() const
{
  std::string result;
  size_t total_size = 0;
  for (const auto& s : m_data)
  {
    total_size += s.size();
  }
  result.reserve(total_size);
  for (const auto& s : m_data)
  {
    result.append(s);
  }
  return result;
}

string_list
string_list::filter(std::function<bool(const std::string&)> predicate) const
{
  string_list result;
  result.reserve(m_data.size());
  for (const auto& s : m_data)
  {
    if (predicate(s))
    {
      result.push_back(s);
    }
  }
  return result;
}

string_list string_list::map(
    std::function<std::string(const std::string&)> transform) const
{
  string_list result;
  result.reserve(m_data.size());
  for (const auto& s : m_data)
  {
    result.push_back(transform(s));
  }
  return result;
}

string_list string_list::filter_containing(std::string_view substr) const
{
  return filter([substr](const std::string& s) {
    return fb::contains(s, substr);
  });
}

string_list string_list::filter_starts_with(std::string_view prefix) const
{
  return filter([prefix](const std::string& s) {
    return fb::starts_with(s, prefix);
  });
}

string_list string_list::filter_ends_with(std::string_view suffix) const
{
  return filter([suffix](const std::string& s) {
    return fb::ends_with(s, suffix);
  });
}

string_list string_list::filter_matching(const std::string& pattern) const
{
  std::regex regex(pattern);
  return filter_matching(regex);
}

string_list string_list::filter_matching(const std::regex& regex) const
{
  return filter([&regex](const std::string& s) {
    return std::regex_search(s, regex);
  });
}

// ============================================================================
// Search Operations
// ============================================================================

bool string_list::contains(std::string_view str) const
{
  return std::find(m_data.begin(), m_data.end(), str) != m_data.end();
}

bool string_list::contains_ignore_case(std::string_view str) const
{
  return std::find_if(m_data.begin(), m_data.end(), [&str](const std::string& s) {
    return fb::equals_ignore_case(s, str);
  }) != m_data.end();
}

std::optional<string_list::size_type>
string_list::index_of(std::string_view str) const
{
  auto it = std::find(m_data.begin(), m_data.end(), str);
  if (it != m_data.end())
  {
    return static_cast<size_type>(std::distance(m_data.begin(), it));
  }
  return std::nullopt;
}

std::optional<string_list::size_type>
string_list::last_index_of(std::string_view str) const
{
  for (size_type i = m_data.size(); i > 0; --i)
  {
    if (m_data[i - 1] == str)
    {
      return i - 1;
    }
  }
  return std::nullopt;
}

string_list::size_type string_list::count(std::string_view str) const
{
  return static_cast<size_type>(std::count(m_data.begin(), m_data.end(), str));
}

// ============================================================================
// Sorting and Ordering
// ============================================================================

string_list& string_list::sort()
{
  std::sort(m_data.begin(), m_data.end());
  return *this;
}

string_list& string_list::sort(
    std::function<bool(const std::string&, const std::string&)> comp)
{
  std::sort(m_data.begin(), m_data.end(), comp);
  return *this;
}

string_list& string_list::sort_ignore_case()
{
  std::sort(m_data.begin(), m_data.end(), [](const std::string& a, const std::string& b) {
    return fb::compare_ignore_case(a, b) < 0;
  });
  return *this;
}

string_list& string_list::sort_natural()
{
  std::sort(m_data.begin(), m_data.end(), [](const std::string& a, const std::string& b) {
    return fb::natural_compare(a, b) < 0;
  });
  return *this;
}

string_list& string_list::reverse()
{
  std::reverse(m_data.begin(), m_data.end());
  return *this;
}

string_list& string_list::unique()
{
  auto it = std::unique(m_data.begin(), m_data.end());
  m_data.erase(it, m_data.end());
  return *this;
}

string_list& string_list::shuffle()
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::shuffle(m_data.begin(), m_data.end(), gen);
  return *this;
}

// ============================================================================
// Utility Operations
// ============================================================================

string_list& string_list::remove_empty()
{
  m_data.erase(std::remove_if(m_data.begin(),
                              m_data.end(),
                              [](const std::string& s) { return s.empty(); }),
               m_data.end());
  return *this;
}

string_list& string_list::remove_blank()
{
  m_data.erase(std::remove_if(m_data.begin(),
                              m_data.end(),
                              [](const std::string& s) {
                                return fb::is_blank(s);
                              }),
               m_data.end());
  return *this;
}

string_list& string_list::trim_all()
{
  for (auto& s : m_data)
  {
    s = fb::trim(s);
  }
  return *this;
}

string_list& string_list::to_lower_all()
{
  for (auto& s : m_data)
  {
    s = fb::to_lower(s);
  }
  return *this;
}

string_list& string_list::to_upper_all()
{
  for (auto& s : m_data)
  {
    s = fb::to_upper(s);
  }
  return *this;
}

string_list& string_list::remove_duplicates()
{
  std::unordered_set<std::string> seen;
  m_data.erase(std::remove_if(m_data.begin(),
                              m_data.end(),
                              [&seen](const std::string& s) {
                                return !seen.insert(s).second;
                              }),
               m_data.end());
  return *this;
}

string_list string_list::take(size_type n) const
{
  size_type count = std::min(n, m_data.size());
  return string_list(m_data.begin(), m_data.begin() + static_cast<long>(count));
}

string_list string_list::skip(size_type n) const
{
  if (n >= m_data.size())
  {
    return string_list();
  }
  return string_list(m_data.begin() + static_cast<long>(n), m_data.end());
}

string_list string_list::take_last(size_type n) const
{
  size_type count = std::min(n, m_data.size());
  return string_list(m_data.end() - static_cast<long>(count), m_data.end());
}

string_list string_list::slice(size_type start, size_type end) const
{
  if (start >= m_data.size())
  {
    return string_list();
  }
  end = std::min(end, m_data.size());
  if (start >= end)
  {
    return string_list();
  }
  return string_list(m_data.begin() + static_cast<long>(start),
                     m_data.begin() + static_cast<long>(end));
}

// ============================================================================
// Conversion
// ============================================================================

std::vector<std::string>& string_list::to_vector() noexcept
{
  return m_data;
}

const std::vector<std::string>& string_list::to_vector() const noexcept
{
  return m_data;
}

std::vector<std::string> string_list::to_vector_copy() const
{
  return m_data;
}

// ============================================================================
// Operators
// ============================================================================

bool string_list::operator==(const string_list& other) const
{
  return m_data == other.m_data;
}

bool string_list::operator!=(const string_list& other) const
{
  return m_data != other.m_data;
}

string_list string_list::operator+(const string_list& other) const
{
  string_list result;
  result.reserve(m_data.size() + other.m_data.size());
  result.m_data.insert(result.m_data.end(), m_data.begin(), m_data.end());
  result.m_data.insert(result.m_data.end(),
                       other.m_data.begin(),
                       other.m_data.end());
  return result;
}

string_list& string_list::operator+=(const string_list& other)
{
  m_data.insert(m_data.end(), other.m_data.begin(), other.m_data.end());
  return *this;
}

string_list& string_list::operator+=(const std::string& str)
{
  m_data.push_back(str);
  return *this;
}

string_list& string_list::operator+=(std::string&& str)
{
  m_data.push_back(std::move(str));
  return *this;
}

string_list& string_list::operator<<(const std::string& str)
{
  m_data.push_back(str);
  return *this;
}

string_list& string_list::operator<<(std::string&& str)
{
  m_data.push_back(std::move(str));
  return *this;
}

} // namespace fb
