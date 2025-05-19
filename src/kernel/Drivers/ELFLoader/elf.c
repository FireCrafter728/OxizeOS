#include <ELFLoader/elf.h>
#include <FAT/fat.h>
#include <stdio.h>
#include <Memory/mmd.h>
#include <memory.h>
#include <io.h>

#define ELF_HEADER_SIZE 52

ELF_ProgramHeader ELF_ReadProgramHeader(ELF_File* elf, ATA_PIO_Device* device, FAT_File* fd, int index)
{
    ELF_ProgramHeader header;

    // char buffer[elf->header.ProgramHeaderEntrySize];

    // FAT_Seek(fd, elf->header.ProgramHeaderOff + index * elf->header.ProgramHeaderEntrySize);
    // FAT_Read(device, fd, elf->header.ProgramHeaderEntrySize, &buffer);
    FAT_Seek(fd, elf->header.ProgramHeaderOff + index * elf->header.ProgramHeaderEntrySize);
    FAT_Read(device, fd, elf->header.ProgramHeaderEntrySize, &header);

    // printf("[ELF Loader] [INFO]: ELF Program Header %d: ", index);
    // for(int i = 0; i < ELF_HEADER_SIZE; i++) {
    //     printf("<0x%x> ", buffer[i]);
    //     if(i > 0 && i % 16 == 0) printf("\r\n");
    // }

    return header;
}

void ELF_ReadProgramHeaders(ELF_File* elf, FAT_File* fd, ATA_PIO_Device* device)
{
    ELF_ProgramHeader header;
    for(int i = 0; i < elf->header.ProgramHeaderEntries; i++) {
        header = ELF_ReadProgramHeader(elf, device, fd, i);
        elf->programHeaders[i] = header;
    }
}

bool ELF_GetFileData(ELF_File* elf, ATA_PIO_Device* device, const char* filePath)
{
    FAT_File* fd = FAT_Open(device, filePath);

    // char buffer[ELF_HEADER_SIZE];

    // FAT_Read(device, fd, ELF_HEADER_SIZE, &buffer);
    // FAT_ResetPos(fd);
    FAT_Read(device, fd, ELF_HEADER_SIZE, &elf->header);

    // printf("[ELF Loader] [INFO]: ELF Header: ");
    // for(int i = 0; i < ELF_HEADER_SIZE; i++) {
    //     printf("<0x%x> ", buffer[i]);
    //     if(i > 0 && i % 16 == 0) printf("\r\n");
    // }

    if(elf->header.Ident[0] != '\x7F' || elf->header.Ident[1] != 'E' || elf->header.Ident[2] != 'L' || elf->header.Ident[3] != 'F') {
        printf("[ELF Loader] [ERROR]: Non ELF file %s\r\n", filePath);
        return false;
    }

    if(elf->header.Endianess != 1) {
        printf("[ELF Loader] [ERROR]: Unsupported file endianess\r\n");
        return false;
    }

    if(elf->header.ELFHeaderVersion != 1) {
        printf("[ELF Loader] [ERROR]: Unsupported header version\r\n");
    }

    if(elf->header.machine != EM_x86) {
        printf("[ELF Loader] [ERROR]: Unsupported file target\r\n");
        return false;
    }

    if(elf->header.Type != ET_EXEC && elf->header.Type != ET_DYN) {
        printf("[ELF Loader] [ERROR]: Unsupported ELF file type\r\n");
        return false;
    }

    ELF_ReadProgramHeaders(elf, fd, device);
    FAT_Close(fd);

    elf->fileName = filePath;
    elf->Size = fd->Size;
    elf->LoadSize = 0;
    for(int i = 0; i < elf->header.ProgramHeaderEntries; i++) if(elf->programHeaders[i].Type == PT_LOAD) elf->LoadSize += elf->programHeaders[i].SegmentMemorySize;
    
    return true;
}

void ELF_LoadElf(ATA_PIO_Device* device, ELF_File* elf, uint32_t PhysAddr)
{
    printf("[ELF Loader] [INFO]: Loading ELF Executable at address 0x%x, Executable type: %d, Machine: %d, LoadMemorySize: 0x%x\r\n", PhysAddr, elf->header.Type, elf->header.machine, elf->LoadSize);

    FAT_File* fd = FAT_Open(device, elf->fileName);

    for(int i = 0; i < elf->header.ProgramHeaderEntries; i++) {
        if(elf->programHeaders[i].Type != PT_LOAD) continue;
        uint8_t buffer[SECTOR_SIZE];
        FAT_Seek(fd, elf->programHeaders[i].Offset);
        uint32_t left = elf->programHeaders[i].SegmentSize;
        uint32_t read;
        uint32_t readTotal = 0;
        while ((read = FAT_Read(device, fd, (left >= SECTOR_SIZE) ? SECTOR_SIZE : left, &buffer))) {
            for(int j = 0; j < read; j++) *((volatile uint8_t*)(PhysAddr + elf->programHeaders[i].VirtualAddress + j + readTotal)) = buffer[j];
            left -= read;
            readTotal += read;
        }
        
        if (elf->programHeaders[i].SegmentMemorySize > elf->programHeaders[i].SegmentSize) memset((void*)(PhysAddr + elf->programHeaders[i].VirtualAddress + elf->programHeaders[i].SegmentSize), 0, elf->programHeaders[i].SegmentMemorySize - elf->programHeaders[i].SegmentSize);
    }

    FAT_Close(fd);

    printf("[ELF Loader] [INFO]: ELF Executable loaded successfully\r\n");
}