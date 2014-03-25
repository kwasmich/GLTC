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

// WARNING: bitfields are filled from lowest to highest bit
// 565 : rrrrrggg gggbbbbb
// 4444: rrrrggggbbbbaaaa
// 5551: rrrrrgggggbbbbba



typedef union {
    struct {
        uint16_t b : 5;
        uint16_t g : 5;
        uint16_t r : 5;
        uint16_t mode : 1;
    };
    
    uint16_t b16;
} rgb555_t;



typedef union {
    struct {
        uint16_t modulation : 1;
        uint16_t b : 4;
        uint16_t g : 5;
        uint16_t r : 5;
        uint16_t mode : 1;
    };
    
    uint16_t b16;
} rgb554_t;



typedef union {
    struct {
        uint16_t b : 4;
        uint16_t g : 4;
        uint16_t r : 4;
        uint16_t a : 3;
        uint16_t mode : 1;
    };
    
    uint16_t b16;
} argb3444_t;



typedef union {
    struct {
        uint16_t modulation : 1;
        uint16_t b : 3;
        uint16_t g : 4;
        uint16_t r : 4;
        uint16_t a : 3;
        uint16_t mode : 1;
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
        uint16_t a;
        uint16_t b;
    };
    
    uint64_t b64;
} PVRTC4Block_t;



#endif