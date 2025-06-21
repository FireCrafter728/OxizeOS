#include <SVGA/svga.hpp>
#include <io.h>
#include <stdio.h>
#include <algorithm>

using namespace x86Kernel::SVGA;

constexpr uint32_t SVGA_FIFO_START = 293 * sizeof(uint32_t);

SVGA::SVGA(Device *device)
{
    if (!Initialize(device))
        HaltSystem();
}

bool SVGA::Initialize(Device *device)
{
    this->device = device;
    PCI::DeviceInfo infoIn;
    infoIn.vendorID = SVGA_VENDOR_ID;
    infoIn.deviceID = SVGA_DEVICE_ID;
    if (!PCI::FindDevice(&infoIn, &device->info))
    {
        printf("[SVGA-II] [ERROR]: Failed to find SVGA-II device in PCI\r\n");
        return false;
    }

    uint16_t command = PCI::ReadConfigWord(&device->info, PCI_COMMAND_REG);
    command |= 0x7;
    PCI::WriteConfigWord(&device->info, PCI_COMMAND_REG, command);

    device->BAR0 = PCI::ReadBAR(&device->info, PCI_BAR0_OFFSET);
    device->FIFO = reinterpret_cast<volatile uint32_t *>(PCI::ReadBAR(&device->info, PCI_BAR2_OFFSET));
    device->FB = reinterpret_cast<volatile uint32_t *>(PCI::ReadBAR(&device->info, PCI_BAR1_OFFSET));

    WriteBAR0(SVGA_REG_ID, SVGA_ID_2);
    if (ReadBAR0(SVGA_REG_ID) != SVGA_ID_2)
    {
        printf("[SVGA-II] [ERROR]: Device doesn't support SVGA-II\r\n");
        return false;
    }

    device->svgaInfo.vramSize = ReadBAR0(SVGA_REG_VRAM_SIZE);
    device->svgaInfo.fbSize = ReadBAR0(SVGA_REG_FB_SIZE);
    device->svgaInfo.fifoSize = ReadBAR0(SVGA_REG_FIFO_SIZE);

    device->svgaInfo.capabilities = ReadBAR0(SVGA_REG_CAPABILITIES);

    if(device->svgaInfo.fbSize < 0x100000)
        printf("[SVGA-II] [WARN]: FB Size is very small, there might be errors with the implementation\r\n");
    if(device->svgaInfo.fifoSize < 0x20000)
        printf("[SVGA-II] [WARN]: FIFO Size is very small, there might be errors with the implementation\r\n");


    device->FIFO[SVGA_FIFO_MIN] = device->FIFO[SVGA_FIFO_NEXT_CMD] = device->FIFO[SVGA_FIFO_STOP] = SVGA_FIFO_START;
    device->FIFO[SVGA_FIFO_MAX] = device->svgaInfo.fifoSize;    

    WriteBAR0(SVGA_REG_ENABLE, 1);
    WriteBAR0(SVGA_REG_CONFIG_DONE, 1);

    return true;
}

uint32_t SVGA::ReadBAR0(uint32_t Reg)
{
    outd(device->BAR0 + SVGA_IO_INDEX, Reg);
    return ind(device->BAR0 + SVGA_IO_VALUE);
}

void SVGA::WriteBAR0(uint32_t Reg, uint32_t Value)
{
    outd(device->BAR0 + SVGA_IO_INDEX, Reg);
    outd(device->BAR0 + SVGA_IO_VALUE, Value);
}

void SVGA::SetVideoMode(VideoMode *mode)
{
    uint32_t maxWidth = ReadBAR0(SVGA_REG_MAX_WIDTH), maxHeight = ReadBAR0(SVGA_REG_MAX_HEIGHT);
    mode->width = std::min(mode->width, maxWidth);
    mode->height = std::min(mode->height, maxHeight);
    mode->bpp = std::min(mode->bpp, (uint32_t)SVGA_MAX_BPP);
    WriteBAR0(SVGA_REG_WIDTH, mode->width);
    WriteBAR0(SVGA_REG_HEIGHT, mode->height);
    WriteBAR0(SVGA_REG_BPP, mode->bpp);
    WriteBAR0(SVGA_REG_ENABLE, 1);
    device->svgaInfo.currentVideoMode = mode;
}

