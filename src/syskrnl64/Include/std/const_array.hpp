// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>
#include <stdlib.hpp>	

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

		const_array(const const_array& other)
		{
			if(!other._Lock || other._Size == 0) return;

			_Size = other._Size;
			_Data = reinterpret_cast<_T*>(kmalloc(_Size * sizeof(_T)));
			_Lock = true;

			for(size_t i = 0; i < _Size; i++) new(&_Data[i]) _T(other._Data[i]);
		}

		const_array& operator=(const const_array& other)
		{
			if(!other._Lock || other._Size == 0) return *this;
			if(_Lock) return *this;

			_Size = other._Size;
			_Data = reinterpret_cast<_T*>(kmalloc(_Size * sizeof(_T)));
			_Lock = true;

			for(size_t i = 0; i < _Size; i++) new(&_Data[i]) _T(other._Data[i]);

			return *this;
		}

		const_array(const_array&& other)
		{
			if(!other._Lock || other._Size == 0) return;

			_Size = other._Size;
			_Data = other._Data;
			_Lock = other._Lock;
			
			other._Size = 0;
			other._Data = nullptr;
			other._Lock = false;
		}

		const_array& operator=(const_array&& other)
		{
			if(!other._Lock || other._Size == 0) return *this;
			if(_Lock) return *this;

			_Size = other._Size;
			_Data = other._Data;
			other._Data = nullptr;
			_Lock = true;

			return *this;
		}

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
			if(_Lock || size == 0) return;
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