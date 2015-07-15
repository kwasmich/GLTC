//
//  ETC_Decompress.c
//  GLTC
//
//  Created by Michael Kwasnicki on 04.01.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#include "ETC_Decompress.h"

#include <assert.h>
#include <iso646.h>
#include <stdio.h>



void computeAlphaPalette( uint8_t out_alphaPalette[ETC_ALPHA_PALETTE_SIZE], const uint8_t in_BASE, const int in_TABLE, const int in_MUL ) {
	const int *LUT = ETC_ALPHA_MODIFIER_TABLE[in_TABLE];
	//assert( in_MUL > 0 ); // An encoder is not allowed to produce a multiplier of zero, but the decoder should still be able to handle also this case (and produce 0× modifier = 0 in that case).
	// etcpack violates this
	
	for ( int p = 0; p < ETC_ALPHA_PALETTE_SIZE; p++ ) {
		out_alphaPalette[p] = clampi( in_BASE + LUT[p] * in_MUL, 0, 255 );
	}
}



static void computeBaseColorsD( rgba8_t * out_c0, rgba8_t * out_c1, const rgb5_t in_C0, const rgb3_t in_C1 ) {
	int temp[3];
	out_c0->r = extend5to8bits( in_C0.r );
	out_c0->g = extend5to8bits( in_C0.g );
	out_c0->b = extend5to8bits( in_C0.b );
	
	temp[0] = in_C0.r + in_C1.r;
	temp[1] = in_C0.g + in_C1.g;
	temp[2] = in_C0.b + in_C1.b;
	
	assert( temp[0] >= 0 and temp[0] < 32 );	// disallow underflow and overflow
	assert( temp[1] >= 0 and temp[1] < 32 );	// disallow underflow and overflow
	assert( temp[2] >= 0 and temp[2] < 32 );	// disallow underflow and overflow
	
	out_c1->r = extend5to8bits( temp[0] );
	out_c1->g = extend5to8bits( temp[1] );
	out_c1->b = extend5to8bits( temp[2] );
}



void computeBaseColorsID( rgba8_t * out_c0, rgba8_t * out_c1, const ETCBlockColor_t in_BLOCK ) {
	if ( in_BLOCK.differential ) {
		ETCBlockD_t block = REINTERPRET(ETCBlockD_t)in_BLOCK;
		rgb5_t bc0 = RGB( block.r, block.g, block.b );
		rgb3_t bc1 = RGB( block.dR, block.dG, block.dB );
		computeBaseColorsD( out_c0, out_c1, bc0, bc1 );
	} else {
		ETCBlockI_t block = REINTERPRET(ETCBlockI_t)in_BLOCK;
		rgb4_t bc0 = RGB( block.r0, block.g0, block.b0 );
		rgb4_t bc1 = RGB( block.r1, block.g1, block.b1 );
		convert444to8888( out_c0, bc0 );
		convert444to8888( out_c1, bc1 );
	}
}



void computeRGBColorPaletteCommonID( rgba8_t out_colorPalette[4], const rgba8_t in_C, const int in_TABLE_INDEX, const int in_TABLE[8][4] ) {
	const int *LUT = in_TABLE[in_TABLE_INDEX];
	
	for ( int i = 0; i < 4; i++ ) {
		rgba8_t col = in_C;
		int luminance = LUT[i];
		col.r = clampi( col.r + luminance, 0, 255 );
		col.g = clampi( col.g + luminance, 0, 255 );
		col.b = clampi( col.b + luminance, 0, 255 );
		out_colorPalette[i] = col;
	}
}



