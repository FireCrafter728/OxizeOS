// SPDX-License-Identifier: GPL-3.0-or-later
//
// OxizeOS Operating System for the x86 amd64(x86_64) architecture
// Copyright (C) 2025-2026 FireCrafter728
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <elf.hpp>

using namespace BootMgr::ELF;

#define ELF64_R_SYM(i) ((uint32_t)((i) >> 32))
#define ELF64_R_TYPE(i) ((uint32_t)(i))
#define R_X86_64_RELATIVE 8

bool ELF::CreateHandle(ELF_Handle* handle, EFI_FILE_PROTOCOL* ElfFile, FS::FS* FileSystem)
{
	if(!handle || !ElfFile || !FileSystem) {
		printf("[BOOTMGR] [ELF-Loader] [ERROR]: Invalid input ptr specified\r\n");
		return false;
	}

	handle->FileSystem = FileSystem;
	handle->ElfFile = ElfFile;

	// Read ELF Header from disk

	if(FileSystem->ReadFile(ElfFile, &handle->header, sizeof(ELF_Header64)) != sizeof(ELF_Header64)) {
		printf("[BOOTMGR] [ELF-Loader] [ERROR]: Failed to read ELF Header\r\n");
		return false;
	}

	// Parse ELF Header

	if(memcmp(handle->header.Signature, ELF_Signature, 4) != 0) {
		printf("[BOOTMGR] [ELF-Loader] [ERROR]: Input file is NOT an ELF File!\r\n");
		return false;
	}

	if(handle->header.Arch != ELF_AMD64 || handle->header.InstructionSet != ELF_ISET_AMD64) {
		printf("[BOOTMGR] [ELF-Loader] [ERROR]: Input executable arch/instruction set is not supported, only AMD64(x86_64) is supported\r\n");
		return false;
	} 

	if(handle->header.Type != ET_DYN) {
		printf("[BOOTMGR] [ELF-Loader] [ERROR]: Only Position Independent Executables can be executed\r\n");
		return false;
	}

	if(handle->header.PhdrEntrySize != sizeof(ELF_Phdr64)) printf("[BOOTMGR] [ELF-Loader] [WARN]: ELF Phdr entry size does not match the expected size\r\n");
	if(handle->header.ShdrEntrySize != sizeof(ELF_Shdr64)) printf("[BOOTMGR] [ELF-Loader] [WARN]: ELF Shdr entry size does not match the expected size\r\n");

	handle->phdrCount = handle->header.PhdrEntryCount;
	handle->phdrOff = handle->header.PhdrOffset;
	handle->shdrCount = handle->header.ShdrEntryCount;
	handle->shdrOff = handle->header.ShdrOffset;

	uintptr_t minAddr = 0xFFFFFFFFFFFFFFFF, maxAddr = 0;
	for(size_t i = 0; i < handle->phdrCount; i++)
	{
		ELF_Phdr64 phdr;
		if(EFI_ERROR(handle->FileSystem->Seek(handle->ElfFile, handle->phdrOff + i * sizeof(ELF_Phdr64)))) {
			printf("[BOOTMGR] [ELF-Loader] [ERROR]: Failed to seek in ELF Executable to phdr offset\r\n");
			return false;
		}

		if(handle->FileSystem->ReadFile(handle->ElfFile, &phdr, sizeof(ELF_Phdr64)) != sizeof(ELF_Phdr64)) {
			printf("[BOOTMGR] [ELF-Loader] [ERROR]: Failed to read ELF Program header\r\n");
			return false;
		}

		if(phdr.SegmentType != PT_LOAD) continue;

		uintptr_t segStart = phdr.virt;
		uintptr_t segEnd = phdr.virt + phdr.LoadSize;

		if(segStart < minAddr) minAddr = segStart;
		if(segEnd > maxAddr) maxAddr = segEnd;
	}

	handle->extraOffset = minAddr;

	uintptr_t minPages = minAddr & ~0xFFFULL;
	uintptr_t maxPages = (maxAddr + 0xFFF) & ~0xFFFULL;
	size_t loadSize = maxPages - minPages;
	handle->LoadPages = loadSize / 0x1000;

	printf("[BOOTMGR] [ELF-Loader] [INFO]: Extra offset: 0x%llX, LoadAddr: 0x%llX, loadPages: %llu\r\n", handle->extraOffset, handle->LoadAddr, handle->LoadPages);

	return true;
}

void ELF::SetLoadAddr(ELF_Handle* handle, uintptr_t Addr)
{
	handle->LoadAddr = Addr;
}

void ELF::SetVirtLoadAddr(ELF_Handle* handle, uintptr_t Addr)
{
	handle->ExecAddr = Addr;
}

