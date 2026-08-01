// SPDX-License-Identifier: GPL-3.0-or-later

#include <fat.hpp>
#include <utils.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <chrono>

using namespace FAT32::FAT;

FAT32_STATUS FAT::Initialize(PartMgr::PartMgr* partMgr, PartMgr::PartDesc* partDesc)
{
	if(!partMgr || !partDesc)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid Initialize Input params\n");
		return FAT32_INVALID_PARAMETER;
	}

	this->partMgr = partMgr;
	this->partDesc = partDesc;

	// Read boot sector

	FAT32_STATUS status = partMgr->ReadSectors(partDesc, 0, 1, &bootSector);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read FAT boot sector\n");
		return status;
	}

	// Determine the FAT type

	// First check the BPB.sectorsPerFat
	if(bootSector.bpb.sectorsPerFat == 0) fatType = FAT_Types::FAT32;
	
	// Calculate various values

	this->totalSectors = bootSector.bpb.totalSectors == 0 ? bootSector.bpb.largeSectorCount : bootSector.bpb.totalSectors;
	this->fatSectors = fatType == FAT_Types::FAT32 ? bootSector.ebr32.sectorsPerFAT : bootSector.bpb.sectorsPerFat;
	this->rootDirSectors = fatType == FAT_Types::FAT32 ? 0 : ((bootSector.bpb.rootEntryCount * sizeof(FAT_DirectoryEntry) + (bootSector.bpb.bytesPerSector - 1)) / bootSector.bpb.bytesPerSector);
	this->dataRegionStartSector = bootSector.bpb.reservedSectorCount + (bootSector.bpb.fatCount * this->fatSectors) + this->rootDirSectors;
	this->fatStartSector = bootSector.bpb.reservedSectorCount;
	this->totalDataSectors = totalSectors - (bootSector.bpb.reservedSectorCount + (bootSector.bpb.fatCount * this->fatSectors) + this->rootDirSectors);
	this->totalClusters = this->totalDataSectors / bootSector.bpb.sectorsPerCluster;
	
	this->clusterSize = bootSector.bpb.bytesPerSector * bootSector.bpb.sectorsPerCluster;

	// Now that we have the needed vars, we can determine if the FS is FAT12 or FAT16

	if(fatType != FAT_Types::FAT32)
	{
		if(this->totalClusters < 4085) fatType = FAT_Types::FAT12;
		else fatType = FAT_Types::FAT16;
	}

	// Calculate the end of cluster chain treshold
	switch(this->fatType)
	{
		case FAT_Types::FAT32:
		{
			EOCTreshold = 0x0FFFFFF8;
			break;
		}
		case FAT_Types::FAT16:
		{
			EOCTreshold = 0xFFF8;
			break;
		}
		case FAT_Types::FAT12:
		{
			EOCTreshold = 0xFF8;
			break;
		}
		default: break;
	};
	
	// Read the FSInfo structure if FAT32
	if(this->fatType == FAT_Types::FAT32)
	{
		fsInfo = {};
		status = partMgr->ReadSectors(partDesc, bootSector.ebr32.fsInfoStruct, 1, &fsInfo);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read FSInfo structure from disk\n");
			return status;
		}
		if(fsInfo.leadSignature == FAT_FSINFO_LEAD_SIGNATURE && fsInfo.secondSignature == FAT_FSINFO_SECOND_SIGNATURE && fsInfo.trailingSignature == FAT_FSINFO_TRAILING_SIGNATURE)fsInfoPresent = true;
		else fsInfoPresent = false;
	}
	else fsInfoPresent = false;

	// Allocate a FAT Cache

	fatCache = malloc(FAT_CACHE_SECTORS * bootSector.bpb.bytesPerSector);
	if(!fatCache)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to allocate memory for the File Allocation Table Cache\n");
		return FAT32_MEMORY_ALLOCATION_FAILED;
	}
	cacheDirty = false;

	// Read the FAT Into the cache starting from FAT sector 0

	fatCacheSector = 0;
	status = partMgr->ReadSectors(partDesc, this->fatStartSector, FAT_CACHE_SECTORS, fatCache);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read FAT from disk\n");
		return status;
	}

	// Setup the handle vector

	handles.resize(FAT_INITIAL_HANDLE_COUNT);

	// Open the root directory handle

	rootDirectory = {};
	rootDirectory.handleID = AllocateHandle();
	FAT_FileHandle* rootDirHandle = &handles[rootDirectory.handleID];
	rootDirectory.generation = ++rootDirHandle->generation;
	rootDirHandle->open = true;
	rootDirHandle->fileName = L"/";

	// Set the root directory start
	// If FAT32, set it to the first usable cluster, which is cluster 2
	// the root directory in FAT12 / FAT16 is not stored in clusters, but right after the FATs.
	// Since in FAT12 / FAT16 the root directory is stored as sectors, use the firstCluster value as the sector value

	if(this->fatType == FAT_Types::FAT32) rootDirHandle->firstCluster = FAT_FIRST_USABLE_CLUSTER;
	else rootDirHandle->firstCluster = this->fatStartSector + bootSector.bpb.fatCount * this->fatSectors;

	rootDirHandle->currentCluster = rootDirHandle->firstCluster;
	rootDirHandle->currentSectorInCluster = 0;
	rootDirHandle->position = 0;

	// Store the size for the root directory if it's FAT12 / FAT16, since we can't traverse the FAT to find it out, unlike in FAT32, where we can
	rootDirHandle->size = this->fatType == FAT_Types::FAT32 ? 0 : this->rootDirSectors * bootSector.bpb.bytesPerSector;

	// Setup a fake root directory entry
	rootDirEntry = {};
	FLAG_SET(rootDirEntry.fileAttribs, FAT_ATTRIB_DIRECTORY);
	memset(rootDirEntry.shortFileName, ' ', FAT_SFN_DISK_LENGTH);
	rootDirEntry.shortFileName[0] = '/';
	rootDirEntry.firstClusterLow = rootDirHandle->firstCluster & 0xFFFF;
	rootDirEntry.firstClusterHigh = (rootDirHandle->firstCluster >> 16) & 0xFFFF;

	// Set root directory flags

	FLAG_SET(rootDirHandle->flags, FAT_FILE_FLAG_DIRECTORY | FAT_FILE_FLAG_ROOT_DIRECTORY);

	return FAT32_SUCCESS;
}

std::expected<FAT_File, FAT32_STATUS> FAT::OpenFile(const std::wstring& path, bool directory)
{
	auto openRes = OpenFileHandle(path, directory);
	if(!openRes) return std::unexpected(openRes.error());

	uint64_t handleID = RegisterHandle(openRes.value());

	FAT_File file;
	file.handleID = handleID;
	file.generation = handles[handleID].generation;
	return file;
}

std::expected<FAT_File, FAT32_STATUS> FAT::CreateFile(const std::wstring& path, FAT_FileFlags fileFlags, bool directory)
{
	auto openRes = CreateFileHandle(path, fileFlags, directory);
	if(!openRes) return std::unexpected(openRes.error());

	uint64_t handleID = RegisterHandle(openRes.value());

	FAT_File file;
	file.handleID = handleID;
	file.generation = handles[handleID].generation;
	return file;
}

std::expected<uint64_t, FAT32_STATUS> FAT::ReadFile(FAT_File* file, uint64_t count, void* bufferOut)
{
	if(!file)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid ReadFile input parameters\n");
		return std::unexpected(FAT32_INVALID_PARAMETER);
	}

	if(count == 0) return 0;

	if(!bufferOut)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid ReadFile input parameters\n");
		return std::unexpected(FAT32_INVALID_PARAMETER);
	}

	if(file->handleID >= handles.size())
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid file handle specified to ReadFile()\n");
		return std::unexpected(FAT32_INVALID_PARAMETER);
	}

	FAT_FileHandle* handle = &handles[file->handleID];
	if(!handle->open || handle->generation != file->generation)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Outdated file handle specified to ReadFile()\n");
		return std::unexpected(FAT32_INVALID_PARAMETER);
	}

	if(FLAG_GET_BOOLEAN(handle->flags, FAT_FILE_FLAG_DIRECTORY))
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Cannot read a directory as a file\n");
		return std::unexpected(FAT32_INVALID_PARAMETER);
	}

	if(handle->position >= handle->size) return 0;

	uint64_t totalRead = std::min(count, handle->size - handle->position);
	uint64_t positionInOutBuffer = 0;
	uint8_t buffer[SECTOR_SIZE];

	while(totalRead > 0)
	{
		uint64_t offsetInBuffer = handle->position % SECTOR_SIZE;
		uint64_t thisRead = std::min(totalRead, SECTOR_SIZE - offsetInBuffer);

		FAT32_STATUS status = partMgr->ReadSectors(partDesc, ClusterToLba(handle->currentCluster) + handle->currentSectorInCluster, 1, buffer);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read the disk to read a file\n");
			return std::unexpected(status);
		}

		memcpy(reinterpret_cast<uint8_t*>(bufferOut) + positionInOutBuffer, buffer + offsetInBuffer, thisRead);

		positionInOutBuffer += thisRead;
		handle->position += thisRead;
		totalRead -= thisRead;

		status = NextSector(handle);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to get the next sector in file\n");
			return std::unexpected(status);
		}
	}

	return positionInOutBuffer;
}