static void computeRGBAColorPaletteCommonID( rgba8_t out_colorPalette[8], const rgba8_t in_C0, const rgba8_t in_C1, const int in_TABLE_INDEX[2], const bool in_OPAQUE ) {
	rgba8_t palette[8];
	
	if ( in_OPAQUE ) {
		computeRGBColorPaletteCommonID( &palette[0], in_C0, in_TABLE_INDEX[0], ETC_MODIFIER_TABLE );
		computeRGBColorPaletteCommonID( &palette[4], in_C1, in_TABLE_INDEX[1], ETC_MODIFIER_TABLE );
	} else {
		computeRGBColorPaletteCommonID( &palette[0], in_C0, in_TABLE_INDEX[0], ETC_MODIFIER_TABLE_NON_OPAQUE );
		computeRGBColorPaletteCommonID( &palette[4], in_C1, in_TABLE_INDEX[1], ETC_MODIFIER_TABLE_NON_OPAQUE );
	}
	
	for ( int p = 0; p < 8; p++ ) {
		bool punchThrough = ( not in_OPAQUE ) and ( ( p bitand 0x3 ) == 0x2 );
		
		if ( punchThrough ) {
			out_colorPalette[p].r = 0;
			out_colorPalette[p].g = 0;
			out_colorPalette[p].b = 0;
			out_colorPalette[p].a = 0;
		} else {
			out_colorPalette[p].r = palette[p].r;
			out_colorPalette[p].g = palette[p].g;
			out_colorPalette[p].b = palette[p].b;
			out_colorPalette[p].a = 255;
		}
	}
}



static void computeRGBColorPaletteID( rgba8_t out_colorPalette[8], const ETCBlockColor_t in_BLOCK ) {
	rgba8_t c0, c1;
	computeBaseColorsID( &c0, &c1, in_BLOCK );
	computeRGBColorPaletteCommonID( &out_colorPalette[0], c0, in_BLOCK.table0, ETC_MODIFIER_TABLE );
	computeRGBColorPaletteCommonID( &out_colorPalette[4], c1, in_BLOCK.table1, ETC_MODIFIER_TABLE );
}



static void computeRGBAColorPaletteID( rgba8_t out_colorPalette[8], const ETCBlockColor_t in_BLOCK ) {
	rgba8_t c0, c1;
	int tableIndex[2] = { in_BLOCK.table0, in_BLOCK.table1 };
	bool opaque = in_BLOCK.differential;
	ETCBlockColor_t blockCopy = in_BLOCK;
	
	if ( not opaque )
		blockCopy.differential = true;
	
	computeBaseColorsID( &c0, &c1, blockCopy );
	computeRGBAColorPaletteCommonID( out_colorPalette, c0, c1, tableIndex, opaque );
}



void computeColorPaletteT( rgba8_t out_colorPalette[4], const rgba8_t in_C0, const rgba8_t in_C1, const int in_DISTNACE, const bool in_OPAQUE ) {
	int dist = ETC_DISTANCE_TABLE[in_DISTNACE];
	out_colorPalette[0] = in_C0;
	out_colorPalette[1].r = clampi( in_C1.r + dist, 0, 255 );
	out_colorPalette[1].g = clampi( in_C1.g + dist, 0, 255 );
	out_colorPalette[1].b = clampi( in_C1.b + dist, 0, 255 );
	out_colorPalette[1].a = 255;
	out_colorPalette[2] = in_C1;
	out_colorPalette[3].r = clampi( in_C1.r - dist, 0, 255 );
	out_colorPalette[3].g = clampi( in_C1.g - dist, 0, 255 );
	out_colorPalette[3].b = clampi( in_C1.b - dist, 0, 255 );
	out_colorPalette[3].a = 255;
	
	if ( not in_OPAQUE )
		out_colorPalette[2] = (rgba8_t)RGBA( 0, 0, 0, 0 );
}



void computeColorPaletteH( rgba8_t out_colorPalette[4], const rgba8_t in_C0, const rgba8_t in_C1, const int in_DISTNACE, const bool in_OPAQUE ) {
	int dist = ETC_DISTANCE_TABLE[in_DISTNACE];
	out_colorPalette[0].r = clampi( in_C0.r + dist, 0, 255 );
	out_colorPalette[0].g = clampi( in_C0.g + dist, 0, 255 );
	out_colorPalette[0].b = clampi( in_C0.b + dist, 0, 255 );
	out_colorPalette[0].a = 255;
	out_colorPalette[1].r = clampi( in_C0.r - dist, 0, 255 );
	out_colorPalette[1].g = clampi( in_C0.g - dist, 0, 255 );
	out_colorPalette[1].b = clampi( in_C0.b - dist, 0, 255 );
	out_colorPalette[1].a = 255;
	out_colorPalette[2].r = clampi( in_C1.r + dist, 0, 255 );
	out_colorPalette[2].g = clampi( in_C1.g + dist, 0, 255 );
	out_colorPalette[2].b = clampi( in_C1.b + dist, 0, 255 );
	out_colorPalette[2].a = 255;
	out_colorPalette[3].r = clampi( in_C1.r - dist, 0, 255 );
	out_colorPalette[3].g = clampi( in_C1.g - dist, 0, 255 );
	out_colorPalette[3].b = clampi( in_C1.b - dist, 0, 255 );
	out_colorPalette[3].a = 255;
	
	if ( not in_OPAQUE )
		out_colorPalette[2] = (rgba8_t)RGBA( 0, 0, 0, 0 );
}



