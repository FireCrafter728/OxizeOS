// SPDX-License-Identifier: GPL-3.0-or-later

#include <API/ResourceMgr/ResourceMgr.hpp>

#include <stdio.hpp>
#include <converter.hpp>
#include <main/utils.hpp>

using namespace API;

struct PACK ImageResourceDirectory
{
	uint32_t characteristics;
	uint32_t timeDateStamp;
	uint16_t majorVersion;
	uint16_t minorVersion;
	uint16_t numberOfNamedEntries;
	uint16_t numberOfIdEntries;
};

struct PACK ImageResourceDirectoryEntry
{
	uint32_t name;
	uint32_t offsetToData;
};

struct PACK ImageResourceDirStringU
{
	uint16_t length;
	wchar_t NameString[];
};

struct PACK ImageResourceDataEntry
{
	uint32_t offsetToData;
	uint32_t size;
	uint32_t codePage;
	uint32_t _Reserved;
};

KRNL_STATUS ResourceMgr::Initialize(SystemTable* System)
{
	if(!System)
	{
		printf("[SYSKRNL64] [RESOURCE MGR] [ERROR]: Invalid Initialize() input parameters\r\n");
		return KRNL_INVALID_PARAMETER;
	}

	this->System = System;

	// Parse the .rsrc section and collect all of the resource descriptors

	ImageResourceDirectory* root = reinterpret_cast<ImageResourceDirectory*>(System->ResourceSectionRootDirectoryAddress);

	// the resource section is structured into a directory tree
	// expect the structure to be:
	// Root directory
	// |-- Type
	// |   |-- Name / ID
	// |   |   |-- Language
	// |   |   |   |-- Data entry

	uintptr_t rootEntryOffset = reinterpret_cast<uintptr_t>(root + 1);

	for(size_t i = 0; i < root->numberOfIdEntries + root->numberOfNamedEntries; i++)
	{
		RMgr_ResTypes currentType = RMgr_ResTypes::None;
		ImageResourceDirectoryEntry* entryInRoot = reinterpret_cast<ImageResourceDirectoryEntry*>(rootEntryOffset);

		// Type entry, decode the Name field
		// If the last bit is set, the rest of the bits specify the offset in the .rsrc section to the string entry
		if(entryInRoot->name & (1 << 31UL))
		{
			ImageResourceDirStringU* strEntry = reinterpret_cast<ImageResourceDirStringU*>(System->ResourceSectionVirtAddr + (entryInRoot->name & 0x7FFFFFFFUL));

			wchar_t* nameBuffer = reinterpret_cast<wchar_t*>(kmalloc((strEntry->length + 1) * sizeof(wchar_t)));
			if(!nameBuffer)
			{
				printf("[SYSKRNL64] [RESOURCE MGR] [ERROR]: Failed to allocate memory for a wide string buffer for the type name\r\n");
				return KRNL_MEMORY_ALLOC_FAILED;
			}
			memcpy(nameBuffer, strEntry->NameString, strEntry->length * sizeof(wchar_t));
			nameBuffer[strEntry->length] = L'\0';

			currentType = GetTypeFromName(stdEx::wideStringToUtf8String(nameBuffer));
			kfree(nameBuffer);
		}
		else currentType = static_cast<RMgr_ResTypes>(entryInRoot->name & 0x7FFFFFFFUL);

		if((entryInRoot->offsetToData & (1 << 31UL)) == 0)
		{
			printf("[SYSKRNL64] [RESOURCE MGR] [ERROR]: First level directory entry points to a data structure, not the next level directory\r\n");
			return KRNL_API_RMGR_INVALID_TREE_STRUCTURE;
		}

		ImageResourceDirectory* firstLevelHdr = reinterpret_cast<ImageResourceDirectory*>((entryInRoot->offsetToData & 0x7FFFFFFFUL) + System->ResourceSectionVirtAddr);

		uintptr_t lvl1EntryOffset = reinterpret_cast<uintptr_t>(firstLevelHdr + 1);

		// Iterate the next level

		for(size_t i = 0; i < firstLevelHdr->numberOfIdEntries + firstLevelHdr->numberOfNamedEntries; i++)
		{
			uint32_t currentID = 0;
			std::string currentName = "";
			
			ImageResourceDirectoryEntry* entryInLvl1 = reinterpret_cast<ImageResourceDirectoryEntry*>(lvl1EntryOffset);
		
			// Name entry, decode the Name field
			// If the last bit is set, the rest of the bits specify the offset in the .rsrc section to the string entry
			if(entryInLvl1->name & (1 << 31UL))
			{
				ImageResourceDirStringU* strEntry = reinterpret_cast<ImageResourceDirStringU*>(System->ResourceSectionVirtAddr + (entryInLvl1->name & 0x7FFFFFFFUL));
			
				wchar_t* nameBuffer = reinterpret_cast<wchar_t*>(kmalloc((strEntry->length + 1) * sizeof(wchar_t)));
				if(!nameBuffer)
				{
					printf("[SYSKRNL64] [RESOURCE MGR] [ERROR]: Failed to allocate memory for a wide string buffer for the resource name\r\n");
					return KRNL_MEMORY_ALLOC_FAILED;
				}
				memcpy(nameBuffer, strEntry->NameString, strEntry->length * sizeof(wchar_t));
				nameBuffer[strEntry->length] = L'\0';
			
				currentName = stdEx::wideStringToUtf8String(nameBuffer);
				kfree(nameBuffer);
			}
			else currentID = entryInLvl1->name & 0x7FFFFFFFUL;
		
			if((entryInLvl1->offsetToData & (1 << 31UL)) == 0)
			{
				printf("[SYSKRNL64] [RESOURCE MGR] [ERROR]: Second level directory entry points to a data structure, not the next level directory\r\n");
				return KRNL_API_RMGR_INVALID_TREE_STRUCTURE;
			}
		
			ImageResourceDirectory* secondLevelHdr = reinterpret_cast<ImageResourceDirectory*>((entryInLvl1->offsetToData & 0x7FFFFFFFUL) + System->ResourceSectionVirtAddr);

			uintptr_t lvl2EntryOffset = reinterpret_cast<uintptr_t>(secondLevelHdr + 1);

			for(size_t i = 0; i < secondLevelHdr->numberOfIdEntries + secondLevelHdr->numberOfNamedEntries; i++)
			{
				ImageResourceDirectoryEntry* entryInLvl2 = reinterpret_cast<ImageResourceDirectoryEntry*>(lvl2EntryOffset);
			
				// Language entry, we do not care about it
			
				if((entryInLvl2->offsetToData & (1 << 31UL)) != 0)
				{
					printf("[SYSKRNL64] [RESOURCE MGR] [ERROR]: Third level directory entry points to the next level directory, not a data structure\r\n");
					return KRNL_API_RMGR_INVALID_TREE_STRUCTURE;
				}
	
				// Parse the resource data

				ImageResourceDataEntry* dataEntry = reinterpret_cast<ImageResourceDataEntry*>((entryInLvl2->offsetToData & 0x7FFFFFFFUL) + System->ResourceSectionVirtAddr);

				RMgr_ResourceDesc desc = {};
				desc.type = currentType;
				desc.resourceID = currentID;
				desc.resourceName = currentName;
				desc.dataPtr = reinterpret_cast<void*>(dataEntry->offsetToData + krnl::paging->GetKrnlStructVirt(System->memLayout.SysKrnl64PhysAddr));
				desc.dataSize = dataEntry->size;
				resources.push_back(desc);

				lvl2EntryOffset += sizeof(ImageResourceDirectoryEntry);
			}

			lvl1EntryOffset += sizeof(ImageResourceDirectoryEntry);
		}

		rootEntryOffset += sizeof(ImageResourceDirectoryEntry);
	}

	return KRNL_SUCCESS;
}

