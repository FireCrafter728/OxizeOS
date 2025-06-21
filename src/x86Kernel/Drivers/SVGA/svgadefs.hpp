#pragma once

#define SVGA_VENDOR_ID                      0x15AD
#define SVGA_DEVICE_ID                      0x0405

#define SVGA_REG_ID                         0x00
#define SVGA_REG_ENABLE                     0x01
#define SVGA_REG_WIDTH                      0x02
#define SVGA_REG_HEIGHT                     0x03
#define SVGA_REG_MAX_WIDTH                  0x04
#define SVGA_REG_MAX_HEIGHT                 0x05
#define SVGA_REG_BPP                        0x07
#define SVGA_REG_FB_START                   0x0D
#define SVGA_REG_FB_OFFSET                  0x0E
#define SVGA_REG_VRAM_SIZE                  0x0F
#define SVGA_REG_FB_SIZE                    0x10
#define SVGA_REG_CAPABILITIES               0x11
#define SVGA_REG_FIFO_START                 0x12
#define SVGA_REG_FIFO_SIZE                  0x13
#define SVGA_REG_CONFIG_DONE                0x14
#define SVGA_REG_SYNC                       0x15
#define SVGA_REG_BUSY                       0x16

#define SVGA_CMD_INVALID_CMD                0
#define SVGA_CMD_UPDATE                     1
#define SVGA_CMD_RECT_FILL                  104
#define SVGA_CMD_RECT_COPY                  105
#define SVGA_CMD_DEFINE_CURSOR              106
#define SVGA_CMD_DEFINE_ALPHA_CURSOR        107
#define SVGA_CMD_UPDATE_VERBOSE             108
#define SVGA_CMD_FRONT_ROP_FILL             109
#define SVGA_CMD_CLEAR_RECT                 110
#define SVGA_CMD_DEFINE_BITMAP              111
#define SVGA_CMD_DEFINE_BITMAP_SCANLINE     112
#define SVGA_CMD_DEFINE_PIXMAP              113
#define SVGA_CMD_DEFINE_PIXMAP_SCANLINE     114
#define SVGA_CMD_RECT_ROP_FILL              115
#define SVGA_CMD_RECT_ROP_COPY              116
#define SVGA_CMD_ESCAPE                     123
#define SVGA_CMD_DEFINE_GMRFB               1048
#define SVGA_CMD_BLIT_GMRFB_TO_SCREEN       1054
#define SVGA_CMD_BLIT_SCREEN_TO_GMRFB       1055

#define SVGA_ID_2                           0x90000002

#define SVGA_FIFO_MIN                       0x00
#define SVGA_FIFO_MAX                       0x01
#define SVGA_FIFO_NEXT_CMD                  0x02
#define SVGA_FIFO_STOP                      0x03
#define SVGA_FIFO_CAPABILITIES              0x04
#define SVGA_FIFO_FLAGS                     0x05
#define SVGA_FIFO_FENCE                     0x06

#define SVGA_CAP_NONE                       0x00000000
#define SVGA_CAP_RECT_FILL                  0x00000001
#define SVGA_CAP_RECT_COPY                  0x00000002
#define SVGA_CAP_CURSOR                     0x00000020
#define SVGA_CAP_CURSOR_BYPASS              0x00000040
#define SVGA_CAP_CURSOR_BYPASS_2            0x00000080
#define SVGA_CAP_8BIT_EMULATION             0x00000100
#define SVGA_CAP_ALPHA_CURSOR               0x00000200
#define SVGA_CAP_GLYPH                      0x00000400
#define SVGA_CAP_OFFSCREEN_1                0x00000800
#define SVGA_CAP_RASTER_OP                  0x00001000
#define SVGA_CAP_CURSOR_BYPASS_3            0x00002000
#define SVGA_CAP_SCREEN_OBJECT_2            0x00004000
#define SVGA_CAP_3D                         0x00008000
#define SVGA_CAP_EXTENDED_FIFO              0x00010000
#define SVGA_CAP_MULTIMON                   0x00020000
#define SVGA_CAP_PITCHLOCK                  0x00040000
#define SVGA_CAP_IRQMASK                    0x00080000
#define SVGA_CAP_DISPLAY_TOPOLOGY           0x00100000
#define SVGA_CAP_GMR                        0x00200000
#define SVGA_CAP_TRACES                     0x00400000
#define SVGA_CAP_GMR2                       0x00800000
#define SVGA_CAP_SCREEN_OBJECT              0x01000000
#define SVGA_CAP_COMMAND_BUFFERS            0x02000000
#define SVGA_CAP_CMD_BUFFERS_2              0x04000000
#define SVGA_CAP_GBOBJECTS                  0x08000000
#define SVGA_CAP_DONOTNEEDGMR               0x10000000

#define SVGA_MAX_BPP                        32

#define SVGA_IO_INDEX                       0
#define SVGA_IO_VALUE                       1