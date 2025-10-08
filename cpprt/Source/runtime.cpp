#include <Global.hpp>

extern "C" {
#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) void (*__init_array_start[])(void) = { nullptr };

#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) void (*__init_array_end[])(void) = { nullptr };

#pragma section(".CRT$XPU", read)
__declspec(allocate(".CRT$XPU")) void (*__fini_array_start[])(void) = { nullptr };

#pragma section(".CRT$XPU", read)
__declspec(allocate(".CRT$XPU")) void (*__fini_array_end[])(void) = { nullptr };
}

extern "C" void __cxx_init_runtime() {
	for (auto ctor = __init_array_start; ctor < __init_array_end; ctor++) (*ctor)();
}

extern "C" void __cxx_fini_runtime() {
	for (auto dtor = __fini_array_start; dtor < __fini_array_end; dtor++) (*dtor)();
}

extern "C" void __cxa_pure_virtual() {
	cpprt_print("a pure virtual function was called, program execution was stopped\r\n");
	cpprt_terminate();
}

void* operator new(size_t size) {
	return cpprt_malloc(size);
}

void* operator new[](size_t size) {
	return operator new(size);
}

void operator delete(void* ptr) noexcept {
	cpprt_free(ptr);
}

void operator delete(void* ptr, size_t) noexcept {
	cpprt_free(ptr);
}

void operator delete[](void* ptr) noexcept {
	operator delete(ptr);
}

void operator delete[](void* ptr, size_t) noexcept {
	operator delete(ptr);
}

extern "C" {
	struct AtexitFunc {
		void (*func)(void*);
		void* arg;
		void* dso_handle;
		AtexitFunc* next;
	};

	struct AtexitFunc* __atexit_head = nullptr;

	static AtexitFunc __atexit_pool[128];
	static unsigned __atexit_used = 0;

	int __cxa_atexit(void (*func)(void*), void* arg, void* dso_handle) {
		if (__atexit_used >= 128) return -1;

		AtexitFunc* e = &__atexit_pool[__atexit_used++];
		e->func = func;
		e->arg = arg;
		e->dso_handle = dso_handle;

		e->next = __atexit_head;
		__atexit_head = e;

		return 0;
	}

	void __cxa_finalize(void* f)
	{
		AtexitFunc* e = __atexit_head;
		while (e)
		{
			if (!f || e->func == f) e->func(e->arg);
			e = e->next;
		}
	}
}