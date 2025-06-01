#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <DISK/DISK.h>

#define MAX_PROGRAM_HEADERS 32
#define MAX_SECTION_HEADERS 64

typedef struct {
    uint8_t Ident[4];
    uint8_t Arch;
    uint8_t Endianess;
    uint8_t ELFHeaderVersion;
    uint8_t OSAbi;
    uint8_t Padding[8];
    uint16_t Type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t ProgramHeaderOff;
    uint32_t SectionHeaderOff;
    uint32_t flags;
    uint16_t ELFHeaderSize;
    uint16_t ProgramHeaderEntrySize;
    uint16_t ProgramHeaderEntries;
    uint16_t ProgramSectionEntrySize;
    uint16_t ProgramSectionEntries;
    uint16_t SectionHeaderStringTableIndex;
} __attribute__((packed)) ELF_FileHeader;

typedef struct {
    uint32_t Type;
    uint32_t Offset;
    uint32_t VirtualAddress;
    uint32_t PhysicalAddress;
    uint32_t SegmentSize;
    uint32_t SegmentMemorySize;
    uint32_t Flags;
    uint32_t Alignment;
} __attribute__((packed)) ELF_ProgramHeader;

typedef struct {
    uint32_t Name;
    uint32_t Type;
    uint32_t Flags;
    uint32_t Address;
    uint32_t Offset;
    uint32_t Size;
    uint32_t Link;
    uint32_t Info;
    uint32_t AddrAlign;
    uint32_t EntrySize;
} __attribute__((packed)) ELF_SectionHeader;

typedef struct {
    uint32_t Offset;
    uint32_t Info;
} __attribute__((packed)) ELF_Rel;

typedef struct {
    uint32_t Offset;
    uint32_t Info;
    int32_t Addend;
} __attribute__((packed)) ELF_Rela;

typedef struct {
    uint32_t Name;
    uint32_t Value;
    uint32_t Size;
    uint8_t Info;
    uint8_t Other;
    uint16_t SectionIndex;
} __attribute__((packed)) ELF_Symbol;

typedef enum {
    ET_NONE         = 0x0,
    ET_REL          = 0x1,
    ET_EXEC         = 0x2,
    ET_DYN          = 0x3,
} ELF_FileType;

typedef enum {
    EM_NONE         = 0x0,
    EM_x86          = 0x3,
    EM_x86_64       = 0x3E,
    EM_ARM          = 0x28,
} ELF_Machine;

typedef enum {
    PT_NULL         = 0x0,
    PT_LOAD         = 0x1,
    PT_DYN          = 0x2,
    PT_INTERP       = 0x3,
    PT_NONE         = 0x4,
    PT_PHDR         = 0x6,
    PT_STACK        = 0x0A,
} ELF_ProgramHeaderType;

typedef enum {
    SHT_NULL        = 0x0,
    SHT_PROGBITS    = 0x1,
    SHT_SYMTAB      = 0x2,
    SHT_STRTAB      = 0x3,
    SHT_RELA        = 0x4,
    SHT_HASH        = 0x5,
    SHT_DYNAMIC     = 0x6,
    SHT_NOTE        = 0x7,
    SHT_NOBITS      = 0x8,
    SHT_REL         = 0x9,
    SHT_SHLIB       = 0xA,
    SHT_DYNSYM      = 0xB,
} ELF_SectionHeaderType;

typedef enum {
    ELF_R_386_NONE  = 0x0,
    ELF_R_386_32    = 0x1,
    ELF_R_386_PC32  = 0x2,
    ELF_R_386_RELAT = 0x8,
} ELF_RelocTypes;

typedef struct {
    ELF_FileHeader header;
    ELF_ProgramHeader programHeaders[MAX_PROGRAM_HEADERS];
    ELF_SectionHeader sectionHeaders[MAX_SECTION_HEADERS];
    uint32_t Size;
    uint32_t LoadSize;
    uint32_t LoadAddr;
    const char* fileName;
} __attribute__((packed)) ELF_File;

bool ELF_GetFileData(ELF_File* elf, DISK* disk, const char* filePath);
void ELF_LoadElf(DISK* disk, ELF_File* elf, uint32_t PhysAddr);