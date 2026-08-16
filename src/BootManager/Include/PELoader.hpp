// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <defs.hpp>

#include <stdint.hpp>
#include <FileSystem.hpp>

#include <Protocol/SimpleFileSystem.h>

namespace BootMgr
{
	namespace PELoader
	{
		// PE Header structures

		// legacy 16-bit DOS "MZ" Header, some PE executables might have it before the PE header, some might not
		struct PACK PE_MZHeader
		{
			uint16_t magic; // ASCII "MZ"
			uint16_t bytesOnLastPage;
			uint16_t pages;
			uint16_t relocs;
			uint16_t headerSzInParagraphs;
			uint16_t minExtraParagraphsNeeded;
			uint16_t maxExtraParagraphsNeeded;
			uint16_t initialSS;
			uint16_t initialSP;
			uint16_t checksum;
			uint16_t initialIP;
			uint16_t initialCS;
			uint16_t relocTableOffsetInFile;
			uint16_t overlayNumber;
			uint16_t _Reserved[4];
			uint16_t oemIdentifier;
			uint16_t oemInfo;
			uint16_t _Reserved1[10];
			uint32_t newHeaderOffsetInFile; // Some implementations might use int32_t here, but because the offset should be positive, using uint32_t is fine
		};

		// PE/COFF Header, alone it's not enough, we need the optional header too
		struct PACK PE_PEHeader
		{
			uint8_t signature[4]; // ASCII "PE\0\0"
			uint16_t machine;
			uint16_t sectionCount;
			uint32_t timeDateStamp;
			uint32_t symbolTablePtr;
			uint32_t symbolCount;
			uint16_t optionalHeaderSize;
			uint16_t characteristics;
		};

		// PE Optional Header, contains very important details about executing the executable
		// Some details might be Windows specific
		struct PACK PE_OptionalHeader
		{
			// Important fields
			uint16_t magic; // 0x020B -> PE32+
			uint8_t majorLinkerVersion;
			uint8_t minorLinkerVersion;
			uint32_t codeSize;
			uint32_t initializedDataSize;
			uint32_t uninitializedDataSize;
			uint32_t entryPointAddr;
			uint32_t baseOfCode;
			uint64_t imageBase;
			uint32_t sectionAlignment;
			uint32_t fileAlignment;

			// Windows / OS specific fields, some values are our own
			// OS and Image versions are combined just for the kernel to form the full 4-number OS version
			uint16_t majorOSVersion; // contains OxizeOS Edition
			uint16_t minorOSVersion; // contains OxizeOS Major version
			uint16_t majorImageVersion; // contains OxizeOS Minor version
			uint16_t minorImageVersion; // contains OxizeOS Build number
			uint16_t majorSubsystemVersion; // Do not care about this for now
			uint16_t minorSubsystemVersion; // Same as above
			uint32_t win32Version; // Do not care about this at all
			uint32_t imageSize;
			uint32_t headersSize;
			uint32_t checksum;
			uint16_t subsystem; // Replaced with our own subsystems, 0x140 for the kernel / external drivers
			uint16_t dllCharacteristics;
			uint64_t stackReserveSize; // Ignored for the kernel, as it allocates it's own stack
			uint64_t stackCommitSize; // Same as above
			uint64_t heapReserveSize; // Bootloader doesn't provide any heap, the kernel creates it's own
			uint64_t heapCommitSize; // Same as above
			uint32_t loaderFlags;
			uint32_t RvaAndSizesCount;
		};

		struct PACK PE_DataDirectoryEntry
		{
			uint32_t virtualAddress;
			uint32_t size;
		};

		// Enums for different fields in the PE Headers

		enum class PE_Subsystem : uint16_t
		{
			Unknown = 0x0000, // Unknown subsystem

			// Windows subsystems

			Win32_Native = 0x0001, // Drivers and native Win32 processes(commonly referred to as NATIVE)
			Win32_WinGUI = 0x0002, // Windows GUI Subsystem(commonly referred to as WINDOWS)
			Win32_WinCUI = 0x0003, // Windows Character Subsystem(commonly referred to as CONSOLE)
			Win32_OS2CUI = 0x0005, // OS/2 Character subsystem
			Win32_POSIXCUI = 0x0007, // POSIX Character subsystem
			Win32_NativeWin = 0x0008, // Native 9x driver
			Win32_WinCEGUI = 0x0009, // Windows CE
			Win32_EFIApplication = 0x000A, // EFI Application
			Win32_EFIBootServiceDriver = 0x000B, // Windows EFI Driver with boot services
			Win32_EFIRuntimeServiceDriver = 0x000C, // Windows EFI Driver with runtime services
			Win32_EFIROMImage = 0x000D, // EFI ROM Image
			Win32_XBOXApplication = 0x000E, // XBOX Application / game
			Win32_WinBootApplication = 0x0010, // Windows boot application

