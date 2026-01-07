#pragma once
#include <string>
#include <fstream>

namespace cppextra
{
    enum class OpenModes : uint8_t
    {
        READ = 0x01,
        WRITE = 0x02,
        OVERWRITE = (0x02 | 0x04),
    };

    inline OpenModes operator|(OpenModes a, OpenModes b)
    {
        return static_cast<OpenModes>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
    }

    inline OpenModes& operator|=(OpenModes& a, OpenModes b)
    {
        a = a | b;
        return a;
    }

    struct File
    {
        OpenModes openMode;
        std::ifstream fileIn;
        std::ofstream fileOut;
        std::string file;
    };

    File* open(const std::string& file, const OpenModes& openMode);
    void close(File* file);
}