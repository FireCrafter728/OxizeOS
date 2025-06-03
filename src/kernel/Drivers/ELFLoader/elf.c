#include <ELFLoader/elf.h>
#include <FAT/fat.h>
#include <stdio.h>
#include <Memory/mmd.h>
#include <memory.h>
#include <io.h>

#define ELF_HEADER_SIZE 52
#define ELF_SECTION_SIZE 40

#define ELF32_R_SYM(info) ((info) >> 8)
#define ELF32_R_TYPE(info) ((uint8_t)(info))

ELF_ProgramHeader ELF_ReadProgramHeader(ELF_File *elf, DISK *disk, FAT_File *fd, int index)
{
    ELF_ProgramHeader header;

    // char buffer[elf->header.ProgramHeaderEntrySize];

    // FAT_Seek(fd, elf->header.ProgramHeaderOff + index * elf->header.ProgramHeaderEntrySize);
    // FAT_Read(disk, fd, elf->header.ProgramHeaderEntrySize, &buffer);
    FAT_Seek(disk, fd, elf->header.ProgramHeaderOff + index * elf->header.ProgramHeaderEntrySize);
    FAT_Read(disk, fd, elf->header.ProgramHeaderEntrySize, &header);

    // printf("[ELF Loader] [INFO]: ELF Program Header %d: ", index);
    // for(int i = 0; i < ELF_HEADER_SIZE; i++) {
    //     printf("<0x%x> ", buffer[i]);
    //     if(i > 0 && i % 16 == 0) printf("\r\n");
    // }

    return header;
}

ELF_SectionHeader ELF_ReadSectionHeader(ELF_File *elf, DISK *disk, FAT_File *fd, int index)
{
    ELF_SectionHeader header;

    // char buffer[elf->header.SectionHeaderEntrySize];

    // FAT_Seek(fd, elf->header.SectionHeaderOff + index * elf->header.SectionHeaderEntrySize);
    // FAT_Read(disk, fd, elf->header.SectionHeaderEntrySize, &buffer);

    // printf("Pos: %lu, off: %lu, index: %d, entry size: %lu\r\n", elf->header.SectionHeaderOff + index * elf->header.SectionHeaderEntrySize, elf->header.SectionHeaderOff, index, elf->header.SectionHeaderEntrySize);
    FAT_Seek(disk, fd, elf->header.SectionHeaderOff + index * elf->header.SectionHeaderEntrySize);
    FAT_Read(disk, fd, elf->header.SectionHeaderEntrySize, &header);

    // printf("[ELF Loader] [INFO]: ELF Program section %d: ", index);
    // for(int i = 0; i < ELF_HEADER_SIZE; i++) {
    //     printf("<0x%x> ", buffer[i]);
    //     if(i > 0 && i % 16 == 0) printf("\r\n");
    // }
    // printf("Section %d type: 0x%x\r\n?", index, header.Type);

    return header;
}

void ELF_ReadProgramHeaders(ELF_File *elf, FAT_File *fd, DISK *disk)
{
    ELF_ProgramHeader header;
    for (int i = 0; i < elf->header.ProgramHeaderEntries; i++)
    {
        header = ELF_ReadProgramHeader(elf, disk, fd, i);
        elf->programHeaders[i] = header;
    }
}

void ELF_ReadSectionHeaders(ELF_File *elf, FAT_File *fd, DISK *disk)
{
    ELF_SectionHeader header;
    for (int i = 0; i < elf->header.SectionHeaderEntries; i++)
    {
        header = ELF_ReadSectionHeader(elf, disk, fd, i);
        elf->sectionHeaders[i] = header;
    }
}

bool ELF_GetFileData(ELF_File *elf, DISK *disk, const char *filePath)
{
    FAT_File *fd = FAT_Open(disk, filePath);

    // char buffer[ELF_HEADER_SIZE];

    // FAT_Read(disk, fd, ELF_HEADER_SIZE, &buffer);
    // FAT_ResetPos(fd);
    FAT_Read(disk, fd, ELF_HEADER_SIZE, &elf->header);

    // printf("[ELF Loader] [INFO]: ELF Header: ");
    // for(int i = 0; i < ELF_HEADER_SIZE; i++) {
    //     printf("<0x%x> ", buffer[i]);
    //     if(i > 0 && i % 16 == 0) printf("\r\n");
    // }

    if (elf->header.Ident[0] != '\x7F' || elf->header.Ident[1] != 'E' || elf->header.Ident[2] != 'L' || elf->header.Ident[3] != 'F')
    {
        printf("[ELF Loader] [ERROR]: Non ELF file %s\r\n", filePath);
        return false;
    }

    if (elf->header.Endianess != 1)
    {
        printf("[ELF Loader] [ERROR]: Unsupported file endianess\r\n");
        return false;
    }

    if (elf->header.ELFHeaderVersion != 1)
    {
        printf("[ELF Loader] [ERROR]: Unsupported header version\r\n");
    }

    if (elf->header.machine != EM_x86)
    {
        printf("[ELF Loader] [ERROR]: Unsupported file target\r\n");
        return false;
    }

    if (elf->header.Type != ET_EXEC && elf->header.Type != ET_DYN)
    {
        printf("[ELF Loader] [ERROR]: Unsupported ELF file type\r\n");
        return false;
    }

    ELF_ReadSectionHeaders(elf, fd, disk);
    ELF_ReadProgramHeaders(elf, fd, disk);
    FAT_Close(fd);

    elf->fileName = filePath;
    elf->Size = fd->Size;
    elf->LoadSize = 0;
    for (int i = 0; i < elf->header.ProgramHeaderEntries; i++)
        if (elf->programHeaders[i].Type == PT_LOAD)
            elf->LoadSize += elf->programHeaders[i].SegmentMemorySize;

    return true;
}