			// OxizeOS Subsystems

			OxizeOS_KernelModeApplication = 0x0140, // Kernel itself or an external driver
			// More subsystems will be added later
		};

		enum class PE_Machines : uint16_t
		{
			IA_32 = 0x014c, // x86 32-bit application
			IA_64 = 0x0200, // Itanium architecture application
			EFIByteCode = 0x0EBC, // For EFI
			AMD64 = 0x8664, // x86-64, AMD64 application
			ARMThumbMixed = 0x01c2, // 32-bit ARM applications mixed with Thumb / Thumb-2 instructions
			ARM64 = 0xAA64, // AArch64 64-bit ARM application
			RISCV32 = 0x5032, // RISC-V 32-bit application
			RISCV64 = 0x5064, // RISC-V 64-bit application
			RISCV128 = 0x5128, // RISC-V 128-bit extension
			LOONGARCH32 = 0x6232, // LoongArch 32-bit application
			LOONGARCH64 = 0x6264, // LoongArch 64-bit application
		};

		enum PE_Characteristics : uint16_t
		{
			PE_CHARACTERISTIC_RELOCS_STRIPPED = (1 << 0), // Relocation entries are stripped
			PE_CHARACTERISTIC_EXECUTABLE_IMAGE = (1 << 1), // File is an executable image(not an object image)
			PE_CHARACTERISTIC_LINE_NUMBERS_STRIPPED = (1 << 2), // Line numbers are stripped
			PE_CHARACTERISTIC_LOCAL_SYMBOLS_STRIPPED = (1 << 3), // Local symbols are stripped
			PE_CHARACTERISTIC_LARGE_ADDRESS_AWARE = (1 << 5), // Supports virtual addresses above 2GiB
			PE_CHARACTERISTIC_BYTES_REVERSED_LO = (1 << 7), // Bytes of machine word are reversed
			PE_CHARACTERISTIC_32BIT_MACHINE = (1 << 8), // 32-bit word machine
			PE_CHARACTERISTIC_DEBUG_STRIPPED = (1 << 9), // Debugging info stripped
			PE_CHARACTERISTIC_SYSTEM = (1 << 12), // System file
			PE_CHARACTERISTIC_DLL = (1 << 13), // Image is a Dynamic Load Library
			PE_CHARACTERISTIC_BYTES_REVERSED_HI = (1 << 15), // Bytes of machine word are reversed
		};
		
		enum class PE_DirectoryEntryTypes : uint16_t
		{
			Export = 0,
			Import = 1,
			Resource = 2,
			Exception = 3,
			Security = 4,
			BaseReloc = 5,
			Debug = 6,
			Architecture = 7,
			GlobalPtr = 8,
			TLS = 9,
			LoadConfig = 10,
			BoundImport = 11,
			IAT = 12,
			DelayImport = 13,
			CLR = 14,
			Reserved = 15,
		};

		enum PE_DLLCharacteristics : uint16_t
		{
			PE_DLL_CHARACTERISTICS_HIGH_ENTROPY_VA = (1 << 5),
			PE_DLL_CHARACTERISTICS_DYNAMIC_BASE = (1 << 6),
			PE_DLL_CHARACTERISTICS_FORCE_INTEGRITY = (1 << 7),
			PE_DLL_CHARACTERISTICS_NX_COMPAT = (1 << 8),
			PE_DLL_CHARACTERISTICS_NO_ISOLATION = (1 << 9),
			PE_DLL_CHARACTERISTICS_NO_SEH = (1 << 10),
			PE_DLL_CHARACTERISTICS_NO_BIND = (1 << 11),
			PE_DLL_CHARACTERISTICS_APP_CONTAINER = (1 << 12),
			PE_DLL_CHARACTERISTICS_WDM_DRIVER = (1 << 13),
			PE_DLL_CHARACTERISTICS_GUARD_CF = (1 << 14),
			PE_DLL_CHARACTERISTICS_TERMINAL_SERVER_AWARE = (1 << 15),
		};

		// Section headers

		struct PE_SectionHeader
		{
			char name[8];
			uint32_t virtualSize, virtualAddress;
			uint32_t sizeOfRawData, rawDataPtr;
			uint32_t relocPtr, lineNumberPtr;
			uint16_t relocCount;
			uint16_t lineNumberCount;
			uint32_t characteristics;
		};

		enum PE_SectionCharacteristics
		{
			PE_SECTION_TYPE_NO_PAD = 0x08,

			PE_SECTION_CNT_CODE = 0x20,
			PE_SECTION_CNT_INITIALIZED_DATA = 0x40,
			PE_SECTION_CNT_UNINITIALIZED_DATA = 0x80,

