#pragma once

#include <stdint.hpp>

extern "C"
{
    inline void* operator new(unsigned long int, void* ptr) noexcept
    {
        return ptr;
    }

    inline void* operator new[](unsigned long int, void* ptr) noexcept
    {
        return ptr;
    }

    inline void operator delete(void*, void*) {};
    inline void operator delete[](void*, void*) {};
}