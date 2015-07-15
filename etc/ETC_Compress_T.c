//
//  ETC_Compress_T.c
//  GLTC
//
//  Created by Michael Kwasnicki on 04.05.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#include "ETC_Compress_T.h"

#include "ETC_Compress_Common.h"
#include "ETC_Decompress.h"

#include <stdio.h>



static ETCUniformColorComposition_t ETC_UNIFORM_COLOR_LUT[ETC_DISTANCE_TABLE_COUNT][ETC_PALETTE_SIZE][256];
static ETCUniformColorComposition_t ETC_UNIFORM_COLOR_LUT_NON_OPAQUE[ETC_DISTANCE_TABLE_COUNT][ETC_PALETTE_SIZE][256];



static void buildBlock( ETCBlockColor_t * out_block, const rgb4_t in_C0, const rgb4_t in_C1, const int in_D, const rgba8_t in_BLOCK_RGBA[4][4], const bool in_OPAQUE ) {
	rgba8_t col8[2], palette[4];
	uint8_t modulation[4][4];
	convert444to8888( &col8[0], in_C0 );
	convert444to8888( &col8[1], in_C1 );
	computeColorPaletteT( &palette[0], col8[0], col8[1], in_D, in_OPAQUE );
	computeBlockError( modulation, in_BLOCK_RGBA, palette, in_OPAQUE );
	uint32_t bitField = generateBitField( modulation );
	
	ETCBlockT_t block;
	block.opaque = in_OPAQUE;
	block.r0a = in_C0.r >> 2;
	block.r0b = in_C0.r bitand 0x3;
	block.g0 = in_C0.g;
	block.b0 = in_C0.b;
	block.r1 = in_C1.r;
	block.g1 = in_C1.g;
	block.b1 = in_C1.b;
	block.da = in_D >> 1;
	block.db = in_D bitand 0x1;
	
	if ( ( in_C0.r >> 2 ) + ( in_C0.r bitand 0x3 ) < 4 ) {
		block.dummy0 = 0x0;
		block.dummy1 = 0x1;
	} else {
		block.dummy0 = 0x7;
		block.dummy1 = 0x0;
	}
	
	block.cBitField = bitField;
	out_block->b64 = block.b64;
}



static uint32_t uniformColor( rgb4_t * out_c0, rgb4_t * out_c1, int * out_d, const rgba8_t in_BLOCK_RGBA[4][4], const bool in_OPAQUE ) {
	rgba8_t col8;
	rgb4_t col4 = RGB( 0, 0, 0 );
	uint32_t error = 0;
	uint32_t bestError = 0xFFFFFFFF;
	uint32_t bestT = 0;
	rgb4_t bestC = RGB( 0, 0, 0 );
	ETCUniformColorComposition_t (* lut)[ETC_PALETTE_SIZE][256] = ( in_OPAQUE ) ? &ETC_UNIFORM_COLOR_LUT[0] : &ETC_UNIFORM_COLOR_LUT_NON_OPAQUE[0];
	
	computeBlockCenter( &col8, in_BLOCK_RGBA );
	
	for ( int t = 0; t < ETC_DISTANCE_TABLE_COUNT; t++ ) {
		for ( int p = 0; p < ETC_PALETTE_SIZE; p++ ) {
			error = 0;
			error += lut[t][p][col8.r].error;
			error += lut[t][p][col8.g].error;
			error += lut[t][p][col8.b].error;
			col4.r = lut[t][p][col8.r].c;
			col4.g = lut[t][p][col8.g].c;
			col4.b = lut[t][p][col8.b].c;
			
			if ( error < bestError ) {
				bestError = error;
				bestC = col4;
				bestT = t;
			}
			
			if ( bestError == 0 )
				goto earlyExit;
		}
	}
	
earlyExit:
	
	*out_c0 = bestC;
	*out_c1 = bestC;
	*out_d = bestT;
	return bestError * 16;
}



