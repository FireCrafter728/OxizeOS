#pragma once

extern "C"
{
	typedef void (*ctor_t)();

	extern ctor_t __init_array_start[];
	extern ctor_t __init_array_end[];

	void __cxa_init_global_ctors();
}

