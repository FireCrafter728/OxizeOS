#pragma once
#include <PCI/pci.hpp>

#define SVGA_CAP_NONE              0x00000000
#define SVGA_CAP_RECT_FILL         0x00000020
#define SVGA_CAP_RECT_COPY         0x00000040
#define SVGA_CAP_CURSOR            0x00000080
#define SVGA_CAP_CURSOR_BYPASS     0x00000100
#define SVGA_CAP_CURSOR_BYPASS_2   0x00000200
#define SVGA_CAP_8BIT_EMULATION    0x00000400
#define SVGA_CAP_ALPHA_CURSOR      0x00000800
#define SVGA_CAP_FIFO              0x00001000
#define SVGA_CAP_ALPHA_BLEND       0x00002000
#define SVGA_CAP_3D                0x00004000

namespace x86Kernel
{
    namespace SVGA
    {
        struct VideoModeDesc {
            uint32_t width, height;
            uint32_t bpp;
        };

        struct SVGADevice
        {
            PCI::DeviceInfo info;
            volatile uint32_t* SVGA2BAR;
            uint16_t SVGA2BARIO;
            volatile uint32_t* FIFO;
            volatile uint32_t* FrameBuffer;
            VideoModeDesc videoMode;
            bool IOBAR = false;
        };
        class SVGA
        {
        public:
            SVGA() = default;
            SVGA(SVGADevice* device);
            bool Initialize(SVGADevice* device);
            inline SVGADevice* getDevice() {return device; };
            void Dispose();
            bool GetCapability(uint32_t Capability);
        private:
            SVGADevice* device;
            void MapBAR();
            bool SelectVersion();
            void InitializeFIFO();
            void SetVideoMode();
            void Flush();
            uint32_t ReadBAR0(uint16_t Addr);
            void WriteBAR0(uint16_t Addr, uint32_t Value);
        };
    }
}