static void computeBaseColorsT( rgba8_t * out_c0, rgba8_t * out_c1, int * out_distance, const ETCBlockT_t in_BLOCK ) {
	int tmpR = ( in_BLOCK.r0a << 2 ) bitor in_BLOCK.r0b;
	*out_distance = ( in_BLOCK.da << 1 ) bitor in_BLOCK.db;
	out_c0->r = extend4to8bits( tmpR );
	out_c0->g = extend4to8bits( in_BLOCK.g0 );
	out_c0->b = extend4to8bits( in_BLOCK.b0 );
	out_c0->a = 255;
	out_c1->r = extend4to8bits( in_BLOCK.r1 );
	out_c1->g = extend4to8bits( in_BLOCK.g1 );
	out_c1->b = extend4to8bits( in_BLOCK.b1 );
	out_c1->a = 255;
}



static void computeBaseColorsH( rgba8_t * out_c0, rgba8_t * out_c1, int * out_distance, const ETCBlockH_t in_BLOCK ) {
	int tmpG = ( in_BLOCK.g0a << 1 ) bitor in_BLOCK.g0b;
	int tmpB = ( in_BLOCK.b0a << 3 ) bitor in_BLOCK.b0b;
	out_c0->r = extend4to8bits( in_BLOCK.r0 );
	out_c0->g = extend4to8bits( tmpG );
	out_c0->b = extend4to8bits( tmpB );
	out_c0->a = 255;
	out_c1->r = extend4to8bits( in_BLOCK.r1 );
	out_c1->g = extend4to8bits( in_BLOCK.g1 );
	out_c1->b = extend4to8bits( in_BLOCK.b1 );
	out_c1->a = 255;
	int tmpC0 = ( out_c0->r << 16 ) bitor ( out_c0->g << 8 ) bitor out_c0->b;
	int tmpC1 = ( out_c1->r << 16 ) bitor ( out_c1->g << 8 ) bitor out_c1->b;
	*out_distance = ( in_BLOCK.da << 2 ) bitor ( in_BLOCK.db << 1 ) bitor ( tmpC0 >= tmpC1 );
}



static void computeBaseColorsP( rgba8_t * out_cO, rgba8_t * out_cH, rgba8_t * out_cV, const ETCBlockP_t in_BLOCK ) {
	int tmpG = ( in_BLOCK.gO1 << 6 ) bitor in_BLOCK.gO2;
	int tmpB = ( in_BLOCK.bO1 << 5 ) bitor ( in_BLOCK.bO2 << 3 ) bitor in_BLOCK.bO3;
	int tmpR = ( in_BLOCK.rH1 << 1 ) bitor in_BLOCK.rH2;
	out_cO->r = extend6to8bits( in_BLOCK.rO );
	out_cO->g = extend7to8bits( tmpG );
	out_cO->b = extend6to8bits( tmpB );
	out_cH->r = extend6to8bits( tmpR );
	out_cH->g = extend7to8bits( in_BLOCK.gH );
	out_cH->b = extend6to8bits( in_BLOCK.bH );
	out_cV->r = extend6to8bits( in_BLOCK.rV );
	out_cV->g = extend7to8bits( in_BLOCK.gV );
	out_cV->b = extend6to8bits( in_BLOCK.bV );
}



