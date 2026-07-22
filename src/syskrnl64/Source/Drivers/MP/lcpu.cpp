// SPDX-License-Identifier: GPL-3.0-or-later

#include <Drivers/MP/lcpu.hpp>

using namespace SysKrnl64::MP;

bool LCPU::Initialize(APIC::APIC* apic)
{
    if(!apic)
    {
        printf("[SYSKRNL64] [LCPU] [ERROR]: Invalid Initialize() input parameters\r\n");
        return false;
    }

    // Get CPUThreads array

    std::vector<APIC::CPUThreadDesc> cpuThreadDesc = apic->getCPUThreads();

    // Initialize lpData const_array
    lpData.init(cpuThreadDesc.size());

    // Setup the BSPs LPSpecificData struct
    LPSpecificData* bspData = &lpData[0];

    // Get BSPs CPUThreadDesc
    APIC::CPUThreadDesc* bspDesc = nullptr;
    for(size_t i = 0; i < cpuThreadDesc.size(); i++)
    {
        APIC::CPUThreadDesc* desc = &cpuThreadDesc[i];
        if(desc->bsp)
        {
            bspDesc = desc;
            break;
        }
    }

    if(!bspDesc)
    {
        printf("[SYSKRNL64] [LCPU] [ERROR]: No BSP ThreadDesc found\r\n");
        return false;
    }

    bspData->self = bspData;
    bspData->identity = {};
    bspData->identity.lpid = 0x00; // BSP Gets LPID 0
    bspData->identity.apicId = bspDesc->apicId;

    // Assign the BSP Data to the IA32_GS_BASE MSR
    msr->WriteMSR(MSR_IA32_GS_BASE, reinterpret_cast<uint64_t>(bspData));

    return true;
}

ASMCALL LPSpecificData* ASM_GetLPDataForCurrentLP();

LPSpecificData* LCPU::GetLPDataForCurrentLP()
{
    return ASM_GetLPDataForCurrentLP();
}