// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |-----------------------------------------------------------------------------| //
// | OxizeOS Kernel Implementation                                               | //
// | CPUID: Driver for collecting various information from the CPUID instruction | //
// |-----------------------------------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

namespace SysKrnl64
{
    namespace CPUID
    {
        struct CPUID_Regs
        {
            uint64_t rax, rbx, rcx, rdx;
        };

        CPUID_Regs GetCPUIDInfo(uint32_t leaf, uint32_t subLeaf = 0);
    }
}