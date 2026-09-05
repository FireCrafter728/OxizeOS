// SPDX-License-Identifier: GPL-3.0-or-later

#include <API/HandleMgr/handle.hpp>

#include <string.hpp>

using namespace API;

std::vector<HandleSlot> HandleMgr::handles;
std::vector<HandleType> HandleMgr::handleTypeRegistry;
std::mutex HandleMgr::hmgrMutex;

void HandleMgr::Initialize()
{
	handles.resize(HMGR_INITIAL_HANDLE_CAPACITY);
	handleTypeRegistry.push_back(0); // Range 0x00-0xFF reserved for the special handle types
}

Handle HandleMgr::CreateHandle()
{
	return CreateHandle(HMGR_RESERVED_HANDLE_TYPE, nullptr, 0);
}

Handle HandleMgr::CreateHandle(HandleType type)
{
	return CreateHandle(type, nullptr, 0);
}

Handle HandleMgr::CreateHandle(HandleType type, void* data, size_t dataLength)
{
	std::lock_guard<std::mutex> lock(hmgrMutex);
	Handle handle = FindFreeHandle();
	HandleSlot handleSlot = OpenHandleSlot(handle);

	handle.generation = handleSlot.generation;
	handle.refID = 0;
	handleSlot.refCount = 1;
	handleSlot.timesOpenedCount = 1;

	handleSlot.handleType = type;
	handleSlot.handleDataPtr = reinterpret_cast<uintptr_t>(data);
	handleSlot.handleDataLength = dataLength;

	handleSlot.openRefIDs.clear();
	handleSlot.openRefIDs.push_back(handle.refID);
	handleSlot.nextFreeRefIDSearchHint = HMGR_NO_FREE_REFID_ENTRY;
	
	SaveHandleSlot(handle, handleSlot);

	return handle;
}

std::expected<Handle, KRNL_STATUS> HandleMgr::DuplicateHandle(Handle orig, bool ref, bool copyData)
{
	std::lock_guard<std::mutex> lock(hmgrMutex);
	KRNL_STATUS status = VerifyHandle(orig);
	if(KRNL_ERROR(status)) return std::unexpected<KRNL_STATUS>(status);
	HandleSlot slot = OpenHandleSlot(orig);

	if(ref)
	{
		Handle res = {.handleID = orig.handleID, .refID = slot.timesOpenedCount++, .generation = orig.generation};
		slot.refCount++;
		MarkHandleAsValid(slot, res.refID);
		SaveHandleSlot(res, slot);
		return res;
	} 

	Handle copy = FindFreeHandle();
	HandleSlot copySlot = OpenHandleSlot(copy);

	if(copyData && slot.handleDataPtr != 0 && slot.handleDataLength != 0)
	{
		copySlot.handleDataPtr = reinterpret_cast<uintptr_t>(kmalloc(slot.handleDataLength));
		copySlot.handleDataLength = slot.handleDataLength;
		if(!copySlot.handleDataPtr) return KRNL_MEMORY_ALLOC_FAILED;
		memcpy(reinterpret_cast<void*>(copySlot.handleDataPtr), reinterpret_cast<void*>(slot.handleDataPtr), slot.handleDataLength);
	}
	else
	{
		// Do not copy over the data to the duplicate handle
		copySlot.handleDataPtr = 0;
		copySlot.handleDataLength = 0;
	}

	copySlot.handleType = slot.handleType;
	copySlot.refCount = 1;
	copySlot.timesOpenedCount = 1;
	copySlot.openRefIDs.push_back(0);
	copySlot.nextFreeRefIDSearchHint = HMGR_NO_FREE_REFID_ENTRY;

	copy.generation = copySlot.generation;
	copy.refID = 0;

	SaveHandleSlot(copy, copySlot);

	return copy;
}

