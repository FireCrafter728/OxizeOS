// SPDX-License-Identifier: GPL-3.0-or-later

#include <PELoader.hpp>

#include <stdio.hpp>
#include <string.hpp>
#include <algorithm>

using namespace BootMgr::PELoader;

bool PE_Loader::OpenImage(EFI_FILE_PROTOCOL* imageFile, FS::FS* FileSystem, PE_ImageHandle* handleOut)
{
	if(!imageFile || !FileSystem || !handleOut)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Invalid OpenImage parameters\r\n");
		return false;
	}

	*handleOut = {};

	handleOut->imageFile = imageFile;
	handleOut->FileSystem = FileSystem;

	// Get the image size and make sure it's not empty
	size_t imageFileSize = FileSystem->GetFileSize(imageFile);
	if(imageFileSize == 0)
	{
		if(FileSystem->GetLastStatus() != EFI_SUCCESS)
		{
			printf("[BOOTMGR] [PE Loader] [ERROR]: Failed to get the file size, error code: 0x%llX\r\n", FileSystem->GetLastStatus());
			return false;
		}
		printf("[BOOTMGR] [PE Loader] [ERROR]: Cannot load a PE Image with a size of 0\r\n");
		return false;
	}

	// Seek to the start of the image to ensure we're reading from the start
	EFI_STATUS status = FileSystem->Seek(imageFile, 0);
	if(EFI_ERROR(status))
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Failed to seek to the start of the image file, error code: 0x%lX\r\n", status);
		return false;
	}

	// Read the first 4 bytes of the image file to determine if it's a valid signature and if the MZ Header is present
	char sigBuffer[4];
	if(FileSystem->ReadFile(imageFile, sigBuffer, 4) != 4) {
		printf("[BOOTMGR] [PE Loader] [ERROR]: Failed to read the first image signature, error code: 0x%llX\r\n", FileSystem->GetLastStatus());
		return false;
	}

	uintptr_t peHeaderOffset = 0;
	if(memcmp(sigBuffer, DOSSignature, sizeof(DOSSignature)) == 0)
	{
		// DOS Header present, read it all, then validate the new header offset and store it
		status = FileSystem->Seek(imageFile, 0);
		if(EFI_ERROR(status))
		{
			printf("[BOOTMGR] [PE Loader] [ERROR]: Failed to reset image file position, error code: 0x%llX\r\n", status);
			return false;
		}

		if(FileSystem->ReadFile(imageFile, &handleOut->optDOSHeader, sizeof(PE_MZHeader)) != sizeof(PE_MZHeader))
		{
			printf("[BOOTMGR] [PE Loader] [ERROR]: Failed to read the DOS MZ Header from the image file, error code: 0x%llX\r\n", FileSystem->GetLastStatus());
			return false;
		}

		if(handleOut->optDOSHeader.newHeaderOffsetInFile >= imageFileSize)
		{
			printf("[BOOTMGR] [PE Loader] [ERROR]: new header offset field in the DOS Header is outside the file\r\n");
			return false;
		}

		if(handleOut->optDOSHeader.newHeaderOffsetInFile < sizeof(PE_MZHeader))
		{
			printf("[BOOTMGR] [PE Loader] [ERROR]: new header offset field in the DOS Header points to the DOS Header itself\r\n");
			return false;
		}

		peHeaderOffset = handleOut->optDOSHeader.newHeaderOffsetInFile;
	}

	// Read the PE Header

	status = FileSystem->Seek(imageFile, peHeaderOffset);
	if(EFI_ERROR(status))
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Failed to seek to the PE Header inside image file\r\n");
		return false;
	}

	if(FileSystem->ReadFile(imageFile, &handleOut->peHeader, sizeof(PE_PEHeader)) != sizeof(PE_PEHeader))
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Failed to read the PE Header, error code: 0x%llX\r\n", FileSystem->GetLastStatus());
		return false;
	}

	// Validate the PE Signature

	if(memcmp(handleOut->peHeader.signature, PESignature, sizeof(PESignature)) != 0)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Non-PE Image file: couldn't find the PE Signature at offset 0x%llX\r\n", peHeaderOffset);
		return false;
	}

	// Check the machine, must be AMD64(x86-64)

	if(handleOut->peHeader.machine != static_cast<uint16_t>(PE_Machines::AMD64))
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Unknown executable format %s\r\n", handleOut->peHeader.machine);
		return false;
	}

	// Validate the rest of the fields

	if(handleOut->peHeader.sectionCount == 0)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Invalid section count in PE Header\r\n");
		return false;
	}

	if(handleOut->peHeader.optionalHeaderSize == 0)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Missing optional header, it is required to execute an executable\r\n");
		return false;
	}

	if(handleOut->peHeader.optionalHeaderSize < sizeof(PE_OptionalHeader))
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Optional header size %llu is below the minimum of %llu", handleOut->peHeader.optionalHeaderSize, sizeof(PE_OptionalHeader));
		return false;
	}

	if(handleOut->peHeader.optionalHeaderSize > imageFileSize)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Optional header is outside the image file\r\n");
		return false;
	}

	// Read the optional header

	if(FileSystem->ReadFile(imageFile, &handleOut->optionalHeader, sizeof(PE_OptionalHeader)) != sizeof(PE_OptionalHeader))
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Failed to read the Optional Header, error code: 0x%llX\r\n", FileSystem->GetLastStatus());
		return false;
	}

	// Verify the Optional Header magic number, must be 0x020B to indicate a 64-bit executable

	if(handleOut->optionalHeader.magic != OptionalHeaderMagicNumber)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Non-PE Image file: couldn't find the Optional Header Signature at offset 0x%llX", peHeaderOffset + sizeof(PE_PEHeader));
		return false;
	}

	if(!IS_POWER_OF_2(handleOut->optionalHeader.fileAlignment))
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: File Alignment field in the optional header is not a power of 2\r\n");
		return false;
	}

	if(!IS_POWER_OF_2(handleOut->optionalHeader.sectionAlignment))
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Section Alignment field in the optional header is not a power of 2\r\n");
		return false;
	}

	if(handleOut->optionalHeader.sectionAlignment < handleOut->optionalHeader.fileAlignment)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Optional Header section alignment value is lower than the file alignment value\r\n");
		return false;
	}

	if(handleOut->optionalHeader.imageSize == 0)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Optional Header image size value is NULL\r\n");
		return false;
	}

	if(handleOut->optionalHeader.headersSize > imageFileSize)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Image headers size field is outside the image\r\n");
		return false;
	}

	if(handleOut->optionalHeader.headersSize % handleOut->optionalHeader.fileAlignment != 0)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Image headers size field isn't aligned to the file alignment value\r\n");
		return false;
	}

	if(handleOut->optionalHeader.entryPointAddr >= handleOut->optionalHeader.imageSize)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Entry point is outside the image\r\n");
		return false;
	}

	if(handleOut->optionalHeader.RvaAndSizesCount > 16)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Data Directory Entry count %lu is outside the maximum of 16\r\n", handleOut->optionalHeader.RvaAndSizesCount);
		return false;
	}

	// Check the PE Characteristics

	if(!(handleOut->peHeader.characteristics & PE_CHARACTERISTIC_EXECUTABLE_IMAGE))
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: PE Image is not an executable image\r\n");
		return false;
	}

	if(handleOut->peHeader.characteristics & PE_CHARACTERISTIC_DLL)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Cannot load a DLL as an executable\r\n");
		return false;
	}

	if(handleOut->peHeader.characteristics & PE_CHARACTERISTIC_RELOCS_STRIPPED)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Cannot load a PE Image without the relocation entries\r\n");
		return false;
	}

	if(handleOut->peHeader.characteristics & PE_CHARACTERISTIC_32BIT_MACHINE)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Cannot load a 32-bit image on a 64-bit system\r\n");
		return false;
	}

	// Read the Data directory entries

	if(FileSystem->ReadFile(imageFile, handleOut->dataDirEntries, handleOut->optionalHeader.RvaAndSizesCount * sizeof(PE_DataDirectoryEntry)) != handleOut->optionalHeader.RvaAndSizesCount * sizeof(PE_DataDirectoryEntry))
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Failed to read the Data Directory entries, error code: 0x%llX\r\n", FileSystem->GetLastStatus());
		return false;
	}

	// Verify all data directory entries
	for(size_t i = 0; i < handleOut->optionalHeader.RvaAndSizesCount; i++)
	{
		PE_DataDirectoryEntry* entry = &handleOut->dataDirEntries[i];
		if(entry->size == 0 && entry->virtualAddress == 0) continue; // non-present entry

		if(entry->virtualAddress > handleOut->optionalHeader.imageSize || entry->size > handleOut->optionalHeader.imageSize - entry->virtualAddress)
		{
			printf("[BOOTMGR] [PE Loader] [ERROR]: data directory entry %llu virtual Address is outside the image\r\n", i);
			return false;
		}
	}

	handleOut->sectionHeaderOffset = FileSystem->GetFilePosition(imageFile);

	// Read each section header and verify it

	PE_SectionHeader sectionHdr = {};

	for(size_t i = 0; i < handleOut->peHeader.sectionCount; i++)
	{
		if(FileSystem->ReadFile(imageFile, &sectionHdr, sizeof(PE_SectionHeader)) != sizeof(PE_SectionHeader))
		{
			printf("[BOOTMGR] [PE Loader] [ERROR]: Failed to read the Section Header %llu, error code: 0x%llX\r\n", i, FileSystem->GetLastStatus());
			return false;
		}

		uint32_t sectionSize = max(sectionHdr.virtualSize, sectionHdr.sizeOfRawData);

		if(sectionHdr.virtualAddress > handleOut->optionalHeader.imageSize || sectionSize > handleOut->optionalHeader.imageSize - sectionHdr.virtualAddress)
		{
			printf("[BOOTMGR] [PE Loader] [ERROR]: section header %llu size is outside the image\r\n", i);
			return false;
		}

		if(sectionHdr.sizeOfRawData != 0)
		{
			if(sectionHdr.rawDataPtr > imageFileSize || sectionHdr.sizeOfRawData > imageFileSize - sectionHdr.rawDataPtr)
			{
				printf("[BOOTMGR] [PE Loader] [ERROR]: section header %llu raw data is outside the image\r\n", i);
				return false;
			}

			if(sectionHdr.sizeOfRawData % handleOut->optionalHeader.fileAlignment != 0 || sectionHdr.rawDataPtr % handleOut->optionalHeader.fileAlignment != 0)
			{
				printf("[BOOTMGR] [PE Loader] [ERROR]: section header %llu raw data size / ptr is not aligned to optional header fileAlignment value\r\n", i);
				return false;
			}
		}

		// Check if the section contains the entry point, and make sure it's executable
		if(sectionHdr.virtualAddress >= handleOut->optionalHeader.entryPointAddr && sectionHdr.virtualAddress + sectionHdr.virtualSize < handleOut->optionalHeader.entryPointAddr)
		{
			if(!(sectionHdr.characteristics & PE_SECTION_MEM_EXECUTE))
			{
				printf("[BOOTMGR] [PE Loader] [ERROR]: Section header %llu that contains the entry point is not executable\r\n", i);
				return false;
			}
		}

		// Check if the section contains the resources
		PE_DataDirectoryEntry* entry = &handleOut->dataDirEntries[static_cast<int>(PE_DirectoryEntryTypes::Resource)];
		if(sectionHdr.virtualAddress <= entry->virtualAddress && entry->virtualAddress < sectionHdr.virtualAddress + sectionHdr.virtualSize)
		{
			// Section is a .rsrc section, store it's size in the handle
			handleOut->resourceSectionOffset = sectionHdr.virtualAddress;
		}
	}

	// Make sure that the section headers aren't outside the headersSize
	if(FileSystem->GetFilePosition(imageFile) > handleOut->optionalHeader.headersSize)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Some section headers are outside the headersSize field in the optional header\r\n");
		return false;
	}
	
	return true;
}

