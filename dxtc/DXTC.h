//
//  DXTC.h
//  GLTC
//
//  Created by Michael Kwasnicki on 14.02.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#ifndef GLTC_DXTC_h
#define GLTC_DXTC_h


#include "colorSpaceReduction.h"

#include <stdbool.h>


bool dxtcReadDXT1RGB( const char in_FILE[], rgb8_t ** out_image, uint32_t * out_width, uint32_t * out_height );

bool dxtcWriteDXT1RGB( const char in_FILE[], const rgb8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT );

bool dxtcFreeRGB( rgb8_t ** in_out_image );



bool dxtcReadDXT1RGBA( const char in_FILE[], rgba8_t ** out_image, uint32_t * out_width, uint32_t * out_height );
bool dxtcReadDXT3RGBA( const char in_FILE[], rgba8_t ** out_image, uint32_t * out_width, uint32_t * out_height );
bool dxtcReadDXT5RGBA( const char in_FILE[], rgba8_t ** out_image, uint32_t * out_width, uint32_t * out_height );

bool dxtcWriteDXT1RGBA( const char in_FILE[], const rgba8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT );
bool dxtcWriteDXT3RGBA( const char in_FILE[], const rgba8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT );
bool dxtcWriteDXT5RGBA( const char in_FILE[], const rgba8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT );

bool dxtcFreeRGBA( rgba8_t ** in_out_image );



#endif


// DXT3
// c0 must be larger than c1
// dither Alpha prior to compression

//DXT5
// c0 must be larger than c1
