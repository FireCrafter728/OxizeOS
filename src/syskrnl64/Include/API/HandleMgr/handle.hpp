// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>

#include <vector>
#include <expected>
#include <mutex>

typedef uint64_t HandleID;
typedef uint64_t HandleType;

struct Handle
{
	HandleID handleID;
	uint64_t refID;
	uint64_t generation;
};

namespace API
{
	struct HandleSlot
	{
		// Validation & other info
		uint64_t generation = 0;
		uint64_t refCount = 0;
		uint64_t timesOpenedCount = 0;

		// Handle data
		HandleType handleType;
		uintptr_t handleDataPtr = 0;
		size_t handleDataLength = 0;

		std::vector<uint64_t> openRefIDs;
		uint64_t nextFreeRefIDSearchHint;
	};

	constexpr HandleType HMGR_UNUSED_HANDLE_TYPE = 0x00000000; // Free to reuse
	constexpr HandleType HMGR_RESERVED_HANDLE_TYPE = 0x00000001; // Reserved, but not intialized
	constexpr HandleType HMGR_INVALID_HANDLE_TYPE = 0x00000002;

	constexpr uint64_t HMGR_UNUSED_REFID_ENTRY = UINT64_MAX;
	constexpr uint64_t HMGR_NO_FREE_REFID_ENTRY = UINT64_MAX;

	constexpr size_t HMGR_UNKNOWN_HANDLE_LENGTH = UINT64_MAX;

	constexpr size_t HMGR_INITIAL_HANDLE_CAPACITY = 16;


	class HandleMgr
	{
	public:
		static void Initialize();

		// Handle management

		static Handle CreateHandle();
		static Handle CreateHandle(HandleType type);
		static Handle CreateHandle(HandleType type, void* data, size_t dataLength = HMGR_UNKNOWN_HANDLE_LENGTH);

		static std::expected<Handle, KRNL_STATUS> DuplicateHandle(Handle orig, bool ref = false, bool copyData = false);
		
		static KRNL_STATUS CloseHandle(Handle handle);
		static KRNL_STATUS DestroyHandle(Handle handle);
		static KRNL_STATUS ForceDestroyHandle(Handle handle); // Can have unintended consequences, use responsively and when truly needed. Prefer DestroyHandle instead

		static std::expected<HandleType, KRNL_STATUS> GetHandleType(Handle handle);
		static KRNL_STATUS SetHandleType(Handle handle, HandleType newType);

		static KRNL_STATUS SetHandleData(Handle handle, void* data, size_t dataLength);
		static std::expected<void*, KRNL_STATUS> GetHandleData(Handle handle);

		static std::expected<size_t, KRNL_STATUS> GetHandleDataLength(Handle handle);

		// Handle type registry management

		static HandleType RegisterTypeRange();
	private:
		static KRNL_STATUS VerifyHandle(Handle handle);
		static Handle FindFreeHandle();

		static HandleSlot OpenHandleSlot(Handle handle);
		static void SaveHandleSlot(Handle handle, HandleSlot handleSlot);

		static void MarkHandleAsValid(HandleSlot& slot, uint64_t handleRefID);

		static std::vector<HandleSlot> handles;
		static std::vector<HandleType> handleTypeRegistry;

		static std::mutex hmgrMutex;
	};
}