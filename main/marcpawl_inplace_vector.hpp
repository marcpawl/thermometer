// Created by marcpawl on 2026-03-01.
//

#pragma once

#ifndef DEVCONTAINER_JSON_MARCPAWL_INPLACE_VECTOR_HPP
#define DEVCONTAINER_JSON_MARCPAWL_INPLACE_VECTOR_HPP


#include <array>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace marcpawl {

// A C++23, header-only approximation of C++26 std::inplace_vector<T, N>
// (fixed-capacity, variable-size sequence with in-place storage).
template <class T, std::size_t N>
class inplace_vector {
  static_assert(N > 0, "inplace_vector<T, N>: N must be > 0");
  static_assert(!std::is_reference_v<T>, "inplace_vector<T, N>: T must not be a reference");
  static_assert(std::is_object_v<T>, "inplace_vector<T, N>: T must be an object type");

public:
  using value_type             = T;
  using size_type              = std::size_t;
  using difference_type        = std::ptrdiff_t;
  using reference              = value_type&;
  using const_reference        = const value_type&;
  using pointer                = value_type*;
  using const_pointer          = const value_type*;
  using iterator               = value_type*;
  using const_iterator         = const value_type*;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  static constexpr size_type static_capacity = N;

  // ---- ctors/dtor/assign ----
  constexpr inplace_vector() noexcept = default;

  constexpr explicit inplace_vector(size_type count) {
    if (count > capacity()) throw std::length_error("inplace_vector: size exceeds capacity");
    for (; m_size < count; ++m_size) {
      ::new (static_cast<void*>(ptr_at(m_size))) value_type();
    }
  }

  constexpr inplace_vector(size_type count, const T& value) {
    if (count > capacity()) throw std::length_error("inplace_vector: size exceeds capacity");
    for (; m_size < count; ++m_size) {
      ::new (static_cast<void*>(ptr_at(m_size))) value_type(value);
    }
  }

  template <class InputIt>
  constexpr inplace_vector(InputIt first, InputIt last) {
    for (; first != last; ++first) {
      push_back(*first);
    }
  }

  constexpr inplace_vector(std::initializer_list<T> init) : inplace_vector(init.begin(), init.end()) {}

  constexpr inplace_vector(const inplace_vector& other) {
    for (size_type i = 0; i < other.size(); ++i) {
      ::new (static_cast<void*>(ptr_at(i))) value_type(other[i]);
    }
    m_size = other.m_size;
  }

  constexpr inplace_vector(inplace_vector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
    for (size_type i = 0; i < other.size(); ++i) {
      ::new (static_cast<void*>(ptr_at(i))) value_type(std::move(other[i]));
    }
    m_size = other.m_size;
    other.clear();
  }

  constexpr ~inplace_vector() { clear(); }

  constexpr inplace_vector& operator=(const inplace_vector& other) {
    if (this == &other) return *this;
    assign(other.begin(), other.end());
    return *this;
  }

  constexpr inplace_vector& operator=(inplace_vector&& other) noexcept(
      std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>) {
    if (this == &other) return *this;
    clear();
    for (size_type i = 0; i < other.size(); ++i) {
      ::new (static_cast<void*>(ptr_at(i))) value_type(std::move(other[i]));
    }
    m_size = other.m_size;
    other.clear();
    return *this;
  }

  constexpr inplace_vector& operator=(std::initializer_list<T> ilist) {
    assign(ilist.begin(), ilist.end());
    return *this;
  }

  // ---- iterators ----
  constexpr iterator begin() noexcept { return data(); }
  constexpr const_iterator begin() const noexcept { return data(); }
  constexpr const_iterator cbegin() const noexcept { return data(); }

  constexpr iterator end() noexcept { return data() + m_size; }
  constexpr const_iterator end() const noexcept { return data() + m_size; }
  constexpr const_iterator cend() const noexcept { return data() + m_size; }

  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

  // ---- capacity ----
  [[nodiscard]] constexpr bool empty() const noexcept { return m_size == 0; }
  [[nodiscard]] constexpr bool full() const noexcept { return m_size == capacity(); }

  [[nodiscard]] constexpr size_type size() const noexcept { return m_size; }
  [[nodiscard]] static constexpr size_type capacity() noexcept { return N; }
  [[nodiscard]] static constexpr size_type max_size() noexcept { return N; }

  // ---- element access ----
  constexpr reference operator[](size_type pos) noexcept { return *ptr_at(pos); }
  constexpr const_reference operator[](size_type pos) const noexcept { return *ptr_at(pos); }

  constexpr reference at(size_type pos) {
    if (pos >= m_size) throw std::out_of_range("inplace_vector::at");
    return (*this)[pos];
  }
  constexpr const_reference at(size_type pos) const {
    if (pos >= m_size) throw std::out_of_range("inplace_vector::at");
    return (*this)[pos];
  }

