//
//  ETC_Compress_H.c
//  GLTC
//
//  Created by Michael Kwasnicki on 04.05.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#include "ETC_Compress_H.h"

#include "ETC_Compress_Common.h"
#include "ETC_Decompress.h"

#include <stdio.h>



static void swapToEncodeLSB( rgb4_t * in_out_c0, rgb4_t * in_out_c1, const int in_D ) {
	rgb8_t col8[2];
	convert444to888( &col8[0], *in_out_c0 );
	convert444to888( &col8[1], *in_out_c1 );
	
	int col0 = ( col8[0].r << 16 ) bitor ( col8[0].g << 8 ) bitor col8[0].b;
	int col1 = ( col8[1].r << 16 ) bitor ( col8[1].g << 8 ) bitor col8[1].b;
	
	if ( ( ( col0 < col1 ) and ( in_D bitand 0x1 ) ) or ( ( col0 >= col1 ) and not ( in_D bitand 0x1 ) ) ) {
		rgb4_t tmpC = *in_out_c0;
		*in_out_c0 = *in_out_c1;
		*in_out_c1 = tmpC;
	}
}



static void buildBlock( ETCBlockColor_t * out_block, const rgb4_t in_C0, const rgb4_t in_C1, const int in_D, const rgb8_t in_BLOCK_RGB[4][4], const bool in_OPAQUE ) {
	rgb4_t c0 = in_C0;
	rgb4_t c1 = in_C1;
	rgb8_t col8[2], palette[4];
	uint8_t modulation[4][4];
	swapToEncodeLSB( &c0, &c1, in_D );
	convert444to888( &col8[0], c0 );
	convert444to888( &col8[1], c1 );
	computeRGBColorPaletteH( &palette[0], col8[0], col8[1], in_D, in_OPAQUE );
	computeBlockError( modulation, in_BLOCK_RGB, palette );
	uint32_t bitField = generateBitField( modulation );
	
	ETCBlockH_t block;
	block.one = 1;
	block.r0 = c0.r;
	block.g0a = c0.g >> 1;
	block.g0b = c0.g bitand 0x1;
	block.b0a = c0.b >> 3;
	block.b0b = c0.b bitand 0x7;
	block.r1 = c1.r;
	block.g1 = c1.g;
	block.b1 = c1.b;
	block.da = in_D >> 2;
	block.db = ( in_D >> 1 ) bitand 0x1;
	
	if ( ( c0.g >> 1 ) >= 4 )
		block.dummy0 = 0x1;
	else
		block.dummy0 = 0x0;
	
	if ( ( ( ( c0.g bitand 0x1 ) << 1 ) bitor ( c0.b >> 3 ) ) + ( ( c0.b bitand 0x7 ) >> 1 ) < 4 ) {
		block.dummy1 = 0x0;
		block.dummy2 = 0x1;
	} else {
		block.dummy1 = 0x7;
		block.dummy2 = 0x0;
	}
	
	block.cBitField = bitField;
	out_block->b64 = block.b64;
}



static void computeBlockChroma( rgb8_t * out_c0, rgb8_t * out_c1, int * out_d, const rgb8_t in_BLOCK_RGB[4][4] ) {
	const rgb8_t * data = &in_BLOCK_RGB[0][0];
	int partitionMap[16];
	int maxA, maxB, cluster, counter;
	uint32_t maxDistance = 0;
	rgb8_t clusterCenter[2];
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
		rgb8_t pixel;
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
	
	*out_c0 = clusterCenter[0];
	*out_c1 = clusterCenter[1];
	*out_d = ( clusterSpread[0] + clusterSpread[1] + 1 ) >> 1;
}



static uint32_t quick( rgb4_t * out_c0, rgb4_t * out_c1, int * out_d, const rgb8_t in_BLOCK_RGB[4][4], const bool in_OPAQUE ) {
	rgb8_t col8[2], palette[4];
	rgb4_t c0, c1;
	int d;
	
	computeBlockChroma( &col8[0], &col8[1], &d, in_BLOCK_RGB );
	
	convert888to444( &c0, col8[0] );
	convert888to444( &c1, col8[1] );
	convert444to888( &col8[0], c0 );
	convert444to888( &col8[1], c1 );
	computeRGBColorPaletteH( &palette[0], col8[0], col8[1], d, in_OPAQUE );
	uint32_t error = computeBlockError( NULL, in_BLOCK_RGB, palette );
	
	*out_c0 = c0;
	*out_c1 = c1;
	*out_d = d;
	return error;
}