FAT32_STATUS FAT::WriteFile(FAT_File* file, uint64_t count, void* buffer)
{
	if(!file)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid WriteFile input parameters\n");
		return FAT32_INVALID_PARAMETER;
	}

	if(count == 0) return FAT32_SUCCESS;

	if(!buffer)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid WriteFile input parameters\n");
		return FAT32_INVALID_PARAMETER;
	}

	if(file->handleID >= handles.size())
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid file handle specified to WriteFile()\n");
		return FAT32_INVALID_PARAMETER;
	}

	FAT_FileHandle* handle = &handles[file->handleID];
	if(!handle->open || handle->generation != file->generation)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Outdated file handle specified to WriteFile()\n");
		return FAT32_INVALID_PARAMETER;
	}

	if(FLAG_GET_BOOLEAN(handle->flags, FAT_FILE_FLAG_DIRECTORY))
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Cannot write data to a directory\n");
		return FAT32_INVALID_PARAMETER;
	}

	FAT32_STATUS status = ExpandFile(handle, handle->position + count);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to expand the file for writing\n");
		return status;
	}

	auto UpdateEntrySize = [&]() -> FAT32_STATUS
	{
		handle->size = handle->position;

		// Update the size in the directory entry

		uint8_t buffer[SECTOR_SIZE];
		FAT32_STATUS status = partMgr->ReadSectors(partDesc, ClusterToLba(handle->dirEntryCluster) + handle->dirEntryOffsetInCluster / SECTOR_SIZE, 1, buffer);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read file directory entry for expanding\n");
			return status;
		}

		FAT_DirectoryEntry* entry = reinterpret_cast<FAT_DirectoryEntry*>(buffer + handle->dirEntryOffsetInCluster % SECTOR_SIZE);
		entry->fileSizeBytes = handle->size;

		status = partMgr->WriteSectors(partDesc, ClusterToLba(handle->dirEntryCluster) + handle->dirEntryOffsetInCluster / SECTOR_SIZE, 1, buffer);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to write the updated directory entry for expanding\n");
			return status;
		}

		return FAT32_SUCCESS;
	};

	uint8_t* u8Buffer = reinterpret_cast<uint8_t*>(buffer);

	if(handle->position % SECTOR_SIZE != 0)
	{
		// position is not sector-aligned, so we need to make sure the data before is not overriden
		uint8_t sectorBuffer[SECTOR_SIZE];
		uint64_t bufferSector = ClusterToLba(handle->currentCluster) + handle->currentSectorInCluster;

		status = partMgr->ReadSectors(partDesc, bufferSector, 1, sectorBuffer);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read existing data in file\n");
			return status;
		}

		uint64_t positionInBuffer = handle->position % SECTOR_SIZE;
		uint64_t write = std::min(count, SECTOR_SIZE - positionInBuffer);

		memcpy(sectorBuffer + positionInBuffer, u8Buffer, write);

		status = partMgr->WriteSectors(partDesc, bufferSector, 1, sectorBuffer);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to write to the file\n");
			return status;
		}

		count -= write;
		u8Buffer += write;
		handle->position += write;

		status = NextSector(handle);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to advance sector while writing to a file\n");
			return status;
		}

		if(count == 0)
		{
			handle->size = handle->position;
			return UpdateEntrySize();
		}
	}

	uint64_t fullSectorsLeft = count / SECTOR_SIZE;
	
	// Write full sectors
	while(fullSectorsLeft > 0)
	{
		uint64_t fullSectorsInThisCluster = std::min<uint64_t>(fullSectorsLeft, bootSector.bpb.sectorsPerCluster - handle->currentSectorInCluster);

		status = partMgr->WriteSectors(partDesc, ClusterToLba(handle->currentCluster) + handle->currentSectorInCluster, fullSectorsInThisCluster, u8Buffer);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to write sector aligned data to the file\n");
			return status;
		}

		fullSectorsLeft -= fullSectorsInThisCluster;
		u8Buffer += fullSectorsInThisCluster * SECTOR_SIZE;
		handle->position += fullSectorsInThisCluster * SECTOR_SIZE;
		count -= fullSectorsInThisCluster * SECTOR_SIZE;

		handle->currentSectorInCluster += fullSectorsInThisCluster;
		if(handle->currentSectorInCluster >= bootSector.bpb.sectorsPerCluster)
		{
			handle->currentSectorInCluster -= bootSector.bpb.sectorsPerCluster;
			auto nextClusterRes = NextCluster(handle->currentCluster);
			if(!nextClusterRes)
			{
				fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to advance to the next cluster\n");
				return nextClusterRes.error();
			}
			handle->currentCluster = nextClusterRes.value();
		}

		if(count == 0)
		{
			handle->size = handle->position;
			return UpdateEntrySize();
		}
	}

	// Write last sector
	
	uint8_t sectorBuffer[SECTOR_SIZE];
	uint64_t bufferSector = ClusterToLba(handle->currentCluster) + handle->currentSectorInCluster;

	status = partMgr->ReadSectors(partDesc, bufferSector, 1, sectorBuffer);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read existing data in file\n");
		return status;
	}

	uint64_t write = count;

	memcpy(sectorBuffer, u8Buffer, write);

	status = partMgr->WriteSectors(partDesc, bufferSector, 1, sectorBuffer);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to write to the file\n");
		return status;
	}

	handle->position += write;
	handle->size = handle->position;

	return UpdateEntrySize();
}

std::expected<FAT_DirectoryEntryInfo, FAT32_STATUS> FAT::GetNextDirectoryEntry(FAT_File* directory)
{
	if(!directory)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid GetNextDirectoryEntry input parameters\n");
		return std::unexpected(FAT32_INVALID_PARAMETER);
	}

	if(directory->handleID >= handles.size())
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid file handle specified to GetNextDirectoryEntry()\n");
		return std::unexpected(FAT32_INVALID_PARAMETER);
	}

	FAT_FileHandle* handle = &handles[directory->handleID];
	if(!handle->open || handle->generation != directory->generation)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: outdated file handle specified to GetNextDirectoryEntry()\n");
		return std::unexpected(FAT32_INVALID_PARAMETER);
	}

	FAT_LFNDirectoryEntry entry;
	while(true)
	{
		auto readRes = ReadEntry(handle);
		if(!readRes)
		{
			if(readRes.error() == FAT32_FAT_END_OF_DIRECTORY) return std::unexpected(readRes.error());
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to get directory entry\n");
			return std::unexpected(readRes.error());
		}

		entry = readRes.value();

		if(FLAG_GET_BOOLEAN(entry.dirEntry.fileAttribs, FAT_ATTRIB_ARCHIVE) || FLAG_GET_BOOLEAN(entry.dirEntry.fileAttribs, FAT_ATTRIB_DIRECTORY)) break;
	}

	FAT_DirectoryEntryInfo info = {};
	info.firstCluster = entry.dirEntry.firstClusterLow | (entry.dirEntry.firstClusterHigh << 16);
	if(FLAG_GET_BOOLEAN(entry.dirEntry.fileAttribs, FAT_ATTRIB_DIRECTORY)) FLAG_SET(info.flags, FAT_FILE_FLAG_DIRECTORY);
	if(FLAG_GET_BOOLEAN(entry.dirEntry.fileAttribs, FAT_ATTRIB_READONLY)) FLAG_SET(info.flags, FAT_FILE_FLAG_READONLY);
	if(FLAG_GET_BOOLEAN(entry.dirEntry.fileAttribs, FAT_ATTRIB_HIDDEN)) FLAG_SET(info.flags, FAT_FILE_FLAG_HIDDEN);
	if(FLAG_GET_BOOLEAN(entry.dirEntry.fileAttribs, FAT_ATTRIB_SYSTEM)) FLAG_SET(info.flags, FAT_FILE_FLAG_SYSTEM);
	info.size = entry.dirEntry.fileSizeBytes;
	if(entry.hasLFN) info.name = entry.lfn;
	else ConvertSFNToName(reinterpret_cast<char*>(entry.dirEntry.shortFileName), FLAG_GET_BOOLEAN(entry.dirEntry.fileAttribs, FAT_ATTRIB_DIRECTORY), info.name);
	info.timeDate = GetTimeDateFromDirEntry(&entry.dirEntry);
	return info;
}

FAT32_STATUS FAT::Seek(FAT_File* file, int64_t offset, FAT_SeekBase base)
{
	if(!file)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid Seek input parameters\n");
		return FAT32_INVALID_PARAMETER;
	}

	if(file->handleID >= handles.size())
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid file handle specified to Seek()\n");
		return FAT32_INVALID_PARAMETER;
	}

	FAT_FileHandle* handle = &handles[file->handleID];
	if(!handle->open || handle->generation != file->generation)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: outdated file handle specified to Seek()\n");
		return FAT32_INVALID_PARAMETER;
	}

	// Calculate the new position

	uint64_t newPosition = 0;
	switch(base)
	{
		case FAT_SeekBase::SeekFromStart:
		{
			if(offset < 0)
			{
				fprintf(stderr, "[FAT32] [FAT] [ERROR]: offset must be positive for seeking from file start\n");
				return FAT32_INVALID_PARAMETER;
			}
			newPosition = static_cast<uint64_t>(offset);
			break;
		}
		case FAT_SeekBase::SeekFromEnd:
		{
			if(offset > 0)
			{
				fprintf(stderr, "[FAT32] [FAT] [ERROR]: Cannot expand the file yet, feature not implemented\n");
				return FAT32_NOT_IMPLEMENTED;
			}
			if(offset < -static_cast<int64_t>(handle->size))
			{
				fprintf(stderr, "[FAT32] [FAT] [ERROR]: Cannot seek back past the file start\n");
				return FAT32_INVALID_PARAMETER;
			}
			newPosition = static_cast<uint64_t>(static_cast<int64_t>(handle->size) + offset);
			break;
		}
		case FAT_SeekBase::SeekFromCurrent:
		{
			int64_t positionSigned = static_cast<int64_t>(handle->position) + offset;
			if(positionSigned < 0)
			{
				fprintf(stderr, "[FAT32] [FAT] [ERROR]: Cannot seek back past the file start\n");
				return FAT32_INVALID_PARAMETER;
			}

			newPosition = static_cast<uint64_t>(positionSigned);
			break;
		}
	}

	// With the new position update the currentCluster and currentSectorInCluster values

	if(newPosition < this->clusterSize)
	{
		// new position is in the first cluster
		handle->currentCluster = handle->firstCluster;
		handle->currentSectorInCluster = newPosition / bootSector.bpb.bytesPerSector;
		handle->position = newPosition;
		return FAT32_SUCCESS;
	}

	uint64_t clustersLeft = newPosition / (this->clusterSize);
	uint32_t currentCluster = handle->firstCluster;
	while(clustersLeft > 0)
	{
		auto nextClusterRes = NextCluster(currentCluster);
		if(!nextClusterRes)
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to traverse cluster chain to seek to file position %lu\n", newPosition);
			return nextClusterRes.error();
		}
		currentCluster = nextClusterRes.value();
		clustersLeft--;
	}

	uint64_t sectorInCluster = newPosition / bootSector.bpb.bytesPerSector % bootSector.bpb.sectorsPerCluster;
	handle->currentCluster = currentCluster;
	handle->currentSectorInCluster = sectorInCluster;
	handle->position = newPosition;
	return FAT32_SUCCESS;
}

