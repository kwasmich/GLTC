//
//  ETC_Compress_P.c
//  GLTC
//
//  Created by Michael Kwasnicki on 04.05.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#include "ETC_Compress_P.h"

#include "ETC_Compress_Common.h"
#include "ETC_Decompress.h"

#include <stdio.h>



static ETCUniformColorComposition_t ETC_UNIFORM_COLOR_LUT_6[256];
static ETCUniformColorComposition_t ETC_UNIFORM_COLOR_LUT_7[256];



static void buildBlock( ETCBlockColor_t * out_block, const rgb676_t in_CO, const rgb676_t in_CH, const rgb676_t in_CV ) {
	ETCBlockP_t block;
	block.one = 1;
	block.rO = in_CO.r;
	block.gO1 = in_CO.g >> 6;
	block.gO2 = in_CO.g bitand 0x3F;
	block.bO1 = in_CO.b >> 5;
	block.bO2 = ( in_CO.b >> 3 ) bitand 0x3;
	block.bO3 = in_CO.b bitand 0x7;
	block.rH1 = in_CH.r >> 1;
	block.rH2 = in_CH.r bitand 0x1;
	block.gH = in_CH.g;
	block.bH = in_CH.b;
	block.rV = in_CV.r;
	block.gV = in_CV.g;
	block.bV = in_CV.b;
	
	block.dummy0 = ( in_CO.r >> 1 ) bitand 0x1;
	block.dummy1 = ( in_CO.g >> 1 ) bitand 0x1;
	
	if ( ( ( in_CO.b >> 3 ) bitand 0x3 ) + ( ( in_CO.b >> 1 ) bitand 0x3 ) < 4 ) {
		block.dummy2 = 0x0;
		block.dummy3 = 0x1;
	} else {
		block.dummy2 = 0x7;
		block.dummy3 = 0x0;
	}
	
	out_block->b64 = block.b64;
}



static uint32_t uniformColor( rgb676_t * out_cO, rgb676_t * out_cH, rgb676_t * out_cV, const rgba8_t in_BLOCK_RGBA[4][4] ) {
	rgba8_t cAvg;
	rgb676_t c = RGB( 0, 0, 0 );
	
	computeBlockCenter( &cAvg, in_BLOCK_RGBA );
	
	c.r = ETC_UNIFORM_COLOR_LUT_6[cAvg.r].c;
	c.g = ETC_UNIFORM_COLOR_LUT_7[cAvg.g].c;
	c.b = ETC_UNIFORM_COLOR_LUT_6[cAvg.b].c;
	
	*out_cO = c;
	*out_cH = c;
	*out_cV = c;
	
	rgba8_t col8;
	convert676to8888( &col8, c );
	return computeBlockErrorP( col8, col8, col8, in_BLOCK_RGBA );
}



