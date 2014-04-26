//
//  DXTC_Read.h
//  GLTC
//
//  Created by Michael Kwasnicki on 22.02.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#ifndef GLTC_DXTC_Read_h
#define GLTC_DXTC_Read_h

#include "DXTC_Common.h"

#pragma mark - pallette decompression

void computeDXT1RGBPalette( rgb8_t out_rgbPalette[4], const rgb565_t in_C0, const rgb565_t in_C1 );
void computeDXT1RGBAPalette( rgba8_t out_rgbaPalette[4], const rgb565_t in_C0, const rgb565_t in_C1 );
void computeDXT3RGBPalette( rgba8_t out_rgbPalette[4], const rgb565_t in_C0, const rgb565_t in_C1 );
void computeDXT5APalette( uint8_t out_alphaPalette[8], const uint8_t in_A0, const uint8_t in_A1 );


#pragma mark - block decompression

void decompressDXT1BlockRGB( rgb8_t out_blockRGB[4][4], const DXT1Block_t in_BLOCK );
void decompressDXT1BlockRGBA( rgba8_t out_blockRGBA[4][4], const DXT1Block_t in_BLOCK );
void decompressDXT3BlockRGBA( rgba8_t out_blockRGBA[4][4], const DXT3Block_t in_BLOCK );
void decompressDXT5BlockRGBA( rgba8_t out_blockRGBA[4][4], const DXT5Block_t in_BLOCK );

#endif
