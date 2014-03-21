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
	rgb8_t linear[16];
    rgb8_t block[4][4];
} linearBlock4x4RGB;


typedef union {
	rgba8_t linear[16];
    rgba8_t block[4][4];
} linearBlock4x4RGBA;


void twiddleBlocksRGB( rgb8_t * in_out_image, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const bool in_REVERSE );
void twiddleBlocksRGBA( rgba8_t * in_out_image, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const bool in_REVERSE );

#endif
