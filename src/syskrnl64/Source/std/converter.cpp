// SPDX-License-Identifier: GPL-3.0-or-later

#include <converter.hpp>

char* stdEx::convertFreqToStr(uint64_t freq, char* bufferOut, size_t* bufferSize)
{
    auto append_u64 = [&](char* out, size_t i, uint64_t v) -> size_t
    {
        char tmp[32];
        size_t n = 0;
        
        do
        {
            tmp[n++] = char('0' + (v % 10));
            v /= 10;
        } while (v);
    
        while (n--)
            out[i++] = tmp[n];
    
        return i;
    };

    auto u64_len = [&](uint64_t v) -> size_t
    {
        size_t n = 1;
        while (v >= 10)
        {
            v /= 10;
            n++;
        }
        return n;
    };

    const char* unit = " Hz";

    uint64_t value = freq;
    uint64_t rem = 0;

    if (freq >= 1000000000ULL)
    {
        value = freq / 1000000000ULL;
        rem   = freq % 1000000000ULL;
        unit  = " GHz";
    }
    else if (freq >= 1000000ULL)
    {
        value = freq / 1000000ULL;
        rem   = freq % 1000000ULL;
        unit  = " MHz";
    }
    else if (freq >= 1000ULL)
    {
        value = freq / 1000ULL;
        rem   = freq % 1000ULL;
        unit  = " kHz";
    }

    uint64_t frac = 0;

    if (rem)
    {
        if (unit[1] == 'G')
            frac = (rem * 100000ULL) / 1000000000ULL;
        else if (unit[1] == 'M')
            frac = (rem * 100000ULL) / 1000000ULL;
        else if (unit[1] == 'k')
            frac = (rem * 100000ULL) / 1000ULL;
    }

    size_t int_len = u64_len(value);
    size_t frac_len = frac ? 6 : 0;
    size_t unit_len = strlen(unit);

    size_t needed = int_len + frac_len + unit_len + 1;

    if (!bufferSize)
        return nullptr;

    if (!bufferOut || *bufferSize < needed)
    {
        *bufferSize = needed;
        return nullptr;
    }

    size_t i = 0;

    i = append_u64(bufferOut, i, value);

    if (frac)
    {
        bufferOut[i++] = '.';

        uint64_t f = frac;
        for (int p = 4; p >= 0; --p)
        {
            uint64_t div = 1;
            for (int k = 0; k < p; k++) div *= 10;

            bufferOut[i++] = char('0' + (f / div) % 10);
        }
    }

    for(size_t j = 0; unit[j]; j++) bufferOut[i++] = unit[j];

    bufferOut[i] = '\0';

    *bufferSize = needed;

    return bufferOut;
}