static void computeBlockChroma( rgba8_t * out_c0, rgba8_t * out_c1, int * out_d, const rgba8_t in_BLOCK_RGBA[4][4] ) {
	const rgba8_t * data = &in_BLOCK_RGBA[0][0];
	int partitionMap[16];
	int maxA, maxB, cluster, counter;
	uint32_t maxDistance = 0;
	rgba8_t clusterCenter[2];
	int32_t dR, dG, dB;
	int32_t sumR, sumG, sumB;
	
	for ( int a = 0; a < 16; a++ ) {
		partitionMap[a] = -1;
		
		for ( int b = a + 1; b < 16; b++ ) {
			dR = data[a].r - data[b].r;
			dG = data[a].g - data[b].g;
			dB = data[a].b - data[b].b;
			uint32_t distance = dR * dR + dG * dG + dB * dB;
			
			if ( distance > maxDistance ) {
				maxDistance = distance;
				maxA = a;
				maxB = b;
			}
		}
	}

	partitionMap[maxA] = 0;
	partitionMap[maxB] = 1;
	clusterCenter[0] = data[maxA];
	clusterCenter[1] = data[maxB];
	
	for ( int i = 0; i < 16; i++ ) {
		if ( partitionMap[i] == -1 ) {
			dR = data[i].r - clusterCenter[0].r;
			dG = data[i].g - clusterCenter[0].g;
			dB = data[i].b - clusterCenter[0].b;
			uint32_t distanceA = dR * dR + dG * dG + dB * dB;
			
			dR = data[i].r - clusterCenter[1].r;
			dG = data[i].g - clusterCenter[1].g;
			dB = data[i].b - clusterCenter[1].b;
			uint32_t distanceB = dR * dR + dG * dG + dB * dB;
			
			cluster = ( distanceA < distanceB ) ? 0 : 1;
			partitionMap[i] = cluster;
			
			counter = 0;
			sumR = 0;
			sumG = 0;
			sumB = 0;
			
			for ( int j = 0; j < 16; j++ ) {
				if ( partitionMap[j] == cluster ) {
					sumR += data[j].r;
					sumG += data[j].g;
					sumB += data[j].b;
					counter++;
				}
			}
			
			clusterCenter[cluster].r = sumR / counter;
			clusterCenter[cluster].g = sumG / counter;
			clusterCenter[cluster].b = sumB / counter;
		}
	}
	
	int clusterSpread[2];

	for ( int c = 0; c < 2; c++ ) {
		rgba8_t pixel;
		uint32_t dot;
		uint32_t min = 0xFFFFFFFF;
		uint32_t max = 0;
		
		for ( int i = 0; i < 16; i++ ) {
			if ( partitionMap[i] == c ) {
				pixel = data[i];
				
				dot = pixel.r + pixel.g + pixel.b;
				min = ( dot < min ) ? dot : min;
				max = ( dot > max ) ? dot : max;
			}
		}
		
		int halfWidth = ( max - min ) / 6; // intentionally round twards zero
		int d0 = 0;
		
		for ( int d = 0; d < ETC_DISTANCE_TABLE_COUNT; d++ ) {
			if ( ETC_DISTANCE_TABLE[d] < halfWidth )
				d0 = d;
		}
		
		clusterSpread[c] = d0;
	}

	cluster = ( clusterSpread[0] > clusterSpread[1] ) ? 0 : 1;

	*out_c0 = clusterCenter[1-cluster];
	*out_c1 = clusterCenter[cluster];
	*out_d = clusterSpread[cluster];
}



static uint32_t quick( rgb4_t * out_c0, rgb4_t * out_c1, int * out_d, const rgba8_t in_BLOCK_RGBA[4][4], const bool in_OPAQUE ) {
	rgba8_t col8[2], palette[4];
	rgb4_t c0, c1;
	int d;
	
	computeBlockChroma( &col8[0], &col8[1], &d, in_BLOCK_RGBA );
	
	convert8888to444( &c0, col8[0] );
	convert8888to444( &c1, col8[1] );
	convert444to8888( &col8[0], c0 );
	convert444to8888( &col8[1], c1 );
	computeColorPaletteT( &palette[0], col8[0], col8[1], d, in_OPAQUE );
	uint32_t error = computeBlockError( NULL, in_BLOCK_RGBA, palette, in_OPAQUE );
	
	*out_c0 = c0;
	*out_c1 = c1;
	*out_d = d;
	return error;
}



static uint32_t brute( rgb4_t * out_c0, rgb4_t * out_c1, int * out_d, const rgba8_t in_BLOCK_RGBA[4][4], const bool in_OPAQUE ) {
	rgba8_t col8[2], center, palette[4];
	rgb4_t c0, bestC0;
	rgb4_t c1, bestC1;
	uint32_t error = 0xFFFFFFFF;
	uint32_t bestError = 0xFFFFFFFF;
	uint32_t bestDistance = 0xFFFFFFFF;
	int d, bestD;
	int distance[2], dR, dG, dB;
	int r0, g0, b0, r1, g1, b1;
	
	computeBlockCenter( &center, in_BLOCK_RGBA );
	
	for ( r0 = 0; r0 < 16; r0++ ) {
		for ( g0 = 0; g0 < 16; g0++ ) {
			for ( b0 = 0; b0 < 16; b0++ ) {
				c0 = (rgb4_t)RGB( r0, g0, b0 );
				convert444to8888( &col8[0], c0 );
				dR = center.r - col8[0].r;
				dG = center.g - col8[0].g;
				dB = center.b - col8[0].b;
				distance[0] = dR * dR + dG * dG + dB * dB;
				
				for ( r1 = 0; r1 < 16; r1++ ) {
					for ( g1 = 0; g1 < 16; g1++ ) {
						for ( b1 = 0; b1 < 16; b1++ ) {
							c1 = (rgb4_t)RGB( r1, g1, b1 );
							convert444to8888( &col8[1], c1 );
							dR = center.r - col8[1].r;
							dG = center.g - col8[1].g;
							dB = center.b - col8[1].b;
							distance[1] = distance[0] + dR * dR + dG * dG + dB * dB;
							
							for ( d = 0; d < ETC_DISTANCE_TABLE_COUNT; d++ ) {
								computeColorPaletteT( &palette[0], col8[0], col8[1], d, in_OPAQUE );
								error = computeBlockError( NULL, in_BLOCK_RGBA, palette, in_OPAQUE );
								
								if ( ( error < bestError ) or ( ( error == bestError ) and ( distance[1] < bestDistance ) ) ) {
									bestError = error;
									bestDistance = distance[1];
									bestC0 = c0;
									bestC1 = c1;
									bestD = d;
								}
								
								if ( ( bestError == 0 ) and ( bestDistance == 0) )
									goto earlyExit;
							}
						}
					}
				}
			}
		}
	}
	
earlyExit:
	
	*out_c0 = bestC0;
	*out_c1 = bestC1;
	*out_d = bestD;
	return bestError;
}



