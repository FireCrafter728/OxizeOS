// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |-----------------------------------------------------------------------------------------------------------| //
// | Minimal freestanding LIBSTDC++ Implementation for the OxizeOS kernel                                      | //
// | const_array: a fixed non-const count array on the heap. Extension to the minimal LIBSTDC++ implementation | //
// |-----------------------------------------------------------------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdint.hpp>

namespace stdEx
{
    template<typename _T>
    class const_array
    {
    public:
        const_array() noexcept // Allow initializing after defining, useful for global / in-class members
        {
            _Size = 0;
            _Data = nullptr;
            _Lock = false;
        }

        const_array(size_t size) noexcept
        {
            _Size = size;
            _Data = reinterpret_cast<_T*>(kmalloc(_Size * sizeof(_T)));
            _Lock = true;

            for(size_t i = 0; i < _Size; i++) new(&_Data[i]) _T();
        }

        const_array(size_t size, const _T& defval) noexcept
        {
            _Size = size;
            _Data = reinterpret_cast<_T*>(kmalloc(_Size * sizeof(_T)));
            _Lock = true;
            
            for(size_t i = 0; i < _Size; i++) new (&_Data[i]) _T(defval);
        }

        // For safety disable copy/move ctors/assignments
        const_array(const const_array&) = delete;
        const_array& operator=(const const_array&) = delete;

        const_array(const const_array&&) = delete;
        const_array& operator=(const const_array&&) = delete;

        _T& operator[](size_t idx) noexcept
        {
            return _Data[idx];
        }

        const _T& operator[](size_t idx) const noexcept
        {
            return _Data[idx];
        }

        void init(size_t size) noexcept
        {
            if(_Lock) return;
            _Size = size;
            _Data = reinterpret_cast<_T*>(kmalloc(_Size * sizeof(_T)));
            _Lock = true;

            for(size_t i = 0; i < _Size; i++) new(&_Data[i]) _T();
        }

        void init(size_t size, const _T& defval) noexcept
        {
            if(_Lock || size == 0) return;
            _Size = size;
            _Data = reinterpret_cast<_T*>(kmalloc(_Size * sizeof(_T)));
            _Lock = true;

            for(size_t i = 0; i < _Size; i++) new(&_Data[i]) _T(defval);
        }

        size_t size() noexcept
        {
            return _Size;
        }

        ~const_array() noexcept
        {
            if(!_Data) return;
            for(size_t i = 0; i < _Size; i++) _Data[i].~_T();
            kfree(_Data);
        }
    private:
        _T* _Data = nullptr;
        size_t _Size = 0;
        bool _Lock = false;
    };
}