bool PE_Loader::LoadImageIntoMemory(PE_ImageHandle* imageHandle)
{
	if(!imageHandle)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Invalid GetImageLoadSize parameters\r\n");
		return false;
	}

	if(imageHandle->loadAddrVirt == 0)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Cannot load an image without a configured virtual address\r\n");
		return false;
	}

	// Zero out the entire load memory

	memset(reinterpret_cast<void*>(imageHandle->loadAddrPhys), 0, imageHandle->optionalHeader.imageSize);

	// Seek to the file start
	EFI_STATUS status = imageHandle->FileSystem->Seek(imageHandle->imageFile, 0);
	if(EFI_ERROR(status))
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Failed to seek back to the start of the file\r\n");
		return false;
	}

	// Read all of the headers into memory

	if(imageHandle->FileSystem->ReadFile(imageHandle->imageFile, reinterpret_cast<void*>(imageHandle->loadAddrPhys), imageHandle->optionalHeader.headersSize) != imageHandle->optionalHeader.headersSize)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Failed to read the PE Headers into load address\r\n");
		return false;
	}

	// Iterate over each section header in memory and load each section into memory

	PE_SectionHeader* sectionHeaders = reinterpret_cast<PE_SectionHeader*>(imageHandle->loadAddrPhys + imageHandle->sectionHeaderOffset);

	for(size_t i = 0; i < imageHandle->peHeader.sectionCount; i++)
	{
		PE_SectionHeader* sectionHdr = &sectionHeaders[i];

		// Check if the section is loadable
		if(sectionHdr->virtualAddress == 0 || (!(sectionHdr->characteristics & PE_SECTION_MEM_EXECUTE) && !(sectionHdr->characteristics & PE_SECTION_MEM_READ) && !(sectionHdr->characteristics & PE_SECTION_MEM_WRITE) && !(sectionHdr->characteristics & PE_SECTION_CNT_CODE) && !(sectionHdr->characteristics & PE_SECTION_CNT_INITIALIZED_DATA) && !(sectionHdr->characteristics & PE_SECTION_CNT_UNINITIALIZED_DATA))) continue;

		// Seek to the section position
		status = imageHandle->FileSystem->Seek(imageHandle->imageFile, sectionHdr->rawDataPtr);
		if(EFI_ERROR(status))
		{
			printf("[BOOTMGR] [PE Loader] [ERROR]: Failed to seek to the section %llu, error code: 0x%llX\r\n", i, status);
			return false;
		}

		// Read the section into memory
		if(imageHandle->FileSystem->ReadFile(imageHandle->imageFile, reinterpret_cast<void*>(imageHandle->loadAddrPhys + sectionHdr->virtualAddress), sectionHdr->sizeOfRawData) != sectionHdr->sizeOfRawData)
		{
			printf("[BOOTMGR] [PE Loader] [ERROR]: Failed to read section %llu into memory at physical address 0x%llX, error code: 0x%llX\r\n", i, imageHandle->loadAddrPhys, imageHandle->FileSystem->GetLastStatus());
			return false;
		}
	}

	// Handle relocations
	if(!HandleRelocations(imageHandle))
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Failed to apply relocations while loading the image\r\n");
		return false;
	}

	return true;
}

