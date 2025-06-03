// #include <stdint.h>
// #include <stddef.h>
// #include <memory.h>
// #include <stdio.h>
// #include <io.h>

extern "C" void (*__init_array_start[])();
extern "C" void (*__init_array_end[])();

extern "C" void run_global_constructors() {
    for(auto func = __init_array_start; func != __init_array_end; ++func) (*func)();
}

// extern "C" void* operator new(size_t size) noexcept {
//     (void)size;
//     return nullptr;
// }

// extern "C" void operator delete(void* ptr) {
//     (void)ptr;
// }

// extern "C" void operator delete(void* ptr, uint32_t size) {
//     memset(ptr, 0, size);
// }

// extern "C" void __cxa_throw(void* thrown_exception, void* type_info, void (*dest)(void*)) {
//     (void)thrown_exception;
//     (void)type_info;
//     (void)dest;
//     printf("Exception hit, what: %s\r\n", "TODO: Implement error");
//     HaltSystem();
// }

// extern "C" void* __cxa_begin_catch(void* exc) {return exc;};
// extern "C" void __cxa_end_catch() {}