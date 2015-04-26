//
//  PVRTC_Common.h
//  GLTC
//
//  Created by Michael Kwasnicki on 04.03.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#ifndef GLTC_PVRTC_Common_h
#define GLTC_PVRTC_Common_h


#include "../colorSpaceReduction.h"

#include <stdbool.h>


// WARNING: bitfields are filled from lowest to highest bit
// 565 : rrrrrggg gggbbbbb
// 4444: rrrrggggbbbbaaaa
// 5551: rrrrrgggggbbbbba


typedef union {
    struct {
        uint16_t b : 5;
        uint16_t g : 5;
        uint16_t r : 5;
    };
    
    uint16_t b16;
} rgb555_t;



typedef union {
    struct {
        uint16_t b : 4;
        uint16_t g : 5;
        uint16_t r : 5;
    };
    
    uint16_t b16;
} rgb554_t;



typedef union {
    struct {
        uint16_t b : 4;
        uint16_t g : 4;
        uint16_t r : 4;
        uint16_t a : 3;
    };
    
    uint16_t b16;
} argb3444_t;



typedef union {
    struct {
        uint16_t b : 3;
        uint16_t g : 4;
        uint16_t r : 4;
        uint16_t a : 3;
    };
    
    uint16_t b16;
} argb3443_t;



typedef union {
    struct {
        uint32_t b : 5;
        uint32_t g : 5;
        uint32_t r : 5;
        uint32_t a : 4;
    };
    
    uint32_t b32;
} argb4555_t;



typedef union {
    struct {
        uint32_t mod;
        uint32_t modMode : 1;
        uint32_t a       : 14;  // RGB554 or ARGB3443
        uint32_t aMode   : 1;
        uint32_t b       : 15;  // RGB555 or ARGB3444
        uint32_t bMode   : 1;
    };
    
    uint64_t b64;
} PVRTC4Block_t;



typedef struct {
    rgba8_t a;
    rgba8_t b;
    bool aMode;
    bool bMode;
    bool modMode;
    uint32_t mod;
} PVRTCIntermediateBlock_t;



typedef union {
    rgba8_t linear[49];
    rgba8_t block[7][7];
} LinearBlock7x7RGBA_t;


#endif
