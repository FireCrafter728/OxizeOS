// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |-----------------------------------------------------------------------------------------------------| //
// | Minimal freestanding LIBSTDC++ Implementation for the OxizeOS kernel                                | //
// | converter: contains various conversion functions. Extension to the minimal LIBSTDC++ implementation | //
// |-----------------------------------------------------------------------------------------------------| //
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdint.hpp>

namespace stdEx
{
    char* convertFreqToStr(uint64_t freq, char* bufferOut, size_t* bufferSize);
}