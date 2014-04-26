//
//  PVRTC_Decompress.h
//  GLTC
//
//  Created by Michael Kwasnicki on 04.03.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#ifndef GLTC_PVRTC_Decompress_h
#define GLTC_PVRTC_Decompress_h

#include "../colorSpaceReduction.h"
#include "PVRTC_Common.h"

void decodeBaseColorA( rgba8_t * out_RGBA, const uint16_t in_COLOR_A );
void decodeBaseColorB( rgba8_t * out_RGBA, const uint16_t in_COLOR_B );

void bilinearFilter4x4( rgba8_t * out_RGBA, const int in_X, const int in_Y, const rgba8_t in_BLOCK[2][2] );

void pvrtcDecodeBlock4BPP( rgba8_t out_blockRGBA[4][4], const PVRTC4Block_t in_BLOCK_PVR[2][2] );

#endif