std::expected<RMgr_ResourceDesc*, KRNL_STATUS> ResourceMgr::GetResourceByID(uint32_t resourceID, RMgr_ResTypes type)
{
	for(size_t i = 0; i < resources.size(); i++)
	{
		RMgr_ResourceDesc* desc = &resources[i];
		if(!desc->resourceName.empty()) continue;
		if(desc->type != type) continue;
		if(desc->resourceID == resourceID) return desc;
	}

	return std::unexpected<KRNL_STATUS>(KRNL_NOT_FOUND);
}

std::expected<RMgr_ResourceDesc*, KRNL_STATUS> ResourceMgr::GetResourceByName(const std::string& resourceName, RMgr_ResTypes type)
{
	for(size_t i = 0; i < resources.size(); i++)
	{
		RMgr_ResourceDesc* desc = &resources[i];
		if(desc->resourceName.empty()) continue;
		if(desc->type != type) continue;
		if(desc->resourceName == resourceName) return desc;
	}

	return std::unexpected<KRNL_STATUS>(KRNL_NOT_FOUND);
}

RMgr_ResTypes ResourceMgr::GetTypeFromName(const std::string& name)
{
	if(name == "EXT_STRING") return RMgr_ResTypes::EXT_STRING;
	return RMgr_ResTypes::None;
}