			PE_SECTION_LINK_OTHER = 0x0100,
			PE_SECTION_LINK_INFO = 0x0200,
			PE_SECTION_LINK_REMOVE = 0x0800,
			PE_SECTION_LINK_COMDAT = 0x1000,

			PE_SECTION_GPREL = 0x8000,

			PE_SECTION_MEM_PURGEABLE = 0x00020000,
			PE_SECTION_MEM_16BIT = 0x00020000,
			PE_SECTION_MEM_LOCKED = 0x00040000,
			PE_SECTION_MEM_PRELOAD = 0x00080000,

			PE_SECTION_ALIGN_1BYTES = 0x00100000,
			PE_SECTION_ALIGN_2BYTES = 0x00200000,
			PE_SECTION_ALIGN_4BYTES = 0x00300000,
			PE_SECTION_ALIGN_8BYTES = 0x00400000,
			PE_SECTION_ALIGN_16BYTES = 0x00500000,
			PE_SECTION_ALIGN_32BYTES = 0x00600000,
			PE_SECTION_ALIGN_64BYTES = 0x00700000,
			PE_SECTION_ALIGN_128BYTES = 0x00800000,
			PE_SECTION_ALIGN_256BYTES = 0x00900000,
			PE_SECTION_ALIGN_512BYTES = 0x00A00000,
			PE_SECTION_ALIGN_1024BYTES = 0x00B00000,
			PE_SECTION_ALIGN_2048BYTES = 0x00C00000,
			PE_SECTION_ALIGN_4096BYTES = 0x00D00000,
			PE_SECTION_ALIGN_8192BYTES = 0x00E00000,

			PE_SECTION_LINK_NRELOC_OVFL = 0x01000000,
			PE_SECTION_MEM_DISCARDABLE = 0x02000000,
			PE_SECTION_MEM_NOT_CACHED = 0x04000000,
			PE_SECTION_MEM_NOT_PAGED = 0x08000000,
			PE_SECTION_MEM_SHARED = 0x10000000,
			PE_SECTION_MEM_EXECUTE = 0x20000000,
			PE_SECTION_MEM_READ = 0x40000000,
			PE_SECTION_MEM_WRITE = 0x80000000,
		};

		struct PACK PE_BaseRelocHeader
		{
			uint32_t virtualAddress;
			uint32_t sizeOfBlock;
		};

		struct PACK PE_BaseRelocEntry
		{
			uint16_t value; // bits 0-3: relocation type, 4-15: relocation offset
		};

		enum class PE_RelocTypes : uint16_t
		{
			Absolute = 0,
			Dir64 = 10,
		};

		// Make sure headers are the correct size

		static_assert(sizeof(PE_MZHeader) == 64);
		static_assert(sizeof(PE_PEHeader) == 24);
		static_assert(sizeof(PE_OptionalHeader) == 112);
		static_assert(sizeof(PE_DataDirectoryEntry) == 8);
		static_assert(sizeof(PE_SectionHeader) == 40);
		static_assert(sizeof(PE_BaseRelocHeader) == 8);
		static_assert(sizeof(PE_BaseRelocEntry) == 2);

		constexpr char DOSSignature[2] = {'M', 'Z'};
		constexpr char PESignature[4] = {'P', 'E', '\0', '\0'};
		constexpr uint16_t OptionalHeaderMagicNumber = 0x020B;

		// Our own structs

		struct PE_ImageHandle
		{
			// Headers
			PE_MZHeader optDOSHeader;
			PE_PEHeader peHeader;
			PE_OptionalHeader optionalHeader; // Even if it says optional, for our loader it's mandatory
			PE_DataDirectoryEntry dataDirEntries[16];

			// Extra information
			uintptr_t sectionHeaderOffset;
			
			// Load information
			uintptr_t loadAddrPhys, loadAddrVirt;

			// Resource section information
			uintptr_t resourceSectionOffset;

			// Filesystem ptrs
			EFI_FILE_PROTOCOL* imageFile;
			FS::FS* FileSystem;
		};

		class PE_Loader
		{
		public:
			bool OpenImage(EFI_FILE_PROTOCOL* imageFile, FS::FS* FileSystem, PE_ImageHandle* handleOut);
			bool LoadImageIntoMemory(PE_ImageHandle* imageHandle);

			size_t GetImageLoadSize(PE_ImageHandle* imageHandle);
			size_t GetImageResSectionOffset(PE_ImageHandle* imageHandle);
			size_t GetImageResSectionRootDirOffset(PE_ImageHandle* imageHandle);
			uintptr_t GetImageAbsoluteEntryPoint(PE_ImageHandle* imageHandle);

			bool SetImageLoadAddr(PE_ImageHandle* imageHandle, uintptr_t phys, uintptr_t virt);
		private:
			bool HandleRelocations(PE_ImageHandle* imageHandle);
			const char* getMachineStr(uint16_t machine);
		};
	}
}