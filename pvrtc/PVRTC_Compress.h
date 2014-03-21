//
//  PVRTC_Compress.h
//  GLTC
//
//  Created by Michael Kwasnicki on 16.03.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#ifndef GLTC_PVRTC_Compress_h
#define GLTC_PVRTC_Compress_h

#include "colorSpaceReduction.h"

void bilinearFilter3_4x4( rgba8_t * out_rgba, const int in_X, const int in_Y, const rgba8_t in_BLOCK[3][3] );
uint32_t computeError( const rgba8_t in_RGBA[8][8], const rgba8_t in_BASE_A[3][3], const rgba8_t in_BASE_B[3][3] );

#endif
