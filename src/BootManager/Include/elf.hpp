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

#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |------------------------------------------------------------------------| //
// | OxizeOS Boot Manager Implementation                                    | //
// | ELF: Driver for loading Executable and Linkable format PIE executables | //
// |------------------------------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdint.hpp>
#include <SysTable.hpp>

#ifndef PACK
#define PACK __attribute__((packed))
#endif

namespace BootMgr
{
	namespace ELF
	{
		enum ELF_Arch : uint8_t
		{
			ELF_X86 = 1,
			ELF_IA32 = 1,
			ELF_AMD64 = 2,
			ELF_X86_64 = 2,
		};

		enum ELF_Types : uint16_t
		{
			ET_NONE = 0,
			ET_REL = 1,
			ET_EXEC = 2,
			ET_DYN = 3,
			ET_CORE = 4,
		};

		enum ELF_InstructionSets : uint16_t
		{
			ELF_ISET_NONE = 0x0,
			ELF_ISET_X86 = 0x3,
			ELF_ISET_ARM32 = 0x28,
			ELF_ISET_AMD64 = 0x3E,
			ELF_ISET_X86_64 = 0x3E,
			ELF_ISET_ARM64 = 0xB7,
		};

		struct PACK ELF_Header64
		{
			uint8_t Signature[4];
			uint8_t Arch;
			uint8_t Endianess;
			uint8_t HeaderVersion;
			uint8_t OSABI;
			uint64_t _Reserved;
			uint16_t Type;
			uint16_t InstructionSet;
			uint32_t ELFVersion;
			uint64_t EntryOffset;
			uint64_t PhdrOffset;
			uint64_t ShdrOffset;
			uint32_t Flags;
			uint16_t HeaderSize;
			uint16_t PhdrEntrySize;
			uint16_t PhdrEntryCount;
			uint16_t ShdrEntrySize;
			uint16_t ShdrEntryCount;
			uint16_t ShdrStringTableSectionIndex;
		};

		enum ELF_SegmentTypes
		{
			PT_NONE = 0x0,
			PT_LOAD = 0x1,
			PT_DYN = 0x2,
			PT_INTERP = 0x3,
			PT_NOTE = 0x4,
			PT_SHLIB = 0x5,
			PT_PHDR = 0x6,
			PT_TLS = 0x7,
		};

		enum ELF_SegmentFlags
		{
			PF_X = 0x1,
			PF_W = 0x2,
			PF_R = 0x4,
		};

		struct PACK ELF_Phdr64
		{
			uint32_t SegmentType;
			uint32_t Flags;
			uint64_t DataOffset;
			uint64_t virt, phys;
			size_t SegmentSize;
			size_t LoadSize;
			uint64_t Alignment;
		};

		enum ELF_SectionTypes
		{
			SHT_NONE = 0x00,
			SHT_PROGBITS = 0x01,
			SHT_SYMTAB = 0x02,
			SHT_STRTAB = 0x03,
			SHT_RELA = 0x04,
			SHT_HASH = 0x05,
			SHT_DYN = 0x06,
			SHT_NOTE = 0x07,
			SHT_NOBITS = 0x08,
			SHT_REL = 0x09,
			SHT_SHLIB = 0x0A,
			SHT_DYNSYM = 0x0B,
			SHT_INIT_ARRAY = 0x0E,
			SHT_FINI_ARRAY = 0x0F,
			SHT_PREINIT_ARRAY = 0x10,
			SHT_GROUP = 0x11,
			SHT_SYMTAB_SHNDX = 0x12,
			SHT_NUM = 0x13,
		};

		enum ELF_SectionFlags
		{
			SHF_WRITE = 0x1,
			SHF_ALLOC = 0x2,
			SHF_EXECINSTR = 0x4,
			SHF_MERGE = 0x10,
			SHF_STRINGS = 0x20,
			SHF_INFO_LINK = 0x40,
			SHF_LINK_ORDER = 0x80,
			SHF_OS_NONCONFORMING = 0x100,
			SHF_GROUP = 0x200,
			SHF_TLS = 0x400,
		};

		struct PACK ELF_Shdr64
		{
			uint32_t SectionNameStringIndex;
			uint32_t SectionType;
			uint64_t Flags;
			uint64_t LoadAddr;
			uint64_t OffsetInImage;
			size_t SectionSize;
			uint32_t SectionLink;
			uint32_t SectionInfo;
			uint64_t AddrAlign;
			size_t EntrySize;
		};

		struct PACK ELF_Rela64
		{
			uintptr_t OffsetInMemory;
			uint32_t Info;
			uint32_t Symbol;
			int64_t Addend;
		};

		enum ELF_DynamicTags
		{
			DT_NULL = 0x00,
			DT_NEEDED = 0x01,
			DT_PLTRELSZ = 0x02,
			DT_PLTGOT = 0x03,
			DT_HASH = 0x04,
			DT_STRTAB = 0x05,
			DT_SYMTAB = 0x06,
			DT_RELA = 0x07,
			DT_RELASZ = 0x08,
			DT_RELAENT = 0x09,
			DT_STRSZ = 0x0A,
			DT_SYMENT = 0x0B,
			DT_INIT = 0x0C,
			DT_FINI = 0x0D,
			DT_SONAME = 0x0E,
			DT_RPATH = 0x0F,
			DT_SYMBOLIC = 0x10,
			DT_REL = 0x11,
			DT_RELSZ = 0x12,
			DT_RELENT = 0x13,
			DT_PLTREL = 0x14,
			DT_DEBUG = 0x15,
			DT_TEXTREL = 0x16,
			DT_JMPREL = 0x17,
			DT_BIND_NOW = 0x18,
			DT_INIT_ARRAY = 0x19,
			DT_FINI_ARRAY = 0x1A,
			DT_INIT_ARRAYSZ = 0x1B,
			DT_FINI_ARRAYSZ = 0x1C,
			DT_RUNPATH = 0x1D,
			DT_FLAGS = 0x1E,
			DT_PREINIT_ARRAY = 0x20,
			DT_PREINIT_ARRAYSZ = 0x21,
			DT_SYMTAB_SHNDX = 0x22,
		};

		struct PACK ELF_Dyn64
		{
			int64_t DynamicTag;
			uint64_t DynamicValue;
		};

		struct ELF_Handle
		{
			EFI_FILE_PROTOCOL* ElfFile;
			uintptr_t LoadAddr; // phys
			uintptr_t ExecAddr; // virt
			size_t LoadPages;
			uintptr_t extraOffset;
			FS::FS* FileSystem;
			ELF_Header64 header;
			size_t phdrCount;
			uintptr_t phdrOff;
			size_t shdrCount;
			uintptr_t shdrOff;
		};

		constexpr uint8_t ELF_Signature[4] = {0x7F, 'E', 'L', 'F'};

		class ELF
		{
		public:
			bool CreateHandle(ELF_Handle* handle, EFI_FILE_PROTOCOL* ElfFile, FS::FS* FileSystem);
			void SetLoadAddr(ELF_Handle* handle, uintptr_t Addr);
			void SetVirtLoadAddr(ELF_Handle* handle, uintptr_t Addr);
			bool LoadImage(ELF_Handle* handle);
		private:
			bool HandleRelocations(ELF_Handle* handle);
		};
	}
}