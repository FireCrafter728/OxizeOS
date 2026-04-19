#include <Drivers/CPUID/msr.hpp>

using namespace SysKrnl64::MSR;

bool MSR::Initialize()
{
    CPUID::CPUID_Regs regs;
    regs = CPUID::GetCPUIDInfo(1);
    if((regs.rdx & (1 << 5)) == 0) return false;
    present = true;
    return true;
}

uint64_t MSR::ReadMSR(uint32_t msr)
{
    if(!present) return 0xFFFFFFFF;
    return RDMSR(msr);
}

void MSR::WriteMSR(uint32_t msr, uint64_t value)
{
    if(!present) return;
    WRMSR(msr, value);
}