FAT32_STATUS FAT::ResetPos(FAT_File* file)
{
	if(!file)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid ResetPos input parameters\n");
		return FAT32_INVALID_PARAMETER;
	}

	if(file->handleID >= handles.size())
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid file handle specified to ResetPos()\n");
		return FAT32_INVALID_PARAMETER;
	}

	FAT_FileHandle* handle = &handles[file->handleID];
	if(!handle->open || handle->generation != file->generation)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: outdated file handle specified to ResetPos()\n");
		return FAT32_INVALID_PARAMETER;
	}

	handle->position = 0;
	handle->currentCluster = handle->firstCluster;
	handle->currentSectorInCluster = 0;
	return FAT32_SUCCESS;
}

std::expected<uint64_t, FAT32_STATUS> FAT::GetFileSize(FAT_File* file)
{
	if(!file)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid GetFileSize input parameters\n");
		return std::unexpected(FAT32_INVALID_PARAMETER);
	}

	if(file->handleID >= handles.size())
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid file handle specified to GetFileSize()\n");
		return std::unexpected(FAT32_INVALID_PARAMETER);
	}

	FAT_FileHandle* handle = &handles[file->handleID];
	if(!handle->open || handle->generation != file->generation)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: outdated file handle specified to GetFileSize()\n");
		return std::unexpected(FAT32_INVALID_PARAMETER);
	}

	return handle->size;
}

FAT32_STATUS FAT::GetVolumeLabel(std::string& strOut)
{
	FAT_FileHandle rootHandle = handles[rootDirectory.handleID];
	while(true)
	{
		auto readRes = ReadEntry(&rootHandle);
		if(!readRes)
		{
			if(readRes.error() == FAT32_FAT_END_OF_DIRECTORY) return readRes.error();
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read root directory entry while searching for the volume label\n");
			return readRes.error();
		}

		FAT_LFNDirectoryEntry entry = readRes.value();
		if(FLAG_GET_BOOLEAN(entry.dirEntry.fileAttribs, FAT_ATTRIB_VOLUMEID))
		{
			for(size_t i = 0; i < FAT_SFN_DISK_LENGTH; i++) strOut.push_back(entry.dirEntry.shortFileName[i]);
			return FAT32_SUCCESS;
		}
	}

	return FAT32_SUCCESS; // Shouldn't be reached, return to avoid compiler warning
}

uint64_t FAT::GetFreeByteCount()
{
	uint64_t freeClusters = 0;

	// Check if FSInfo is present and it's lastKnownFreeClusterCount value is valid, if yes, use that, else fallback to full FAT scan
	if(fsInfoPresent && fsInfo.lastKnownFreeClusterCount != 0xFFFFFFFF && fsInfo.lastKnownFreeClusterCount < this->totalClusters + 2) 
	{
		freeClusters = fsInfo.lastKnownFreeClusterCount;
	}
	else
	{
		for (uint32_t cluster = 2; cluster < this->totalClusters + 2; cluster++) if (ReadFAT(cluster) == 0) freeClusters++;
	}

	return freeClusters * bootSector.bpb.sectorsPerCluster * bootSector.bpb.bytesPerSector;
}

// Extra functions

std::expected<FAT_FileHandle, FAT32_STATUS> FAT::OpenFileHandle(std::wstring_view path, bool directory)
{
	if(path == L"/") return handles[rootDirectory.handleID];

	if(path[0] == L'/') path = path.substr(1);
	if(path[path.length() - 1] == L'/') path = path.substr(0, path.length() - 1);

	size_t delimPos = 0, lastPos = 0;
	FAT_FileHandle parent = handles[rootDirectory.handleID];
	std::vector<FAT_FileHandle> grandparents = {};

	// Go trough the path and open directories until the last one
	while((delimPos = path.find(L"/", lastPos)) != std::wstring::npos)
	{
		std::wstring_view current = path.substr(lastPos, delimPos - lastPos);
		lastPos = delimPos + 1;
		if(current.empty() || current == L".") continue; // Correctly handle paths such as 'dir1//entry.txt' or 'dir1/./entry.txt'
		if(current == L"..")
		{
			if(grandparents.empty()) continue;
			parent = grandparents.back();
			grandparents.pop_back();
			continue;
		}

		auto res = OpenFileHandleInDir(parent, current, true);
		if(!res)
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to open directory %ls\n", current.data());
			return std::unexpected(res.error());
		}
		grandparents.push_back(parent);
		parent = res.value();
	}

	// last part of the path should be the target entry
	std::wstring_view filename = path.substr(lastPos);

	auto res = OpenFileHandleInDir(parent, filename, directory);
	if(!res)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to open %s %ls\n", directory ? "directory" : "file", filename.data());
		return std::unexpected(res.error());
	}
	return res.value();
}

std::expected<FAT_FileHandle, FAT32_STATUS> FAT::OpenFileHandleInDir(FAT_FileHandle dirHandle, const std::wstring_view name, bool directory)
{
	auto openRes = OpenDirectoryEntry(dirHandle, name, directory);
	if(!openRes) return std::unexpected(openRes.error());
	FAT_LFNDirectoryEntry entry = openRes.value();

	FAT_FileHandle fileHandle = {};
	fileHandle.firstCluster = entry.dirEntry.firstClusterLow | (entry.dirEntry.firstClusterHigh << 16);
	fileHandle.currentCluster = fileHandle.firstCluster;
	fileHandle.currentSectorInCluster = 0;
	fileHandle.fileName = name;
	fileHandle.position = 0;
	fileHandle.size = entry.dirEntry.fileSizeBytes;
	FLAG_SET(fileHandle.flags, directory ? FAT_FILE_FLAG_DIRECTORY : FAT_FILE_FLAG_FILE);
	if(FLAG_GET_BOOLEAN(entry.dirEntry.fileAttribs, FAT_ATTRIB_READONLY)) FLAG_SET(fileHandle.flags, FAT_FILE_FLAG_READONLY);
	if(FLAG_GET_BOOLEAN(entry.dirEntry.fileAttribs, FAT_ATTRIB_HIDDEN)) FLAG_SET(fileHandle.flags, FAT_FILE_FLAG_HIDDEN);
	if(FLAG_GET_BOOLEAN(entry.dirEntry.fileAttribs, FAT_ATTRIB_SYSTEM)) FLAG_SET(fileHandle.flags, FAT_FILE_FLAG_SYSTEM);
	auto ccLengthRes = GetClusterChainLength(fileHandle.firstCluster);
	if(!ccLengthRes)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to get file %ls cluster chain length\n", name.data());
		return std::unexpected(ccLengthRes.error());
	}
	fileHandle.allocatedSize = ccLengthRes.value() * this->clusterSize;

	fileHandle.dirEntryCluster = entry.cluster;
	fileHandle.dirEntryOffsetInCluster = entry.offsetInCluster;

	return fileHandle;
}

std::expected<FAT_FileHandle, FAT32_STATUS> FAT::CreateFileHandle(std::wstring_view path, FAT_FileFlags fileFlags, bool directory)
{
	std::wstring currentPath;
	if(path == L"/")
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Cannot create the root directory\n");
		return std::unexpected(FAT32_INVALID_PARAMETER);
	}

	if(path[0] == L'/')
	{
		path = path.substr(1);
		currentPath += L'/';
	}
	if(path[path.length() - 1] == L'/') path = path.substr(0, path.length() - 1);

	size_t delimPos = 0, lastPos = 0;
	FAT_FileHandle parent = handles[rootDirectory.handleID];
	std::vector<FAT_FileHandle> grandparents = {};
	FAT_DirectoryEntry parentEntry = rootDirEntry;

	// Go trough the path and open directories until the last one
	while((delimPos = path.find(L"/", lastPos)) != std::wstring::npos)
	{
		std::wstring_view current = path.substr(lastPos, delimPos - lastPos);
		currentPath += current;
		currentPath += L'/';

		lastPos = delimPos + 1;
		if(current.empty() || current == L".") continue;// Correctly handle paths such as 'dir1//entry.txt' or 'dir1/./entry.txt'
		if(current == L"..")
		{
			if(grandparents.empty()) continue;
			parent = grandparents.back();
			grandparents.pop_back();
			continue;
		}

		if(path.find(L'/', delimPos + 1) == std::wstring_view::npos)
		{
			// Last subdirectory
			auto openRes = OpenDirectoryEntry(parent, current, true);
			if(!openRes)
			{
				fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to open parent's directory entry when trying to create a new %s %ls\n", directory ? "directory" : "file", path.data());
				return std::unexpected(openRes.error());
			}
			parentEntry = openRes.value().dirEntry;
		}

		auto res = OpenFileHandleInDir(parent, current, true);
		if(!res)
		{
			if(res.error() == FAT32_NOT_FOUND) 
			{
				fprintf(stderr, "[FAT32] [FAT] [ERROR]: Cannot open directory %ls because directory doesn't exist\n", currentPath.data());
				return std::unexpected(res.error());
			}
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to open directory %ls\n", current.data());
			return std::unexpected(res.error());
		}
		grandparents.push_back(parent);
		parent = res.value();
	}

	// last part of the path should be the target entry
	std::wstring_view filename = path.substr(lastPos);

	// Check if a directory with the same name already exists
	auto res = OpenFileHandleInDir(parent, filename, true);
	if(res)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Cannot create %s %ls because a directory with the same name already exists\n", directory ? "directory" : "file", currentPath.data());
		return std::unexpected(FAT32_FAT_ALREADY_EXISTS);
	}
	else if(res.error() != FAT32_NOT_FOUND)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to check if a directory %ls already exists\n", currentPath.data());
		return std::unexpected(res.error());
	}
	
	// Check if a file with the same name already exists
	res = OpenFileHandleInDir(parent, filename, false);
	if(res)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Cannot create %s %ls because a file with the same name already exists\n", directory ? "directory" : "file", currentPath.data());
		return std::unexpected(FAT32_FAT_ALREADY_EXISTS);
	}
	else if(res.error() != FAT32_NOT_FOUND)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to check if a file %ls already exists\n", currentPath.data());
		return std::unexpected(res.error());
	}

	// Create a file handle in that directory and return it
	res = CreateFileHandleInDir(parent, filename, fileFlags, directory, &parentEntry);
	if(!res)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to create %s %ls\n", directory ? "directory" : "file", currentPath.data());
		return std::unexpected(res.error());
	}

	return res.value();
}

