#pragma once
#include <Boot/bootparams.h>
#include <ELFLoader/elf.h>
#include <FAT/fat.h>
#include <stdbool.h>

// #define MAX_EXECUTABLE_COUNT 16

typedef struct {
    MemoryInfo memoryInfo;
    // ELF_File* ELFFiles[MAX_EXECUTABLE_COUNT];
    uint16_t MemoryRegions;
} MMD_MemoryDriver;

void MMD_Initialize(MMD_MemoryDriver* driver, MemoryInfo memoryInfo);
uint32_t MMD_FindMemoryLocation(MMD_MemoryDriver* driver, uint32_t Size);
// int MMD_AddElfFile(MMD_MemoryDriver* driver, ELF_File* elf);
// void MMD_RemoveElfFile(MMD_MemoryDriver* driver, uint8_t index);