//
// Created by marcpawl on 2026-03-01.
//

#pragma once


#ifndef DEVCONTAINER_JSON_MARCPAWL_INPLACE_STRING_HPP
#define DEVCONTAINER_JSON_MARCPAWL_INPLACE_STRING_HPP

#include <algorithm>
#include <string>
#include <string_view>
#include <stdexcept>

#include "marcpawl_inplace_vector.hpp"


namespace marcpawl {

    // Assuming marcpawl::inplace_vector exists with C++26 API
    template <size_t N>
    class inplace_string {
    private:
        // We use N + 1 to ensure there is always room for the null terminator
        marcpawl::inplace_vector<char, N + 1> storage;

        void update_null_terminator() {
            if (storage.size() < storage.capacity()) {
                // If we have space, ensure the element after the last char is \0
                // Note: inplace_vector doesn't count the \0 in its size()
                *(storage.data() + storage.size()) = '\0';
            }
        }

    public:
        // Constructors
        inplace_string() {
            storage.push_back('\0');
            storage.pop_back(); // Initialize internal data and maintain \0
        }

        inplace_string(std::string_view sv) {
            *this = sv;
        }

        // Assignment Operators
        inplace_string& operator=(const inplace_string& other) {
            if (this != &other) {
                storage = other.storage;
            }
            return *this;
        }

        inplace_string& operator=(std::string_view sv) {
            if (sv.size() > N) throw std::out_of_range("string_view exceeds inplace_string capacity");
            storage.assign(sv.begin(), sv.end());
            update_null_terminator();
            return *this;
        }

        inplace_string& operator=(const std::string& s) {
            return *this = std::string_view(s);
        }

        inplace_string& operator=(const char* s) {
            return *this = std::string_view(s);
        }

        // Accessors
        size_t size() const noexcept { return storage.size(); }
        size_t capacity() const noexcept { return N; }
        bool empty() const noexcept { return storage.empty(); }

        // Conversion Operators
        // Requirement: Returns a null-terminated string
        operator const char*() const noexcept {
            return storage.data();
        }

        operator std::string() const {
            return std::string(storage.begin(), storage.end());
        }

        // Iterators
        auto begin() noexcept { return storage.begin(); }
        auto end() noexcept { return storage.end(); }
        auto begin() const noexcept { return storage.begin(); }
        auto end() const noexcept { return storage.end(); }

        const char* data() const noexcept { return storage.data(); }
    };
}

#endif //DEVCONTAINER_JSON_MARCPAWL_INPLACE_STRING_HPP