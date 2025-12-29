
#pragma once

/******************************************************************************
 * Toolbox Circular Buffer
 *****************************************************************************/

#include "circular_buffer_iterator.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace fb {

/**
 * @brief A STL Style circular buffer.
 *
 * You can add data to the end using the @ref push_back function, read data
 * using @ref front() and remove data using @ref pop_front().
 *
 * The class also provides random access through the @ref operator[]()
 * function and its random access iterator.
 *
 * @note Subscripting the array with an invalid (out of range) index number
 * is undefined, for both for reading and writing.
 *
 * This class template accepts three template parameters:
 *   - T                            The type of object contained
 *   - always_accept_data_when_full Determines the behavior of
 *                                     @ref push_back when the buffer is full.
 *                                     Set to true new data is always added, the
 *                                     old "end" data is thrown away.
 *                                     Set to false, the new data is not added.
 *                                     No error is returned neither is an
 *                                     exception raised.
 *   - Alloc                        Allocator type to use (in line with other
 *                                     STL containers).
 */
template <typename T,
          bool     always_accept_data_when_full = true,
          typename Alloc                        = std::allocator<T> >
class circular_buffer
{
public:

  // Typedefs
  typedef circular_buffer<T, always_accept_data_when_full, Alloc>
  self_type;

  using allocator_type   = Alloc;
  using allocator_traits = std::allocator_traits<allocator_type>;

  using value_type      = typename allocator_traits::value_type;
  using pointer         = typename allocator_traits::pointer;
  using const_pointer   = typename allocator_traits::const_pointer;
  using reference       = value_type&;
  using const_reference = const value_type&;

  typedef size_t         size_type;

  using difference_type  = typename allocator_traits::difference_type;

  typedef circular_buffer_iterator<self_type, self_type>iterator;
  typedef circular_buffer_iterator<const self_type, self_type, const value_type>
  const_iterator;

  typedef std::reverse_iterator<iterator>       reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;


  explicit circular_buffer(size_t capacity );

  circular_buffer(const circular_buffer &other);

  template <class InputIterator>
  circular_buffer(InputIterator from, InputIterator to)
    : m_allocatorT()
    , m_data(nullptr)
    , m_arraySize(1)
    , m_head(0)
    , m_tail(0)
    , m_size(0)
  {
    m_data = allocator_traits::allocate(m_allocatorT, m_arraySize);
    reset_positions();
    circular_buffer tmp(1);
    tmp.assign_and_reserve(from, to);
    swap(tmp);
  }

  ~circular_buffer();

  circular_buffer &operator=(const self_type &other)
  {
    circular_buffer tmp(other);
    swap(tmp);
    return *this;
  }

  void swap(circular_buffer &other);

  allocator_type get_allocator() const { return m_allocatorT; }

  iterator         begin()              { return iterator(this, 0);              }
  iterator         end()                { return iterator(this, size());         }
  const_iterator   begin()        const { return const_iterator(this, 0);        }
  const_iterator   end()          const { return const_iterator(this, size());   }
  reverse_iterator rbegin()             { return reverse_iterator(end());        }
  reverse_iterator rend()               { return reverse_iterator(begin());      }
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end());  }
  const_reverse_iterator rend()   const { return const_reverse_iterator(begin());}
  size_t size()                const    { return m_size; }
  size_t capacity()            const    { return m_arraySize; }
  bool      empty()               const { return !m_size; }
  bool      full()                const { return m_size == m_arraySize; }

  size_t max_size() const;
  void reserve(size_t new_size);

  reference       front()       {return m_data[m_head];}
  reference       back()        {return m_data[m_tail];}
  const_reference front() const {return m_data[m_head];}
  const_reference back() const  {return m_data[m_tail];}

  void push_back(const value_type &item)
  {
    const size_t next = next_tail();
    bool inserted = false;
    if (m_size == m_arraySize)
    {
      if (always_accept_data_when_full)
      {
        m_data[next] = item;
        increment_head();
        inserted = true;
      }
    }
    else
    {
      allocator_traits::construct(m_allocatorT, m_data + next, item);
      inserted = true;
    }
    if (inserted)
    {
      increment_tail();
    }
  }

  void pop_front();

  void clear();

  reference       operator[](size_t n)       {return at_unchecked(n);}
  const_reference operator[](size_t n) const {return at_unchecked(n);}

  reference       at(size_t n)               {return at_checked(n);}
  const_reference at(size_t n) const         {return at_checked(n);}

