//
//  ETC_Common.h
//  GLTC
//
//  Created by Michael Kwasnicki on 04.01.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#ifndef GLTC_ETC_Common_h
#define GLTC_ETC_Common_h



#include "../blockCompressionCommon.h"
#include "../colorSpaceReduction.h"

#include <stdlib.h>



// WARNING: bitfields are filled from lowest to highest bit
// 565 : rrrrrggg gggbbbbb
// 4444: rrrrggggbbbbaaaa
// 5551: rrrrrgggggbbbbba


typedef enum {
    kETC_I, // Individual
    kETC_D, // Differential
    kETC_T, // T-Mode
    kETC_H, // H-Mode
    kETC_P, // Plane-Mode
    kETC_INVALID
} ETCMode_t;


typedef union {
    struct {
        uint32_t cBitField;
        uint32_t flip : 1;
        uint32_t differential : 1; // opacity in RGB8A1
        uint32_t table1 : 3;
        uint32_t table0 : 3;
        int32_t dB : 3;
        uint32_t b : 5;
        int32_t dG : 3;
        uint32_t g : 5;
        int32_t dR : 3;
        uint32_t r : 5;
    };

    uint64_t b64;
} ETCBlockColor_t;



typedef ETCBlockColor_t ETCBlockD_t;



typedef union {
    struct {
        uint32_t cBitField;
        uint32_t flip : 1;
        uint32_t zero : 1;
        uint32_t table1 : 3;
        uint32_t table0 : 3;
        uint32_t b1 : 4;
        uint32_t b0 : 4;
        uint32_t g1 : 4;
        uint32_t g0 : 4;
        uint32_t r1 : 4;
        uint32_t r0 : 4;
    };

    uint64_t b64;
} ETCBlockI_t;



typedef union {
    struct {
        uint32_t cBitField;
        uint32_t db : 1;
        uint32_t opaque : 1; // 1 except for RGB8A1
        uint32_t da : 2;
        uint32_t b1 : 4;
        uint32_t g1 : 4;
        uint32_t r1 : 4;
        uint32_t b0 : 4;
        uint32_t g0 : 4;
        uint32_t r0b : 2;
        uint32_t dummy1 : 1;
        uint32_t r0a : 2;
        uint32_t dummy0 : 3;
    };

    uint64_t b64;
} ETCBlockT_t;



typedef union {
    struct {
        uint32_t cBitField;
        uint32_t db : 1;
        uint32_t opaque : 1; // 1 except for RGB8A1
        uint32_t da : 1;
        uint32_t b1 : 4;
        uint32_t g1 : 4;
        uint32_t r1 : 4;
        uint32_t b0b : 3;
        uint32_t dummy2 : 1;
        uint32_t b0a : 1;
        uint32_t g0b : 1;
        uint32_t dummy1 : 3;
        uint32_t g0a : 3;
        uint32_t r0 : 4;
        uint32_t dummy0 : 1;
    };

    uint64_t b64;
} ETCBlockH_t;



typedef union {
    struct {
        uint32_t bV : 6;
        uint32_t gV : 7;
        uint32_t rV : 6;
        uint32_t bH : 6;
        uint32_t gH : 7;
        uint32_t rH2 : 1;
        uint32_t one : 1; // ALWAYS 1
        uint32_t rH1 : 5;
        uint32_t bO3 : 3;
        uint32_t dummy3 : 1;
        uint32_t bO2 : 2;
        uint32_t dummy2 : 3;
        uint32_t bO1 : 1;
        uint32_t gO2 : 6;
        uint32_t dummy1 : 1;
        uint32_t gO1 : 1;
        uint32_t rO : 6;
        uint32_t dummy0 : 1;
    };

    uint64_t b64;
} ETCBlockP_t;



typedef union {
    struct {
        uint64_t aBitField : 48;
        uint64_t table : 4;
        uint64_t mul : 4;
        uint64_t base : 8;
    };

    uint64_t b64;
} EACBlock_t;

typedef union {
    struct {
        uint64_t aBitField : 48;
        uint64_t table : 4;
        uint64_t mul : 4;
        int64_t base : 8;
    };

    uint64_t b64;
} EACBlockSigned_t;

typedef EACBlock_t EACBlockR11_t;
typedef EACBlockSigned_t EACBlockR11Signed_t;

typedef struct {
    EACBlock_t r;
    EACBlock_t g;
} EACBlockRG11_t;

typedef struct {
    EACBlockSigned_t r;
    EACBlockSigned_t g;
} EACBlockRG11Signed_t;




typedef EACBlock_t ETCBlockAlpha_t;

typedef struct {
    ETCBlockAlpha_t alpha;
    ETCBlockColor_t color;
} ETC2BlockRGBA_t;


#define ETC_TABLE_COUNT 8
#define ETC_PALETTE_SIZE 4


static const int ETC_MODIFIER_TABLE[ETC_TABLE_COUNT][ETC_PALETTE_SIZE] = {
    {  2,   8,  -2,   -8 },
    {  5,  17,  -5,  -17 },
    {  9,  29,  -9,  -29 },
    { 13,  42, -13,  -42 },
    { 18,  60, -18,  -60 },
    { 24,  80, -24,  -80 },
    { 33, 106, -33, -106 },
    { 47, 183, -47, -183 }
};



static const int ETC_MODIFIER_TABLE_NON_OPAQUE[ETC_TABLE_COUNT][ETC_PALETTE_SIZE] = {
    { 0,   8, 0,   -8 },
    { 0,  17, 0,  -17 },
    { 0,  29, 0,  -29 },
    { 0,  42, 0,  -42 },
    { 0,  60, 0,  -60 },
    { 0,  80, 0,  -80 },
    { 0, 106, 0, -106 },
    { 0, 183, 0, -183 }
};


#define ETC_DISTANCE_TABLE_COUNT 8


static const int ETC_DISTANCE_TABLE[ETC_DISTANCE_TABLE_COUNT] = { 3, 6, 11, 16, 23, 32, 41, 64 };


#define ETC_ALPHA_TABLE_COUNT 16
#define ETC_ALPHA_PALETTE_SIZE 8
#define ETC_ALPHA_MULTIPLIER_COUNT 16


static const int ETC_ALPHA_MODIFIER_TABLE[ETC_ALPHA_TABLE_COUNT][ETC_ALPHA_PALETTE_SIZE] = {
    { -3, -6,  -9, -15, 2, 5, 8, 14 },
    { -3, -7, -10, -13, 2, 6, 9, 12 },
    { -2, -5,  -8, -13, 1, 4, 7, 12 },
    { -2, -4,  -6, -13, 1, 3, 5, 12 },
    { -3, -6,  -8, -12, 2, 5, 7, 11 },
    { -3, -7,  -9, -11, 2, 6, 8, 10 },
    { -4, -7,  -8, -11, 3, 6, 7, 10 },
    { -3, -5,  -8, -11, 2, 4, 7, 10 },
    { -2, -6,  -8, -10, 1, 5, 7,  9 },
    { -2, -5,  -8, -10, 1, 4, 7,  9 },
    { -2, -4,  -8, -10, 1, 3, 7,  9 },
    { -2, -5,  -7, -10, 1, 4, 6,  9 },
    { -3, -4,  -7, -10, 2, 3, 6,  9 },
    { -1, -2,  -3, -10, 0, 1, 2,  9 },
    { -4, -6,  -8,  -9, 3, 5, 7,  8 },
    { -3, -5,  -7,  -9, 2, 4, 6,  8 }
};


#endif
