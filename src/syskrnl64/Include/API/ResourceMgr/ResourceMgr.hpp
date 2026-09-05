// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>

#include <string>
#include <expected>
#include <vector>

#include <SysTable.hpp>

namespace API
{
	enum class RMgr_ResTypes : uint32_t
	{
		// Predefined types
		None = 0,
		RT_CURSOR = 1,
		RT_BITMAP = 2,
		RT_ICON = 3,
		RT_MENU = 4,
		RT_DIALOG = 5,
		RT_STRING = 6,
		RT_FONTDIR = 7,
		RT_FONT = 8,
		RT_ACCELERATOR = 9,
		RT_RCDATA = 10,
		RT_MESSAGETABLE = 11,
		RT_GROUP_CURSOR = 12,
		RT_GROUP_ICON = 14,
		RT_VERSION = 16,
		RT_DLGINCLUDE = 17,
		RT_PLUGPLAY = 19,
		RT_VXD = 20,
		RT_ANICURSOR = 21,
		RT_ANIICON = 22,
		RT_HTML = 23,
		RT_MANIFEST = 24,

		// Our own types
		EXT_STRING = 0xFFFF0000,
	};

	struct RMgr_ResourceDesc
	{
		RMgr_ResTypes type;
		const void* dataPtr;
		size_t dataSize;
		
		uint32_t resourceID;
		std::string resourceName;
	};

	class ResourceMgr
	{
	public:
		KRNL_STATUS Initialize(SystemTable* System);

		std::expected<RMgr_ResourceDesc*, KRNL_STATUS> GetResourceByID(uint32_t resourceID, RMgr_ResTypes type);
		std::expected<RMgr_ResourceDesc*, KRNL_STATUS> GetResourceByName(const std::string& resourceName, RMgr_ResTypes type);
	private:
		SystemTable* System;
		std::vector<RMgr_ResourceDesc> resources;

		RMgr_ResTypes GetTypeFromName(const std::string& name);
	};
}