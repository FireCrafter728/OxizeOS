#pragma once

// |||||||||||||||||||||||||||||||||||||||||||||||||||||||| // 
// |------------------------------------------------------| //
// | OxizeOS Kernel Implementation                        | //
// | GDT: Driver for managing the Global Descriptor Table | //
// |------------------------------------------------------| //
// |||||||||||||||||||||||||||||||||||||||||||||||||||||||| //

#include <stdint.hpp>

#ifndef PACK
#define PACK __attribute__((packed))
#endif

#ifndef FPACK
#define FPACK __attribute__((packed, aligned(1)))
#endif

#ifndef ASMCALL
#define ASMCALL extern "C"
#endif

#define GDT_ACCESS_CODE_READABLE            SysKrnl64::GDT::ACCESS_CODE_READABLE
#define GDT_ACCESS_DATA_WRITEABLE           SysKrnl64::GDT::ACCESS_DATA_WRITEABLE
#define GDT_ACCESS_CODE_CONFORMING          SysKrnl64::GDT::ACCESS_CODE_CONFORMING
#define GDT_ACCESS_DATA_DIRECTION_NORMAL    SysKrnl64::GDT::ACCESS_DATA_DIRECTION_NORMAL
#define GDT_ACCESS_DATA_DIRECTION_DOWN      SysKrnl64::GDT::ACCESS_DATA_DIRECTION_DOWN
#define GDT_ACCESS_CODE_SEGMENT             SysKrnl64::GDT::ACCESS_CODE_SEGMENT
#define GDT_ACCESS_DATA_SEGMENT             SysKrnl64::GDT::ACCESS_DATA_SEGMENT
#define GDT_ACCESS_DESCRIPTOR_TSS           SysKrnl64::GDT::ACCESS_DESCRIPTOR_TSS
#define GDT_ACCESS_RING0                    SysKrnl64::GDT::ACCESS_RING0
#define GDT_ACCESS_RING1                    SysKrnl64::GDT::ACCESS_RING1
#define GDT_ACCESS_RING2                    SysKrnl64::GDT::ACCESS_RING2
#define GDT_ACCESS_RING3                    SysKrnl64::GDT::ACCESS_RING3
#define GDT_ACCESS_PRESENT                  SysKrnl64::GDT::ACCESS_PRESENT
#define GDT_FLAG_64BIT                      SysKrnl64::GDT::FLAG_64BIT
#define GDT_FLAG_32BIT                      SysKrnl64::GDT::FLAG_32BIT
#define GDT_FLAG_16BIT                      SysKrnl64::GDT::FLAG_16BIT
#define GDT_FLAG_GRANULARITY_1B             SysKrnl64::GDT::FLAG_GRANULARITY_1B
#define GDT_FLAG_GRANULARITY_4K             SysKrnl64::GDT::FLAG_GRANULARITY_4K

#define GDT_LIMIT_LOW(limit)                ((limit) & 0xFFFF)
#define GDT_BASE_LOW(base)                  ((base) & 0xFFFF)
#define GDT_BASE_MIDDLE(base)               (((base) >> 16) & 0xFF)
#define GDT_FLAGS_LIMIT_HIGH(limit, flags)  ((((limit) >> 16) & 0xF) | ((flags) & 0xF0))
#define GDT_BASE_HIGH(base)                 (((base) >> 24) & 0xFF)

#define GDT_ENTRY(base, limit, access, flags) { \
    GDT_LIMIT_LOW(limit),                       \
    GDT_BASE_LOW(base),                         \
    GDT_BASE_MIDDLE(base),                      \
    (access),                                     \
    GDT_FLAGS_LIMIT_HIGH(limit, flags),         \
    GDT_BASE_HIGH(base)                         \
}

extern const uint16_t GDT_64BIT_RING0_CODESEG;
extern const uint16_t GDT_64BIT_RING0_DATASEG;
extern const uint16_t GDT_64BIT_RING3_CODESEG;
extern const uint16_t GDT_64BIT_RING3_DATASEG;

extern const uint16_t GDT_32BIT_RING0_CODESEG;
extern const uint16_t GDT_32BIT_RING0_DATASEG;
extern const uint16_t GDT_32BIT_RING3_CODESEG;
extern const uint16_t GDT_32BIT_RING3_DATASEG;

namespace SysKrnl64
{
    namespace GDT
    {
        struct PACK GDT_Entry
        {
            uint16_t LimitLow;
            uint16_t BaseLow;
            uint8_t BaseMiddle;
            uint8_t Access;
            uint8_t FlagsLimitHigh;
            uint8_t BaseHigh;
        };

        struct FPACK GDT_Desc
        {
            uint16_t Limit;
            GDT_Entry* entries;
        };

        enum GDT_ACCESS {
            ACCESS_CODE_READABLE = 0x02,
            ACCESS_DATA_WRITEABLE = 0x02,

            ACCESS_CODE_CONFORMING = 0x04,
            ACCESS_DATA_DIRECTION_NORMAL = 0x00,
            ACCESS_DATA_DIRECTION_DOWN = 0x04,

            ACCESS_CODE_SEGMENT = 0x18,
            ACCESS_DATA_SEGMENT = 0x10,

            ACCESS_DESCRIPTOR_TSS = 0x00,
            
            ACCESS_RING0 = 0x00,
            ACCESS_RING1 = 0x20,
            ACCESS_RING2 = 0x40,
            ACCESS_RING3 = 0x60,

            ACCESS_PRESENT = 0x80,
        };

        enum GDT_FLAGS
        {
            FLAG_64BIT = 0x20,
            FLAG_32BIT = 0x40,
            FLAG_16BIT = 0x00,
            
            FLAG_GRANULARITY_1B = 0x00,
            FLAG_GRANULARITY_4K = 0x80,
        };

        class GDT
        {
        public:
            void Initialize(GDT_Entry* entries, size_t entryCount, uint16_t newCs, uint16_t newDs);
        private:
            GDT_Desc gdtDesc;
        };
    }
}