private:

  reference at_unchecked(size_t index)
  {
    return m_data[index_to_subscript(index)];
  }

  const_reference at_unchecked(size_t index) const
  {
    return m_data[index_to_subscript(index)];
  }

  reference at_checked(size_t index)
  {
    validate_index(index);
    return at_unchecked(index);
  }

  const_reference at_checked(size_t index) const
  {
    validate_index(index);
    return at_unchecked(index);
  }

  constexpr size_t normalize(size_t n) const
  {
    return n % m_arraySize;
  }

  // Converts external index to an array subscript
  size_t index_to_subscript(size_t index) const
  {
    return normalize(index + m_head);
  }

  void validate_index(size_t index) const
  {
    if (index >= m_size)
    {
      std::stringstream str("fb::circular_buffer::at_checked() out of range ");
      str << " index " << index << " >= contents_size of "
          << m_size;

      throw std::out_of_range(str.str());
    }
  }

  void increment_tail();

  size_t next_tail();

  void increment_head();

  void reset_positions()
  {
    m_head = (m_arraySize > 1) ? 1 : 0;
    m_tail = 0;
    m_size = 0;
  }

  template <typename f_iter>
  void assign_into(f_iter from, f_iter to)
  {
    if (m_size) clear();

    while (from != to)
    {
      push_back(*from);
      ++from;
    }
  }

  template <typename f_iter>
  void assign_and_reserve(f_iter from, f_iter to)
  {
    if (m_size) clear();

    while (from != to)
    {
      if (m_size == m_arraySize)
      {
        const size_t grown_capacity = std::max(m_arraySize + 1,
                                               static_cast<size_t>(m_arraySize * 1.5));
        reserve(grown_capacity);
      }
      push_back(*from);
      ++from;
    }
  }

  void destroy_all_elements();

  allocator_type  m_allocatorT;
  value_type     *m_data;
  size_t       m_arraySize;
  size_t       m_head;
  size_t       m_tail;
  size_t       m_size;
};

template <typename T,
          bool consume_policy,
          typename Alloc>
