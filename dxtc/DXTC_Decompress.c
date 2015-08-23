//
//  DXTC_Read.c
//  GLTC
//
//  Created by Michael Kwasnicki on 22.02.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#include "DXTC_Decompress.h"

#include <iso646.h>
#include <stdio.h>

#pragma mark - pallette decompression


void computeDXT1RGBPalette(rgb8_t out_rgbPalette[4], const rgb565_t in_C0, const rgb565_t in_C1) {
    rgb8_t c0;
    rgb8_t c1;
    convert565to888(&c0, in_C0);
    convert565to888(&c1, in_C1);
    out_rgbPalette[0] = c0;
    out_rgbPalette[1] = c1;

    // floating point interpolation differs up to one bit from integer variant
    if (not kFloatingPoint) {
        if (in_C0.b16 > in_C1.b16) {
            out_rgbPalette[2].r = (2 * c0.r + c1.r) / 3;
            out_rgbPalette[2].g = (2 * c0.g + c1.g) / 3;
            out_rgbPalette[2].b = (2 * c0.b + c1.b) / 3;
            out_rgbPalette[3].r = (c0.r + 2 * c1.r) / 3;
            out_rgbPalette[3].g = (c0.g + 2 * c1.g) / 3;
            out_rgbPalette[3].b = (c0.b + 2 * c1.b) / 3;
        } else {
            out_rgbPalette[2].r = (c0.r + c1.r) >> 1;
            out_rgbPalette[2].g = (c0.g + c1.g) >> 1;
            out_rgbPalette[2].b = (c0.b + c1.b) >> 1;
            out_rgbPalette[3].r = 0;
            out_rgbPalette[3].g = 0;
            out_rgbPalette[3].b = 0;
        }
    } else {
        float paletteF2[3] = { 0, 0, 0 };
        float paletteF3[3] = { 0, 0, 0 };

        if (in_C0.b16 > in_C1.b16) {
            paletteF2[0] = (float)(2 * c0.r + c1.r) / 3.0f;
            paletteF2[1] = (float)(2 * c0.g + c1.g) / 3.0f;
            paletteF2[2] = (float)(2 * c0.b + c1.b) / 3.0f;
            paletteF3[0] = (float)(c0.r + 2 * c1.r) / 3.0f;
            paletteF3[1] = (float)(c0.g + 2 * c1.g) / 3.0f;
            paletteF3[2] = (float)(c0.b + 2 * c1.b) / 3.0f;

            // using ANSI round-to-floor to get round-to-nearest
            out_rgbPalette[2].r = (uint8_t)(paletteF2[0] + 0.5f);
            out_rgbPalette[2].g = (uint8_t)(paletteF2[1] + 0.5f);
            out_rgbPalette[2].b = (uint8_t)(paletteF2[2] + 0.5f);
            out_rgbPalette[3].r = (uint8_t)(paletteF3[0] + 0.5f);
            out_rgbPalette[3].g = (uint8_t)(paletteF3[1] + 0.5f);
            out_rgbPalette[3].b = (uint8_t)(paletteF3[2] + 0.5f);
        } else {
            paletteF2[0] = (float)(c0.r + c1.r) * 0.5f;
            paletteF2[1] = (float)(c0.g + c1.g) * 0.5f;
            paletteF2[2] = (float)(c0.b + c1.b) * 0.5f;

            // using ANSI round-to-floor to get round-to-nearest
            out_rgbPalette[2].r = (uint8_t)(paletteF2[0] + 0.5f);
            out_rgbPalette[2].g = (uint8_t)(paletteF2[1] + 0.5f);
            out_rgbPalette[2].b = (uint8_t)(paletteF2[2] + 0.5f);
            out_rgbPalette[3].r = 0;
            out_rgbPalette[3].g = 0;
            out_rgbPalette[3].b = 0;
        }
    }
}



void computeDXT1RGBAPalette(rgba8_t out_rgbaPalette[4], const rgb565_t in_C0, const rgb565_t in_C1) {
    rgb8_t palette[4];
    computeDXT1RGBPalette(palette, in_C0, in_C1);

    for (int i = 0; i < 4; i++) {
        out_rgbaPalette[i].r = palette[i].r;
        out_rgbaPalette[i].g = palette[i].g;
        out_rgbaPalette[i].b = palette[i].b;
        out_rgbaPalette[i].a = 255;
    }

    if (in_C0.b16 <= in_C1.b16) {
        out_rgbaPalette[3].a = 0;
    }
}



