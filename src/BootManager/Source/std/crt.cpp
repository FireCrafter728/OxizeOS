#include <crt.hpp>

extern "C"
{
	void __cxa_init_global_ctors()  {
		for(ctor_t* ctor = __init_array_start; ctor < __init_array_end; ctor++) (*ctor)();
	}
}