size_t PE_Loader::GetImageLoadSize(PE_ImageHandle* imageHandle)
{
	if(!imageHandle)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Invalid GetImageLoadSize parameters\r\n");
		return false;
	}

	return imageHandle->optionalHeader.imageSize;
}

size_t PE_Loader::GetImageResSectionOffset(PE_ImageHandle* imageHandle)
{
	if(!imageHandle)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Invalid GetImageResSectionSize parameters\r\n");
		return false;
	}

	return imageHandle->resourceSectionOffset;
}

size_t PE_Loader::GetImageResSectionRootDirOffset(PE_ImageHandle* imageHandle)
{
	if(!imageHandle)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Invalid GetImageResSectionSize parameters\r\n");
		return false;
	}

	return imageHandle->dataDirEntries[static_cast<int>(PE_DirectoryEntryTypes::Resource)].virtualAddress + imageHandle->loadAddrVirt;
}

uintptr_t PE_Loader::GetImageAbsoluteEntryPoint(PE_ImageHandle* imageHandle)
{
	if(!imageHandle)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Invalid GetImageAbsoluteEntryPoint parameters\r\n");
		return false;
	}

	return imageHandle->loadAddrVirt + imageHandle->optionalHeader.entryPointAddr;
}

bool PE_Loader::SetImageLoadAddr(PE_ImageHandle* imageHandle, uintptr_t phys, uintptr_t virt)
{
	if(!imageHandle || virt < MapAddr)
	{
		printf("[BOOTMGR] [PE Loader] [ERROR]: Invalid SetImageLoadAddr parameters\r\n");
		return false;
	}

	imageHandle->loadAddrPhys = phys;
	imageHandle->loadAddrVirt = virt;
	imageHandle->resourceSectionOffset += virt;

	return true;
}