  constexpr reference front() noexcept { return (*this)[0]; }
  constexpr const_reference front() const noexcept { return (*this)[0]; }

  constexpr reference back() noexcept { return (*this)[m_size - 1]; }
  constexpr const_reference back() const noexcept { return (*this)[m_size - 1]; }

  constexpr pointer data() noexcept {
    // Storage is contiguous; return pointer to first element (if any).
    return m_size ? ptr_at(0) : ptr_at(0);
  }
  constexpr const_pointer data() const noexcept {
    return m_size ? ptr_at(0) : ptr_at(0);
  }

  // ---- modifiers ----
  constexpr void clear() noexcept {
    destroy_range(0, m_size);
    m_size = 0;
  }

  constexpr void pop_back() noexcept(std::is_nothrow_destructible_v<T>) {
    if (m_size == 0) return;
    --m_size;
    ptr_at(m_size)->~value_type();
  }

  constexpr void push_back(const T& value) { emplace_back(value); }
  constexpr void push_back(T&& value) { emplace_back(std::move(value)); }

  template <class... Args>
  constexpr reference emplace_back(Args&&... args) {
    if (m_size >= capacity()) throw std::length_error("inplace_vector: capacity exceeded");
    ::new (static_cast<void*>(ptr_at(m_size))) value_type(std::forward<Args>(args)...);
    ++m_size;
    return back();
  }

  constexpr void resize(size_type count) {
    if (count > capacity()) throw std::length_error("inplace_vector::resize: exceeds capacity");
    if (count < m_size) {
      destroy_range(count, m_size);
      m_size = count;
      return;
    }
    while (m_size < count) {
      ::new (static_cast<void*>(ptr_at(m_size))) value_type();
      ++m_size;
    }
  }

  constexpr void resize(size_type count, const T& value) {
    if (count > capacity()) throw std::length_error("inplace_vector::resize: exceeds capacity");
    if (count < m_size) {
      destroy_range(count, m_size);
      m_size = count;
      return;
    }
    while (m_size < count) {
      ::new (static_cast<void*>(ptr_at(m_size))) value_type(value);
      ++m_size;
    }
  }

  constexpr void swap(inplace_vector& other) noexcept(
      std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>) {
    if (this == &other) return;

    // Swap element-wise up to min size, then move the tail.
    const size_type common = (m_size < other.m_size) ? m_size : other.m_size;

    for (size_type i = 0; i < common; ++i) {
      using std::swap;
      swap((*this)[i], other[i]);
    }

    if (m_size > other.m_size) {
      // move tail from *this into other
      for (size_type i = common; i < m_size; ++i) {
        ::new (static_cast<void*>(other.ptr_at(i))) value_type(std::move((*this)[i]));
        (*this)[i].~value_type();
      }
    } else if (other.m_size > m_size) {
      // move tail from other into *this
      for (size_type i = common; i < other.m_size; ++i) {
        ::new (static_cast<void*>(ptr_at(i))) value_type(std::move(other[i]));
        other[i].~value_type();
      }
    }

    std::swap(m_size, other.m_size);
  }

  template <class InputIt>
  constexpr void assign(InputIt first, InputIt last) {
    clear();
    for (; first != last; ++first) {
      push_back(*first);
    }
  }

  constexpr void assign(size_type count, const T& value) {
    clear();
    if (count > capacity()) throw std::length_error("inplace_vector::assign: exceeds capacity");
    for (size_type i = 0; i < count; ++i) {
      ::new (static_cast<void*>(ptr_at(i))) value_type(value);
    }
    m_size = count;
  }

  constexpr void assign(std::initializer_list<T> ilist) { assign(ilist.begin(), ilist.end()); }

  // Single-element insert
  constexpr iterator insert(const_iterator pos, const T& value) { return emplace(pos, value); }
  constexpr iterator insert(const_iterator pos, T&& value) { return emplace(pos, std::move(value)); }

  // Insert count copies
  constexpr iterator insert(const_iterator pos, size_type count, const T& value) {
    const size_type idx = index_of(pos);
    if (count == 0) return begin() + static_cast<difference_type>(idx);
    if (m_size + count > capacity()) throw std::length_error("inplace_vector::insert: exceeds capacity");

    // Make room
    shift_right(idx, count);
    for (size_type i = 0; i < count; ++i) {
      ::new (static_cast<void*>(ptr_at(idx + i))) value_type(value);
    }
    m_size += count;
    return begin() + static_cast<difference_type>(idx);
  }

