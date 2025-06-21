#include <Graphics/graphics.hpp>

using namespace x86Kernel::Graphics;

Graphics::Graphics(SVGA::SVGA* svga)
{
    Initialize(svga);
}

void Graphics::Initialize(SVGA::SVGA* svga)
{
    this->svga = svga;
    this->device = svga->getDevice();
}

void Graphics::ClearScreen(Vector4i clearColor)
{
    if(!device->FrameBuffer) return;

    uint32_t pixelColor = ((clearColor.w & 0xFF) << 24) |
                          ((clearColor.x & 0xFF) << 16) |
                          ((clearColor.y & 0xFF) << 8) |
                          ((clearColor.z & 0xFF));

    uint32_t totalPixels = this->device->videoMode.width * this->device->videoMode.height;

    for(uint32_t i = 0; i < totalPixels; i++) this->device->FrameBuffer[i] = pixelColor;
}

void Graphics::Dispose()
{
    
}