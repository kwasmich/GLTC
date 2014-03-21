//
//  ETC_Decompress.h
//  GLTC
//
//  Created by Michael Kwasnicki on 04.01.13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#ifndef GLTC_ETC_Decompress_h
#define GLTC_ETC_Decompress_h

#include "colorSpaceReduction.h"

#include "ETC_Common.h"
#include "lib.h"

#include <stdbool.h>


static inline uint8_t computePlaneColor( const int in_BX, const int in_BY, const uint8_t in_CO, const uint8_t in_CH, const uint8_t in_CV ) {
	return clampi( ( ( in_BX * ( in_CH - in_CO ) ) + ( in_BY * ( in_CV - in_CO ) ) + 4 * in_CO + 2 ) >> 2, 0, 255 );
}


void computeBaseColorsID( rgb8_t * out_c0, rgb8_t * out_c1, const ETCBlockColor_t in_BLOCK );
void computeRGBColorPaletteCommonID( rgb8_t out_colorPalette[4], const rgb8_t in_C, const int in_TABLE_INDEX, const int in_TABLE[8][4] );
void computeRGBColorPaletteT( rgb8_t out_colorPalette[4], const rgb8_t in_C0, const rgb8_t in_C1, const int in_DISTNACE );
void computeRGBColorPaletteH( rgb8_t out_colorPalette[4], const rgb8_t in_C0, const rgb8_t in_C1, const int in_DISTNACE );
void computeRGBColorPaletteTHP( rgb8_t out_colorPalette[4], const ETCBlockColor_t in_BLOCK, const ETCMode_t in_MODE );

ETCMode_t etcGetBlockMode( const ETCBlockColor_t in_BLOCK, const bool in_PUNCH_THROUGH );

void decompressETC1BlockRGB( rgb8_t out_blockRGB[4][4], const ETCBlockColor_t in_BLOCK );
void decompressETC2BlockRGB( rgb8_t out_blockRGB[4][4], const ETCBlockColor_t in_BLOCK );

void decompressETC2BlockRGBA( rgba8_t out_blockRGBA[4][4], const ETC2BlockRGBA_t in_BLOCK );
void decompressETC2BlockRGBAPunchThrough( rgba8_t out_blockRGBA[4][4], const ETCBlockColor_t in_BLOCK );

#endif
