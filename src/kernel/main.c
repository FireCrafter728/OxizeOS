#include <stdint.h>
#include <stdio.h>
#include <memory.h>
#include <hal/hal.h>
#include <io.h>
#include <Boot/bootparams.h>
#include <DISK/DISK.h>
#include <stddef.h>
#include <FAT/fat.h>
#include <Memory/mmd.h>
#include <ELFLoader/elf.h>
#include <DISK/SATA_AHCI.h>
#include <HAL/isr.h>
#include <Memory/memdefs.h>

extern uint8_t __bss_start;
extern uint8_t __end;

typedef void (*__attribute__((cdecl)) x86Kernel)();


void __attribute__((section(".entry"))) start(SystemInfo* System)
{
    memset(&__bss_start, 0, (&__end) - (&__bss_start));

    clrscr();

    SystemInfo* SystemOut = (SystemInfo*)SYSTEM_PARAMETER_BLOCK_ADDR;

    HAL_Initialize();

    DISK disk;
    if(!DISK_Initialize(&disk)) {
        printf("[KERNEL] [INFO]: Failed to initialize DISK\r\n");
        HaltSystem();
    }

    if(!FAT_Initialize(&disk)) {
        printf("[KERNEL] [INFO]: Failed to initialize FAT32 driver\r\n");
        HaltSystem();
    }

    MMD_MemoryDriver driver;
    MMD_Initialize(&driver, System->memoryInfo);
    System->ISRHandlers = ISR_GetHandlersAddr();

    static ELF_File file;
    if(!ELF_GetFileData(&file, &disk, "BOOT/KERNEL/x86Kern.exe")) {
        printf("[KERNEL] [ERROR]: Failed to read x86Kernel.exe header\r\n");
        HaltSystem();
    }

    uint32_t loadAddr = MMD_FindMemoryLocation(&driver, file.LoadSize);

    ELF_LoadElf(&disk, &file, loadAddr);

    System->x86KernelElf = &file;

    memcpy(SystemOut, System, sizeof(SystemInfo));

    x86Kernel kernelx86 = (x86Kernel)(loadAddr + file.header.entry);
    kernelx86();

    HaltSystem();
}