static uint32_t brute( rgb676_t * out_cO, rgb676_t * out_cH, rgb676_t * out_cV, const rgba8_t in_BLOCK_RGBA[4][4] ) {
	rgba8_t col, pixel, cO, cH, cV;
	uint32_t error = 0xFFFFFFFF;
	uint32_t bestError[3] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	rgb676_t bestCO, bestCH, bestCV;
	int dR, dG, dB;
	int r0, g0, b0, rH, gH, bH, rV, gV, bV;
	int bx, by;
		
	for ( r0 = 0; r0 < 64; r0++ ) {
		cO.r = extend6to8bits( r0 );
		
		for ( rH = 0; rH < 64; rH++ ) {
			cH.r = extend6to8bits( rH );
			
			for ( rV = 0; rV < 64; rV++ ) {
				cV.r = extend6to8bits( rV );
				error = 0;
				
				for ( by = 0; by < 4; by++ ) {
					for ( bx = 0; bx < 4; bx++ ) {
						pixel = in_BLOCK_RGBA[by][bx];
						col.r = computePlaneColor( bx, by, cO.r, cH.r, cV.r );
						dR = col.r - pixel.r;
						error += dR * dR;
					}
				}
				
				if ( error < bestError[0] ) {
					bestError[0] = error;
					bestCO.r = r0;
					bestCH.r = rH;
					bestCV.r = rV;
					
					if ( bestError[0] == 0 )
						goto earlyExitR;
				}
			}
		}
	}
	
earlyExitR:
	
	for ( g0 = 0; g0 < 128; g0++ ) {
		cO.g = extend7to8bits( g0 );
		
		for ( gH = 0; gH < 128; gH++ ) {
			cH.g = extend7to8bits( gH );
			
			for ( gV = 0; gV < 128; gV++ ) {
				cV.g = extend7to8bits( gV );
				error = 0;
				
				for ( by = 0; by < 4; by++ ) {
					for ( bx = 0; bx < 4; bx++ ) {
						pixel = in_BLOCK_RGBA[by][bx];
						col.g = computePlaneColor( bx, by, cO.g, cH.g, cV.g );
						dG = col.g - pixel.g;
						error += dG * dG;
					}
				}
				
				if ( error < bestError[1] ) {
					bestError[1] = error;
					bestCO.g = g0;
					bestCH.g = gH;
					bestCV.g = gV;
					
					if ( bestError[1] == 0 )
						goto earlyExitG;
				}
			}
		}
	}
	
earlyExitG:
	
	for ( b0 = 0; b0 < 64; b0++ ) {
		cO.b = extend6to8bits( b0 );
		
		for ( bH = 0; bH < 64; bH++ ) {
			cH.b = extend6to8bits( bH );
			
			for ( bV = 0; bV < 64; bV++ ) {
				cV.b = extend6to8bits( bV );
				error = 0;
				
				for ( by = 0; by < 4; by++ ) {
					for ( bx = 0; bx < 4; bx++ ) {
						pixel = in_BLOCK_RGBA[by][bx];
						col.b = computePlaneColor( bx, by, cO.b, cH.b, cV.b );
						dB = col.b - pixel.b;
						error += dB * dB;
					}
				}
				
				if ( error < bestError[2] ) {
					bestError[2] = error;
					bestCO.b = b0;
					bestCH.b = bH;
					bestCV.b = bV;
					
					if ( bestError[2] == 0 )
						goto earlyExitB;
				}
			}
		}
	}
	
earlyExitB:
	
	*out_cO = bestCO;
	*out_cH = bestCH;
	*out_cV = bestCV;
	return bestError[0] + bestError[1] + bestError[2];
}



static uint32_t analytical( rgb676_t * out_cO, rgb676_t * out_cH, rgb676_t * out_cV, const rgba8_t in_BLOCK_RGBA[4][4] ) {
	rgba8_t pixel, cO, cH, cV;
	rgb676_t bestCO, bestCH, bestCV;

	static const int matrix[3][16] = {
		{ 23,  17,  11,   5,  17,  11,   5,  -1,  11,   5,  -1,  -7,   5,  -1,  -7, -13 },
		{ -1,   9,  19,  29,  -7,   3,  13,  23, -13,  -3,   7,  17, -19,  -9,   1,  11 },
		{ -1,  -7, -13, -19,   9,   3,  -3,  -9,  19,  13,   7,   1,  29,  23,  17,  11 }
	};
	
	const int SCALE = 80;
	int sum0[3] = { 0, 0, 0 };
	int sumH[3] = { 0, 0, 0 };
	int sumV[3] = { 0, 0, 0 };
	int index = 0;
	
	for ( int by = 0; by < 4; by++ ) {
		for ( int bx = 0; bx < 4; bx++ ) {
			pixel = in_BLOCK_RGBA[by][bx];
			sum0[0] += matrix[0][index] * pixel.r;
			sum0[1] += matrix[0][index] * pixel.g;
			sum0[2] += matrix[0][index] * pixel.b;
			sumH[0] += matrix[1][index] * pixel.r;
			sumH[1] += matrix[1][index] * pixel.g;
			sumH[2] += matrix[1][index] * pixel.b;
			sumV[0] += matrix[2][index] * pixel.r;
			sumV[1] += matrix[2][index] * pixel.g;
			sumV[2] += matrix[2][index] * pixel.b;
			index++;
		}
	}
	
	for ( int i = 0; i < 3; i++ ) {
		sum0[i] = clampi( sum0[i] / SCALE, 0, 255 );
		sumH[i] = clampi( sumH[i] / SCALE, 0, 255 );
		sumV[i] = clampi( sumV[i] / SCALE, 0, 255 );
	}

	bestCO.r = reduce8to6bits( sum0[0] );
	bestCH.r = reduce8to6bits( sumH[0] );
	bestCV.r = reduce8to6bits( sumV[0] );
	
	bestCO.g = reduce8to7bits( sum0[1] );
	bestCH.g = reduce8to7bits( sumH[1] );
	bestCV.g = reduce8to7bits( sumV[1] );
	
	bestCO.b = reduce8to6bits( sum0[2] );
	bestCH.b = reduce8to6bits( sumH[2] );
	bestCV.b = reduce8to6bits( sumV[2] );
	
	*out_cO = bestCO;
	*out_cH = bestCH;
	*out_cV = bestCV;
	
	convert676to8888( &cO, bestCO );
	convert676to8888( &cH, bestCH );
	convert676to8888( &cV, bestCV );
	return computeBlockErrorP( cO, cH, cV, in_BLOCK_RGBA );
}