void computeDXT3RGBPalette(rgba8_t out_rgbPalette[4], const rgb565_t in_C0, const rgb565_t in_C1) {
    rgb8_t c0;
    rgb8_t c1;
    convert565to888(&c0, in_C0);
    convert565to888(&c1, in_C1);
    out_rgbPalette[0].r = c0.r;
    out_rgbPalette[0].g = c0.g;
    out_rgbPalette[0].b = c0.b;
    out_rgbPalette[0].a = 255;
    out_rgbPalette[1].r = c1.r;
    out_rgbPalette[1].g = c1.g;
    out_rgbPalette[1].b = c1.b;
    out_rgbPalette[1].a = 255;

    // floating point interpolation differs up to one bit from integer variant
    if (not kFloatingPoint) {
        out_rgbPalette[2].r = (2 * c0.r + c1.r) / 3;
        out_rgbPalette[2].g = (2 * c0.g + c1.g) / 3;
        out_rgbPalette[2].b = (2 * c0.b + c1.b) / 3;
        out_rgbPalette[2].a = 255;
        out_rgbPalette[3].r = (c0.r + 2 * c1.r) / 3;
        out_rgbPalette[3].g = (c0.g + 2 * c1.g) / 3;
        out_rgbPalette[3].b = (c0.b + 2 * c1.b) / 3;
        out_rgbPalette[3].a = 255;
    } else {
        float paletteF2[3] = { 0, 0, 0 };
        float paletteF3[3] = { 0, 0, 0 };

        paletteF2[0] = (float)(2 * c0.r + c1.r) / 3.0f;
        paletteF2[1] = (float)(2 * c0.g + c1.g) / 3.0f;
        paletteF2[2] = (float)(2 * c0.b + c1.b) / 3.0f;
        paletteF3[0] = (float)(c0.r + 2 * c1.r) / 3.0f;
        paletteF3[1] = (float)(c0.g + 2 * c1.g) / 3.0f;
        paletteF3[2] = (float)(c0.b + 2 * c1.b) / 3.0f;

        // using ANSI round-to-floor to get round-to-nearest
        out_rgbPalette[2].r = (uint8_t)(paletteF2[0] + 0.5f);
        out_rgbPalette[2].g = (uint8_t)(paletteF2[1] + 0.5f);
        out_rgbPalette[2].b = (uint8_t)(paletteF2[2] + 0.5f);
        out_rgbPalette[2].a = 255;
        out_rgbPalette[3].r = (uint8_t)(paletteF3[0] + 0.5f);
        out_rgbPalette[3].g = (uint8_t)(paletteF3[1] + 0.5f);
        out_rgbPalette[3].b = (uint8_t)(paletteF3[2] + 0.5f);
        out_rgbPalette[2].a = 255;
    }
}



void computeDXT5APalette(uint8_t out_alphaPalette[8], const uint8_t in_A0, const uint8_t in_A1) {
    out_alphaPalette[0] = in_A0;
    out_alphaPalette[1] = in_A1;

    if (not kFloatingPoint) {
        if (in_A0 > in_A1) {
            out_alphaPalette[2] = (6 * in_A0 + 1 * in_A1) / 7;
            out_alphaPalette[3] = (5 * in_A0 + 2 * in_A1) / 7;
            out_alphaPalette[4] = (4 * in_A0 + 3 * in_A1) / 7;
            out_alphaPalette[5] = (3 * in_A0 + 4 * in_A1) / 7;
            out_alphaPalette[6] = (2 * in_A0 + 5 * in_A1) / 7;
            out_alphaPalette[7] = (1 * in_A0 + 6 * in_A1) / 7;
        } else {
            out_alphaPalette[2] = (4 * in_A0 + 1 * in_A1) / 5;
            out_alphaPalette[3] = (3 * in_A0 + 2 * in_A1) / 5;
            out_alphaPalette[4] = (2 * in_A0 + 3 * in_A1) / 5;
            out_alphaPalette[5] = (1 * in_A0 + 4 * in_A1) / 5;
            out_alphaPalette[6] = 0;
            out_alphaPalette[7] = 255;
        }
    } else {
        float alphaPaletteF[8];
        alphaPaletteF[0] = in_A0;
        alphaPaletteF[1] = in_A1;

        if (in_A0 > in_A1) {
            alphaPaletteF[2] = (float)(6 * in_A0 + 1 * in_A1) / 7.0f;
            alphaPaletteF[3] = (float)(5 * in_A0 + 2 * in_A1) / 7.0f;
            alphaPaletteF[4] = (float)(4 * in_A0 + 3 * in_A1) / 7.0f;
            alphaPaletteF[5] = (float)(3 * in_A0 + 4 * in_A1) / 7.0f;
            alphaPaletteF[6] = (float)(2 * in_A0 + 5 * in_A1) / 7.0f;
            alphaPaletteF[7] = (float)(1 * in_A0 + 6 * in_A1) / 7.0f;
        } else {
            alphaPaletteF[2] = (float)(4 * in_A0 + 1 * in_A1) / 5.0f;
            alphaPaletteF[3] = (float)(3 * in_A0 + 2 * in_A1) / 5.0f;
            alphaPaletteF[4] = (float)(2 * in_A0 + 3 * in_A1) / 5.0f;
            alphaPaletteF[5] = (float)(1 * in_A0 + 4 * in_A1) / 5.0f;
            alphaPaletteF[6] = 0.0f;
            alphaPaletteF[7] = 255.0f;
        }

        // using ANSI round-to-floor to get round-to-nearest
        out_alphaPalette[2] = (uint8_t)(alphaPaletteF[2] + 0.5);
        out_alphaPalette[3] = (uint8_t)(alphaPaletteF[3] + 0.5);
        out_alphaPalette[4] = (uint8_t)(alphaPaletteF[4] + 0.5);
        out_alphaPalette[5] = (uint8_t)(alphaPaletteF[5] + 0.5);
        out_alphaPalette[6] = (uint8_t)(alphaPaletteF[6] + 0.5);
        out_alphaPalette[7] = (uint8_t)(alphaPaletteF[7] + 0.5);
    }
}