std::expected<FAT_FileHandle, FAT32_STATUS> FAT::CreateFileHandleInDir(FAT_FileHandle dirHandle, const std::wstring_view name, FAT_FileFlags fileFlags, bool directory, FAT_DirectoryEntry* parentEntry)
{
	// Check if the name is SFN Compatible
	char sfnName[FAT_SFN_DISK_LENGTH];
	bool sfnCompatible = ConvertNameToSFN(name, directory, sfnName);
	uint8_t sfnChecksum;
	std::vector<FAT_LFNEntry> lfnEntries;
	if(!sfnCompatible)
	{
		// Generate an alias SFN to store in the directory entry, calculate it's checksum and construct the LFN entries
		GenerateLFNAliasSFN(name, directory, sfnName);
		sfnChecksum = CalculateSFNChecksum(sfnName);
		auto constructRes = ConstructLFNEntries(name, sfnChecksum);
		if(!constructRes)
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to construct LFN Entries for creating a %s %ls\n", directory ? "directory" : "file", name.data());
			return std::unexpected(constructRes.error());
		}
		lfnEntries = constructRes.value();
	}
	else
	{
		// Calculate the converted SFN checksum
		sfnChecksum = CalculateSFNChecksum(sfnName);
	}

	size_t freeEntriesNeeded = lfnEntries.size() + 1; // 1 for each LFN entry + 1 for the actual directory entry

	auto searchRes = FindFreeEntryGap(&dirHandle, freeEntriesNeeded);
	if(!searchRes)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to find a free directory entry gap to create a new %s %ls\n", directory ? "directory" : "file", name.data());
		return std::unexpected(searchRes.error());
	}

	std::pair<uint32_t, uint64_t> gapLocation = searchRes.value();
	uint32_t gapCluster = gapLocation.first;
	uint64_t gapOffsetInCluster = gapLocation.second;

	// Get the main directory entry cluster and offset in cluster values
	uint32_t dirEntryCluster = gapCluster;
	uint64_t dirEntryOffsetInCluster = gapOffsetInCluster + lfnEntries.size() * sizeof(FAT_LFNEntry);

	if(dirEntryOffsetInCluster >= this->clusterSize)
	{
		dirEntryOffsetInCluster -= this->clusterSize;
		auto nextClusterRes = NextCluster(dirEntryCluster);
		if(!nextClusterRes)
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to get next cluster to store the directory entry location\n");
			return std::unexpected(nextClusterRes.error());
		}

		dirEntryCluster = nextClusterRes.value();
	}

	bool crossesSectorBoundary = false, crossesClusters = false;

	// Calculate if the gap crosses the sector boundary
	uint64_t gapLength = freeEntriesNeeded * sizeof(FAT_DirectoryEntry);
	uint64_t offsetInSector = gapOffsetInCluster % SECTOR_SIZE;
	uint64_t bytesUntilSectorBoundary = SECTOR_SIZE - offsetInSector;
	uint64_t gapEndOffset = gapOffsetInCluster + gapLength;

	crossesSectorBoundary = gapLength > bytesUntilSectorBoundary;

	if(crossesSectorBoundary)
	{
		// Check if the gap is accross 2 clusters
		if (gapEndOffset > this->clusterSize) crossesClusters = true;
	}

	// Construct the main directory entry

	FAT_DirectoryEntry mainEntry = {};

	memcpy(mainEntry.shortFileName, sfnName, FAT_SFN_DISK_LENGTH);

	// Set attributes
	FLAG_SET(mainEntry.fileAttribs, directory ? FAT_ATTRIB_DIRECTORY : FAT_ATTRIB_ARCHIVE);
	if(FLAG_GET_BOOLEAN(fileFlags, FAT_FILE_FLAG_READONLY)) FLAG_SET(mainEntry.fileAttribs, FAT_ATTRIB_READONLY);
	if(FLAG_GET_BOOLEAN(fileFlags, FAT_FILE_FLAG_HIDDEN)) FLAG_SET(mainEntry.fileAttribs, FAT_ATTRIB_HIDDEN);
	if(FLAG_GET_BOOLEAN(fileFlags, FAT_FILE_FLAG_SYSTEM)) FLAG_SET(mainEntry.fileAttribs, FAT_ATTRIB_SYSTEM);

	// Store current time
	FAT_TimeDate timeDate = GetCurrentTimeDate();
	SetDirectoryEntryTimeDate(&mainEntry, &timeDate, FAT_TIMESTAMP_TYPE_ALL);

	mainEntry.fileSizeBytes = 0;

	// Allocate a cluster for the new file / directory
	auto clusterAllocRes = AllocateCluster(0);
	if(!clusterAllocRes)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to allocate an initial data cluster for the %s %ls\n", directory ? "directory" : "file", name.data());
		return std::unexpected(clusterAllocRes.error());
	}

	uint32_t newCluster = clusterAllocRes.value();
	mainEntry.firstClusterLow = newCluster & 0xFFFF;
	mainEntry.firstClusterHigh = (newCluster >> 16) & 0xFFFF;

	// Store the entries to the disk

	uint8_t buffer[SECTOR_SIZE];
	uint64_t bufferSector;

	bool specialRootDirHandling = FLAG_GET_BOOLEAN(dirHandle.flags, FAT_FILE_FLAG_ROOT_DIRECTORY) && fatType != FAT_Types::FAT32;

	if(specialRootDirHandling) bufferSector = gapCluster;
	else bufferSector = ClusterToLba(gapCluster) + (gapOffsetInCluster / bootSector.bpb.bytesPerSector);

	// Read the current data so we wouldn't overwrite already existing data when writing the buffer back
	FAT32_STATUS status = partMgr->ReadSectors(partDesc, bufferSector, 1, buffer);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read directory entries to add new ones\n");
		return std::unexpected(status);
	}

	// Write the first part of the entries
	uint64_t offsetInBuffer = gapOffsetInCluster % bootSector.bpb.bytesPerSector;
	uint8_t entriesThatBufferFits = (SECTOR_SIZE - offsetInBuffer) / sizeof(FAT_DirectoryEntry);
	uint8_t entriesInThisBuffer = static_cast<uint8_t>(std::min<size_t>(entriesThatBufferFits, freeEntriesNeeded));
	FAT_DirectoryEntry* entries = reinterpret_cast<FAT_DirectoryEntry*>(buffer + offsetInBuffer);

	uint8_t i = 0;
	for(; i < static_cast<uint8_t>(std::min<size_t>(entriesInThisBuffer, lfnEntries.size())); i++)
	{
		entries[i] = *reinterpret_cast<FAT_DirectoryEntry*>(&lfnEntries[i]);
		entriesInThisBuffer--;
	}

	if(entriesInThisBuffer > 0) entries[i] = mainEntry;
	
	// Write back the buffer
	status = partMgr->WriteSectors(partDesc, bufferSector, 1, buffer);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to write the updated buffer to the directory\n");
		return std::unexpected(status);
	}

	// Check if crosses sector boundary
	if(crossesSectorBoundary)
	{
		// Second copy part is needed
		// Get the next sector
		if(crossesClusters)
		{
			auto nextClusterRes = NextCluster(gapCluster);
			if(!nextClusterRes)
			{
				fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to get the next directory cluster for creating the new %s %ls\n", directory ? "directory" : "file", name.data());
				return std::unexpected(nextClusterRes.error());
			}
			uint32_t nextCluster = nextClusterRes.value();
			bufferSector = ClusterToLba(nextCluster);
		}
		else bufferSector++;

		status = partMgr->ReadSectors(partDesc, bufferSector, 1, buffer);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read the second sector of directory entries to add new ones\n");
			return std::unexpected(status);
		}

		// Write the rest of the entries
		uint8_t lfnEntriesLeft = static_cast<uint8_t>(lfnEntries.size()) - i;
		entries = reinterpret_cast<FAT_DirectoryEntry*>(buffer);
		for(i = 0; i < lfnEntriesLeft; i++)
		{
			entries[i] = *reinterpret_cast<FAT_DirectoryEntry*>(&lfnEntries[lfnEntries.size() - lfnEntriesLeft + i]);
		}

		entries[i] = mainEntry;

		// Write back the buffer
		status = partMgr->WriteSectors(partDesc, bufferSector, 1, buffer);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to write the second updated buffer to the directory\n");
			return std::unexpected(status);
		}
	}

	// Setup the new handle
	FAT_FileHandle handleOut = {};
	handleOut.firstCluster = mainEntry.firstClusterLow | (mainEntry.firstClusterHigh << 16);
	handleOut.currentCluster = handleOut.firstCluster;
	handleOut.currentSectorInCluster = 0;
	handleOut.fileName = name;
	handleOut.position = 0;
	handleOut.size = mainEntry.fileSizeBytes;
	handleOut.flags = fileFlags;
	handleOut.allocatedSize = this->clusterSize;
	handleOut.dirEntryCluster = dirEntryCluster;
	handleOut.dirEntryOffsetInCluster = dirEntryOffsetInCluster;

	if(directory)
	{
		// Add the . & .. entries to the new directory
		memset(buffer, 0, sizeof(buffer));

		entries = reinterpret_cast<FAT_DirectoryEntry*>(buffer);
		
		FAT_DirectoryEntry* dotEntry = &entries[0];
		*dotEntry = mainEntry;
		memset(dotEntry->shortFileName, ' ', FAT_SFN_DISK_LENGTH);
		dotEntry->shortFileName[0] = '.';

		FAT_DirectoryEntry* dotdotEntry = &entries[1];
		*dotdotEntry = *parentEntry;
		memset(dotdotEntry->shortFileName, ' ', FAT_SFN_DISK_LENGTH);
		dotdotEntry->shortFileName[0] = '.';
		dotdotEntry->shortFileName[1] = '.';

		status = partMgr->WriteSectors(partDesc, ClusterToLba(handleOut.firstCluster), 1, buffer);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to write initial new directory entries\n");
			return std::unexpected(status);
		}
	}

	return handleOut;
}

