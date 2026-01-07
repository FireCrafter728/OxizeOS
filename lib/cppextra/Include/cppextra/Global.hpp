#pragma once
#include <print>
#include <string>
#include <iostream>

typedef signed long int int32;
typedef unsigned long int uint32;
typedef signed long long int int64;
typedef unsigned long long int uint64;

namespace cppextra
{
    void cppextra_assert(bool condition, const std::string& expr);
    [[noreturn]] void cppextra_error(const std::string& expr);
}