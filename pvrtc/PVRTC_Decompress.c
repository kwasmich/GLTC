//
//  PVRTC_Decompress.c
//  GLTC
//
//  Created by Michael Kwasnicki on 04.03.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#include "PVRTC_Decompress.h"

#include <assert.h>
#include <iso646.h>
#include <stdbool.h>
#include <stdio.h>



typedef enum {
    HV,
    H,
    V
} PVRInterpolation_t;



static void decodeBaseColorA(rgba8_t *out_rgba, const bool in_OPAQUE, const uint16_t in_COLOR_A) {
    if (in_OPAQUE) {
        rgb554_t colorA;
        colorA.b16 = in_COLOR_A;
        out_rgba->r = extend5to8bits(colorA.r);
        out_rgba->g = extend5to8bits(colorA.g);
        out_rgba->b = extend4to8bits(colorA.b);
        out_rgba->a = 255;
    } else {
        argb3443_t colorA;
        colorA.b16 = in_COLOR_A;
        out_rgba->r = extend4to8bits(colorA.r);
        out_rgba->g = extend4to8bits(colorA.g);
        out_rgba->b = extend3to8bits(colorA.b);
        out_rgba->a = extend3to8bits(colorA.a);
    }
}



static void decodeBaseColorB(rgba8_t *out_rgba, const bool in_OPAQUE, const uint16_t in_COLOR_B) {
    if (in_OPAQUE) {
        rgb555_t colorB;
        colorB.b16 = in_COLOR_B;
        out_rgba->r = extend5to8bits(colorB.r);
        out_rgba->g = extend5to8bits(colorB.g);
        out_rgba->b = extend5to8bits(colorB.b);
        out_rgba->a = 255;
    } else {
        argb3444_t colorB;
        colorB.b16 = in_COLOR_B;
        out_rgba->r = extend4to8bits(colorB.r);
        out_rgba->g = extend4to8bits(colorB.g);
        out_rgba->b = extend4to8bits(colorB.b);
        out_rgba->a = extend3to8bits(colorB.a);
    }
}



static void blockModulation(int8_t out_mod[4][8], const PVRTC4Block_t in_BLOCK_PVR) {
    bool modulationMode = in_BLOCK_PVR.modMode;
    int rawMod = in_BLOCK_PVR.mod;

    for (int by = 0; by < 4; by++) {
        for (int bx = 0; bx < 8; bx++) {
            out_mod[by][bx] = -1;
        }
    }

    if (modulationMode) {
        PVRInterpolation_t interpolationMode = HV;            // H & V

        // pixel (0, 0) has 1 bit accuracy it's least significant bit is used to tell that we are only using interpolation in one dimension
        if (rawMod bitand 0x1) {            // H or V only
            // pixel (4, 2) has 1 bit accuracy it's least significant bit is used to tell in which direction to interpolate ( 0 = H, 1 = V )
            if (rawMod bitand 0x100000) {   // V only
                interpolationMode = V;
            } else {                        // H only
                interpolationMode = H;
            }

            if (rawMod bitand 0x200000) {
                rawMod = rawMod bitor 0x300000;
            } else {
                rawMod = rawMod bitand 0xFFCFFFFF;
            }
        }

        // convert 1 bit accuracy to two bit accuracy;
        if (rawMod bitand 0x2) {
            rawMod = rawMod bitor 0x3;
        } else {
            rawMod = rawMod bitand 0xFFFFFFFC;
        }

        // fill in the checkerboard modulation
        for (int by = 0; by < 4; by++) {
            const int start = by bitand 0x1;

            for (int bx = start; bx < 8; bx += 2) {
                switch (rawMod bitand 0x3) {
                    case 0:
                        out_mod[by][bx] = 16;
                        break;

                    case 1:
                        out_mod[by][bx] = 10;
                        break;

                    case 2:
                        out_mod[by][bx] = 6;
                        break;

                    case 3:
                        out_mod[by][bx] = 0;
                        break;
                }

                rawMod >>= 2;
            }
        }
    } else {
        for (int by = 0; by < 4; by++) {
            for (int bx = 0; bx < 8; bx++) {
                if (rawMod bitand 0x1) {
                    out_mod[by][bx] = 0;
                } else {
                    out_mod[by][bx] = 16;
                }

                rawMod >>= 1;
            }
        }
    }
}



