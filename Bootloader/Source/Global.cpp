#include <print.hpp>

uintptr_t CONV_MEMORY_ADDR;
constexpr uintptr_t CONV_MEMORY_SIZE = 16384;

void cpprt_print(const char* msg) {
	Print.Print(msg);
}
void cpprt_terminate()
{
	HaltSystem();
}
void* cpprt_malloc(size_t size)
{
	return MemoryAlloc(size);
}
void cpprt_free(void* addr)
{
	MemoryFree(addr);
}

void* MemoryAlloc(size_t alloc)
{
	if (!gSystem) {
		Print.PrintW(L"[GLOBAL] [MEMORY ALLOC] [ERROR]: global System ptr is uninitialized!\r\n");
		return nullptr;
	}
	void* addr;
	EFI_STATUS status = gSystem->BootServices->AllocatePool(EfiLoaderData, alloc, &addr);
	if (EFI_ERROR(status)) return nullptr;
	return addr;
}

void MemoryFree(void* addr)
{
	if (addr) gSystem->BootServices->FreePool(addr);
}

const wchar_t* AsciiToUnicode(const char* str)
{
	if (!str) return nullptr;
	if (!CONV_MEMORY_ADDR) {
		CONV_MEMORY_ADDR = (uintptr_t)MemoryAlloc(CONV_MEMORY_SIZE);
		if (!CONV_MEMORY_ADDR) {
			Print.PrintW(L"Failed to allocate memory for CONV_MEMORY_ADDR\r\n");
			HaltSystem();
			return nullptr;
		}
	}

	wchar_t* dst = (wchar_t*)CONV_MEMORY_ADDR;
	size_t i = 0;

	while (str[i] && i < (CONV_MEMORY_SIZE / sizeof(wchar_t) - 1)) {
		dst[i] = (wchar_t)str[i];
		i++;
	}

	dst[i] = L'\0';

	return dst;
};

void HaltSystem()
{
	printf("System halted");
	HaltSystemImpl();
}