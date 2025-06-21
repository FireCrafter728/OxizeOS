#include <io.h>
#include <stdio.h>
#include <Boot/bootparams.h>
#include <Memory/memdefs.hpp>
#include <Interrupts/isr.hpp>
#include <Interrupts/irq.hpp>
#include <SVGA/svga.hpp>
#include <print.h>
#include <FAT/fat.hpp>
#include <VFS/vfs.hpp>

SystemInfo* System = nullptr;

x86Kernel::ISR::ISR* isr;
x86Kernel::PIC::PIC* pic;
x86Kernel::IRQ::IRQ* irq;

x86Kernel::SVGA::SVGA* svga;
x86Kernel::SVGA::Device* svgaDevice;

x86Kernel::DISK::DISK* disk;
x86Kernel::MBR::MBR* mbr;
x86Kernel::MBR::MBRDesc* mbrDesc;
x86Kernel::FAT32::FAT32* fat;
x86Kernel::VFS::VFS* vfs;
x86Kernel::DISK::DiskParams* diskParams;
x86Kernel::FAT32::File* logfile;

void __attribute__((section(".entry"))) InitializeInterrupts()
{
    static x86Kernel::ISR::ISR T_isr;
    static x86Kernel::PIC::PIC T_pic;
    static x86Kernel::IRQ::IRQ T_irq;

    isr = &T_isr;
    pic = &T_pic;
    irq = &T_irq;

    isr->Initialize(System->ISRHandlers);
    irq->Initialize(pic, isr);

    for(int i = 0; i < PIC_CASCADE_MAX_IRQS; i++) pic->Mask(i);

    STI();
}

void __attribute__((section(".entry"))) SetupFileLogging()
{
    static x86Kernel::MBR::MBR T_Mbr;
    static x86Kernel::MBR::MBRDesc T_MbrDesc;
    static x86Kernel::FAT32::FAT32 T_Fat;
    static x86Kernel::VFS::VFS T_Vfs;
    static x86Kernel::DISK::DISK T_Disk;
    static x86Kernel::DISK::DiskParams T_DiskParams;
    static x86Kernel::FAT32::File T_Logfile;

    mbr = &T_Mbr;
    mbrDesc = &T_MbrDesc;
    fat = &T_Fat;
    vfs = &T_Vfs;
    disk = &T_Disk;
    diskParams = &T_DiskParams;
    logfile = &T_Logfile;

    int passIndex = 0;
    if(!disk->Initialize(diskParams)) HaltSystem();
    printf("Passed %d\r\n", passIndex++);
    if(!mbr->Initialize(disk, mbrDesc)) HaltSystem();
    printf("Passed %d\r\n", passIndex++);
    if(!fat->Initialize(mbr)) HaltSystem();
    printf("Passed %d\r\n", passIndex++);
    if(!(logfile = fat->Create("/BOOT/KERNEL/logfile.log"))) HaltSystem();
    HaltSystem();
    printf("Passed %d\r\n", passIndex++);
    vfs->Initialize(fat, logfile);
    printf("Passed %d\r\n", passIndex++);
}

void __attribute__((section(".entry"))) SetupTTY()
{
    static x86Kernel::SVGA::SVGA T_Svga;
    static x86Kernel::SVGA::Device T_SvgaDevice;

    svga = &T_Svga;
    svgaDevice = &T_SvgaDevice;

    if(!svga->Initialize(svgaDevice)) HaltSystem();
    static x86Kernel::SVGA::VideoMode videoMode;
    videoMode.width = 1600;
    videoMode.height = 900;
    videoMode.bpp = 32;
    svga->SetVideoMode(&videoMode);
    svga->PrintDeviceInfo();
    svga->ClearScreen({255, 0, 0, 255});
    svga->Dispose();
}

extern "C" void __attribute__((cdecl)) __attribute__((section(".entry"))) _x86kernel()
{
    clrscr();
    printf("Hello from x86Kern in C++!!!\r\n");

    System = (SystemInfo*)SYSTEM_PARAMETER_BLOCK_ADDR;

    // SetupFileLogging();

    InitializeInterrupts();

    // SetupTTY();

    HaltSystem();
}