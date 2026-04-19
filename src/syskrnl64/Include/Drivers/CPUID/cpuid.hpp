#pragma once

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