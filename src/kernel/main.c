#include <stdint.h>
#include <stdio.h>
#include <memory.h>
#include <hal/hal.h>
#include <io.h>
#include <irq.h>
#include <Boot/bootparams.h>
#include <DISK/ATA_PIO.h>
#include <stddef.h>
#include <FAT/fat.h>
#include <Memory/mmd.h>
#include <ELFLoader/elf.h>

extern uint8_t __bss_start;
extern uint8_t __end;

typedef void (*x86Kernel)(SystemInfo* System);

void __attribute__((section(".entry"))) start(SystemInfo* System)
{
    memset(&__bss_start, 0, (&__end) - (&__bss_start));

    clrscr();

    HAL_Initialize();

    ATA_PIO_Device device;
    if(!ATA_PIO_Initialize(&device)) {
        printf("[KERNEL] [ERROR]: Failed to initialize ATA PIO Device\r\n");
        HaltSystem();
    }

    printf("[KERNEL] [INFO]: ATA PIO Device found: Disk model: %s, Sector count: %lu\r\n", device.driveInfo.model, device.driveInfo.sectorCount);

    if(!FAT_Initialize(&device)) {
        printf("[KERNEL] [INFO]: Failed to initialize FAT32 driver\r\n");
        HaltSystem();
    }

    MMD_MemoryDriver driver;

    MMD_Initialize(&driver, System->memoryInfo);

    ELF_File file;
    if(!ELF_GetFileData(&file, &device, "BOOT/KERNEL/x86Kern.exe")) {
        printf("[KERNEL] [ERROR]: Failed to read x86Kernel.exe header\r\n");
        HaltSystem();
    }

    uint32_t loadAddr = MMD_FindMemoryLocation(&driver, file.LoadSize);

    ELF_LoadElf(&device, &file, loadAddr);

    x86Kernel kernelx86 = (x86Kernel)(loadAddr + file.header.entry);
    kernelx86(System);

    HaltSystem();
}
