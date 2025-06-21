#include <SVGA/svga.hpp>
#include <io.h>
#include <stdio.h>

#define SVGA_SVGA_VENDOR_ID 0x15AD
#define SVGA_SVGA2_DEVICE_ID 0x0405
#define SVGA_DISPLAY_CONTROLLER_CLASS 0x03
#define SVGA_VGA_COMPATIBLE_CONTROLLER 0x00

#define SVGA_ID_0 0x900000
#define SVGA_ID_1 0x900001
#define SVGA_ID_2 0x900002

#define SVGA_REG_ID 0x00
#define SVGA_REG_ENABLE 0x01
#define SVGA_REG_WIDTH 0x02
#define SVGA_REG_HEIGHT 0x03
#define SVGA_REG_MAX_WIDTH 0x04
#define SVGA_REG_MAX_HEIGHT 0x05
#define SVGA_REG_BPP 0x07
#define SVGA_REG_FB_START 0x0D
#define SVGA_REG_FB_OFFSET 0x0E
#define SVGA_REG_VRAM_SIZE 0x0F
#define SVGA_REG_FB_SIZE 0x10
#define SVGA_REG_CAPABILITIES 0x11
#define SVGA_REG_CONFIG_DONE 0x14
#define SVGA_REG_SYNC 0x15
#define SVGA_REG_BUSY 0x16
#define SVGA_REG_FIFO_START 0x30
#define SVGA_REG_FIFO_SIZE 0x31

#define SVGA_FIFO_MIN 0x00
#define SVGA_FIFO_MAX 0x01
#define SVGA_FIFO_NEXT_CMD 0x02
#define SVGA_FIFO_STOP 0x03

#define SVGA_CMD_UPDATE 1

#define PCI_COMMAND_REG 0x04

constexpr uint32_t SVGA_FIFO_CMD_START = 293 * 4;

using namespace x86Kernel::SVGA;

void SVGA::MapBAR()
{
    uint32_t barAddr = PCI::ReadBAR(&this->device->info, PCI_BAR0_OFFSET);
    if (PCI::IsBARIO(&this->device->info, PCI_BAR0_OFFSET))
    {
        PCI::WriteConfigWord(&this->device->info, 0x10, 0xD0000000 & 0xFFFF);
        PCI::WriteConfigWord(&this->device->info, 0x12, (0xD0000000 >> 16) & 0xFFFF);

        if((PCI::ReadBAR(&this->device->info, PCI_BAR0_OFFSET) & 0x1)) HaltSystem();
    }
    this->device->SVGA2BAR = (volatile uint32_t *)barAddr;
}

bool SVGA::SelectVersion()
{
    uint32_t deviceVersion = ReadBAR0(SVGA_REG_ID);
    if (deviceVersion == 0xFFFFFFFF)
    {
        printf("[SVGA-II] [ERROR]: Invalid SVGA ID Register(device not responding)\r\n");
        return false;
    }

    printf("[SVGA-II] [INFO]: SVGA Device version 0x%x\r\n", deviceVersion);

    WriteBAR0(SVGA_REG_ID, SVGA_ID_2);

    uint32_t confirmed = ReadBAR0(SVGA_REG_ID);

    if (confirmed != SVGA_ID_2)
    {
        printf("[SVGA-II] [ERROR]: SVGA-II Device rejected SVGA version II, got: 0x%x\r\n", confirmed);
        return false;
    }

    printf("[SVGA-II] [INFO]: SVGA Version II accepted\r\n");
    return true;
}

void SVGA::InitializeFIFO()
{
    this->device->FIFO = (volatile uint32_t *)ReadBAR0(SVGA_REG_FIFO_START);
    this->device->FrameBuffer = (volatile uint32_t *)(ReadBAR0(SVGA_REG_FB_START) + ReadBAR0(SVGA_REG_FB_OFFSET));

    this->device->FIFO[SVGA_FIFO_MIN] = this->device->FIFO[SVGA_FIFO_NEXT_CMD] = this->device->FIFO[SVGA_FIFO_STOP] = SVGA_FIFO_CMD_START;
    this->device->FIFO[SVGA_FIFO_MAX] = ReadBAR0(SVGA_REG_FIFO_SIZE);

    WriteBAR0(SVGA_REG_CONFIG_DONE, 1);
}

