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


#include <stdbool.h>


#include "../colorSpaceReduction.h"
#include "PVRTC_Common.h"


void bilinearFilter4x4( rgba8_t * out_rgba, const int in_X, const int in_Y, const rgba8_t in_BLOCK[3][3] );
void bilinearFilter8x4( rgba8_t * out_rgba, const int in_X, const int in_Y, const rgba8_t in_BLOCK[3][3] );

void pvrtcDecodeBlock4BPP( rgba8_t out_blockRGBA[4][4], const PVRTC4Block_t in_BLOCK_PVR[3][3] );
void pvrtcDecodeBlock2BPP( rgba8_t out_blockRGBA[4][8], const PVRTC4Block_t in_BLOCK_PVR[3][3] );


#endif