KRNL_STATUS HandleMgr::CloseHandle(Handle handle)
{
	std::lock_guard<std::mutex> lock(hmgrMutex);
	KRNL_STATUS status = VerifyHandle(handle);
	if(KRNL_ERROR(status)) return status;

	HandleSlot slot = OpenHandleSlot(handle);
	slot.refCount--;
	if(slot.refCount == 0)
	{
		// No more references, destroy the handle
		slot.generation++;
		slot.handleType = HMGR_UNUSED_HANDLE_TYPE;
		slot.openRefIDs.clear();
		
		SaveHandleSlot(handle, slot);
		return KRNL_SUCCESS;
	}

	handle.handleID = 0;

	for(size_t i = 0; i < slot.openRefIDs.size(); i++)
	{
		if(slot.openRefIDs[i] == handle.refID)
		{
			slot.openRefIDs[i] = UINT64_MAX;
			slot.nextFreeRefIDSearchHint = i;
			break;
		}
	}
	SaveHandleSlot(handle, slot);

	return KRNL_SUCCESS;
}

KRNL_STATUS HandleMgr::DestroyHandle(Handle handle)
{
	std::lock_guard<std::mutex> lock(hmgrMutex);
	KRNL_STATUS status = VerifyHandle(handle);
	if(KRNL_ERROR(status)) return status;

	HandleSlot slot = OpenHandleSlot(handle);

	if(slot.refCount > 1) return KRNL_API_HMGR_CANNOT_DESTROY_HANDLE_WITH_ACTIVE_HANDLES;

	slot.generation++;
	slot.handleType = HMGR_UNUSED_HANDLE_TYPE;
	slot.openRefIDs.clear();

	SaveHandleSlot(handle, slot);

	return KRNL_SUCCESS;
}

KRNL_STATUS HandleMgr::ForceDestroyHandle(Handle handle)
{
	std::lock_guard<std::mutex> lock(hmgrMutex);
	KRNL_STATUS status = VerifyHandle(handle);
	if(KRNL_ERROR(status)) return status;

	HandleSlot slot = OpenHandleSlot(handle);

	slot.generation++;
	slot.handleType = HMGR_UNUSED_HANDLE_TYPE;
	slot.openRefIDs.clear();

	SaveHandleSlot(handle, slot);

	return KRNL_SUCCESS;
}

std::expected<HandleType, KRNL_STATUS> HandleMgr::GetHandleType(Handle handle)
{
	std::lock_guard<std::mutex> lock(hmgrMutex);
	KRNL_STATUS status = VerifyHandle(handle);
	if(KRNL_ERROR(status)) return std::unexpected<KRNL_STATUS>(status);

	HandleSlot slot = OpenHandleSlot(handle);
	return slot.handleType;
}

KRNL_STATUS HandleMgr::SetHandleType(Handle handle, HandleType newType)
{
	std::lock_guard<std::mutex> lock(hmgrMutex);
	KRNL_STATUS status = VerifyHandle(handle);
	if(KRNL_ERROR(status)) return status;

	HandleSlot slot = OpenHandleSlot(handle);
	slot.handleType = newType;
	SaveHandleSlot(handle, slot);
	return KRNL_SUCCESS;
}

KRNL_STATUS HandleMgr::SetHandleData(Handle handle, void* data, size_t dataLength)
{
	std::lock_guard<std::mutex> lock(hmgrMutex);
	KRNL_STATUS status = VerifyHandle(handle);
	if(KRNL_ERROR(status)) return status;

	HandleSlot slot = OpenHandleSlot(handle);
	slot.handleDataPtr = reinterpret_cast<uintptr_t>(data);
	slot.handleDataLength = dataLength;
	SaveHandleSlot(handle, slot);

	return KRNL_SUCCESS;
}

std::expected<void*, KRNL_STATUS> HandleMgr::GetHandleData(Handle handle)
{
	std::lock_guard<std::mutex> lock(hmgrMutex);
	KRNL_STATUS status = VerifyHandle(handle);
	if(KRNL_ERROR(status)) return std::unexpected<KRNL_STATUS>(status);

	HandleSlot slot = OpenHandleSlot(handle);
	return reinterpret_cast<void*>(slot.handleDataPtr);
}