ETCMode_t etcGetBlockMode( const ETCBlockColor_t in_BLOCK, const bool in_PUNCH_THROUGH ) {
	ETCMode_t mode = kETC_INVALID;
	
	if ( in_BLOCK.differential or in_PUNCH_THROUGH ) {
		if ( ( in_BLOCK.r + in_BLOCK.dR ) < 0 or ( in_BLOCK.r + in_BLOCK.dR ) > 31 ) {
			mode = kETC_T;
		} else if ( ( in_BLOCK.g + in_BLOCK.dG ) < 0 or ( in_BLOCK.g + in_BLOCK.dG ) > 31 ) {
			mode = kETC_H;
		} else if ( ( in_BLOCK.b + in_BLOCK.dB ) < 0 or ( in_BLOCK.b + in_BLOCK.dB ) > 31 ) {
			mode = kETC_P;
		} else {
			mode = kETC_D;
		}
	} else {
		mode = kETC_I;
	}
	
	return mode;
}



static void decompressETC2BlockAlpha( uint8_t out_blockA[4][4], const ETCBlockAlpha_t in_BLOCK ) {
	uint8_t alphaPalette[8];
	uint64_t aBitField = in_BLOCK.aBitField;
	computeAlphaPalette( alphaPalette, in_BLOCK.base, in_BLOCK.table, in_BLOCK.mul );

	for ( int bx = 3; bx >= 0; bx-- ) {
		for ( int by = 3; by >= 0; by-- ) {
			int paletteIndex = aBitField bitand 0x7;
			out_blockA[by][bx] = alphaPalette[paletteIndex];
			aBitField >>= 3;
		}
	}
}



static void _decompressEACBlockR11( rgba8_t out_blockRGBA[4][4], const EACBlockR11_t in_BLOCK ) {
	const int *LUT = ETC_ALPHA_MODIFIER_TABLE[in_BLOCK.table];
	int base = ( in_BLOCK.base << 3 ) bitor 0x4; // base * 8 + 4
	int mul = in_BLOCK.mul << 3; // mul * 8
	int bx, by;
	int codewords[4][4];
	uint64_t bitField = in_BLOCK.aBitField;
	
	for ( bx = 3; bx >= 0; bx-- ) {
		for ( by = 3; by >= 0; by-- ) {
			codewords[by][bx] = bitField bitand 0x7;
			bitField >>= 3;
		}
	}
	
	for ( by = 0; by < 4; by++ ) {
		for ( bx = 0; bx < 4; bx++ ) {
			int16_t value11;
			int16_t value16;
			
			if ( mul > 0 ) {
				value11 = clampi( base + LUT[codewords[by][bx]] * mul, 0, 2047 );
				// alpha = value >> 3 but not exactly because of mul = 0
			} else {
				value11 = clampi( base + LUT[codewords[by][bx]], 0, 2047 );
				// alpha = value >> 3 but not exactly because of mul = 0
			}
			
			value16 = ( value11 << 5 ) bitor ( value11 >> 6 );
            printf( "%i\n", value16 );
		}
	}
	
	assert( false ); // not yet implemented;
}



static void _decompressEACBlockR11Signed( rgba8_t out_blockRGBA[4][4], const EACBlockR11Signed_t in_BLOCK ) {
	assert( in_BLOCK.base != -128 ); // It is a two’s-complement value in the range [−127, 127], and where the value −128 is not allowed; however, if it should occur anyway it must be treated as −127.
	
	const int *LUT = ETC_ALPHA_MODIFIER_TABLE[in_BLOCK.table];
	int base = clampi( in_BLOCK.base, -127, 127 ) << 3; // base * 8
	int mul = in_BLOCK.mul << 3; // mul * 8
	int bx, by;
	int codewords[4][4];
	uint64_t bitField = in_BLOCK.aBitField;
	
	for ( bx = 3; bx >= 0; bx-- ) {
		for ( by = 3; by >= 0; by-- ) {
			codewords[by][bx] = bitField bitand 0x7;
			bitField >>= 3;
		}
	}
	
	for ( by = 0; by < 4; by++ ) {
		for ( bx = 0; bx < 4; bx++ ) {
			int16_t value11;
			int16_t value16;
			
			if ( mul > 0 ) {
				value11 = clampi( base + LUT[codewords[by][bx]] * mul, -1023, 1023 );
				// alpha = value >> 3 but not exactly because of mul = 0
			} else {
				value11 = clampi( base + LUT[codewords[by][bx]], -1023, 1023 );
				// alpha = value >> 3 but not exactly because of mul = 0
			}
			
			if ( value11 < 0 ) {
				value11 = -value11;
				value16 = ( value11 << 5 ) bitor ( value11 >> 5 );
				value16 = -value16;
				value11 = -value11;
			} else {
				value16 = ( value11 << 5 ) bitor ( value11 >> 5 );
			}
            
            printf( "%i %i\n", value11, value16 );
		}
	}
	
    
	assert( false ); // not yet implemented;
}



