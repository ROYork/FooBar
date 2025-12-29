#pragma once

#include <exception>
#include <iterator>
#include <memory>
#include <sstream>

#include <cinttypes>

namespace fb {
/******************************************************************************
 * Iterators
 *****************************************************************************/

/**
 * Iterator type for the circular_buffer class.
 */
template <typename T,
          typename T_nonconst,
          typename elem_type = typename T::value_type>
class circular_buffer_iterator
{
public:

  typedef circular_buffer_iterator<T,T_nonconst,elem_type> self_type;

  typedef T                                   cbuf_type;
  typedef std::random_access_iterator_tag     iterator_category;
  typedef typename cbuf_type::value_type      value_type;
  typedef typename cbuf_type::size_type       size_type;
  typedef typename cbuf_type::pointer         pointer;
  typedef typename cbuf_type::const_pointer   const_pointer;
  typedef typename cbuf_type::reference       reference;
  typedef typename cbuf_type::const_reference const_reference;
  typedef typename cbuf_type::difference_type difference_type;

  circular_buffer_iterator(cbuf_type *b,
                           size_type p):
    m_buffer(b),
    m_position(p)
  { }

  circular_buffer_iterator(const circular_buffer_iterator<T_nonconst, T_nonconst, typename T_nonconst::value_type>
                           &other):
    m_buffer(other.m_buffer),
    m_position(other.m_position)
  { }

  friend class circular_buffer_iterator<const T, T, const elem_type>;


  elem_type &operator*()  { return (*m_buffer)[m_position]; }
  elem_type *operator->() { return &(operator*()); }

  self_type &operator++()
  {
    m_position += 1;
    return *this;
  }

  self_type operator++(int32_t)
  {
    self_type tmp(*this);
    ++(*this);
    return tmp;
  }

  self_type &operator--()
  {
    m_position -= 1;
    return *this;
  }

  self_type operator--(int32_t)
  {
    self_type tmp(*this);
    --(*this);
    return tmp;
  }

  self_type operator+(difference_type n) const
  {
    self_type tmp(*this);
    const difference_type new_position =
      static_cast<difference_type>(tmp.m_position) + n;
    tmp.m_position = static_cast<size_type>(new_position);
    return tmp;
  }

  self_type &operator+=(difference_type n)
  {
    const difference_type new_position =
      static_cast<difference_type>(m_position) + n;
    m_position = static_cast<size_type>(new_position);
    return *this;
  }

  self_type operator-(difference_type n) const
  {
    self_type tmp(*this);
    const difference_type new_position =
      static_cast<difference_type>(tmp.m_position) - n;
    tmp.m_position = static_cast<size_type>(new_position);
    return tmp;
  }

  self_type &operator-=(difference_type n)
  {
    m_position -= n;
    return *this;
  }

  difference_type operator-(const self_type &c) const
  {
    return static_cast<difference_type>(m_position) -
           static_cast<difference_type>(c.m_position);
  }

  bool operator==(const self_type &other) const
  {
    return m_position == other.m_position && m_buffer == other.m_buffer;
  }

  bool operator!=(const self_type &other) const
  {
    return !(*this == other);
  }

  bool operator>(const self_type &other) const
  {
    return m_position > other.m_position;
  }

  bool operator>=(const self_type &other) const
  {
    return m_position >= other.m_position;
  }

  bool operator<(const self_type &other) const
  {
    return m_position < other.m_position;
  }

  bool operator<=(const self_type &other) const
  {
    return m_position <= other.m_position;
  }

private:

  cbuf_type *m_buffer;
  size_type  m_position;
};


//------------------------------------------------------------------------------
// Inline Implementations
//------------------------------------------------------------------------------


template <typename circular_buffer_iterator_t>
circular_buffer_iterator_t operator+(
    typename circular_buffer_iterator_t::difference_type n,
    circular_buffer_iterator_t it)
{
  return it + n;
}


} //namespace