bool ELF::HandleRelocations(ELF_Handle* handle)
{
    // Iterate all section headers and find .rela sections
    uintptr_t ShdrOffset = 0;

    for (size_t i = 0; i < handle->shdrCount; i++)
    {
        ELF_Shdr64 shdr;

        if (EFI_ERROR(handle->FileSystem->Seek(
                handle->ElfFile,
                handle->shdrOff + ShdrOffset)))
        {
            printf("[BOOTMGR] [ELF-Loader] [ERROR]: Failed to seek to shdr\n");
            return false;
        }

        if (handle->FileSystem->ReadFile(
                handle->ElfFile,
                &shdr,
                sizeof(ELF_Shdr64)) != sizeof(ELF_Shdr64))
        {
            printf("[BOOTMGR] [ELF-Loader] [ERROR]: Failed to read shdr\n");
            return false;
        }

        ShdrOffset += sizeof(ELF_Shdr64);

        if (shdr.SectionType != SHT_RELA)
            continue;

		printf("Found .RELA.DYN Section %llu\r\n", i);

        if (shdr.EntrySize == 0)
        {
            printf("[BOOTMGR] [ELF-Loader] [ERROR]: RELA EntrySize is 0\n");
            return false;
        }

        size_t entryCount = shdr.SectionSize / shdr.EntrySize;

        ELF_Rela64 relaEntries[16];
        size_t relaEntriesArraySize =
            sizeof(relaEntries) / sizeof(relaEntries[0]);

        uintptr_t relaOffset = 0;

        while (entryCount > 0)
        {
			
            size_t nextEntryCount =
                min(relaEntriesArraySize, entryCount);
            size_t nextChunkSize =
                nextEntryCount * sizeof(ELF_Rela64);

            if (EFI_ERROR(handle->FileSystem->Seek(
                    handle->ElfFile,
                    shdr.OffsetInImage + relaOffset)))
            {
                printf("[BOOTMGR] [ELF-Loader] [ERROR]: Failed to seek to RELA\n");
                return false;
            }

            if (handle->FileSystem->ReadFile(
                    handle->ElfFile,
                    relaEntries,
                    nextChunkSize) != nextChunkSize)
            {
                printf("[BOOTMGR] [ELF-Loader] [ERROR]: Failed to read RELA\n");
                return false;
            }

            for (size_t k = 0; k < nextEntryCount; k++)
            {
                ELF_Rela64* rela = &relaEntries[k];
                uint32_t type = ELF64_R_TYPE(rela->Info);

                uint64_t* target =
                    reinterpret_cast<uint64_t*>(
                        handle->LoadAddr + rela->OffsetInMemory);

                if (type == R_X86_64_RELATIVE)
                {
                    *target = handle->ExecAddr + rela->Addend;
					printf("Relocating entry %llu at 0x%llX to 0x%llX\r\n", shdr.SectionSize / shdr.EntrySize - entryCount, target, handle->ExecAddr + rela->Addend);
                }
            }

            entryCount -= nextEntryCount;
            relaOffset += nextChunkSize;
        }
    }

    return true;
}

bool ELF::LoadImage(ELF_Handle* handle)
{
	if(!handle) {
		printf("[BOOTMGR] [ELF-Loader] [ERROR]: Invalid input ptr specified\r\n");
		return false;
	}

	if(!handle->LoadAddr) {
		printf("[BOOTMGR] [ELF-Loader] [ERROR]: Load address not specified\r\n");
		return false;
	}

	for(size_t i = 0; i < handle->phdrCount; i++)
	{
		ELF_Phdr64 phdr;
		if(EFI_ERROR(handle->FileSystem->Seek(handle->ElfFile, handle->phdrOff + i * sizeof(ELF_Phdr64)))) {
			printf("[BOOTMGR] [ELF-Loader] [ERROR]: Failed to seek in ELF Executable to phdr offset\r\n");
			return false;
		}

		if(handle->FileSystem->ReadFile(handle->ElfFile, &phdr, sizeof(ELF_Phdr64)) != sizeof(ELF_Phdr64)) {
			printf("[BOOTMGR] [ELF-Loader] [ERROR]: Failed to read ELF Program header\r\n");
			return false;
		}

		if(phdr.SegmentType != PT_LOAD) continue;
		
		void* addr = reinterpret_cast<void*>(phdr.virt - handle->extraOffset + handle->LoadAddr);

		if(EFI_ERROR(handle->FileSystem->Seek(handle->ElfFile, phdr.DataOffset))) {
			printf("[BOOTMGR] [ELF-Loader] [ERROR]: Failed to seek in ELF Executable to program header data\r\n");
			return false;
		}

		if(handle->FileSystem->ReadFile(handle->ElfFile, addr, phdr.SegmentSize) != phdr.SegmentSize) {
			printf("[BOOTMGR] [ELF-Loader] [ERROR]: Failed to read Program header data\r\n");
			return false;
		}

		if(phdr.LoadSize > phdr.SegmentSize) memset(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(addr) + phdr.SegmentSize), 0, phdr.LoadSize - phdr.SegmentSize);
	}

	HandleRelocations(handle);

	return true;
}