bool PE_Loader::HandleRelocations(PE_ImageHandle* imageHandle)
{
	PE_DataDirectoryEntry* dirEntry = &imageHandle->dataDirEntries[static_cast<int>(PE_DirectoryEntryTypes::BaseReloc)];
	uintptr_t relocSectionOffset = dirEntry->virtualAddress;
	PE_BaseRelocHeader* relocHeader = reinterpret_cast<PE_BaseRelocHeader*>(imageHandle->loadAddrPhys + relocSectionOffset);

	int64_t relocationDelta = static_cast<int64_t>(imageHandle->loadAddrVirt) - static_cast<int64_t>(imageHandle->optionalHeader.imageBase);
	size_t relocOffset = 0;

	// Relocation section is split in blocks, each describing up to a 4KiB block of relocation entries
	while(relocOffset < dirEntry->size)
	{
		if(relocHeader->sizeOfBlock < 8 || relocHeader->sizeOfBlock > dirEntry->size - relocOffset)
		{
			printf("[BOOTMGR] [PE Loader] [ERROR]: Invalid relocation header block size field %lu\r\n", relocHeader->sizeOfBlock);
			return false;
		}

		size_t entriesInThisBlock = (relocHeader->sizeOfBlock - sizeof(PE_BaseRelocHeader)) / sizeof(PE_BaseRelocEntry);
		PE_BaseRelocEntry* relocEntry = reinterpret_cast<PE_BaseRelocEntry*>(reinterpret_cast<uintptr_t>(relocHeader) + sizeof(PE_BaseRelocHeader));
		// Each relocation entry has a 4 bit type and 12 bit offset field, the absolute offset is the header->virtualAddress + offset
		for(size_t i = 0; i < entriesInThisBlock; i++)
		{
			uint16_t type = relocEntry->value >> 12;
			uint16_t offset = relocEntry->value & 0x0FFF;

			switch(static_cast<PE_RelocTypes>(type))
			{
				case PE_RelocTypes::Dir64: // 64-bit relocations
				{
					uint64_t* target = reinterpret_cast<uint64_t*>(imageHandle->loadAddrPhys + relocHeader->virtualAddress + offset);
					if(reinterpret_cast<uintptr_t>(target) >= imageHandle->loadAddrPhys + imageHandle->optionalHeader.imageSize)
					{
						printf("[BOOTMGR] [PE Loader] [ERROR]: Relocation target is outside the image\r\n");
						return false;
					}

					*target += relocationDelta;
					break;
				};
				case PE_RelocTypes::Absolute: break; // Ignore, padding
				default: // Unsupported relocation, log error
				{
					printf("[BOOTMGR] [PE Loader] [ERROR]: Unknown relocation type %u\r\n", type);
					return false;
				}
			}

			// Advance to the next entry
			relocEntry++;
		}

		relocOffset += relocHeader->sizeOfBlock;

		// Advance to the next header
		relocHeader = reinterpret_cast<PE_BaseRelocHeader*>(reinterpret_cast<uintptr_t>(relocHeader) + relocHeader->sizeOfBlock);
	}

	return relocOffset == dirEntry->size;
}

const char* PE_Loader::getMachineStr(uint16_t machine)
{
	switch(static_cast<PE_Machines>(machine))
	{
		case PE_Machines::IA_32: return "x86";
		case PE_Machines::IA_64: return "IA-64";
		case PE_Machines::EFIByteCode: return "EBC";
		case PE_Machines::AMD64: return "x86-64";
		case PE_Machines::ARMThumbMixed: return "ARMThumb-Mixed";
		case PE_Machines::ARM64: return "AArch64";
		case PE_Machines::RISCV32: return "RISC-V 32-bit";
		case PE_Machines::RISCV64: return "RISC-V 64-bit";
		case PE_Machines::RISCV128: return "RISC-V 128-bit";
		case PE_Machines::LOONGARCH32: return "LoongArch 32-bit";
		case PE_Machines::LOONGARCH64: return "LoongArch 64-bit";
		default: break;
	}

	return "Unknown";
}