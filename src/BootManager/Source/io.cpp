// SPDX-License-Identifier: GPL-3.0-or-later

#include <io.hpp>

using namespace BootMgr;

void HaltSystem()
{
	// gSystem->ConOut->OutputString(gSystem->ConOut, (CHAR16*)L"System Halted");
	HaltSystemImpl();
}