  // Insert range
  template <class InputIt>
  constexpr iterator insert(const_iterator pos, InputIt first, InputIt last) {
    const size_type idx = index_of(pos);

    // Count elements (input iterators will be consumed; this overload expects multi-pass)
    size_type count = 0;
    for (auto it = first; it != last; ++it) ++count;

    if (count == 0) return begin() + static_cast<difference_type>(idx);
    if (m_size + count > capacity()) throw std::length_error("inplace_vector::insert: exceeds capacity");

    shift_right(idx, count);

    size_type i = 0;
    try {
      for (auto it = first; it != last; ++it, ++i) {
        ::new (static_cast<void*>(ptr_at(idx + i))) value_type(*it);
      }
    } catch (...) {
      // Roll back partially constructed new elements and restore gap as "empty"
      destroy_range(idx, idx + i);
      // Shift back left to restore original layout
      shift_left(idx + count, count);
      throw;
    }

    m_size += count;
    return begin() + static_cast<difference_type>(idx);
  }

  constexpr iterator insert(const_iterator pos, std::initializer_list<T> ilist) {
    return insert(pos, ilist.begin(), ilist.end());
  }

  template <class... Args>
  constexpr iterator emplace(const_iterator pos, Args&&... args) {
    const size_type idx = index_of(pos);
    if (m_size >= capacity()) throw std::length_error("inplace_vector::emplace: capacity exceeded");

    if (idx == m_size) {
      emplace_back(std::forward<Args>(args)...);
      return begin() + static_cast<difference_type>(idx);
    }

    // Make room for one element
    shift_right(idx, 1);

    // Construct new element in the gap
    ::new (static_cast<void*>(ptr_at(idx))) value_type(std::forward<Args>(args)...);
    ++m_size;
    return begin() + static_cast<difference_type>(idx);
  }

  constexpr iterator erase(const_iterator pos) {
    const size_type idx = index_of(pos);
    if (idx >= m_size) return end();

    // Destroy element at idx, then shift left tail
    ptr_at(idx)->~value_type();
    shift_left(idx + 1, 1);
    --m_size;
    return begin() + static_cast<difference_type>(idx);
  }

  constexpr iterator erase(const_iterator first, const_iterator last) {
    const size_type idx_first = index_of(first);
    const size_type idx_last  = index_of(last);
    if (idx_first >= m_size || idx_first >= idx_last) return begin() + static_cast<difference_type>(idx_first);

    const size_type count = idx_last - idx_first;

    destroy_range(idx_first, idx_last);
    shift_left(idx_last, count);
    m_size -= count;
    return begin() + static_cast<difference_type>(idx_first);
  }

private:
  using storage_t = std::aligned_storage_t<sizeof(T), alignof(T)>;

  std::array<storage_t, N> m_storage{};
  size_type m_size{0};

  constexpr pointer ptr_at(size_type i) noexcept {
    return std::launder(reinterpret_cast<pointer>(&m_storage[i]));
  }
  constexpr const_pointer ptr_at(size_type i) const noexcept {
    return std::launder(reinterpret_cast<const_pointer>(&m_storage[i]));
  }

  constexpr size_type index_of(const_iterator it) const noexcept {
    return static_cast<size_type>(it - begin());
  }

  constexpr void destroy_range(size_type from, size_type to) noexcept {
    for (size_type i = to; i > from; --i) {
      ptr_at(i - 1)->~value_type();
    }
  }

  // Shift elements in [idx, m_size) right by 'count' positions.
  // Precondition: m_size + count <= capacity()
  constexpr void shift_right(size_type idx, size_type count) {
    // Move-construct into uninitialized tail, then move-assign/move-construct backwards.
    // We do it backwards to avoid clobber.
    for (size_type i = m_size; i > idx; --i) {
      const size_type src = i - 1;
      const size_type dst = src + count;

      ::new (static_cast<void*>(ptr_at(dst))) value_type(std::move((*this)[src]));
      (*this)[src].~value_type();
    }
  }

  // Shift elements in [start, m_size) left by 'count' positions.
  // This overwrites already-destroyed (or to-be-overwritten) slots.
  constexpr void shift_left(size_type start, size_type count) noexcept(std::is_nothrow_move_assignable_v<T>) {
    for (size_type i = start; i < m_size; ++i) {
      const size_type src = i;
      const size_type dst = i - count;

      if constexpr (std::is_nothrow_move_assignable_v<T>) {
        (*this)[dst] = std::move((*this)[src]);
        (*this)[src].~value_type();
      } else {
        // Stronger correctness over performance: move-construct then destroy old.
        (*this)[dst].~value_type();
        ::new (static_cast<void*>(ptr_at(dst))) value_type(std::move((*this)[src]));
        (*this)[src].~value_type();
      }
    }
  }
};

// Non-member swap
template <class T, std::size_t N>
constexpr void swap(inplace_vector<T, N>& a, inplace_vector<T, N>& b) noexcept(noexcept(a.swap(b))) {
  a.swap(b);
}

} // namespace marcpawl
#endif //DEVCONTAINER_JSON_MARCPAWL_INPLACE_VECTOR_HPP
//