void SVGA::SetVideoMode()
{
    WriteBAR0(SVGA_REG_WIDTH, this->device->videoMode.width);
    WriteBAR0(SVGA_REG_HEIGHT, this->device->videoMode.height);
    WriteBAR0(SVGA_REG_BPP, this->device->videoMode.bpp);

    printf("[SVGA-II] [INFO]: Video mode: %lux%lu, BPP:%lu\r\n", this->device->videoMode.width, this->device->videoMode.height, this->device->videoMode.bpp);
}

SVGA::SVGA(SVGADevice *device)
{
    if (!Initialize(device))
        HaltSystem();
}

bool SVGA::Initialize(SVGADevice *device)
{
    PCI::DeviceInfo infoIn;
    infoIn.vendorID = SVGA_SVGA_VENDOR_ID;
    infoIn.deviceID = SVGA_SVGA2_DEVICE_ID;
    infoIn.classCode = SVGA_DISPLAY_CONTROLLER_CLASS;
    infoIn.subclass = infoIn.progIF = SVGA_VGA_COMPATIBLE_CONTROLLER;
    if (!PCI::FindDevice(&infoIn, &device->info))
    {
        printf("[SVGA-II] [ERROR]: Failed to find SVGA-II device\r\n");
        return false;
    }
    printf("[SVGA-II] [INFO]: SVGA Device found in PCI\r\n");

    this->device = device;

    uint16_t command = PCI::ReadConfigWord(&device->info, PCI_COMMAND_REG);
    command |= 0x7;
    PCI::WriteConfigWord(&device->info, PCI_COMMAND_REG, command);

    MapBAR();
    if (!SelectVersion())
        return false;

    printf("FIFO: %lx, FB: %lx\r\n", ReadBAR0(SVGA_REG_FIFO_START), ReadBAR0(SVGA_REG_FB_START));
    HaltSystem();
    InitializeFIFO();
    SetVideoMode();

    WriteBAR0(SVGA_REG_ENABLE, 1);

    return true;
}

void SVGA::Flush()
{
    while (this->device->FIFO[SVGA_FIFO_STOP] != this->device->FIFO[SVGA_FIFO_NEXT_CMD])
        sleep(1000);
}

void SVGA::Dispose()
{
    uint32_t nextCmd = this->device->FIFO[SVGA_FIFO_NEXT_CMD];

    this->device->FIFO[nextCmd++] = SVGA_CMD_UPDATE;

    this->device->FIFO[nextCmd++] = 0;                              // x
    this->device->FIFO[nextCmd++] = 0;                              // y
    this->device->FIFO[nextCmd++] = this->device->videoMode.width;  // width
    this->device->FIFO[nextCmd++] = this->device->videoMode.height; // height

    if (nextCmd >= this->device->FIFO[SVGA_FIFO_MAX])
        nextCmd = this->device->FIFO[SVGA_FIFO_MIN];
    this->device->FIFO[SVGA_FIFO_NEXT_CMD] = nextCmd;
    this->device->FIFO[SVGA_FIFO_STOP] = nextCmd;
    Flush();
}

bool SVGA::GetCapability(uint32_t Capability)
{
    return (ReadBAR0(SVGA_REG_CAPABILITIES) & Capability);
}

uint32_t SVGA::ReadBAR0(uint16_t Addr)
{
    if (this->device->IOBAR)
        return ind(this->device->SVGA2BARIO + Addr);
    else
        return *(volatile uint32_t *)((uintptr_t)this->device->SVGA2BAR + (Addr * 4));
}

void SVGA::WriteBAR0(uint16_t Addr, uint32_t Value)
{
    if (this->device->IOBAR)
        outd(this->device->SVGA2BARIO + Addr, Value);
    else
        *(volatile uint32_t *)((uintptr_t)this->device->SVGA2BAR + (Addr * 4)) = Value;
}