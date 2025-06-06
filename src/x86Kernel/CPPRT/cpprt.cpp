#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <io.h>

extern "C" void (*__init_array_start[])();
extern "C" void (*__init_array_end[])();

extern "C" void run_global_constructors() {
    for(auto func = __init_array_start; func != __init_array_end; ++func) (*func)();
}

void* operator new(uint32_t size) noexcept {
    (void)size;
    return nullptr;
}

void operator delete(void* ptr) {
    (void)ptr;
}

void operator delete(void* ptr, uint32_t size) {
    memset(ptr, 0, size);
}

extern "C" void __cxa_pure_virtual() {
    printf("[CPPRT] [ERROR]: Pure virtual function was called\r\n");
    HaltSystem();
}

extern "C" int __cxa_guard_acquire(void *guard) { return !*(char *)(guard); }
extern "C" void __cxa_guard_release(void *guard) { *(char *)(guard) = 1; }
extern "C" void __cxa_guard_abort(void *guard) {}