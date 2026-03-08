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

namespace marcpawl
{
  // A C++23, header-only approximation of C++26 std::inplace_vector<T, N>
  // (fixed-capacity, variable-size sequence with in-place storage).
  template <class T, std::size_t N>
  class inplace_vector
  {
    static_assert(N > 0, "inplace_vector<T, N>: N must be > 0");
    static_assert(!std::is_reference_v<T>, "inplace_vector<T, N>: T must not be a reference");
    static_assert(std::is_object_v<T>, "inplace_vector<T, N>: T must be an object type");

  public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = value_type*;
    using const_iterator = const value_type*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr size_type static_capacity = N;

    // ---- iterators ----
    [[nodiscard]] constexpr iterator begin() noexcept { return data(); }
    [[nodiscard]] constexpr const_iterator begin() const noexcept { return data(); }
    [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return data(); }

    [[nodiscard]] constexpr iterator end() noexcept { return data() + m_size; }
    [[nodiscard]] constexpr const_iterator end() const noexcept { return data() + m_size; }
    [[nodiscard]] constexpr const_iterator cend() const noexcept { return data() + m_size; }

    [[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

    [[nodiscard]] constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    // ---- capacity ----
    [[nodiscard]] constexpr bool empty() const noexcept { return m_size == 0; }
    [[nodiscard]] constexpr bool full() const noexcept { return m_size == capacity(); }

    [[nodiscard]] constexpr size_type size() const noexcept { return m_size; }
    [[nodiscard]] static constexpr size_type capacity() noexcept { return N; }
    [[nodiscard]] static constexpr size_type max_size() noexcept { return N; }

    // ---- element access ----
    constexpr reference operator[](size_type pos) noexcept { return m_storage[pos]; }
    constexpr const_reference operator[](size_type pos) const noexcept { return m_storage[pos]; }

    constexpr reference at(size_type pos)
    {
      if (pos >= m_size) { throw std::out_of_range("inplace_vector::at: index out of range"); }
      return m_storage.at(pos);
    }

    constexpr const_reference at(size_type pos) const
    {
      if (pos >= m_size) { throw std::out_of_range("inplace_vector::at: index out of range"); }
      return m_storage.at(pos);
    }


    constexpr reference front()
    {
      if (size() == 0) throw std::out_of_range("inplace_vector::front: empty vector");
      return (*this)[0];
    }

    constexpr const_reference front() const
    {
      if (size() == 0) throw std::out_of_range("inplace_vector::front: empty vector");
      return (*this)[0];
    }

    constexpr reference back()
    {
      if (size() == 0) throw std::out_of_range("inplace_vector::back: empty vector");
      return (*this)[m_size - 1];
    }

    constexpr const_reference back() const
    {
      if (size() == 0) throw std::out_of_range("inplace_vector::back: empty vector");
      return (*this)[m_size - 1];
    }

    constexpr pointer data() noexcept
    {
      // Storage is contiguous; return pointer to first element (if any).
      return ptr_at(0);
    }

    constexpr const_pointer data() const noexcept
    {
      return ptr_at(0);
    }

    // ---- modifiers ----

    template <class... Args>
    constexpr reference emplace_back(Args&&... args)
    {
      if (m_size >= capacity()) throw std::length_error("inplace_vector: capacity exceeded");
      m_storage[m_size] = value_type(std::forward<Args>(args)...);
      ++m_size;
      return back();
    }


    constexpr void swap(inplace_vector& other) noexcept
    {
      std::swap(m_size, other.m_size);
      std::swap( m_storage, other.m_storage);
    }


  private:
    std::array<T, N> m_storage{};
    size_type m_size{0};

    constexpr pointer ptr_at(size_type i) noexcept
    {
      return std::launder(reinterpret_cast<pointer>(&m_storage[i]));
    }

    constexpr const_pointer ptr_at(size_type i) const noexcept
    {
      return std::launder(reinterpret_cast<const_pointer>(&m_storage[i]));
    }
  };


  // Non-member swap
  template <class T, std::size_t N>
  constexpr void swap(inplace_vector<T, N>& a, inplace_vector<T, N>& b) noexcept(noexcept(a.swap(b)))
  {
    a.swap(b);
  }
} // namespace marcpawl
#endif //DEVCONTAINER_JSON_MARCPAWL_INPLACE_VECTOR_HPP
//