#pragma mark - exposed interface



uint32_t compressT( ETCBlockColor_t * out_block, const rgba8_t in_BLOCK_RGBA[4][4], const Strategy_t in_STRATEGY, const bool in_OPAQUE ) {
	rgb4_t c0, c1;
	int d;
	uint32_t blockError = 0xFFFFFFFF;
	
	if ( isUniformColorBlock( in_BLOCK_RGBA ) ) {
		blockError = uniformColor( &c0, &c1, &d, in_BLOCK_RGBA, in_OPAQUE );
	} else {
		switch ( in_STRATEGY ) {
			case kFAST:
				blockError = quick( &c0, &c1, &d, in_BLOCK_RGBA, in_OPAQUE );
				break;
				
			case kBEST:
				blockError = brute( &c0, &c1, &d, in_BLOCK_RGBA, in_OPAQUE );
				break;
		}
	}
	
	buildBlock( out_block, c0, c1, d, in_BLOCK_RGBA, in_OPAQUE );
	return blockError;
}



void computeUniformColorLUTT() {
	ETCUniformColorComposition_t tmp = { 0, 65535 };
	
	// set everything to undefined
	for ( int t = 0; t < ETC_DISTANCE_TABLE_COUNT; t++ ) {
		for ( int p = 0; p < ETC_PALETTE_SIZE; p++ ) {
			for ( int c = 0; c < 256; c++ ) {
				ETC_UNIFORM_COLOR_LUT[t][p][c] = tmp;
				ETC_UNIFORM_COLOR_LUT_NON_OPAQUE[t][p][c] = tmp;
			}
		}
	}
	
	// compute all colors that can be constructed with 4bpp
	for ( int col4 = 0; col4 < 16; col4++ ) {
		int col8 = extend4to8bits( col4 );
		
		for ( int t = 0; t < ETC_DISTANCE_TABLE_COUNT; t++ ) {
			int col = col8;
			ETC_UNIFORM_COLOR_LUT[t][0][col] = (ETCUniformColorComposition_t){ col4, 0 };
			ETC_UNIFORM_COLOR_LUT[t][2][col] = (ETCUniformColorComposition_t){ col4, 0 };
			col = clampi( col8 + ETC_DISTANCE_TABLE[t], 0, 255 );
			ETC_UNIFORM_COLOR_LUT[t][1][col] = (ETCUniformColorComposition_t){ col4, 0 };
			col = clampi( col8 - ETC_DISTANCE_TABLE[t], 0, 255 );
			ETC_UNIFORM_COLOR_LUT[t][3][col] = (ETCUniformColorComposition_t){ col4, 0 };
			
			col = col8;
			ETC_UNIFORM_COLOR_LUT_NON_OPAQUE[t][0][col] = (ETCUniformColorComposition_t){ col4, 0 };
			col = clampi( col8 + ETC_DISTANCE_TABLE[t], 0, 255 );
			ETC_UNIFORM_COLOR_LUT_NON_OPAQUE[t][1][col] = (ETCUniformColorComposition_t){ col4, 0 };
			col = clampi( col8 - ETC_DISTANCE_TABLE[t], 0, 255 );
			ETC_UNIFORM_COLOR_LUT_NON_OPAQUE[t][3][col] = (ETCUniformColorComposition_t){ col4, 0 };
		}
	}
	
	// fill the gaps with the nearest color
	fillUniformColorPaletteLUTGaps( ETC_UNIFORM_COLOR_LUT );
	fillUniformColorPaletteLUTGaps( ETC_UNIFORM_COLOR_LUT_NON_OPAQUE );
}



void printInfoT( ETCBlockColor_t * in_BLOCK ) {
	ETCBlockT_t * block = (ETCBlockT_t *)in_BLOCK;
	int tmpR = ( block->r0a << 2 ) bitor block->r0b;
	int distance = ( block->da << 1 ) bitor block->db;
	printf( "T: (%3i %3i %3i) (%3i %3i %3i) %i\n", tmpR, block->g0, block->b0, block->r1, block->g1, block->b1, distance );
	printf( "T: (%3i %3i %3i) (%3i %3i %3i) %i\n", extend4to8bits( tmpR ), extend4to8bits( block->g0 ), extend4to8bits( block->b0 ), extend4to8bits( block->r1 ), extend4to8bits( block->g1 ), extend4to8bits( block->b1 ), ETC_DISTANCE_TABLE[distance] );
}