static uint32_t brute( rgb4_t * out_c0, rgb4_t * out_c1, int * out_d, const rgb8_t in_BLOCK_RGB[4][4], const bool in_OPAQUE ) {
	rgb8_t palette[4];
	rgb8_t center;
	rgb8_t col8[2];
	rgb4_t c0, bestC0;
	rgb4_t c1, bestC1;
	uint32_t error = 0xFFFFFFFF;
	uint32_t bestError = 0xFFFFFFFF;
	uint32_t bestDistance = 0xFFFFFFFF;
	int d, bestD;
	int distance[2], dR, dG, dB;
	int r0, g0, b0, r1, g1, b1;
	int col0, col1;
	
	computeBlockCenter( &center, in_BLOCK_RGB );
	
	for ( r0 = 0; r0 < 16; r0++ ) {
		for ( g0 = 0; g0 < 16; g0++ ) {
			for ( b0 = 0; b0 < 16; b0++ ) {
				c0 = (rgb4_t){ b0, g0, r0 };
				convert444to888( &col8[0], c0 );
				col0 = ( col8[0].r << 16 ) bitor ( col8[0].g << 8 ) bitor col8[0].b;
				dR = center.r - col8[0].r;
				dG = center.g - col8[0].g;
				dB = center.b - col8[0].b;
				distance[0] = dR * dR + dG * dG + dB * dB;
				
				for ( r1 = 0; r1 < 16; r1++ ) {
					for ( g1 = 0; g1 < 16; g1++ ) {
						for ( b1 = 0; b1 < 16; b1++ ) {
							c1 = (rgb4_t){ b1, g1, r1 };
							convert444to888( &col8[1], c1 );
							col1 = ( col8[1].r << 16 ) bitor ( col8[1].g << 8 ) bitor col8[1].b;
							
							if ( col0 >= col1 )
								continue;
														
							dR = center.r - col8[1].r;
							dG = center.g - col8[1].g;
							dB = center.b - col8[1].b;
							distance[1] = distance[0] + dR * dR + dG * dG + dB * dB;
							
							for ( d = 0; d < ETC_DISTANCE_TABLE_COUNT; d++ ) {
								computeRGBColorPaletteH( &palette[0], col8[0], col8[1], d, in_OPAQUE );
								error = computeBlockError( NULL, in_BLOCK_RGB, palette );
								
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



uint32_t compressH( ETCBlockColor_t * out_block, const rgb8_t in_BLOCK_RGB[4][4], const Strategy_t in_STRATEGY, const bool in_OPAQUE ) {
	rgb4_t c0, c1;
	int d;
	uint32_t blockError = 0xFFFFFFFF;
	
	if ( isUniformColorBlock( in_BLOCK_RGB ) ) {
		return blockError;
	} else {
		switch ( in_STRATEGY ) {
			case kFAST:
				blockError = quick( &c0, &c1, &d, in_BLOCK_RGB, in_OPAQUE );
				break;
				
			case kBEST:
				blockError = brute( &c0, &c1, &d, in_BLOCK_RGB, in_OPAQUE );
				break;
		}
	}
	
	buildBlock( out_block, c0, c1, d, in_BLOCK_RGB, in_OPAQUE );
	return blockError;
}



void printInfoH( ETCBlockColor_t * in_BLOCK ) {
	ETCBlockH_t * block = (ETCBlockH_t *)in_BLOCK;
	rgb8_t c0, c1;
	int tmpG = ( block->g0a << 1 ) bitor block->g0b;
	int tmpB = ( block->b0a << 3 ) bitor block->b0b;
	c0.r = extend4to8bits( block->r0 );
	c0.g = extend4to8bits( tmpG );
	c0.b = extend4to8bits( tmpB );
	c1.r = extend4to8bits( block->r1 );
	c1.g = extend4to8bits( block->g1 );
	c1.b = extend4to8bits( block->b1 );
	int tmpC0 = ( c0.r << 16 ) bitor ( c0.g << 8 ) bitor c0.b;
	int tmpC1 = ( c1.r << 16 ) bitor ( c1.g << 8 ) bitor c1.b;
	int distance = ( block->da << 2 ) bitor ( block->db << 1 ) bitor ( tmpC0 >= tmpC1 );
	printf( "H: (%3i %3i %3i) (%3i %3i %3i) %i\n", block->r0, tmpG, tmpB, block->r1, block->g1, block->b1, distance );
	printf( "H: (%3i %3i %3i) (%3i %3i %3i) %i\n", extend4to8bits( block->r0 ), extend4to8bits( tmpG ), extend4to8bits( tmpB ), extend4to8bits( block->r1 ), extend4to8bits( block->g1 ), extend4to8bits( block->b1 ), ETC_DISTANCE_TABLE[distance] );
}