FAT32_STATUS FAT::ExpandFile(FAT_FileHandle* fileHandle, size_t newEnd)
{
	if(newEnd <= fileHandle->allocatedSize) return FAT32_SUCCESS;

	uint64_t extraBytesNeeded = newEnd - fileHandle->allocatedSize;
	uint32_t clustersToAllocate = (extraBytesNeeded + this->clusterSize - 1) / this->clusterSize;

	if(fileHandle->firstCluster == 0)
	{
		// File has no clusters, allocate a new one
		auto allocRes = AllocateCluster(0);
		if(!allocRes)
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to allocate a new cluster for expanding the file\n");
			return allocRes.error();
		}

		fileHandle->firstCluster = allocRes.value();
		fileHandle->currentCluster = fileHandle->firstCluster;

		// Update the directory entry

		uint8_t buffer[SECTOR_SIZE];
		FAT32_STATUS status = partMgr->ReadSectors(partDesc, ClusterToLba(fileHandle->dirEntryCluster) + fileHandle->dirEntryOffsetInCluster / SECTOR_SIZE, 1, buffer);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read file directory entry for expanding\n");
			return status;
		}

		FAT_DirectoryEntry* entry = reinterpret_cast<FAT_DirectoryEntry*>(buffer + fileHandle->dirEntryOffsetInCluster % SECTOR_SIZE);
		entry->firstClusterLow = fileHandle->firstCluster & 0xFFFF;
		entry->firstClusterHigh = (fileHandle->firstCluster >> 16) & 0xFFFF;

		status = partMgr->WriteSectors(partDesc, ClusterToLba(fileHandle->dirEntryCluster) + fileHandle->dirEntryOffsetInCluster / SECTOR_SIZE, 1, buffer);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to write the updated directory entry for expanding\n");
			return status;
		}

		clustersToAllocate--;
		fileHandle->allocatedSize += this->clusterSize;
	}

	auto lastClusterRes = GetLastCluster(fileHandle->currentCluster); // Accepts any cluster inside the chain, the closer to the end, the faster it returns
	if(!lastClusterRes)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to get the cluster chain last cluster to expand the file\n");
		return lastClusterRes.error();
	}
	uint32_t lastCluster = lastClusterRes.value();

	for(size_t i = 0; i < clustersToAllocate; i++)
	{
		auto allocRes = AllocateCluster(lastCluster);
		if(!allocRes)
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to allocate a cluster to extend a file\n");
			return allocRes.error();
		}
		lastCluster = allocRes.value();
	}

	fileHandle->allocatedSize += clustersToAllocate * this->clusterSize;
	return FAT32_SUCCESS;
}

std::expected<FAT_LFNDirectoryEntry, FAT32_STATUS> FAT::ReadEntry(FAT_FileHandle* dirHandle)
{
	if(dirHandle->currentCluster >= EOCTreshold) return std::unexpected(FAT32_FAT_END_OF_DIRECTORY);

	// Read the current sector into the buffer
	uint8_t buffer[SECTOR_SIZE];
	uint64_t bufferSector;
	if(FLAG_GET_BOOLEAN(dirHandle->flags, FAT_FILE_FLAG_ROOT_DIRECTORY) && fatType != FAT_Types::FAT32) bufferSector = dirHandle->currentCluster; // In FAT16 / FAT12 root directory current cluster is stored as a sector value, since the root directory is not inside the data area
	else bufferSector = ClusterToLba(dirHandle->currentCluster) + dirHandle->currentSectorInCluster;

	FAT32_STATUS status = partMgr->ReadSectors(partDesc, bufferSector, 1, buffer);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read directory entry from disk at sector %lu\n", bufferSector);
		return std::unexpected(status);
	}

	FAT_LFNDirectoryEntry entryOut = {};

	// Get the next entry
	FAT_DirectoryEntry* entry = reinterpret_cast<FAT_DirectoryEntry*>(buffer + dirHandle->position % bootSector.bpb.bytesPerSector);
	if(entry->fileAttribs == FAT_ATTRIB_LFN)
	{
		// Current entry is an LFN entry, we need to read entries until we reach the regular directory entry
		entryOut.hasLFN = true;

		FAT_LFNEntry* lfnEntry = reinterpret_cast<FAT_LFNEntry*>(entry);
		do {
			std::wstring chunk;

			// Copy chars to separate buffers to prevent compiler warning about unaligned pointers
			char16_t chars1[5];
			char16_t chars2[6];
			char16_t chars3[2];

			memcpy(chars1, lfnEntry->chars1, sizeof(chars1));
			memcpy(chars2, lfnEntry->chars2, sizeof(chars2));
			memcpy(chars3, lfnEntry->chars3, sizeof(chars3));

			AppendLFNChars(chars1, chunk, 5);
			AppendLFNChars(chars2, chunk, 6);
			AppendLFNChars(chars3, chunk, 2);

			entryOut.lfn = chunk + entryOut.lfn; // The order of the LFN entries is reversed, so we need to add each chunk to the start of the string

			dirHandle->position += sizeof(FAT_DirectoryEntry);
			status = NextSector(dirHandle);
			if(FAT32_ERROR(status))
			{
				fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to advance the current cluster in file handle\n");
				return std::unexpected(status);
			}
			uint64_t newLBA;
			if(FLAG_GET_BOOLEAN(dirHandle->flags, FAT_FILE_FLAG_ROOT_DIRECTORY) && fatType != FAT_Types::FAT32) newLBA = dirHandle->currentCluster;
			else newLBA = ClusterToLba(dirHandle->currentCluster) + dirHandle->currentSectorInCluster;
			if(newLBA != bufferSector)
			{
				// Read the new sector
				bufferSector = newLBA;
				status = partMgr->ReadSectors(partDesc, bufferSector, 1, buffer);
				if(FAT32_ERROR(status))
				{
					fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read the next directory sector\n");
					return std::unexpected(status);
				}
			}
			lfnEntry = reinterpret_cast<FAT_LFNEntry*>(buffer + dirHandle->position % bootSector.bpb.bytesPerSector);
		} while(lfnEntry->attribute == FAT_ATTRIB_LFN);
		entry = reinterpret_cast<FAT_DirectoryEntry*>(lfnEntry);
	}

	// Now we have the actual directory entry, store it and update the position
	entryOut.dirEntry = *entry;

	// Store the entry cluster and offsetInCluster
	entryOut.cluster = dirHandle->currentCluster;
	entryOut.offsetInCluster = dirHandle->currentSectorInCluster * bootSector.bpb.bytesPerSector + dirHandle->position % bootSector.bpb.bytesPerSector;

	dirHandle->position += sizeof(FAT_DirectoryEntry);
	status = NextSector(dirHandle);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to advance the current cluster in file handle\n");
		return std::unexpected(status);
	}
	uint64_t newLBA;
	if(FLAG_GET_BOOLEAN(dirHandle->flags, FAT_FILE_FLAG_ROOT_DIRECTORY) && fatType != FAT_Types::FAT32) newLBA = dirHandle->currentCluster;
	else newLBA = ClusterToLba(dirHandle->currentCluster) + dirHandle->currentSectorInCluster;
	if(newLBA != bufferSector)
	{
		// Read the new sector
		bufferSector = newLBA;
		status = partMgr->ReadSectors(partDesc, bufferSector, 1, buffer);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read the next directory sector\n");
			return std::unexpected(status);
		}
	}
	if(entryOut.dirEntry.shortFileName[0] == 0x00) return std::unexpected(FAT32_FAT_END_OF_DIRECTORY);
	return entryOut;
}