bool operator==(const circular_buffer<T, consume_policy, Alloc> &a,
                const circular_buffer<T, consume_policy, Alloc> &b)
{
  // Be careful how you understand std::equal here, it can be confusing.
  // https://en.cppreference.com/w/cpp/algorithm/equal
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

template <typename T,
          bool consume_policy,
          typename Alloc>
bool operator!=(const circular_buffer<T, consume_policy, Alloc> &a,
                const circular_buffer<T, consume_policy, Alloc> &b)
{
  return a.size() != b.size() || !std::equal(a.begin(), a.end(), b.begin());
}

template <typename T,
          bool consume_policy,
          typename Alloc> inline
bool operator<(const circular_buffer<T, consume_policy, Alloc> &a,
               const circular_buffer<T, consume_policy, Alloc> &b)
{
  return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}


/**
 * @brief Class Constructor.
 */
template<typename T,
         bool consume_policy,
         typename Alloc> inline
circular_buffer<T, consume_policy, Alloc>::circular_buffer(size_t capacity):
  m_allocatorT()
  , m_data(nullptr)
  , m_arraySize(capacity)
  , m_head(0)
  , m_tail(0)
  , m_size(0)
{
  if (capacity == 0)
  {
    throw std::invalid_argument("circular_buffer capacity must be greater than zero");
  }
  m_data = allocator_traits::allocate(m_allocatorT, m_arraySize);
  reset_positions();
}

/**
 * @brief Class Destructor.
 */
template<typename T,
         bool consume_policy,
         typename Alloc> inline
circular_buffer<T, consume_policy, Alloc>::~circular_buffer()
{
  destroy_all_elements();
  allocator_traits::deallocate(m_allocatorT, m_data, m_arraySize);
}

/**
 * @brief
 */
template<typename T,
         bool consume_policy,
         typename Alloc> inline
size_t circular_buffer<T, consume_policy, Alloc>::max_size() const
{
  return allocator_traits::max_size(m_allocatorT);
}

/**
 * @brief
 */
template<typename T,
         bool consume_policy,
         typename Alloc> inline
void circular_buffer<T, consume_policy, Alloc>::reserve(size_t new_size)
{
  if (capacity() < new_size)
  {
    circular_buffer tmp(new_size);
    tmp.assign_into(begin(), end());
    swap(tmp);
  }
}


/**
 * @brief
 */
template<typename T,
         bool consume_policy,
         typename Alloc> inline
void circular_buffer<T, consume_policy, Alloc>::pop_front()
{
  size_t destroy_pos = m_head;
  increment_head();
  allocator_traits::destroy(m_allocatorT, m_data + destroy_pos);
}

/**
 * @brief Provides swap functionality.
 */
template<typename T,
         bool consume_policy,
         typename Alloc> inline
void circular_buffer<T, consume_policy, Alloc>::swap(circular_buffer<T, consume_policy, Alloc> &other)
{
  std::swap(m_allocatorT,    other.m_allocatorT);
  std::swap(m_data,         other.m_data);
  std::swap(m_arraySize,    other.m_arraySize);
  std::swap(m_head,          other.m_head);
  std::swap(m_tail,          other.m_tail);
  std::swap(m_size, other.m_size);
}

/**
 * @brief Copy Constructor.
 */
template<typename T,
         bool consume_policy,
         typename Alloc> inline
circular_buffer<T, consume_policy, Alloc>::circular_buffer(const circular_buffer<T, consume_policy, Alloc> &other):
  m_allocatorT(allocator_traits::select_on_container_copy_construction(other.m_allocatorT))
  , m_data(nullptr)
  , m_arraySize(other.m_arraySize)
  , m_head(other.m_head)
  , m_tail(other.m_tail)
  , m_size(other.m_size)
{
  m_data = allocator_traits::allocate(m_allocatorT, m_arraySize);
  try
  {
    assign_into(other.begin(), other.end());
  }

  catch (...)
  {
    destroy_all_elements();
    allocator_traits::deallocate(m_allocatorT, m_data, m_arraySize);
    throw;
  }
}

/**
 * @brief Clears out the container.
 */
template<typename T,
         bool consume_policy,
         typename Alloc> inline
void circular_buffer<T, consume_policy, Alloc>::clear()
{
  for (size_t n = 0; n < m_size; ++n)
  {
    allocator_traits::destroy(m_allocatorT, m_data + index_to_subscript(n));
  }
  reset_positions();
}

/**
 * @brief
 */
template<typename T,
         bool consume_policy,
         typename Alloc> inline
void circular_buffer<T, consume_policy, Alloc>::increment_tail()
{
  ++m_size;
  m_tail = next_tail();
}

/**
 * @brief
 */
template<typename T,
         bool consume_policy,
         typename Alloc> inline
size_t circular_buffer<T, consume_policy, Alloc>::next_tail()
{
  return (m_tail+1 == m_arraySize) ? 0 : m_tail+1;
}

/**
 * @brief
 */
template<typename T,
         bool consume_policy,
         typename Alloc> inline
void circular_buffer<T, consume_policy, Alloc>::increment_head()
{
  ++m_head;
  --m_size;

  if (m_head == m_arraySize) m_head = 0;
}

/**
 * @brief
 */
template<typename T,
         bool consume_policy,
         typename Alloc> inline
void circular_buffer<T, consume_policy, Alloc>::destroy_all_elements()
{
  for (size_t n = 0; n < m_size; ++n)
  {
    allocator_traits::destroy(m_allocatorT, m_data + index_to_subscript(n));
  }
}

} //namespace

