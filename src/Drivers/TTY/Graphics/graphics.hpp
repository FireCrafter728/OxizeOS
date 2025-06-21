#pragma once
#include <SVGA/svga.hpp>
#include <math/Vector.hpp>

namespace x86Kernel
{
    namespace Graphics
    {
        class Graphics
        {
        public:
            Graphics() = default;
            Graphics(SVGA::SVGA* svga);
            void Initialize(SVGA::SVGA* svga);
            void ClearScreen(Vector4i clearColor);
            void Dispose();
        private:
            SVGA::SVGA* svga;
            SVGA::SVGADevice* device;
        };
    }
}