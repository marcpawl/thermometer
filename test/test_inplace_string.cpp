#include <cassert>
#include <string>
#include <stdexcept>
#include <iostream>
#include <string_view>
#include "../main/marcpawl_inplace_string.hpp"

using namespace marcpawl;

consteval void test_string_trivially_copyable()
{
    static_assert(std::is_trivially_copyable_v<inplace_string<10>>);
}

void test_string_default_constructor() {
    inplace_string<10> s;
    assert(s.empty());
    assert(s.size() == 0);
    assert(s.capacity() == 10);
    assert(s.data()[0] == '\0');
    assert(std::string(s.data()) == "");
}

void test_string_cstring_constructor()
{
    inplace_string<10> s("hello");
    assert(s.size() == 5);
    assert(std::string(s.data()) == "hello");
}

void test_string_copy_constructor()
{
    std::string expected = "abcdef";
    inplace_string<10> src(expected);
    inplace_string<10> dest(src);
    assert(dest.size() == expected.size());
    assert(std::string(dest.data()) == expected);
}

void test_string_move_constructor()
{
    std::string expected = "abcdef";
    inplace_string<10> src(expected);
    inplace_string<10> dest(std::move(src));
    assert(dest.size() == expected.size());
    assert(std::string(dest.data()) == expected);
}

void test_string_sv_constructor() {
    inplace_string<10> s("hello");
    assert(s.size() == 5);
    assert(std::string_view(s.data()) == "hello");

    bool thrown = false;
    try {
        inplace_string<5> s_too_long("123456");
    } catch (const std::out_of_range& e) {
        thrown = true;
    }
    assert(thrown);
}

void test_string_assignment() {
    inplace_string<20> s;
    s = "hello world";
    assert(s.size() == 11);
    assert(std::string_view(s.data()) == "hello world");

    std::string std_str = "another string";
    s = std_str;
    assert(s.size() == 14);
    assert(std::string_view(s.data()) == "another string");

    std::string_view sv = "a string_view";
    s = sv;
    assert(s.size() == 13);
    assert(std::string_view(s.data()) == "a string_view");

    bool thrown = false;
    try {
        inplace_string<5> s_short;
        s_short = "this is way too long";
    } catch (const std::out_of_range& e) {
        thrown = true;
    }
    assert(thrown);
}

void test_string_assignment_const()
{
    inplace_string<20> sut(std::string_view("abcd"));
    sut = "def";
    assert(sut.size() == 3);
    assert(std::string_view(sut.data()) == "def");
}

void test_string_conversion() {
    inplace_string<15> s{"test string"};
    const char* c_str = s;
    assert(std::string(c_str) == "test string");

    std::string std_s = static_cast<std::string>(s);
    assert(std_s == "test string");
}

void test_string_iterators() {
    inplace_string<10> s {"abcdef"};
    std::string temp;
    for(char c : s) {
        temp += c;
    }
    assert(temp == "abcdef");
}

void test_equality()
{
    inplace_string<8> s{"abcd"};
    inplace_string<10> t{"abcd"};
    assert(s == t);

    inplace_string<8> u("abcde");
    assert(s != u);

    inplace_string<8> v{"abcx"};
    assert(s != v);
}

int main() {
    test_string_trivially_copyable();
    test_string_default_constructor();
    test_string_copy_constructor();
    test_string_move_constructor();
    test_string_sv_constructor();
    test_string_cstring_constructor();
    test_string_assignment();
    test_string_assignment_const();
    test_string_conversion();
    test_string_iterators();
    test_equality();
    std::cout << "All marcpawl::inplace_string tests passed!" << std::endl;
    return 0;
}