static void _decompressEACBlockRG11( rgba8_t out_blockRGBA[4][4], const EACBlockRG11_t in_BLOCK ) {
	_decompressEACBlockR11( out_blockRGBA, in_BLOCK.r );
	_decompressEACBlockR11( out_blockRGBA, in_BLOCK.g );
}



static void _decompressEACBlockRG11Signed( rgba8_t out_blockRGBA[4][4], const EACBlockRG11Signed_t in_BLOCK ) {
	_decompressEACBlockR11Signed( out_blockRGBA, in_BLOCK.r );
	_decompressEACBlockR11Signed( out_blockRGBA, in_BLOCK.g );
}



void decompressETC1BlockRGB( rgba8_t out_blockRGBA[4][4], const ETCBlockColor_t in_BLOCK ) {
	rgba8_t palette[8];
	int paletteIndex, paletteShift;
	uint32_t bitField = in_BLOCK.cBitField;
	computeRGBColorPaletteID( palette, in_BLOCK );
	
	for ( int bx = 0; bx < 4; bx++ ) {
		for ( int by = 0; by < 4; by++ ) {
			// if we are in the are of the second part then we use the second half of the palette
			if ( ( not in_BLOCK.flip and ( bx >> 1 ) ) or ( in_BLOCK.flip and ( by >> 1 ) ) )
				paletteShift = 4;
			else
				paletteShift = 0;
			
			paletteIndex = ( ( bitField bitand 0x1 ) bitor ( bitField >> 15 bitand 0x2 ) ) + paletteShift;
			out_blockRGBA[by][bx] = palette[paletteIndex];
			bitField >>= 1;
		}
	}
}



static void decodeRGBBlockTH( rgba8_t out_blockRGBA[4][4], const rgba8_t in_PALETTE[4], const ETCBlockColor_t in_BLOCK ) {
	uint32_t bitField = in_BLOCK.cBitField;
	
	for ( int bx = 0; bx < 4; bx++ ) {
		for ( int by = 0; by < 4; by++ ) {
			int paletteIndex = ( bitField bitand 0x1 ) bitor ( bitField >> 15 bitand 0x2 );
			out_blockRGBA[by][bx] = in_PALETTE[paletteIndex];
			bitField >>= 1;
		}
	}
}



static void decodeRGBABlockTH( rgba8_t out_blockRGBA[4][4], const rgba8_t in_PALETTE[4], const ETCBlockColor_t in_BLOCK ) {
	uint32_t bitField = in_BLOCK.cBitField;
	
	for ( int bx = 0; bx < 4; bx++ ) {
		for ( int by = 0; by < 4; by++ ) {
			int paletteIndex = ( bitField bitand 0x1 ) bitor ( bitField >> 15 bitand 0x2 );
			out_blockRGBA[by][bx] = in_PALETTE[paletteIndex];
			bitField >>= 1;
		}
	}
}



static void decompressETC2BlockP( rgba8_t out_blockRGBA[4][4], const ETCBlockColor_t in_BLOCK ) {
	rgba8_t palette[3];
    rgba8_t col = RGBA( 0, 0, 0, 255 );
	computeBaseColorsP( &palette[0], &palette[1], &palette[2], REINTERPRET(ETCBlockP_t)in_BLOCK );
	
	for ( int by = 0; by < 4; by++ ) {
		for ( int bx = 0; bx < 4; bx++ ) {
			col.r = computePlaneColor( bx, by, palette[0].r, palette[1].r, palette[2].r );
			col.g = computePlaneColor( bx, by, palette[0].g, palette[1].g, palette[2].g );
			col.b = computePlaneColor( bx, by, palette[0].b, palette[1].b, palette[2].b );
			out_blockRGBA[by][bx] = col;
		}
	}
}



