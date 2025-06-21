#pragma once
#include <stdint.h>
#include <PCI/pci.hpp>
#include <SVGA/svgadefs.hpp>
#include <math/Vector.hpp>

namespace x86Kernel
{
    namespace SVGA
    {
        struct VideoMode
        {
            uint32_t width, height;
            uint32_t bpp;
        };

        struct SVGAInfo
        {
            uint32_t vramSize, fifoSize, fbSize;
            uint32_t capabilities;
            VideoMode* currentVideoMode;
        };
        
        struct Device
        {
            PCI::DeviceInfo info;
            SVGAInfo svgaInfo;
            uint16_t BAR0;
            volatile uint32_t* FIFO;
            volatile uint32_t* FB;
            uint32_t currentColor;
        };

        class SVGA
        {
        public:
            SVGA() = default;
            SVGA(Device* device);
            bool Initialize(Device* device);
            void SetVideoMode(VideoMode* mode);
            bool CheckCapability(uint32_t Capability);
            void ClearScreen(Vector4i clearColor);
            void SetColor(Vector4i Color);
            void FillRect(Vector2i Pos, Vector2i Size);
            void DrawPixel(Vector2i Pos);
            void Dispose();
            void PrintDeviceInfo();
            void Disable();
        private:
            void FIFOWrite(uint32_t value);
            uint32_t ReadBAR0(uint32_t Reg);
            void WriteBAR0(uint32_t Reg, uint32_t Value);
            uint32_t Vector4iToARGB(Vector4i color);
            void FIFOCommit();
            Device* device;
        };
    }
}