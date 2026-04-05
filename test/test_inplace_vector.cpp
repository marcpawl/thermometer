#include <cassert>
#include <string>
#include <stdexcept>
#include <iostream>
#include "../main/marcpawl_inplace_vector.hpp"

using namespace marcpawl;

consteval void test_trivially_copyable()
{
    // If the type is trivially copyable, then the vector is trivially copyable
    static_assert(std::is_trivially_copyable_v<char>);
    static_assert(std::is_trivially_copyable_v<inplace_vector<char, 4>>);
}

void test_default_constructor() {
    inplace_vector<int, 10> vec;
    assert(vec.empty());
    assert(vec.size() == 0);
    assert(vec.capacity() == 10);
    assert(vec.begin() == vec.end());
}

void test_emplace_back() {
    inplace_vector<int, 5> vec;
    vec.emplace_back(123);
    assert(vec.size() == 1);
    assert(!vec.empty());
    assert(vec[0] == 123);

    vec.emplace_back(456);
    assert(vec.size() == 2);
    assert(vec[1] == 456);
}

void test_empty()
{
    inplace_vector<int, 2> vec;
    assert(vec.empty());
    vec.emplace_back(1);
    assert(!vec.empty());
}

void test_full() {
    inplace_vector<int, 2> vec;
    vec.emplace_back(1);
    vec.emplace_back(2);
    assert(vec.full());
    bool thrown = false;
    try {
        vec.emplace_back(3);
    } catch (const std::length_error& e) {
        thrown = true;
    }
    assert(thrown);
}

void test_accessors() {
    inplace_vector<int, 3> vec;
    vec.emplace_back(10);
    vec.emplace_back(20);

    assert(vec.front() == 10);
    assert(vec.back() == 20);

    vec.front() = 11;
    assert(vec[0] == 11);

    const auto& cvec = vec;
    assert(cvec.front() == 11);
    assert(cvec.back() == 20);
    assert(cvec.at(1) == 20);

    bool thrown = false;
    try {
        (void)cvec.at(2);
    } catch(const std::out_of_range& e) {
        thrown = true;
    }
    assert(thrown);
}

void test_iterators() {
    inplace_vector<int, 4> vec;
    vec.emplace_back(1);
    vec.emplace_back(2);
    vec.emplace_back(3);

    int i = 1;
    for (const auto& val : vec) {
        assert(val == i++);
    }

    auto it = vec.begin();
    assert(*it == 1);
    ++it;
    assert(*it == 2);
    *it = 22;
    assert(vec[1] == 22);
}

void test_const_iterators()
{
    const inplace_vector<int, 4> vec{1, 2, 3, 4};
    assert( 1 == *vec.begin() );
    assert( 1 == *vec.cbegin() );

    assert( 4 == *(vec.end()-1) );
}

void test_data()
{
    inplace_vector<int, 4> vec = {1,2};
    assert(vec.data()[0] == 1);
    assert(vec.data()[1] == 2);

    auto const& cvec = vec;
    assert(vec.data()[0] == 1);
    assert(vec.data()[1] == 2);
}

void test_assign()
{
    inplace_vector<int, 5> vec;

    int values[] = {7, 8, 9};
    vec.assign(std::begin(values), std::end(values));

    assert(vec.size() == 3);
    assert(vec[0] == 7);
    assert(vec[1] == 8);
    assert(vec[2] == 9);

    vec.assign({1, 2});
    assert(vec.size() == 2);
    assert(vec[0] == 1);
    assert(vec[1] == 2);

    bool thrown = false;
    try {
        int too_many[] = {1, 2, 3, 4};
        inplace_vector<int, 3> small;
        small.assign(std::begin(too_many), std::end(too_many));
    } catch (const std::length_error&) {
        thrown = true;
    }
    assert(thrown);

    vec.assign({10,11});
    assert(vec.size() == 2);
    assert(vec[0] == 10);
    assert(vec[1] == 11);
}

void test_push_back()
{
    inplace_vector<int, 5> vec;
    vec.push_back(1);
    vec.push_back(2);
    assert(vec.size() == 2);
    assert(vec[0] == 1);
    assert(vec[1] == 2);
}

void test_equality()
{
    inplace_vector<int, 5> s = {1,3,4};
    assert(s == s);
    inplace_vector<int, 6> t = {1,3,4};
    assert(s == t);

    inplace_vector<int, 5> u = {1,3,6};
    assert(s != u);

    inplace_vector<int, 5> v = {1, 3,4,6};
    assert(s != v);
}


int main() {
    test_trivially_copyable();
    test_default_constructor();
    test_emplace_back();
    test_empty();
    test_full();
    test_accessors();
    test_iterators();
    test_data();
    test_assign();
    test_push_back();
    test_equality();
    std::cout << "All marcpawl::inplace_vector tests passed!" << std::endl;
    return 0;
}