void decompressETC2BlockRGB( rgba8_t out_blockRGBA[4][4], const ETCBlockColor_t in_BLOCK ) {
	rgba8_t c0, c1, palette[4];
	int distance;
	ETCMode_t mode = etcGetBlockMode( in_BLOCK, false );

	switch ( mode ) {
		case kETC_I:
		case kETC_D:
			decompressETC1BlockRGB( out_blockRGBA, in_BLOCK );
			break;
			
		case kETC_T:
			computeBaseColorsT( &c0, &c1, &distance, REINTERPRET(ETCBlockT_t)in_BLOCK );
			computeColorPaletteT( palette, c0, c1, distance, true );
			decodeRGBBlockTH( out_blockRGBA, palette, in_BLOCK );
			break;
			
		case kETC_H:
			computeBaseColorsH( &c0, &c1, &distance, REINTERPRET(ETCBlockH_t)in_BLOCK );
			computeColorPaletteH( palette, c0, c1, distance, true );
			decodeRGBBlockTH( out_blockRGBA, palette, in_BLOCK );
			break;
			
		case kETC_P:
			decompressETC2BlockP( out_blockRGBA, in_BLOCK );
			break;
			
		case kETC_INVALID:
			assert( false );
	}
}



void decompressETC2BlockRGBA8( rgba8_t out_blockRGBA[4][4], const ETC2BlockRGBA_t in_BLOCK ) {
	rgba8_t color[4][4];
	uint8_t alpha[4][4];
	decompressETC2BlockRGB( color, in_BLOCK.color );
	decompressETC2BlockAlpha( alpha, in_BLOCK.alpha );
	
	for ( int by = 0; by < 4; by++ ) {
		for ( int bx = 0; bx < 4; bx++ ) {
			out_blockRGBA[by][bx].r = color[by][bx].r;
			out_blockRGBA[by][bx].g = color[by][bx].g;
			out_blockRGBA[by][bx].b = color[by][bx].b;
			out_blockRGBA[by][bx].a = alpha[by][bx];
		}
	}
}



void decompressETC2BlockRGB8A1( rgba8_t out_blockRGBA[4][4], const ETCBlockColor_t in_BLOCK ) {
	ETCMode_t mode = etcGetBlockMode( in_BLOCK, true );
	rgba8_t c0, c1, paletteRGBA[8];
	int distance, paletteIndex, paletteShift;
	uint32_t bitField = in_BLOCK.cBitField;
	
	switch ( mode ) {
		case kETC_D:
			computeRGBAColorPaletteID( paletteRGBA, in_BLOCK );
			
			for ( int bx = 0; bx < 4; bx++ ) {
				for ( int by = 0; by < 4; by++ ) {
					// if we are in the are of the second part then we use the second half of the palette
					if ( ( not in_BLOCK.flip and ( bx >> 1 ) ) or ( in_BLOCK.flip and ( by >> 1 ) ) )
						paletteShift = 4;
					else
						paletteShift = 0;
					
					paletteIndex = ( ( bitField bitand 0x1 ) bitor ( bitField >> 15 bitand 0x2 ) ) + paletteShift;
					out_blockRGBA[by][bx] = paletteRGBA[paletteIndex];
					bitField >>= 1;
				}
			}
			
			break;
			
		case kETC_T:
			computeBaseColorsT( &c0, &c1, &distance, REINTERPRET(ETCBlockT_t)in_BLOCK );
			computeColorPaletteT( paletteRGBA, c0, c1, distance, in_BLOCK.differential );
			decodeRGBABlockTH( out_blockRGBA, paletteRGBA, in_BLOCK );
			break;
			
		case kETC_H:
			computeBaseColorsH( &c0, &c1, &distance, REINTERPRET(ETCBlockH_t)in_BLOCK );
			computeColorPaletteH( paletteRGBA, c0, c1, distance, in_BLOCK.differential );
			decodeRGBABlockTH( out_blockRGBA, paletteRGBA, in_BLOCK );
			break;
			
		case kETC_P:
			decompressETC2BlockP( out_blockRGBA, in_BLOCK );
			break;
			
		case kETC_I:
		case kETC_INVALID:
			assert( false );
	}
}
