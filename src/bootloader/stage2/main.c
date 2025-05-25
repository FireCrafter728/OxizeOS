#include <stdint.h>
#include "stdio.h"
#include "x86.h"
#include "disk.h"
#include "fat.h"
#include "memdefs.h"
#include "memory.h"
#include "mbr.h"
#include "memdetect.h"

uint8_t* KernelLoadBuffer = (uint8_t*)MEMORY_LOAD_KERNEL;
uint8_t* Kernel = (uint8_t*)MEMORY_KERNEL_ADDR;

SystemInfo System;

typedef void (*KernelStart)(SystemInfo* System);

void __attribute__((cdecl)) start(uint16_t bootDrive, void* partition)
{
    clrscr();

    DISK disk;
    if (!DISK_Initialize(&disk, bootDrive))
    {
        printf("Disk init error\r\n");
        goto end;
    }

    MBR_Partition part;
    MBR_DetectPartition(&part, &disk, partition);

    if (!FAT_Initialize(&part))
    {
        printf("FAT init error\r\n");
        goto end;
    }

    // Prepare System Information struct for the kernel
    Memory_Detect(&System.memoryInfo);
    System.bootDrive = bootDrive;

    // load kernel
    FAT_File* fd = FAT_Open(&part, "BOOT/BIOS/kernel.bin");
    uint32_t read = 0;
    uint8_t* kernelBuffer = Kernel;
    while ((read = FAT_Read(&part, fd, MEMORY_LOAD_SIZE, KernelLoadBuffer)))
    {
        memcpy(kernelBuffer, KernelLoadBuffer, read);
        kernelBuffer += read;
    }
    FAT_Close(fd);

    // execute kernel
    KernelStart kernelStart = (KernelStart)Kernel;
    kernelStart(&System);

end:
    for (;;);
}