#pragma mark - exposed interface



uint32_t compressP( ETCBlockColor_t * out_block, const rgba8_t in_BLOCK_RGBA[4][4], const Strategy_t in_STRATEGY, const bool UNUSED(in_OPAQUE) ) {
	rgb676_t cO, cH, cV;
	uint32_t blockError = 0xFFFFFFFF;
	
	if ( isUniformColorBlock( in_BLOCK_RGBA ) ) {
		blockError = uniformColor( &cO, &cH, &cV, in_BLOCK_RGBA );
	} else {
		switch ( in_STRATEGY ) {
			case kFAST:
				blockError = analytical( &cO, &cH, &cV, in_BLOCK_RGBA );
				break;
				
			case kBEST:
				blockError = brute( &cO, &cH, &cV, in_BLOCK_RGBA );
				break;
		}
	}
	
	buildBlock( out_block, cO, cH, cV );
	return blockError;
}



void computeUniformColorLUTP() {
	int col8;
	ETCUniformColorComposition_t tmp = { 0, 65535 };
	
	// set everything to undefined
	for ( int c = 0; c < 256; c++ ) {
		ETC_UNIFORM_COLOR_LUT_6[c] = tmp;
		ETC_UNIFORM_COLOR_LUT_7[c] = tmp;
	}
	
	// compute all colors that can be constructed with 6bpp
	for ( int col6 = 0; col6 < 64; col6++ ) {
		col8 = extend6to8bits( col6 );
		ETC_UNIFORM_COLOR_LUT_6[col8] = (ETCUniformColorComposition_t){ col6, 0 };
	}
	
	// compute all colors that can be constructed with 7bpp
	for ( int col7 = 0; col7 < 128; col7++ ) {
		col8 = extend7to8bits( col7 );
		ETC_UNIFORM_COLOR_LUT_7[col8] = (ETCUniformColorComposition_t){ col7, 0 };
	}
	
	// fill the gaps with the nearest color
	fillUniformColorLUTGaps( ETC_UNIFORM_COLOR_LUT_6 );
	fillUniformColorLUTGaps( ETC_UNIFORM_COLOR_LUT_7 );
}



void printInfoP( ETCBlockColor_t * in_BLOCK ) {
	ETCBlockP_t * block = (ETCBlockP_t *)in_BLOCK;
	int tmpG = ( block->gO1 << 6 ) bitor block->gO2;
	int tmpB = ( block->bO1 << 5 ) bitor ( block->bO2 << 3 ) bitor block->bO3;
	int tmpR = ( block->rH1 << 1 ) bitor block->rH2;
	printf( "P: (%3i %3i %3i) (%3i %3i %3i) (%3i %3i %3i)\n", block->rO, tmpG, tmpB, tmpR, block->gH, block->bH, block->rV, block->gV, block->bV );
	printf( "P: (%3i %3i %3i) (%3i %3i %3i) (%3i %3i %3i)\n", extend6to8bits( block->rO ), extend7to8bits( tmpG ), extend6to8bits( tmpB ), extend6to8bits( tmpR ), extend7to8bits( block->gH ), extend6to8bits( block->bH ), extend6to8bits( block->rV ), extend7to8bits( block->gV ), extend6to8bits( block->bV ) );
}
