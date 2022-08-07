#if !defined(BMP_H)
#define BMP_H

#pragma pack(push, 1)
typedef struct BmpHeader
{
    u16 file_type;
    u32 file_size;
    u16 reserved1;
    u16 reserved2;
    u32 bmp_offset;
    u32 header_size;
    s32 width;
    s32 height;
    u16 planes;
    u16 bpp;
    u32 compression;
    u32 bmp_size;
    s32 horz;
    s32 verz;
    u32 colors_used;
    u32 colors_important;
    u32 red_mask;
    u32 green_mask;
    u32 blue_mask;
    u32 alpha_mask;
} BmpHeader;
#pragma pack(pop)

#endif   // BMP_H
