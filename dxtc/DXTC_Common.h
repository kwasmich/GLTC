//
//  DXTC_Common.h
//  GLTC
//
//  Created by Michael Kwasnicki on 22.02.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#ifndef GLTC_DXTC_Common_h
#define GLTC_DXTC_Common_h

#include "colorSpaceReduction.h"

#include <stdbool.h>
#include <stdlib.h>

static bool kFloatingPoint = false;
static bool kDitherDXT3Alpha = true;



typedef union {
    struct {
        rgb565_t c0;
        rgb565_t c1;
        uint32_t cBitField;
    };
    
    uint64_t b64;
} DXT1Block_t;



typedef uint64_t DXT3AlphaBlock_t;



typedef struct {
    DXT3AlphaBlock_t a;
    DXT1Block_t rgb;
} DXT3Block_t;


#warning try bitfields with : 48 instead of this cheat
typedef union {
    union {
        struct {
            uint8_t a0;
            uint8_t a1;
        };
        
        uint64_t aBitField;    // actually 48-bit but there is no such data type. That is why we use the cheat with Union
    };
    
    uint64_t b64;
} DXT5AlphaBlock_t;



typedef struct {
    DXT5AlphaBlock_t a;
    DXT1Block_t rgb;
} DXT5Block_t;



#endif
