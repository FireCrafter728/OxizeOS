// SPDX-License-Identifier: GPL-3.0-or-later

#include <arch/x86_64/MP/lpdata.hpp>

#include <arch/x86_64/Utility/io.hpp>
#include <arch/x86_64/Utility/alloc.hpp>

#include <stdio.hpp>
#include <main/utils.hpp>

using namespace krnl;

bool LPData::InitializeBSP(LPSpecificData* dataOut)
{
	if (!dataOut)
	{
		printf("[SYSKRNL64] [LPDATA] [ERROR]: Invalid InitializeBSP() input parameters\r\n");
		return false;
	}

	dataOut->self = dataOut;
	
	// Allocate an interrupt handler stack for the BSP

	uintptr_t ihStackPtr = AllocateStack(LP_INTHANDLER_STACK_SIZE, "BSP Interrupt handler stack", "LPDATA");
	if(!ihStackPtr) return false;

	dataOut->ihStackTopPtr = ihStackPtr;

	dataOut->intDepth = 0;
	
	// Temporarily assign the dataOut to gs for interrupts to function normally
	StoreCurrentLPData(dataOut);

	return true;
}

bool LPData::Initialize(APIC* apic, LPSpecificData* bspData)
{
	if(!apic || !bspData)
	{
		printf("[SYSKRNL64] [LPDATA] [ERROR]: Invalid Initialize() input parameters\r\n");
		return false;
	}

	// Setup lpData array from the CPU Threads array from APIC
	std::vector<APIC_CPUThreadDesc> cpuThreadDesc = apic->getCPUThreads();
	lpData.init(cpuThreadDesc.size());
	
	// Find the BSPs index in the CPU Threads array
	int64_t bspIdx = -1;
	for(size_t i = 0; i < cpuThreadDesc.size(); i++)
	{
		if(cpuThreadDesc[i].bsp)
		{
			bspIdx = i;
			lpData[i] = *bspData;
			break;
		}
	}

	if(bspIdx < 0)
	{
		printf("[SYSKRNL64] [LPDATA] [ERROR]: Couldn't find the BSP Descriptor in APIC\r\n");
		return false;
	}
	
	for(size_t i = 0; i < cpuThreadDesc.size(); i++)
	{
		// For each LP set up it's identity fields
		APIC_CPUThreadDesc* tDesc = &cpuThreadDesc[i];
		LPSpecificData* data = &lpData[i];
		data->identity = {};
		data->identity.lpid = i;
		data->identity.apicId = tDesc->apicId;
	}

	// Assign the BSP Data pointer to the GS segment using MSRs
	StoreCurrentLPData(&lpData[bspIdx]);

	return true;
}

bool LPData::InitializeLP(LPID lpId)
{
	if(lpId >= lpData.size()) 
	{
		printf("[SYSKRNL64] [LPDATA] [ERROR]: Cannot initialize LP's specific data: LP ID %lu is invalid\r\n", lpId);
		return false;
	}

	LPSpecificData* data = &lpData[lpId];

	// Allocate an interrupt handler stack for the LP

	uintptr_t ihStackPtr = AllocateStack(LP_INTHANDLER_STACK_SIZE, "LP Interrupt handler stack", "LPDATA");
	if(!ihStackPtr) return false;

	data->ihStackTopPtr = ihStackPtr;
	data->intDepth = 0;

	StoreCurrentLPData(data);

	return true;
}

LPSpecificData* LPData::GetLPDataForCurrentLP()
{
	return reinterpret_cast<LPSpecificData*>(GetCurrentLPSpecificData());
}

void LPData::StoreCurrentLPData(LPSpecificData* data)
{
	data->self = data; // Make sure that self points to the actual structure specified in the MSR
	msr->WriteMSR(MSR_IA32_GS_BASE, reinterpret_cast<uint64_t>(data));
}