std::expected<std::pair<uint32_t, uint64_t>, FAT32_STATUS> FAT::FindFreeEntryGap(FAT_FileHandle* dirHandle, size_t totalEntries)
{
	// Setup variables

	uint32_t currentCluster = dirHandle->firstCluster;
	uint32_t currentSectorInCluster = 0;
	uint64_t position = 0;

	size_t runCount = 0;
	uint32_t runStartCluster = 0;
	uint32_t runStartOffsetInCluster = 0;

	uint8_t buffer[SECTOR_SIZE];
	uint64_t bufferSector;

	bool exclusiveRootDirHandling = false;
	if(FLAG_GET_BOOLEAN(dirHandle->flags, FAT_FILE_FLAG_ROOT_DIRECTORY) && fatType != FAT_Types::FAT32) 
	{
		exclusiveRootDirHandling = true;
		bufferSector = currentCluster;
	}
	else bufferSector = ClusterToLba(currentCluster);

	// Make sure cluster is not an EOF and doesn't point to a reserved cluster value
	if(!exclusiveRootDirHandling && (dirHandle->firstCluster >= EOCTreshold || dirHandle->firstCluster < 2))
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid FindFreeEntryGap input parameters\n");
		return std::unexpected(FAT32_INVALID_PARAMETER);
	}

	// Read the first directory sector
	FAT32_STATUS status = partMgr->ReadSectors(partDesc, bufferSector, 1, buffer);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read the directory to find a free entry gap\n");
		return std::unexpected(status);
	}
	bool endReached = false;
	bool found = false;

	while(true)
	{
		// If FAT12 / FAT16, instead of checking if current cluster is EOF, check if it's above the root directory end
		// For FAT12 / FAT16 current cluster of the root directory is stored as sectors, not actual clusters
		if(exclusiveRootDirHandling && currentCluster - dirHandle->firstCluster >= rootDirSectors) break;

		// Check if the current entry is empty and update vars
		FAT_DirectoryEntry* dirEntry = reinterpret_cast<FAT_DirectoryEntry*>(buffer + position % SECTOR_SIZE);
		if(dirEntry->shortFileName[0] == 0xE5 || dirEntry->shortFileName[0] == 0x00)
		{
			if(dirEntry->shortFileName[0] == 0x00) endReached = true;
			if(runCount == 0)
			{
				runStartCluster = currentCluster;
				runStartOffsetInCluster = position % (this->clusterSize);
			}
			runCount++;
			if(runCount >= totalEntries)
			{
				found = true;
				break;
			}
		}
		else if(!endReached) runCount = 0;

		// Advance position, read next sector if needed, advance to next cluster if needed
		position += sizeof(FAT_DirectoryEntry);
		if(position % SECTOR_SIZE < sizeof(FAT_DirectoryEntry))
		{
			// Crossed sector boundary

			if(exclusiveRootDirHandling) bufferSector = ++currentCluster;
			else
			{
				currentSectorInCluster++;
				if(currentSectorInCluster >= bootSector.bpb.sectorsPerCluster)
				{
					auto nextRes = NextCluster(currentCluster);
					if(!nextRes)
					{
						fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to get the next cluster for finding a free directory entry gap\n");
						return std::unexpected(nextRes.error());
					}
					if(nextRes.value() >= EOCTreshold) break;
					currentCluster = nextRes.value();
					currentSectorInCluster = 0;
				}
				bufferSector = ClusterToLba(currentCluster) + currentSectorInCluster;
			}

			status = partMgr->ReadSectors(partDesc, bufferSector, 1, buffer);
			if(FAT32_ERROR(status))
			{
				fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read the directory to find a free entry gap while advancing position\n");
				return std::unexpected(status);
			}
		}
	}

	if(!found)
	{
		// Reached the end of the directory, expand the directory by allocating an extra cluster to it
		auto allocRes = AllocateCluster(currentCluster);
		if(!allocRes)
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to allocate a new cluster to expand the directory for finding a free entry gap\n");
			return std::unexpected(allocRes.error());
		}
		if(runCount == 0)
		{
			uint32_t newCluster = allocRes.value();
			runStartCluster = newCluster;
			runStartOffsetInCluster = 0;
		}
		
		// assume the new cluster we allocated + previous gap is big enough to return
	}

	return std::pair(runStartCluster, runStartOffsetInCluster);
}

std::expected<FAT_LFNDirectoryEntry, FAT32_STATUS> FAT::OpenDirectoryEntry(FAT_FileHandle dirHandle, const std::wstring_view name, bool directory)
{
	if(name.length() > FAT_MAX_FILE_NAME_LENGTH)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: name %ls is too long(limit is %lu)\n", name.data(), FAT_MAX_FILE_NAME_LENGTH);
		return std::unexpected(FAT32_FAT_NAME_TOO_LONG);
	}

	char sfnBuffer[FAT_SFN_MAX_LENGTH];
	bool checkLFN = !ConvertNameToSFN(name, directory, sfnBuffer);

	std::expected<FAT_LFNDirectoryEntry, FAT32_STATUS> readEntryStatus;
	while(true)
	{
		readEntryStatus = ReadEntry(&dirHandle);
		if(!readEntryStatus)
		{
			if(readEntryStatus.error() == FAT32_FAT_END_OF_DIRECTORY) return std::unexpected(FAT32_NOT_FOUND);
			else
			{
				fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read an entry from a directory\n");
				return std::unexpected(readEntryStatus.error());
			}
		}

		FAT_LFNDirectoryEntry entry = readEntryStatus.value();

		// Check if the directory entry is the one we're looking for
		if(entry.dirEntry.shortFileName[0] == 0xE5) continue; // Deleted entry
		if(entry.dirEntry.shortFileName[0] == 0x05) entry.dirEntry.shortFileName[0] = 0xE5; // 0x05 should be replaced with 0xE5 because 0xE5 is a valid SFN character, but on the disk it's used as a deletion marker, and if the character is 0x05, it means that it was supposed to be the ASCII character 0xE5
		if(checkLFN != entry.hasLFN) continue; // One of the entries is SFN, other is LFN, means it doesn't match
		if(FLAG_GET_BOOLEAN(entry.dirEntry.fileAttribs, FAT_ATTRIB_DIRECTORY) != directory) continue; // Entries have different attributes
		if(FLAG_GET_BOOLEAN(entry.dirEntry.fileAttribs, FAT_ATTRIB_ARCHIVE) != !directory) continue; // Entries have different attributes

		if(checkLFN)
		{
			if(name == entry.lfn)
			{
				// Found the entry, return it
				return entry;
			}
			continue;
		}
		else
		{
			if(memcmp(sfnBuffer, entry.dirEntry.shortFileName, FAT_SFN_DISK_LENGTH) == 0)
			{
				// Found the entry, return it
				return entry;
			}
			continue;
		}
	}

	return std::unexpected(FAT32_DISK_ERROR); // Normally unreachable, return any error
}

// Utilities

uint64_t FAT::AllocateHandle()
{
	for(size_t i = 0; i < handles.size(); i++)
	{
		FAT_FileHandle* handle = &handles[i];
		if(!handle->open) return i;
	}

	handles.push_back({});
	return handles.size() - 1;
}

std::expected<uint32_t, FAT32_STATUS> FAT::NextCluster(uint32_t previous)
{
	return ReadFAT(previous);
}

std::expected<uint32_t, FAT32_STATUS> FAT::AllocateCluster(uint32_t previousLast)
{
	auto scanFatForFreeCluster = [&](uint32_t startCluster) -> std::expected<uint32_t, FAT32_STATUS>
	{
		uint32_t current = startCluster;
		while(current < this->totalClusters + 2)
		{
			auto readRes = ReadFAT(current);
			if(!readRes)
			{
				fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to read FAT to find a free cluster\n");
				return std::unexpected(readRes.error());
			}
			if(readRes.value() == 0) return current;
			current++;
		}
		return 0;
	};

	if(fsInfoPresent && fsInfo.nextFreeClusterHint != 0xFFFFFFFF && fsInfo.nextFreeClusterHint < this->totalClusters + 2)
	{
		// Scan from the cluster hint in FSInfo
		auto scanRes = scanFatForFreeCluster(fsInfo.nextFreeClusterHint);
		if(!scanRes) return std::unexpected(scanRes.error());

		uint32_t value = scanRes.value();
		if(value == 0)
		{
			// No free clusters found, try scanning from the start of the data region
			scanRes = scanFatForFreeCluster(2);
			if(!scanRes) return std::unexpected(scanRes.error());
			value = scanRes.value();
			if(value == 0) return std::unexpected(FAT32_FAT_OUT_OF_SPACE);
		}

		// Update the FAT to mark the cluster as allocated, and if specified, mark the previous last cluster to point to the new one instead of EOC
		FAT32_STATUS status = WriteFAT(value, EOCTreshold);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to mark newly allocated cluster as used\n");
			return std::unexpected(status);
		}
		if(previousLast != 0)
		{
			status = WriteFAT(previousLast, value);
			if(FAT32_ERROR(status))
			{
				fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to link previous last allocated cluster to the newly allocated one\n");
				return std::unexpected(status);
			}
		}

		// Update FSInfo
		if(fsInfo.lastKnownFreeClusterCount != 0xFFFFFFFF && fsInfo.lastKnownFreeClusterCount < this->totalClusters + 2) fsInfo.lastKnownFreeClusterCount--;
		fsInfo.nextFreeClusterHint = value + 1;
		if(fsInfo.nextFreeClusterHint > totalClusters + 1) fsInfo.nextFreeClusterHint = 2;

		ZeroCluster(value);
		return value;
	}
	else
	{
		// FSInfo isn't present, scan from 2nd cluster
		auto scanRes = scanFatForFreeCluster(2);
		if(!scanRes) return std::unexpected(scanRes.error());

		uint32_t value = scanRes.value();
		if(value == 0) return std::unexpected(FAT32_FAT_OUT_OF_SPACE);

		// Update the FAT to mark the cluster as allocated, and if specified, mark the previous last cluster to point to the new one instead of EOC
		FAT32_STATUS status = WriteFAT(value, EOCTreshold);
		if(FAT32_ERROR(status))
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to mark newly allocated cluster as used\n");
			return std::unexpected(status);
		}
		if(previousLast != 0)
		{
			status = WriteFAT(previousLast, value);
			if(FAT32_ERROR(status))
			{
				fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to link previous last allocated cluster to the newly allocated one\n");
				return std::unexpected(status);
			}
		}

		ZeroCluster(value);
		return value;
	}
}

