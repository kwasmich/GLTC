//
//  ETC_Decompress.h
//  GLTC
//
//  Created by Michael Kwasnicki on 04.01.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#ifndef GLTC_ETC_Decompress_h
#define GLTC_ETC_Decompress_h


#include "ETC_Common.h"

#include "../lib.h"

#include <stdbool.h>


static inline uint8_t computePlaneColor( const int in_BX, const int in_BY, const uint8_t in_CO, const uint8_t in_CH, const uint8_t in_CV ) {
	return (uint8_t)clampi( ( ( in_BX * ( in_CH - in_CO ) ) + ( in_BY * ( in_CV - in_CO ) ) + 4 * in_CO + 2 ) >> 2, 0, 255 );
}

void computeAlphaPalette( uint8_t out_alphaPalette[ETC_ALPHA_PALETTE_SIZE], const uint8_t in_BASE, const int in_TABLE, const int in_MUL );
void computeBaseColorsID( rgba8_t * out_c0, rgba8_t * out_c1, const ETCBlockColor_t in_BLOCK );
void computeRGBColorPaletteCommonID( rgba8_t out_colorPalette[4], const rgba8_t in_C, const int in_TABLE_INDEX, const int in_TABLE[8][4] );
void computeColorPaletteT( rgba8_t out_colorPalette[4], const rgba8_t in_C0, const rgba8_t in_C1, const int in_DISTNACE, const bool in_OPAQUE );
void computeColorPaletteH( rgba8_t out_colorPalette[4], const rgba8_t in_C0, const rgba8_t in_C1, const int in_DISTNACE, const bool in_OPAQUE );

ETCMode_t etcGetBlockMode( const ETCBlockColor_t in_BLOCK, const bool in_OPAQUE );

void decompressETC1BlockRGB( rgba8_t out_blockRGBA[4][4], const ETCBlockColor_t in_BLOCK );
void decompressETC2BlockRGB( rgba8_t out_blockRGBA[4][4], const ETCBlockColor_t in_BLOCK );

void decompressETC2BlockRGBA8( rgba8_t out_blockRGBA[4][4], const ETC2BlockRGBA_t in_BLOCK );
void decompressETC2BlockRGB8A1( rgba8_t out_blockRGBA[4][4], const ETCBlockColor_t in_BLOCK );

#endif