#pragma mark - exposed interface



void bilinearFilter4x4(rgba8_t *out_rgba, const int in_X, const int in_Y, const rgba8_t in_BLOCK[3][3]) {
    static const int w[3][4] = {
        { 2, 1, 0, 0 },
        { 2, 3, 4, 3 },
        { 0, 0, 0, 1 }
    };

    for (int i = 0; i < 4; i++) {
        uint32_t filtered = 0;
        filtered += in_BLOCK[0][0].array[i] * w[0][in_X] * w[0][in_Y];
        filtered += in_BLOCK[0][1].array[i] * w[1][in_X] * w[0][in_Y];
        filtered += in_BLOCK[0][2].array[i] * w[2][in_X] * w[0][in_Y];
        filtered += in_BLOCK[1][0].array[i] * w[0][in_X] * w[1][in_Y];
        filtered += in_BLOCK[1][1].array[i] * w[1][in_X] * w[1][in_Y];
        filtered += in_BLOCK[1][2].array[i] * w[2][in_X] * w[1][in_Y];
        filtered += in_BLOCK[2][0].array[i] * w[0][in_X] * w[2][in_Y];
        filtered += in_BLOCK[2][1].array[i] * w[1][in_X] * w[2][in_Y];
        filtered += in_BLOCK[2][2].array[i] * w[2][in_X] * w[2][in_Y];
        filtered >>= 4;
        out_rgba->array[i] = filtered;
    }
}



void bilinearFilter8x4(rgba8_t *out_rgba, const int in_X, const int in_Y, const rgba8_t in_BLOCK[3][3]) {
    static const int wY[3][4] = {
        { 2, 1, 0, 0 },
        { 2, 3, 4, 3 },
        { 0, 0, 0, 1 }
    };

    static const int wX[3][8] = {
        { 4, 3, 2, 1, 0, 0, 0, 0 },
        { 4, 5, 6, 7, 8, 7, 6, 5 },
        { 0, 0, 0, 0, 0, 1, 2, 3 }
    };


    for (int i = 0; i < 4; i++) {
        uint32_t filtered = 0;
        filtered += in_BLOCK[0][0].array[i] * wX[0][in_X] * wY[0][in_Y];
        filtered += in_BLOCK[0][1].array[i] * wX[1][in_X] * wY[0][in_Y];
        filtered += in_BLOCK[0][2].array[i] * wX[2][in_X] * wY[0][in_Y];
        filtered += in_BLOCK[1][0].array[i] * wX[0][in_X] * wY[1][in_Y];
        filtered += in_BLOCK[1][1].array[i] * wX[1][in_X] * wY[1][in_Y];
        filtered += in_BLOCK[1][2].array[i] * wX[2][in_X] * wY[1][in_Y];
        filtered += in_BLOCK[2][0].array[i] * wX[0][in_X] * wY[2][in_Y];
        filtered += in_BLOCK[2][1].array[i] * wX[1][in_X] * wY[2][in_Y];
        filtered += in_BLOCK[2][2].array[i] * wX[2][in_X] * wY[2][in_Y];
        filtered >>= 5;
        out_rgba->array[i] = filtered;
    }
}



//  9 PVRTC Blocks
// [] Base Color
// ·· Decoded area
//+--------+--------+--------+
//|        |        |        |
//|        |        |        |
//|    []  |    []  |    []  |
//|        |        |        |
//+--------+--------+--------+
//|        |········|        |
//|        |········|        |
//|    []  |····[]··|    []  |
//|        |········|        |
//+--------+--------+--------+
//|        |        |        |
//|        |        |        |
//|    []  |    []  |    []  |
//|        |        |        |
//+--------+--------+--------+

