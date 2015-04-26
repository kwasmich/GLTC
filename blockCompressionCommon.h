//
//  blockCompressionCommon.h
//  GLTC
//
//  Created by Michael Kwasnicki on 02.03.14.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#ifndef GLTC_blockCompressionCommon_h
#define GLTC_blockCompressionCommon_h

#include "colorSpaceReduction.h"

#include <stdbool.h>



typedef union {
	rgba8_t linear[16];
    rgba8_t block[4][4];
} LinearBlock4x4RGBA_t;


void twiddleBlocksRGBA( rgba8_t * in_out_image, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const bool in_REVERSE );


//typedef uint8_t Strategy_t; // intended only for values [0..9]

typedef enum {
	kFAST = 0,
//	kS1 = 1,
//	kS2 = 2,
//	kS3 = 3,
//	kS4 = 4,
//	kS5 = 5,
//	kS6 = 6,
//	kS7 = 7,
//	kS8 = 8,
    kBEST = 9
} Strategy_t;


#endif
