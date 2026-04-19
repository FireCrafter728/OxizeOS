#include <Drivers/CPUID/cpuid.hpp>

using namespace SysKrnl64;

CPUID::CPUID_Regs CPUID::GetCPUIDInfo(uint32_t leaf, uint32_t subLeaf)
{
    CPUID_Regs regs;
    GetCPUID(leaf, subLeaf, &regs);
    return regs;
}