void pvrtcDecodeBlock4BPP(rgba8_t out_blockRGBA[4][4], const PVRTC4Block_t in_BLOCK_PVR[3][3]) {
    rgba8_t blockA[3][3];
    rgba8_t blockB[3][3];

    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            decodeBaseColorA(&blockA[y][x], in_BLOCK_PVR[y][x].aMode, in_BLOCK_PVR[y][x].a);
            decodeBaseColorB(&blockB[y][x], in_BLOCK_PVR[y][x].bMode, in_BLOCK_PVR[y][x].b);
        }
    }

    bool modulationMode = in_BLOCK_PVR[1][1].modMode;
    int rawMod = in_BLOCK_PVR[1][1].mod;

    for (int by = 0; by < 4; by++) {
        for (int bx = 0; bx < 4; bx++) {
            rgba8_t a;
            rgba8_t b;
            rgba8_t c;
            bilinearFilter4x4(&a, bx, by, blockA);
            bilinearFilter4x4(&b, bx, by, blockB);

            int mod = rawMod bitand 0x3;
            rawMod >>= 2;

            //            a.r = 0;
            //            a.g = 0;
            //            a.b = 0;
            //
            //            b.r = 255;
            //            b.g = 255;
            //            b.b = 255;

            switch (mod) {
                case 0:
                    // 0/8
                    c = a;
                    break;

                case 1:
                    if (modulationMode) {
                        // 4/8
                        c.r = (a.r + b.r + 1) >> 1;
                        c.g = (a.g + b.g + 1) >> 1;
                        c.b = (a.b + b.b + 1) >> 1;
                        c.a = (a.a + b.a + 1) >> 1;
                    } else {
                        // 3/8
                        c.r = (a.r * 5 + b.r * 3 + 4) >> 3;
                        c.g = (a.g * 5 + b.g * 3 + 4) >> 3;
                        c.b = (a.b * 5 + b.b * 3 + 4) >> 3;
                        c.a = (a.a * 5 + b.a * 3 + 4) >> 3;
                    }

                    break;

                case 2:
                    if (modulationMode) {
                        // 4/8 alpha 0
                        c.r = (a.r + b.r + 1) >> 1;
                        c.g = (a.g + b.g + 1) >> 1;
                        c.b = (a.b + b.b + 1) >> 1;
                        c.a = 0;
                    } else {
                        // 5/8
                        c.r = (a.r * 3 + b.r * 5 + 4) >> 3;
                        c.g = (a.g * 3 + b.g * 5 + 4) >> 3;
                        c.b = (a.b * 3 + b.b * 5 + 4) >> 3;
                        c.a = (a.a * 3 + b.a * 5 + 4) >> 3;
                    }

                    break;

                case 3:
                    // 8/8
                    c = b;
                    break;
            }

            out_blockRGBA[by][bx] = c;
        }
    }
}



//  9 PVRTC Blocks
// [] Base Color
// ·· Decoded area
//    interpolated area
// ·  adjacent block area
//+----------------+----------------+----------------+
//|                |                |                |
//|                |                |                |
//|        []      |        []      |        []      |
//|                |  ·   ·   ·   · |                |
//+----------------+----------------+----------------+
//|                |··  ··  ··  ··  |·               |
//|              · |  ··  ··  ··  ··|                |
//|        []      |··  ··  []  ··  |·       []      |
//|              · |  ··  ··  ··  ··|                |
//+----------------+----------------+----------------+
//|                |·   ·   ·   ·   |                |
//|                |                |                |
//|        []      |        []      |        []      |
//|                |                |                |
//+----------------+----------------+----------------+

