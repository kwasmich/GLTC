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


#include "PVRTC_Common.h"


void bilinearFilter3_4x4( rgba8_t * out_rgba, const int in_X, const int in_Y, const rgba8_t in_BLOCK[3][3] );
uint32_t computeMetaBlockError( uint8_t out_modulation[4][4], rgba8_t out_rgba[7][7], const rgba8_t in_RGBA[7][7], const rgba8_t in_BASE_A[7][7], const rgba8_t in_BASE_B[7][7] );

void packMetaBlocksRGBA( LinearBlock7x7RGBA_t * out_image, const rgba8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT );
void initialGuess( PVRTCIntermediateBlock_t * out_block, const rgba8_t in_RGBA[7][7] );

void fakeCompress4BPP( rgba8_t out_rgba[4][4], const rgba8_t in_RGBA[7][7], const PVRTCIntermediateBlock_t in_BLOCKS[3][3] );

#endif