bool SVGA::CheckCapability(uint32_t Capability)
{
    return (device->FIFO[SVGA_FIFO_CAPABILITIES] & Capability);
}

void SVGA::ClearScreen(Vector4i clearColor)
{
    device->currentColor = Vector4iToARGB(clearColor);
    FillRect({0, 0}, {(int)ReadBAR0(SVGA_REG_WIDTH), (int)ReadBAR0(SVGA_REG_HEIGHT)});
}

void SVGA::Dispose()
{
    WriteBAR0(SVGA_REG_SYNC, 1);
}

void SVGA::FIFOWrite(uint32_t value)
{
    uint32_t nextCmd = device->FIFO[SVGA_FIFO_NEXT_CMD];
    device->FIFO[nextCmd / 4] = value;
    nextCmd += sizeof(uint32_t);
    if (nextCmd == device->FIFO[SVGA_FIFO_MAX])
        nextCmd = device->FIFO[SVGA_FIFO_MIN];
    device->FIFO[SVGA_FIFO_NEXT_CMD] = nextCmd;
}

void SVGA::SetColor(Vector4i Color)
{
    device->currentColor = Vector4iToARGB(Color);
}

void SVGA::FillRect(Vector2i Pos, Vector2i Size)
{
    if (Pos.x < 0 || Pos.y < 0)
    {
        printf("[SVGA-II] [WARNING]: Tried to fill a rect at negative coords\r\n");
        return;
    }
    Pos.x = std::min(Pos.x, (int)ReadBAR0(SVGA_REG_MAX_WIDTH));
    Pos.y = std::min(Pos.y, (int)ReadBAR0(SVGA_REG_MAX_HEIGHT));
    Size.x = std::min(Size.x, (int)ReadBAR0(SVGA_REG_MAX_WIDTH));
    Size.y = std::min(Size.y, (int)ReadBAR0(SVGA_REG_MAX_HEIGHT));
    FIFOWrite(SVGA_CMD_RECT_FILL);
    FIFOWrite(device->currentColor);
    FIFOWrite(Pos.x);
    FIFOWrite(Pos.y);
    FIFOWrite(Size.x);
    FIFOWrite(Size.y);
    FIFOCommit();
}

void SVGA::DrawPixel(Vector2i Pos)
{
    if (Pos.x < 0 || Pos.y < 0)
    {
        printf("[SVGA-II] [WARNING]: Tried to draw a pixel at negative coords\r\n");
        return;
    }
    Pos.x = std::min(Pos.x, (int)ReadBAR0(SVGA_REG_MAX_WIDTH));
    Pos.y = std::min(Pos.y, (int)ReadBAR0(SVGA_REG_MAX_HEIGHT));
    device->FB[Pos.y * ReadBAR0(SVGA_REG_MAX_WIDTH) + Pos.x] = device->currentColor;
}

uint32_t SVGA::Vector4iToARGB(Vector4i color)
{
    return ((color.w << 24) | (color.x << 16) | (color.y << 8) | (color.z));
}

void SVGA::FIFOCommit()
{
    device->FIFO[SVGA_FIFO_STOP] = device->FIFO[SVGA_FIFO_NEXT_CMD];
    int timeout = 100000;
    while (ReadBAR0(SVGA_REG_BUSY) && timeout-- > 0)
        sleep(1000);
}

void SVGA::PrintDeviceInfo()
{
    printf("[SVGA-II] [INFO]: Device info: \r\n");
    printf("MaxWidth: %lu, MaxHeight: %lu\r\n", ReadBAR0(SVGA_REG_MAX_WIDTH), ReadBAR0(SVGA_REG_MAX_HEIGHT));
    printf("VRAM SIZE: %lu\r\n", ReadBAR0(SVGA_REG_VRAM_SIZE));
    printf("[SVGA-II] [INFO]: Device info end\r\n");
}

void SVGA::Disable()
{
    WriteBAR0(SVGA_REG_ENABLE, 0);
}