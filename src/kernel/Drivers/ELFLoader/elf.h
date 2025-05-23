#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <DISK/ATA_PIO.h>

#define MAX_PROGRAM_HEADERS 32

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

typedef enum {
    ET_NONE     = 0x0,
    ET_REL      = 0x1,
    ET_EXEC     = 0x2,
    ET_DYN      = 0x3,
} ELF_FileType;

typedef enum {
    EM_NONE     = 0x0,
    EM_x86      = 0x3,
    EM_x86_64   = 0x3E,
    EM_ARM      = 0x28,
} ELF_Machine;

typedef enum {
    PT_NULL     = 0x0,
    PT_LOAD     = 0x1,
    PT_DYN      = 0x2,
    PT_INTERP   = 0x3,
    PT_NONE     = 0x4,
    PT_PHDR     = 0x6,
    PT_STACK    = 0x0A,
} ELF_ProgramHeaderType;

typedef struct {
    ELF_FileHeader header;
    ELF_ProgramHeader programHeaders[MAX_PROGRAM_HEADERS];
    uint32_t Size;
    uint32_t LoadSize;
    uint32_t LoadAddr;
    const char* fileName;
} __attribute__((packed)) ELF_File;

bool ELF_GetFileData(ELF_File* elf, ATA_PIO_Device* device, const char* filePath);
void ELF_LoadElf(ATA_PIO_Device* device, ELF_File* elf, uint32_t PhysAddr);