// SPDX-License-Identifier: GPL-3.0-or-later

#include <main/utils.hpp>

#include <arch/x86_64/Utility/alloc.hpp>

#include <stdio.hpp>

uintptr_t krnl::AllocateStack(size_t stackSizeBytes, const std::string& stackDesc, const std::string& subClass)
{
	if(stackSizeBytes == 0)
	{
		printf("[SYSKRNL64] [ALLOC] [ERROR]: Invalid AllocateStack() input parameters\r\n");
		return 0;
	}

	std::string subClassStr = subClass.empty() ? "" : "[" + subClass + "] ";

	size_t stackBlocks = BLOCK_COUNT(stackSizeBytes);
	size_t allocBlocks = stackBlocks + 1; // 1 extra guard page

	auto stackAllocRes = virtAlloc->AllocateBlocks(allocBlocks, VA_NODE_FLAG_PHYSICALLY_NOT_BACKED | VA_NODE_FLAG_USED);
	if(!stackAllocRes)
	{
		printf("[SYSKRNL64] %s[ERROR]: Failed to reserve virtual memory for the %s of size 0x%llX, error: %lu\r\n", subClassStr.c_str(), stackDesc.c_str(), stackSizeBytes, stackAllocRes.error());
		return 0;
	}

	KRNL_STATUS stackPhysAllocRes = physAlloc->AllocSparseBlocksToContiguousVirtualRange(stackBlocks, reinterpret_cast<uintptr_t>(stackAllocRes.value()) + BLOCK_SIZE, PTE_PRESENT | PTE_RW);
	if(stackPhysAllocRes != KRNL_SUCCESS)
	{
		printf("[SYSKRNL64] %s[ERROR]: Failed to allocate physical memory for the %s of size 0x%llX, error: %lu\r\n", subClassStr.c_str(), stackDesc.c_str(), stackSizeBytes, stackPhysAllocRes);
		return 0;
	}

	return reinterpret_cast<uintptr_t>(stackAllocRes.value()) + allocBlocks * BLOCK_SIZE;
}