static uint32_t ELF_GetSymbolValue(ELF_File *elf, DISK *disk, FAT_File *fd, uint32_t symtabSectionIndex, uint32_t symIndex)
{
    uint32_t symOffset = elf->sectionHeaders[symtabSectionIndex].Offset + symIndex * sizeof(ELF_Symbol);
    ELF_Symbol symbol;

    FAT_Seek(disk, fd, symOffset);
    FAT_Read(disk, fd, sizeof(ELF_Symbol), &symbol);
    return symbol.Value;
}

static void ELF_Relocate(ELF_File *elf, DISK *disk, FAT_File *fd, uint32_t PhysAddr)
{
    for (uint8_t i = 0; i < elf->header.SectionHeaderEntries; i++)
    {
        // printf("Program section entry found, type: 0x%x\r\n", elf->sectionHeaders[i].Type);
        if (elf->sectionHeaders[i].Type != SHT_REL && elf->sectionHeaders[i].Type != SHT_RELA)
            continue;
        uint32_t count = elf->sectionHeaders[i].Size / elf->sectionHeaders[i].EntrySize;
        for (uint32_t j = 0; j < count; j++)
        {
            uint32_t relOffset = elf->sectionHeaders[i].Offset + j * elf->sectionHeaders[i].EntrySize;
            switch (elf->sectionHeaders[i].Type)
            {
            case SHT_REL:
            {
                ELF_Rel rel;
                FAT_Seek(disk, fd, relOffset);
                FAT_Read(disk, fd, sizeof(ELF_Rel), &rel);
                uint32_t *relocAddr = (uint32_t *)(PhysAddr + rel.Offset);
                uint32_t symValue = ELF_GetSymbolValue(elf, disk, fd, elf->sectionHeaders[i].Link, ELF32_R_SYM(rel.Info));

                switch (ELF32_R_TYPE(rel.Info))
                {
                case ELF_R_386_32:
                    *relocAddr += symValue;
                    break;
                case ELF_R_386_PC32:
                    *relocAddr += symValue - (uint32_t)relocAddr;
                    break;
                case ELF_R_386_RELAT:
                    *relocAddr += PhysAddr;
                    break;
                default:
                    break;
                }
                break;
            }
            case SHT_RELA:
            {
                ELF_Rela rela;
                FAT_Seek(disk, fd, relOffset);
                FAT_Read(disk, fd, sizeof(ELF_Rela), &rela);

                uint32_t *relocAddr = (uint32_t *)(PhysAddr + rela.Offset);
                uint32_t symValue = ELF_GetSymbolValue(elf, disk, fd, elf->sectionHeaders[i].Link, ELF32_R_SYM(rela.Info));

                switch (ELF32_R_TYPE(rela.Info))
                {
                case ELF_R_386_32:
                    *relocAddr = symValue + rela.Addend;
                    break;
                case ELF_R_386_PC32:
                    *relocAddr = symValue + rela.Addend - (uint32_t)relocAddr;
                    break;
                case ELF_R_386_RELAT:
                    *relocAddr = PhysAddr + rela.Addend;
                    break;
                default:
                    break;
                }
                break;
            }
            default:
                break;
            }
        }
    }
}

void ELF_LoadElf(DISK *disk, ELF_File *elf, uint32_t PhysAddr)
{
    FAT_File *fd = FAT_Open(disk, elf->fileName);

    elf->LoadAddr = PhysAddr;

    for (int i = 0; i < elf->header.ProgramHeaderEntries; i++)
    {
        if (elf->programHeaders[i].Type != PT_LOAD)
            continue;
        uint8_t buffer[SECTOR_SIZE];
        FAT_Seek(disk, fd, elf->programHeaders[i].Offset);
        uint32_t left = elf->programHeaders[i].SegmentSize;
        uint32_t read;
        uint32_t readTotal = 0;
        while ((read = FAT_Read(disk, fd, (left >= SECTOR_SIZE) ? SECTOR_SIZE : left, &buffer)))
        {
            for (uint32_t j = 0; j < read; j++)
                *((volatile uint8_t *)(PhysAddr + elf->programHeaders[i].VirtualAddress + j + readTotal)) = buffer[j];
            left -= read;
            readTotal += read;
        }

        if (elf->programHeaders[i].SegmentMemorySize > elf->programHeaders[i].SegmentSize)
            memset((void *)(PhysAddr + elf->programHeaders[i].VirtualAddress + elf->programHeaders[i].SegmentSize), 0, elf->programHeaders[i].SegmentMemorySize - elf->programHeaders[i].SegmentSize);
    }

    ELF_Relocate(elf, disk, fd, PhysAddr);

    FAT_Close(fd);
}