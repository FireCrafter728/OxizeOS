// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>

#include <arch/x86_64/MMD/virt.hpp>
#include <arch/x86_64/MMD/phys.hpp>
#include <arch/x86_64/std/stddef.hpp>

#include <string>

namespace krnl
{
	uintptr_t AllocateStack(size_t stackSizeBytes, const std::string& stackDesc, const std::string& subClass = "");
}