FAT32_STATUS FAT::ZeroCluster(uint32_t cluster)
{
	uint8_t* clusterBuffer = reinterpret_cast<uint8_t*>(calloc(bootSector.bpb.sectorsPerCluster, bootSector.bpb.bytesPerSector));
	if(!clusterBuffer)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to allocate a cluster buffer to zero out the cluster\n");
		return FAT32_MEMORY_ALLOCATION_FAILED;
	}

	FAT32_STATUS status = partMgr->WriteSectors(partDesc, ClusterToLba(cluster), bootSector.bpb.sectorsPerCluster, clusterBuffer);
	if(FAT32_ERROR(status))
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to write the zeroed out cluster to the disk\n");
		free(clusterBuffer);
		return status;
	}

	free(clusterBuffer);
	return FAT32_SUCCESS;
}

std::expected<uint32_t, FAT32_STATUS> FAT::GetClusterChainLength(uint32_t startCluster)
{
	if(startCluster < 2 || startCluster >= EOCTreshold) return 0;

	uint32_t count = 1;
	uint32_t current = startCluster;

	while(true)
	{
		auto nextRes = NextCluster(current);
		if(!nextRes)
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to get cluster chain length\n");
			return std::unexpected(nextRes.error());
		}
		current = nextRes.value();
		if(current >= EOCTreshold) return count;
		count++;
		if(count >= this->totalClusters)
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Cluster chain length exceeds the total disk cluster count\n");
			return std::unexpected(FAT32_FAT_CORRUPTED_FILE_ALLOCATION_TABLE);
		}
	}
}

std::expected<uint32_t, FAT32_STATUS> FAT::GetLastCluster(uint32_t chainCluster)
{
	if(chainCluster < 2) return 0;
	if(chainCluster >= EOCTreshold) return chainCluster;
 
	uint32_t current = chainCluster;
	uint32_t count = 0;

	while(true)
	{
		auto nextRes = NextCluster(current);
		if(!nextRes)
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to get cluster chain length\n");
			return std::unexpected(nextRes.error());
		}
		uint32_t valueAt = nextRes.value();
		if(valueAt >= EOCTreshold) return current;
		current = valueAt;
		count++;
		if(count >= this->totalClusters)
		{
			fprintf(stderr, "[FAT32] [FAT] [ERROR]: Cluster chain length exceeds the total disk cluster count\n");
			return std::unexpected(FAT32_FAT_CORRUPTED_FILE_ALLOCATION_TABLE);
		}
	}
}

uint64_t FAT::RegisterHandle(FAT_FileHandle handle)
{
	uint64_t handleID = AllocateHandle();

	FAT_FileHandle* handleOut = &handles[handleID];
	*handleOut = handle;
	handleOut->generation++;
	handleOut->open = true;

	return handleID;
}

uint64_t FAT::ClusterToLba(uint32_t cluster)
{
	return ((cluster - 2) * bootSector.bpb.sectorsPerCluster) + this->dataRegionStartSector;
}

std::expected<uint32_t, FAT32_STATUS> FAT::ReadFAT(uint32_t cluster)
{
	uint64_t positionInFAT = cluster * sizeof(uint32_t);
	uint64_t sectorInFAT = positionInFAT / bootSector.bpb.bytesPerSector;
	FAT32_STATUS status;
	if(sectorInFAT < this->fatCacheSector || sectorInFAT >= this->fatCacheSector + FAT_CACHE_SECTORS)
	{
		if(cacheDirty)
		{
			// Write first FAT
			status = partMgr->WriteSectors(partDesc, this->fatStartSector + this->fatCacheSector, FAT_CACHE_SECTORS, this->fatCache);
			if(FAT32_ERROR(status)) return status;
			cacheDirty = false;

			// Write second FAT
			status = partMgr->WriteSectors(partDesc, this->fatStartSector + this->fatSectors + this->fatCacheSector, FAT_CACHE_SECTORS, this->fatCache);
			if(FAT32_ERROR(status)) return status;
			cacheDirty = false;
		}

		this->fatCacheSector = sectorInFAT;
		status = partMgr->ReadSectors(partDesc, this->fatStartSector + this->fatCacheSector, FAT_CACHE_SECTORS, this->fatCache);
		if(FAT32_ERROR(status)) return std::unexpected(status);
	}

	uint64_t positionInCache = (sectorInFAT - fatCacheSector) * bootSector.bpb.bytesPerSector + positionInFAT % bootSector.bpb.bytesPerSector;
	return *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(fatCache) + positionInCache);
}

FAT32_STATUS FAT::WriteFAT(uint32_t cluster, uint32_t value)
{
	uint64_t positionInFAT = cluster * sizeof(uint32_t);
	uint64_t sectorInFAT = positionInFAT / bootSector.bpb.bytesPerSector;
	FAT32_STATUS status;
	if(sectorInFAT < this->fatCacheSector || sectorInFAT >= this->fatCacheSector + FAT_CACHE_SECTORS)
	{
		if(cacheDirty)
		{
			// Write first FAT
			status = partMgr->WriteSectors(partDesc, this->fatStartSector + this->fatCacheSector, FAT_CACHE_SECTORS, this->fatCache);
			if(FAT32_ERROR(status)) return status;
			cacheDirty = false;

			// Write second FAT
			status = partMgr->WriteSectors(partDesc, this->fatStartSector + this->fatSectors + this->fatCacheSector, FAT_CACHE_SECTORS, this->fatCache);
			if(FAT32_ERROR(status)) return status;
			cacheDirty = false;
		}

		this->fatCacheSector = sectorInFAT;
		status = partMgr->ReadSectors(partDesc, this->fatStartSector + this->fatCacheSector, FAT_CACHE_SECTORS, this->fatCache);
		if(FAT32_ERROR(status)) return status;
	}

	uint64_t positionInCache = (sectorInFAT - fatCacheSector) * bootSector.bpb.bytesPerSector + positionInFAT % bootSector.bpb.bytesPerSector;
	*reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(fatCache) + positionInCache) = value;
	cacheDirty = true;
	return FAT32_SUCCESS;
}

bool FAT::ConvertNameToSFN(const std::wstring_view name, bool directory, char* bufferOut)
{
	for(wchar_t c : name)
	{
		if(c > 0x7F) return false;	
	}

	std::string asciiName;
	asciiName.reserve(name.length());

	for(wchar_t c : name) asciiName += (char)c;

	return ConvertNameToSFN(std::string_view(asciiName), directory, bufferOut);
}

bool FAT::ConvertNameToSFN(const std::string_view name, bool directory, char* bufferOut)
{
	size_t len = name.length();
	if(len == 0 || len > FAT_SFN_MAX_LENGTH) return false;

	static const std::string invalid = "\"*+,/:;<=>?[\\]| ";

	size_t dotPos = name.find('.');

	if(directory)
	{
		if(dotPos != std::string::npos) return false;
	}
	else
	{
		if(dotPos != std::string::npos)
		{
			if(name.find('.', dotPos + 1) != std::string::npos) return false;
			if(dotPos == 0) return false;
			if(dotPos + 1 == len) return false;
			if(dotPos > 8) return false;
			if(len - dotPos - 1 > 3) return false;
		}
		else if(len > 8) return false;
	}

	memset(bufferOut, ' ', FAT_SFN_DISK_LENGTH);

	size_t out = 0;
	for(size_t i = 0; i < len && name[i] != '.'; i++)
	{
		unsigned char c = (unsigned char)name[i];
		if(c < 0x20 || c == 0x7F) return false;
		if(c < 0x80 && invalid.find(c) != std::string::npos) return false;
		if(islower(c)) return false;
		bufferOut[out++] = c;
	}

	if(dotPos != std::string::npos)
	{
		out = 8;

		for(size_t i = dotPos + 1; i < len; i++)
		{
			unsigned char c = (unsigned char)name[i];

			if(c < 0x20 || c == 0x7F) return false;
			if(c < 0x80 && invalid.find(c) != std::string::npos) return false;
			if(islower(c)) return false;
			bufferOut[out++] = c;
		}
	}

	return true;
}

void FAT::ConvertSFNToName(char* sfnBuffer, bool directory, std::wstring& nameOut)
{
	std::string name;
	if(directory)
	{
		for(size_t i = 0; i < FAT_SFN_DISK_LENGTH && sfnBuffer[i] != ' '; i++) name.push_back(sfnBuffer[i]);
		nameOut = utf8StringToWideString(name);
		return;
	}

	for(size_t i = 0; i < 8 && sfnBuffer[i] != ' '; i++) name.push_back(sfnBuffer[i]);
	name.push_back('.');
	for(size_t i = 8; i < FAT_SFN_DISK_LENGTH && sfnBuffer[i] != ' '; i++) name.push_back(sfnBuffer[i]);

	nameOut = utf8StringToWideString(name);

	return;
}