std::expected<size_t, KRNL_STATUS> HandleMgr::GetHandleDataLength(Handle handle)
{
	std::lock_guard<std::mutex> lock(hmgrMutex);
	KRNL_STATUS status = VerifyHandle(handle);
	if(KRNL_ERROR(status)) return std::unexpected<KRNL_STATUS>(status);

	HandleSlot slot = OpenHandleSlot(handle);
	return slot.handleDataLength;
}

KRNL_STATUS HandleMgr::VerifyHandle(Handle handle)
{
	if(handle.handleID >= handles.size()) return KRNL_API_HMGR_INVALID_HANDLE_ID;
	
	HandleSlot slot = handles[handle.handleID];

	if(handle.refID >= slot.timesOpenedCount) return KRNL_API_HMGR_INVALID_HANDLE_REF_ID;

	if(slot.generation != handle.generation) return KRNL_API_HMGR_OUTDATED_HANDLE;
	if(slot.handleType == HMGR_UNUSED_HANDLE_TYPE) return KRNL_API_HMGR_OUTDATED_HANDLE;
	if(slot.timesOpenedCount == 0) return KRNL_API_HMGR_INVALID_HANDLE_OBJECT;

	bool found = false;
	for(size_t i = 0; i < slot.openRefIDs.size(); i++)
	{
		if(slot.openRefIDs[i] == handle.refID)
		{
			found = true;
			break;
		}
	}

	if(!found) return KRNL_API_HMGR_CLOSED_HANDLE; // Closed handle

	return KRNL_SUCCESS;
}

Handle HandleMgr::FindFreeHandle()
{
	for(size_t i = 0; i < handles.size(); i++)
	{
		HandleSlot currentHandle = handles[i];
		if(currentHandle.handleType == HMGR_UNUSED_HANDLE_TYPE)
		{
			return {.handleID = i, .refID = 0, .generation = 0};
		}
	}
	HandleSlot freeSlot = {};
	freeSlot.handleType = HMGR_UNUSED_HANDLE_TYPE;
	handles.push_back(freeSlot);

	return {.handleID = handles.size() - 1, .refID = 0, .generation = 0};
}

HandleSlot HandleMgr::OpenHandleSlot(Handle handle)
{
	return handles[handle.handleID];
}

void HandleMgr::SaveHandleSlot(Handle handle, HandleSlot handleSlot)
{
	handles[handle.handleID] = handleSlot;
}

void HandleMgr::MarkHandleAsValid(HandleSlot& slot, uint64_t handleRefID)
{
	if(slot.nextFreeRefIDSearchHint == HMGR_NO_FREE_REFID_ENTRY || slot.nextFreeRefIDSearchHint >= slot.openRefIDs.size())
	{
		slot.openRefIDs.push_back(handleRefID);
		slot.nextFreeRefIDSearchHint = HMGR_NO_FREE_REFID_ENTRY;
		return;
	}
	for(size_t i = slot.nextFreeRefIDSearchHint; i < slot.openRefIDs.size(); i++)
	{
		if(slot.openRefIDs[i] == HMGR_UNUSED_REFID_ENTRY)
		{
			slot.openRefIDs[i] = handleRefID;
			slot.nextFreeRefIDSearchHint = i + 1;
			return;
		}
	}
	slot.openRefIDs.push_back(handleRefID);
	slot.nextFreeRefIDSearchHint = HMGR_NO_FREE_REFID_ENTRY;
}

HandleType HandleMgr::RegisterTypeRange()
{
	std::lock_guard<std::mutex> lock(hmgrMutex);
	handleTypeRegistry.push_back(handleTypeRegistry.size());
	return (handleTypeRegistry.size() - 1) * 0x100;
}