void pvrtcDecodeBlock2BPP(rgba8_t out_blockRGBA[4][8], const PVRTC4Block_t in_BLOCK_PVR[3][3]) {
    int8_t modulation[6][10];
    int8_t bModT[4][8];
    int8_t bModB[4][8];
    int8_t bModL[4][8];
    int8_t bModR[4][8];
    int8_t bModC[4][8];

    for (int by = 0; by < 6; by++) {
        for (int bx = 0; bx < 10; bx++) {
            modulation[by][bx] = -1;
        }
    }

    blockModulation(bModT, in_BLOCK_PVR[0][1]);
    blockModulation(bModB, in_BLOCK_PVR[2][1]);
    blockModulation(bModL, in_BLOCK_PVR[1][0]);
    blockModulation(bModR, in_BLOCK_PVR[1][2]);
    blockModulation(bModC, in_BLOCK_PVR[1][1]);

    for (int bx = 0; bx < 8; bx++) {
        modulation[0][bx + 1] = bModT[3][bx];
        modulation[5][bx + 1] = bModB[0][bx];
    }

    for (int by = 0; by < 4; by++) {
        modulation[by + 1][0] = bModL[by][7];
        modulation[by + 1][9] = bModR[by][0];
    }

    for (int by = 0; by < 4; by++) {
        for (int bx = 0; bx < 8; bx++) {
            modulation[by + 1][bx + 1] = bModC[by][bx];
        }
    }

    bool modulationMode = in_BLOCK_PVR[1][1].modMode;
    int rawMod = in_BLOCK_PVR[1][1].mod;

    if (modulationMode) {
        PVRInterpolation_t interpolationMode = HV;            // H & V

        // pixel (0, 0) has 1 bit accuracy it's least significant bit is used to tell that we are only using interpolation in one dimension
        if (rawMod bitand 0x1) {            // H or V only
            // pixel (4, 2) has 1 bit accuracy it's least significant bit is used to tell in which direction to interpolate ( 0 = H, 1 = V )
            if (rawMod bitand 0x100000) {   // V only
                interpolationMode = V;
            } else {                        // H only
                interpolationMode = H;
            }
        }

        // fill the gaps
        for (int by = 0; by < 4; by++) {
            for (int bx = 0; bx < 8; bx++) {
                if (((bx xor by) bitand 0x1) and (modulation[by + 1][bx + 1] == -1)) {       // the gabs in the checkboard
                    int avg = 0;

                    switch (interpolationMode) {
                        case HV:
                            avg += modulation[by + 1][bx];
                            avg += modulation[by + 1][bx + 2];
                            avg += modulation[by][bx + 1];
                            avg += modulation[by + 2][bx + 1];
                            avg = (avg + 2) >> 2;
                            break;

                        case H:
                            avg += modulation[by + 1][bx];
                            avg += modulation[by + 1][bx + 2];
                            avg = (avg + 1) >> 1;
                            break;

                        case V:
                            avg += modulation[by][bx + 1];
                            avg += modulation[by + 2][bx + 1];
                            avg = (avg + 1) >> 1;
                            break;
                    }

                    modulation[by + 1][bx + 1] = avg;
                }
            }
        }
    }

    rgba8_t blockA[3][3];
    rgba8_t blockB[3][3];

    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            decodeBaseColorA(&blockA[y][x], in_BLOCK_PVR[y][x].aMode, in_BLOCK_PVR[y][x].a);
            decodeBaseColorB(&blockB[y][x], in_BLOCK_PVR[y][x].bMode, in_BLOCK_PVR[y][x].b);
        }
    }

    for (int by = 0; by < 4; by++) {
        for (int bx = 0; bx < 8; bx++) {
            rgba8_t a;
            rgba8_t b;
            rgba8_t c;
            bilinearFilter8x4(&a, bx, by, blockA);
            bilinearFilter8x4(&b, bx, by, blockB);

            //            a.r = 0;
            //            a.g = 0;
            //            a.b = 0;
            //
            //            b.r = 255;
            //            b.g = 255;
            //            b.b = 255;

            int mod = modulation[by + 1][bx + 1];
            c.r = (a.r * mod + b.r * (16 - mod) + 8) >> 4;
            c.g = (a.g * mod + b.g * (16 - mod) + 8) >> 4;
            c.b = (a.b * mod + b.b * (16 - mod) + 8) >> 4;
            c.a = (a.a * mod + b.a * (16 - mod) + 8) >> 4;

            out_blockRGBA[by][bx] = c;
        }
    }
}
