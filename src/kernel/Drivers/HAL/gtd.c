#include <gdt.h>
#include <stdint.h>

typedef struct {
    uint16_t LimitLow;
    uint16_t BaseLow;
    uint8_t BaseMiddle;
    uint8_t Access;
    uint8_t FlagsLimitHigh;
    uint8_t BaseHigh;
} __attribute__((packed)) GDTEntry;

typedef struct {
    uint16_t Limit;
    GDTEntry* Ptr;
} __attribute__((packed)) GDTDescriptor;

typedef enum {
    GDT_ACCESS_CODE_READABLE            = 0x02,
    GDT_ACCESS_DATA_WRITEABLE           = 0x02,

    GDT_ACCESS_CODE_CONFORMING          = 0x04,
    GDT_ACCESS_DATA_DIRECTION_NORMAL    = 0x00,
    GDT_ACCESS_DATA_DIRECTION_DOWN      = 0x04,

    GDT_ACCESS_DATA_SEGMENT             = 0x10,
    GDT_ACCESS_CODE_SEGMENT             = 0x18,

    GDT_ACCESS_DESC_TSS                 = 0x00,

    GDT_ACCESS_RING0                    = 0x00,
    GDT_ACCESS_RING1                    = 0x20,
    GDT_ACCESS_RING2                    = 0x40,
    GDT_ACCESS_RING3                    = 0x60,

    GDT_ACCESS_PRESENT                  = 0x80,

} GDT_ACCESS;

typedef enum {
    GDT_FLAG_64BIT          = 0x20,
    GDT_FLAG_32BIT          = 0x40,
    GDT_FLAG_16BIT          = 0x00,

    GDT_FLAG_GRANULARITY_1B = 0x00,
    GDT_FLAG_GRANULARITY_4K = 0x80,

} GDT_FLAGS;

#define GDT_LIMIT_LOW(limit)                (limit & 0xFFFF)
#define GDT_BASE_LOW(base)                  (base & 0xFFFF)
#define GDT_BASE_MIDDLE(base)               ((base >> 16) & 0xFF)
#define GDT_FLAGS_LIMIT_HIGH(limit, flags)  (((limit >> 16) & 0xF) | (flags & 0xF0))
#define GDT_BASE_HIGH(base)                 ((base >> 24) & 0xFF)

#define GDT_ENTRY(base, limit, access, flags) { \
    GDT_LIMIT_LOW(limit), \
    GDT_BASE_LOW(base), \
    GDT_BASE_MIDDLE(base), \
    access, \
    GDT_FLAGS_LIMIT_HIGH(limit, flags), \
    GDT_BASE_HIGH(base) \
}

GDTEntry GDT[] = {
    GDT_ENTRY(0, 0, 0, 0),

    GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE, GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),

    GDT_ENTRY(0, 0xFFFFF, GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITEABLE, GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4K),
    
};

GDTDescriptor GDTDesc = {
    sizeof(GDT) - 1,
    GDT
};

void __attribute__((cdecl)) GDT_Load(GDTDescriptor* GDTDesc, uint16_t codeSeg, uint16_t dataSeg);

void GDT_Initialize()
{
    GDT_Load(&GDTDesc, GDT_CODE_SEG, GDT_DATA_SEG);
}