#pragma mark - block decompression



void decompressDXT1BlockRGB(rgb8_t out_blockRGB[4][4], const DXT1Block_t in_BLOCK) {
    rgb8_t palette[4];
    uint32_t cBitField = in_BLOCK.cBitField;
    computeDXT1RGBPalette(palette, in_BLOCK.c0, in_BLOCK.c1);

    for (int by = 0; by < 4; by++) {
        for (int bx = 0; bx < 4; bx++) {
            int paletteIndex = cBitField bitand 0x3;
            out_blockRGB[by][bx] = palette[paletteIndex];
            cBitField >>= 2;
        }
    }
}



void decompressDXT1BlockRGBA(rgba8_t out_blockRGBA[4][4], const DXT1Block_t in_BLOCK) {
    rgba8_t palette[4];
    uint32_t cBitField = in_BLOCK.cBitField;
    computeDXT1RGBAPalette(palette, in_BLOCK.c0, in_BLOCK.c1);

    for (int by = 0; by < 4; by++) {
        for (int bx = 0; bx < 4; bx++) {
            int paletteIndex = cBitField bitand 0x3;
            out_blockRGBA[by][bx] = palette[paletteIndex];
            cBitField >>= 2;
        }
    }
}



void decompressDXT3BlockRGBA(rgba8_t out_blockRGBA[4][4], const DXT3Block_t in_BLOCK) {
    rgba8_t palette[4];
    uint64_t aBitField = in_BLOCK.a;
    uint32_t cBitField = in_BLOCK.rgb.cBitField;
    computeDXT3RGBPalette(palette, in_BLOCK.rgb.c0, in_BLOCK.rgb.c1);

    for (int by = 0; by < 4; by++) {
        for (int bx = 0; bx < 4; bx++) {
            int paletteIndex = cBitField bitand 0x3;
            uint8_t a = aBitField bitand 0xF;
            a = (a << 4) bitor a;   // expand 4 bit alpha to 8 bit alpha
            out_blockRGBA[by][bx] = palette[paletteIndex];
            out_blockRGBA[by][bx].a = a;
            cBitField >>= 2;
            aBitField >>= 4;
        }
    }
}



void decompressDXT5BlockRGBA(rgba8_t out_blockRGBA[4][4], const DXT5Block_t in_BLOCK) {
    uint8_t alphaPalette[8];
    computeDXT5APalette(alphaPalette, in_BLOCK.a.a0, in_BLOCK.a.a1);
    uint64_t aBitField = in_BLOCK.a.aBitField >> 16; // get rid of a0 and a1

    rgba8_t palette[4];
    uint32_t cBitField = in_BLOCK.rgb.cBitField;
    computeDXT3RGBPalette(palette, in_BLOCK.rgb.c0, in_BLOCK.rgb.c1);

    for (int by = 0; by < 4; by++) {
        for (int bx = 0; bx < 4; bx++) {
            int paletteIndex = cBitField bitand 0x3;
            out_blockRGBA[by][bx] = palette[paletteIndex];
            out_blockRGBA[by][bx].a = alphaPalette[aBitField bitand 0x7];
            cBitField >>= 2;
            aBitField >>= 3;
        }
    }
}