void FAT::GenerateLFNAliasSFN(const std::wstring_view name, bool directory, char* bufferOut)
{
	static const std::string invalid = "\"*+,/:;<=>?[\\]| ";

	memset(bufferOut, ' ', FAT_SFN_MAX_LENGTH);

	std::string ascii;
	ascii.reserve(name.length());
	for(wchar_t c : name)
	{
		if(c > 0x7F) continue;

		unsigned char ch = (unsigned char)c;

		if(ch < 0x20 || ch == 0x7F) continue;
		if(invalid.find(ch) != std::string::npos) continue;

		ascii += (char)toupper(ch);
	}

	std::string base, extension;

	size_t dotPos = ascii.find_last_of('.');

	if(!directory && dotPos != std::string::npos)
	{
		base = ascii.substr(0, dotPos);
		extension = ascii.substr(dotPos + 1);
	}
	else base = ascii;

	if(base.length() > 6) base.resize(6);

	memcpy(bufferOut, base.data(), base.length());

	bufferOut[base.length()] = '~';
	bufferOut[base.length() + 1] = '1';

	if(!directory && !extension.empty())
	{
		if(extension.length() > 3) extension.resize(3);
		memcpy(bufferOut + 8, extension.data(), extension.length());
	}
}

uint8_t FAT::CalculateSFNChecksum(char* sfn)
{
	uint8_t checksum = 0;

	for(size_t i = 0; i < FAT_SFN_DISK_LENGTH; i++) checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + (uint8_t)sfn[i];

	return checksum;
}

FAT32_STATUS FAT::NextSector(FAT_FileHandle* handle)
{
	if(FLAG_GET_BOOLEAN(handle->flags, FAT_FILE_FLAG_ROOT_DIRECTORY) && fatType != FAT_Types::FAT32)
	{
		if(handle->position / bootSector.bpb.bytesPerSector >= handle->currentCluster - handle->firstCluster)
		{
			handle->currentCluster++; // In FAT12/FAT16 the root directory is linear and fixed size, so a simple increment is enough, no cluster chain traversal is needed
			if(handle->currentCluster - handle->firstCluster >= this->rootDirSectors) return FAT32_FAT_END_OF_DIRECTORY;
		}
		return FAT32_SUCCESS;
	}

	size_t currentSector = (handle->position / bootSector.bpb.bytesPerSector - this->dataRegionStartSector) % bootSector.bpb.sectorsPerCluster;

	if(currentSector != handle->currentSectorInCluster)
	{
		handle->currentSectorInCluster++;

		if(handle->currentSectorInCluster >= bootSector.bpb.sectorsPerCluster)
		{
			handle->currentSectorInCluster = 0;
			auto nextClusterRes = NextCluster(handle->currentCluster);
			if(!nextClusterRes)
			{
				fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to get the next cluster\n");
				return nextClusterRes.error();
			}
			handle->currentCluster = nextClusterRes.value();
		}
	}
	return FAT32_SUCCESS;
}

void FAT::AppendLFNChars(const char16_t* chars, std::wstring& strOut, size_t count)
{
	for(size_t i = 0; i < count; i++)
	{
		if(chars[i] == 0x0000) return;
		if(chars[i] != 0xFFFF) strOut.push_back(static_cast<wchar_t>(chars[i]));
	}
}

std::expected<std::vector<FAT_LFNEntry>, FAT32_STATUS> FAT::ConstructLFNEntries(const std::wstring_view name, uint8_t sfnChecksum)
{
	if(name.length() > FAT_MAX_FILE_NAME_LENGTH)
	{
		fprintf(stderr, "[FAT32] [FAT] [ERROR]: Name %ls is too long(max is %lu chars)\n", name.data(), FAT_MAX_FILE_NAME_LENGTH);
		return std::unexpected(FAT32_FAT_NAME_TOO_LONG);
	}

	// Convert name from UTF-32 to UTF-16 if sizeof(wchar_t) == 4
	std::vector<char16_t> nameChars;
	nameChars.reserve(name.length());

	if(sizeof(wchar_t) == 4)
	{
		char16_t buffer[2];
		for(size_t i = 0; i < name.length(); i++)
		{
			codepoint_t cp = utf32ToCodepoint(name[i]);
			size_t used = codepointToUtf16(cp, buffer);
			if(used == 0)
			{
				fprintf(stderr, "[FAT32] [FAT] [ERROR]: Invalid Unicode character detected in name %ls\n", name.data());
				return std::unexpected(FAT32_FAT_INVALID_NAME);
			}
			for(size_t j = 0; j < used; j++) nameChars.push_back(buffer[j]);
		}
	}
	else
	{
		nameChars.resize(name.length());
		memcpy(nameChars.data(), name.data(), name.length() * sizeof(char16_t));
	}

	size_t lfnEntriesNeeded = (nameChars.size() + FAT_CHARS_PER_LFN_ENTRY - 1) / FAT_CHARS_PER_LFN_ENTRY;

	std::vector<FAT_LFNEntry> lfnEntries;
	lfnEntries.reserve(lfnEntriesNeeded);

	// Construct the LFN Entries
	size_t vectorOffset = 0;
	for(size_t i = 0; i < lfnEntriesNeeded; i++)
	{
		FAT_LFNEntry entry = {};

		entry.order = i + 1; // Because we're constructing in the normal order, entries are orderded from 1 to 0x40
		if(i == lfnEntriesNeeded - 1) entry.order |= 0x40;

		entry.attribute = FAT_ATTRIB_LFN;
		entry.checksum = sfnChecksum;

		char16_t charBuffer[FAT_CHARS_PER_LFN_ENTRY];
		
		memset(charBuffer, 0xFF, sizeof(charBuffer));
		
		size_t charsLeft = nameChars.size() - vectorOffset;
		size_t charsToCopy = std::min(charsLeft, FAT_CHARS_PER_LFN_ENTRY);

		memcpy(charBuffer, nameChars.data() + vectorOffset, charsToCopy * sizeof(char16_t));

		if(charsToCopy < FAT_CHARS_PER_LFN_ENTRY) charBuffer[charsToCopy] = 0x0000;

		memcpy(entry.chars1, charBuffer, sizeof(entry.chars1));
		memcpy(entry.chars2, charBuffer + 5, sizeof(entry.chars2));
		memcpy(entry.chars3, charBuffer + 11, sizeof(entry.chars3));

		vectorOffset += charsToCopy;

		lfnEntries.push_back(entry);
	}

	// Reverse the order of the LFN entries in the vector to the order the FAT expects

	std::reverse(lfnEntries.begin(), lfnEntries.end());
	return lfnEntries;
}

FAT_TimeDate FAT::GetTimeDateFromDirEntry(FAT_DirectoryEntry* entry)
{
	FAT_TimeDate res = {};

	if(!entry) return res;

	res.year = 1980 + ((entry->lastModificationDate >> 9) & 0x7F);
	res.month = ((entry->lastModificationDate >> 5) & 0x0F);
	res.day = entry->lastModificationDate & 0x1F;

	res.hour = (entry->lastModificationTime >> 11) & 0x1F;
	res.minute = (entry->lastModificationTime >> 5) & 0x3F;
	res.second = (entry->lastModificationTime & 0x1F) * 2;
	res.millisecond = 0;

	return res;
}

FAT_TimeDate FAT::GetCurrentTimeDate()
{
	std::chrono::time_point now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
	std::chrono::zoned_time local = std::chrono::zoned_time(std::chrono::current_zone(), now);

	std::chrono::local_seconds localTime = local.get_local_time();
	std::chrono::local_days dp = std::chrono::floor<std::chrono::days>(localTime);

	std::chrono::year_month_day ymd(dp);
	std::chrono::hh_mm_ss hms(localTime - dp);

	FAT_TimeDate td = {};

	td.year = static_cast<uint16_t>(static_cast<int>(ymd.year()));
	td.month = static_cast<uint8_t>(static_cast<unsigned>(ymd.month()));
	td.day = static_cast<uint8_t>(static_cast<unsigned>(ymd.day()));

	td.hour = static_cast<uint8_t>(hms.hours().count());
	td.minute = static_cast<uint8_t>(hms.minutes().count());
	td.second = static_cast<uint8_t>(hms.seconds().count());

	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(localTime - std::chrono::floor<std::chrono::seconds>(localTime));
	td.millisecond = static_cast<uint16_t>(ms.count());

	return td;
}

void FAT::SetDirectoryEntryTimeDate(FAT_DirectoryEntry* entry, FAT_TimeDate* td, FAT_TimestampTypes update)
{
	uint16_t fatTime = ((td->hour & 0x1F) << 11) | ((td->minute & 0x3F) << 5) | ((td->second / 2) & 0x1F);
	uint16_t fatDate = (((td->year - 1980) & 0x7F) << 9) | ((td->month & 0x0F) << 5) | (td->day & 0x1F);

	if(FLAG_GET_BOOLEAN(update, FAT_TIMESTAMP_TYPE_CREATED))
	{
		entry->creationTime = fatTime;
		entry->creationDate = fatDate;
		entry->creationTimeTenths = static_cast<uint8_t>(td->millisecond / 10); // Store time in hundreds, not tenths
	}

	if(FLAG_GET_BOOLEAN(update, FAT_TIMESTAMP_TYPE_ACCESSED)) entry->lastAccessDate = fatDate;
	if(FLAG_GET_BOOLEAN(update, FAT_TIMESTAMP_TYPE_MODIFIED))
	{
		entry->lastModificationTime = fatTime;
		entry->lastModificationDate = fatDate;
	}
}

FAT::~FAT()
{
	// Write the FAT Cache back to the disk if marked dirty
	if(cacheDirty)
	{
		FAT32_STATUS status = partMgr->WriteSectors(partDesc, this->fatStartSector + this->fatCacheSector, FAT_CACHE_SECTORS, fatCache);
		if(FAT32_ERROR(status)) fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to write the FAT cache back\n");

		status = partMgr->WriteSectors(partDesc, this->fatStartSector + this->fatSectors + this->fatCacheSector, FAT_CACHE_SECTORS, fatCache);
		if(FAT32_ERROR(status)) fprintf(stderr, "[FAT32] [FAT] [ERROR]: Failed to write the FAT cache back to the second FAT\n");
	}
	free(fatCache);
}