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



static void computeBaseColorsDifferential( rgb8_t * out_c0, rgb8_t * out_c1, const rgb5_t in_C0, const rgb3_t in_C1 ) {
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



void computeBaseColorsID( rgb8_t * out_c0, rgb8_t * out_c1, const ETCBlockColor_t in_BLOCK ) {
	if ( in_BLOCK.differential ) {
		ETCBlockD_t block = REINTERPRET(ETCBlockD_t)in_BLOCK;
		rgb5_t bc0 = { block.b, block.g, block.r };
		rgb3_t bc1 = { block.dB, block.dG, block.dR };
		computeBaseColorsDifferential( out_c0, out_c1, bc0, bc1 );
	} else {
		ETCBlockI_t block = REINTERPRET(ETCBlockI_t)in_BLOCK;
		rgb4_t bc0 = { block.b0, block.g0, block.r0 };
		rgb4_t bc1 = { block.b1, block.g1, block.r1 };
		convert444to888( out_c0, bc0 );
		convert444to888( out_c1, bc1 );
	}
}



void computeRGBColorPaletteCommonID( rgb8_t out_colorPalette[4], const rgb8_t in_C, const int in_TABLE_INDEX, const int in_TABLE[8][4] ) {
	const int *LUT = in_TABLE[in_TABLE_INDEX];
	
	for ( int i = 0; i < 4; i++ ) {
		rgb8_t col = in_C;
		int luminance = LUT[i];
		col.r = clampi( col.r + luminance, 0, 255 );
		col.g = clampi( col.g + luminance, 0, 255 );
		col.b = clampi( col.b + luminance, 0, 255 );
		out_colorPalette[i] = col;
	}
}



static void computeRGBAColorPaletteCommonID( rgba8_t out_colorPalette[8], const rgb8_t in_C0, const rgb8_t in_C1, const int in_TABLE_INDEX[2], const bool in_OPAQUE ) {
	rgb8_t palette[8];
	
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



static void computeRGBColorPaletteID( rgb8_t out_colorPalette[8], const ETCBlockColor_t in_BLOCK ) {
	rgb8_t c0;
	rgb8_t c1;
	computeBaseColorsID( &c0, &c1, in_BLOCK );
	computeRGBColorPaletteCommonID( &out_colorPalette[0], c0, in_BLOCK.table0, ETC_MODIFIER_TABLE );
	computeRGBColorPaletteCommonID( &out_colorPalette[4], c1, in_BLOCK.table1, ETC_MODIFIER_TABLE );
}



static void computeRGBAColorPaletteID( rgba8_t out_colorPalette[8], const ETCBlockColor_t in_BLOCK ) {
	rgb8_t c0;
	rgb8_t c1;
	int tableIndex[2] = { in_BLOCK.table0, in_BLOCK.table1 };
	bool opaque = in_BLOCK.differential;
	ETCBlockColor_t blockCopy = in_BLOCK;
	
	if ( not opaque )
		blockCopy.differential = true;
	
	computeBaseColorsID( &c0, &c1, blockCopy );
	computeRGBAColorPaletteCommonID( out_colorPalette, c0, c1, tableIndex, opaque );
}



void computeRGBColorPaletteT( rgb8_t out_colorPalette[4], const rgb8_t in_C0, const rgb8_t in_C1, const int in_DISTNACE ) {
	int dist = ETC_DISTANCE_TABLE[in_DISTNACE];
	out_colorPalette[0] = in_C0;
	out_colorPalette[1].r = clampi( in_C1.r + dist, 0, 255 );
	out_colorPalette[1].g = clampi( in_C1.g + dist, 0, 255 );
	out_colorPalette[1].b = clampi( in_C1.b + dist, 0, 255 );
	out_colorPalette[2] = in_C1;
	out_colorPalette[3].r = clampi( in_C1.r - dist, 0, 255 );
	out_colorPalette[3].g = clampi( in_C1.g - dist, 0, 255 );
	out_colorPalette[3].b = clampi( in_C1.b - dist, 0, 255 );
}



void computeRGBColorPaletteH( rgb8_t out_colorPalette[4], const rgb8_t in_C0, const rgb8_t in_C1, const int in_DISTNACE ) {
	int dist = ETC_DISTANCE_TABLE[in_DISTNACE];
	out_colorPalette[0].r = clampi( in_C0.r + dist, 0, 255 );
	out_colorPalette[0].g = clampi( in_C0.g + dist, 0, 255 );
	out_colorPalette[0].b = clampi( in_C0.b + dist, 0, 255 );
	out_colorPalette[1].r = clampi( in_C0.r - dist, 0, 255 );
	out_colorPalette[1].g = clampi( in_C0.g - dist, 0, 255 );
	out_colorPalette[1].b = clampi( in_C0.b - dist, 0, 255 );
	out_colorPalette[2].r = clampi( in_C1.r + dist, 0, 255 );
	out_colorPalette[2].g = clampi( in_C1.g + dist, 0, 255 );
	out_colorPalette[2].b = clampi( in_C1.b + dist, 0, 255 );
	out_colorPalette[3].r = clampi( in_C1.r - dist, 0, 255 );
	out_colorPalette[3].g = clampi( in_C1.g - dist, 0, 255 );
	out_colorPalette[3].b = clampi( in_C1.b - dist, 0, 255 );
}



void computeRGBColorPaletteTHP( rgb8_t out_colorPalette[4], const ETCBlockColor_t in_BLOCK, const ETCMode_t in_MODE ) {
	rgb8_t c0;
	rgb8_t c1;
	
	// get the base colors of the sub blocks
	if ( in_MODE == kETC_T ) {
		ETCBlockT_t block = REINTERPRET(ETCBlockT_t)in_BLOCK;
		int tmpR = ( block.r0a << 2 ) bitor block.r0b;
		int distance = ( block.da << 1 ) bitor block.db;
		c0.r = extend4to8bits( tmpR );
		c0.g = extend4to8bits( block.g0 );
		c0.b = extend4to8bits( block.b0 );
		c1.r = extend4to8bits( block.r1 );
		c1.g = extend4to8bits( block.g1 );
		c1.b = extend4to8bits( block.b1 );
		computeRGBColorPaletteT( out_colorPalette, c0, c1, distance );
	} else if ( in_MODE == kETC_H ) {
		ETCBlockH_t block = REINTERPRET(ETCBlockH_t)in_BLOCK;
		int tmpG = ( block.g0a << 1 ) bitor block.g0b;
		int tmpB = ( block.b0a << 3 ) bitor block.b0b;
		c0.r = extend4to8bits( block.r0 );
		c0.g = extend4to8bits( tmpG );
		c0.b = extend4to8bits( tmpB );
		c1.r = extend4to8bits( block.r1 );
		c1.g = extend4to8bits( block.g1 );
		c1.b = extend4to8bits( block.b1 );
		int tmpC0 = ( c0.r << 16 ) bitor ( c0.g << 8 ) bitor c0.b;
		int tmpC1 = ( c1.r << 16 ) bitor ( c1.g << 8 ) bitor c1.b;
		int distance = ( block.da << 2 ) bitor ( block.db << 1 ) bitor ( tmpC0 >= tmpC1 );
		computeRGBColorPaletteH( out_colorPalette, c0, c1, distance );
	} else if ( in_MODE == kETC_P ) {
		ETCBlockP_t block = REINTERPRET(ETCBlockP_t)in_BLOCK;
		int tmpG = ( block.g01 << 6 ) bitor block.g02;
		int tmpB = ( block.b01 << 5 ) bitor ( block.b02 << 3 ) bitor block.b03;
		int tmpR = ( block.rH1 << 1 ) bitor block.rH2;
		out_colorPalette[0].r = extend6to8bits( block.r0 );
		out_colorPalette[0].g = extend7to8bits( tmpG );
		out_colorPalette[0].b = extend6to8bits( tmpB );
		out_colorPalette[1].r = extend6to8bits( tmpR );
		out_colorPalette[1].g = extend7to8bits( block.gH );
		out_colorPalette[1].b = extend6to8bits( block.bH );
		out_colorPalette[2].r = extend6to8bits( block.rV );
		out_colorPalette[2].g = extend7to8bits( block.gV );
		out_colorPalette[2].b = extend6to8bits( block.bV );
	}
}



static void computeRGBAColorPaletteTHP( rgba8_t out_colorPalette[4], const ETCBlockColor_t in_BLOCK, const ETCMode_t in_MODE ) {
	assert( in_MODE != kETC_P );
	rgb8_t palette[4];
	bool opaque = in_BLOCK.differential or (in_MODE == kETC_P);
	computeRGBColorPaletteTHP( palette, in_BLOCK, in_MODE );
	
	for ( int p = 0; p < 4; p++ ) {
		bool punchThrough = ( not opaque ) and ( p == 0x2 );
		
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



void decompressETC1BlockRGB( rgb8_t out_blockRGB[4][4], const ETCBlockColor_t in_BLOCK ) {
	rgb8_t palette[8];
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
			out_blockRGB[by][bx] = palette[paletteIndex];
			bitField >>= 1;
		}
	}
}



void decompressETC2BlockRGB( rgb8_t out_blockRGB[4][4], const ETCBlockColor_t in_BLOCK ) {
	ETCMode_t mode = etcGetBlockMode( in_BLOCK, false );
	rgb8_t palette[8];
	rgb8_t col = {{ 0, 0, 0 }};
	int paletteIndex;
	uint32_t bitField = in_BLOCK.cBitField;
	
	switch ( mode ) {
		case kETC_I:
		case kETC_D:
			decompressETC1BlockRGB( out_blockRGB, in_BLOCK );
			break;
			
		case kETC_T:
		case kETC_H:
			computeRGBColorPaletteTHP( palette, in_BLOCK, mode );
			
			for ( int bx = 0; bx < 4; bx++ ) {
				for ( int by = 0; by < 4; by++ ) {
					paletteIndex = ( bitField bitand 0x1 ) bitor ( bitField >> 15 bitand 0x2 );
					out_blockRGB[by][bx] = palette[paletteIndex];
					bitField >>= 1;
				}
			}
			
			break;
			
		case kETC_P:
			computeRGBColorPaletteTHP( palette, in_BLOCK, mode );
			
			for ( int by = 0; by < 4; by++ ) {
				for ( int bx = 0; bx < 4; bx++ ) {
					col.r = computePlaneColor( bx, by, palette[0].r, palette[1].r, palette[2].r );
					col.g = computePlaneColor( bx, by, palette[0].g, palette[1].g, palette[2].g );
					col.b = computePlaneColor( bx, by, palette[0].b, palette[1].b, palette[2].b );
					out_blockRGB[by][bx] = col;
				}
			}
			
			break;
			
		case kETC_INVALID:
		default:
			assert( false );
	}
}



void decompressETC2BlockRGBA8( rgba8_t out_blockRGBA[4][4], const ETC2BlockRGBA_t in_BLOCK ) {
	rgb8_t color[4][4];
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
	rgba8_t palette[8];
	rgb8_t palette2[3];
	rgba8_t col = {{ 0, 0, 0, 255 }};
	int paletteIndex, paletteShift;
	uint32_t bitField = in_BLOCK.cBitField;
	
	switch ( mode ) {
		case kETC_D:
			computeRGBAColorPaletteID( palette, in_BLOCK );
			
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
			
			break;
			
		case kETC_T:
		case kETC_H:
			computeRGBAColorPaletteTHP( palette, in_BLOCK, mode );
			
			for ( int bx = 0; bx < 4; bx++ ) {
				for ( int by = 0; by < 4; by++ ) {
					paletteIndex = ( bitField bitand 0x1 ) bitor ( bitField >> 15 bitand 0x2 );
					out_blockRGBA[by][bx] = palette[paletteIndex];
					bitField >>= 1;
				}
			}
			
			break;
			
		case kETC_P:
			computeRGBColorPaletteTHP( palette2, in_BLOCK, mode );
			
			for ( int by = 0; by < 4; by++ ) {
				for ( int bx = 0; bx < 4; bx++ ) {
					col.r = computePlaneColor( bx, by, palette2[0].r, palette2[1].r, palette2[2].r );
					col.g = computePlaneColor( bx, by, palette2[0].g, palette2[1].g, palette2[2].g );
					col.b = computePlaneColor( bx, by, palette2[0].b, palette2[1].b, palette2[2].b );
					out_blockRGBA[by][bx] = col;
				}
			}
			
			break;
			
		case kETC_I:
		case kETC_INVALID:
			assert( false );
	}
}
