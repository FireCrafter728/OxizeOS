#include <io.hpp>

using namespace BootMgr;

void HaltSystem()
{
	gSystem->ConOut->OutputString(gSystem->ConOut, (CHAR16*)L"System Halted");
	HaltSystemImpl();
}