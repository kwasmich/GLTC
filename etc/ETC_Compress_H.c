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

#include <assert.h>
#include <iso646.h>
#include <stdio.h>



static void buildBlock( ETCBlockColor_t * out_block, const rgb4_t in_C0, const rgb4_t in_C1, const int in_D, const uint8_t in_MODULATION[4][4] ) {
	uint32_t bitField = generateBitField( in_MODULATION );
	
	ETCBlockH_t block;
	block.one = 1;
	block.r0 = in_C0.r;
	block.g0a = in_C0.g >> 1;
	block.g0b = in_C0.g bitand 0x1;
	block.b0a = in_C0.b >> 3;
	block.b0b = in_C0.b bitand 0x7;
	block.r1 = in_C1.r;
	block.g1 = in_C1.g;
	block.b1 = in_C1.b;
	block.da = in_D >> 2;
	block.db = ( in_D >> 1 ) bitand 0x1;
	
	if ( ( in_C0.g >> 1 ) >= 4 )
		block.dummy0 = 0x1;
	else
		block.dummy0 = 0x0;
	
	if ( ( ( ( in_C0.g bitand 0x1 ) << 1 ) bitor ( in_C0.b >> 3 ) ) + ( ( in_C0.b bitand 0x7 ) >> 1 ) < 4 ) {
		block.dummy1 = 0x0;
		block.dummy2 = 0x1;
	} else {
		block.dummy1 = 0x7;
		block.dummy2 = 0x0;
	}
	
	block.cBitField = bitField;
	out_block->b64 = block.b64;
}



struct edge_t {
	uint32_t dist;
	uint8_t a;
	uint8_t b;
};



static int edgeSort( const void * in_A, const void * in_B ) {
	struct edge_t a = *(struct edge_t*)in_A;
	struct edge_t b = *(struct edge_t*)in_B;
	return a.dist - b.dist;
}



// Uses the minimum spanning tree algorithm to form cluster until there are two clusters.
// Use the average of a cluster as color and use the average width of both clusters as the width.
static void computeBlockChroma( rgb8_t * out_c0, rgb8_t * out_c1, int * out_d1, const rgb8_t in_BLOCK_RGB[4][4] ) {
	rgb8_t pixel;
	int clusterPartition[16];
	int clusterSize[16];
	struct edge_t edge[120];
	const rgb8_t * data = &in_BLOCK_RGB[0][0];
	int dX, dY, dZ;
	int maxClusterSize = 0;
	
	for ( int i = 0; i < 16; i++ ) {
		clusterPartition[i] = i;
		clusterSize[i] = 1;
	}
	
	int k = 0;
	
	for ( int i = 0; i < 16; i++ ) {
		for ( int j = i + 1; j < 16; j++ ) {
			dX = data[i].r - data[j].r;
			dY = data[i].g - data[j].g;
			dZ = data[i].b - data[j].b;
			edge[k].a = i;
			edge[k].b = j;
			edge[k].dist = dX * dX + dY * dY + dZ * dZ;
			k++;
		}
	}
	
	mergesort( edge, 120, sizeof( struct edge_t ), edgeSort );
	
	assert( false );
	
	for ( int i = 0; i < 120; i++ ) {
		int old = clusterPartition[edge[i].b];
		int new = clusterPartition[edge[i].a];
		
		if ( old == new )
			continue;
		
		for ( int j = 0; j < 16; j++ ) {
			if ( ( clusterPartition[j] == old ) and ( edge[i].a != j ) ) {
				clusterPartition[j] = new;
				clusterSize[clusterPartition[j]] += clusterSize[j];
				clusterSize[j] = 0;
			}
		}
		
		bool done = true;
		
		for ( int j = 0; j < 16; j++ ) {
			if ( clusterSize[j] == 1 ) {
				done = false;
				break;
			}
		}
		
		if ( done )
			break;
	}
	
	for ( int i = 0; i < 16; i++ ) {
		if ( maxClusterSize < clusterSize[i] )
			maxClusterSize = clusterSize[i];
	}
	
	for ( int i = 0; i < 16; i++ ) {
		if ( maxClusterSize == clusterSize[i] ) {
			int cluster = clusterPartition[i];
			int c0[3] = { 0, 0, 0 };
			int c1[3] = { 0, 0, 0 };
			int dot0, dot1;
			int min0 = 255;
			int max0 = 0;
			int min1 = 255;
			int max1 = 0;
			
			for ( int j = 0; j < 16; j++ ) {
				if ( clusterPartition[j] == cluster ) {
					c0[0] += data[j].r;
					c0[1] += data[j].g;
					c0[2] += data[j].b;
					
					dot0 = data[j].r + data[j].g + data[j].b;
					min0 = ( dot0 < min0 ) ? dot0 : min0;
					max0 = ( dot0 > max0 ) ? dot0 : max0;
				} else {
					c1[0] += data[j].r;
					c1[1] += data[j].g;
					c1[2] += data[j].b;
					
					dot1 = data[j].r + data[j].g + data[j].b;
					min1 = ( dot1 < min1 ) ? dot1 : min1;
					max1 = ( dot1 > max1 ) ? dot1 : max1;
				}
			}
			
			c0[0] /= maxClusterSize;
			c0[1] /= maxClusterSize;
			c0[2] /= maxClusterSize;
			
			if ( maxClusterSize < 16 ) {
				c1[0] /= 16 - maxClusterSize;
				c1[1] /= 16 - maxClusterSize;
				c1[2] /= 16 - maxClusterSize;
			} else {
				c1[0] = c0[0];
				c1[1] = c0[1];
				c1[2] = c0[2];
				min1 = min0;
				max1 = max0;
			}
			
			int halfWidth = ( max1 - min1 ) / 6; // intentionally round twards zero
			
			//printf( "C%2i %3i %3i %3i - %3i %3i %3i (%i)\n", i, c0[0], c0[1], c0[2], c1[0], c1[1], c1[2], halfWidth );
			out_c0->r = c0[0];
			out_c0->g = c0[1];
			out_c0->b = c0[2];
			out_c1->r = c1[0];
			out_c1->g = c1[1];
			out_c1->b = c1[2];
			*out_d1 = 0;
			
			for ( int d = 0; d < ETC_DISTANCE_TABLE_COUNT; d++ ) {
				if ( ETC_DISTANCE_TABLE[d] < halfWidth )
					*out_d1 = d;
			}
			
			return;
		}
	}
}



static uint32_t quick( rgb4_t * out_c0, rgb4_t * out_c1, int * out_d, uint8_t out_modulation[4][4], const rgb8_t in_BLOCK_RGB[4][4] ) {
	rgb8_t col8[2], palette[4];
	rgb4_t c0, c1;
	int d1;
	
	computeBlockChroma( &col8[0], &col8[1], &d1, in_BLOCK_RGB );
	
	convert888to444( &c0, col8[0] );
	convert888to444( &c1, col8[1] );
	convert444to888( &col8[0], c0 );
	convert444to888( &col8[1], c1 );
	computeRGBColorPaletteT( &palette[0], col8[0], col8[1], d1 );
	
	//printf( "%3i %3i %3i - %3i %3i %3i\n", col8[0].r, col8[0].g, col8[0].b, col8[1].r, col8[1].g, col8[1].b );
	
	*out_c0 = c0;
	*out_c1 = c1;
	*out_d = d1;
	
	return computeBlockError( out_modulation, in_BLOCK_RGB, palette );
}



static uint32_t brute( rgb4_t * out_c0, rgb4_t * out_c1, int * out_d, uint8_t out_modulation[4][4], const rgb8_t in_BLOCK_RGB[4][4] ) {
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
	
	computeBlockCenter( &center, in_BLOCK_RGB );
	
	for ( b0 = 0; b0 < 16; b0++ ) {
		for ( g0 = 0; g0 < 16; g0++ ) {
			for ( r0 = 0; r0 < 16; r0++ ) {
				c0 = (rgb4_t){ b0, g0, r0 };
				convert444to888( &col8[0], c0 );
				dR = center.r - col8[0].r;
				dG = center.g - col8[0].g;
				dB = center.b - col8[0].b;
				distance[0] = dR * dR + dG * dG + dB * dB;
				
				for ( d = 0; d < ETC_DISTANCE_TABLE_COUNT; d++ ) {
					for ( b1 = 0; b1 < 16; b1++ ) {
						for ( g1 = 0; g1 < 16; g1++ ) {
							for ( r1 = 0; r1 < 16; r1++ ) {
								c1 = (rgb4_t){ b1, g1, r1 };
								convert444to888( &col8[1], c1 );
								computeRGBColorPaletteH( &palette[0], col8[0], col8[1], d );
								error = computeBlockError( NULL, in_BLOCK_RGB, palette );
								dR = center.r - col8[1].r;
								dG = center.g - col8[1].g;
								dB = center.b - col8[1].b;
								distance[1] = distance[0] + dR * dR + dG * dG + dB * dB;
								
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
	
	convert444to888( &col8[0], bestC0 );
	convert444to888( &col8[1], bestC1 );
	
	int col0 = ( col8[0].r << 16 ) bitor ( col8[0].g << 8 ) bitor col8[0].b;
	int col1 = ( col8[1].r << 16 ) bitor ( col8[1].g << 8 ) bitor col8[1].b;
	
	if ( ( ( col0 < col1 ) and ( bestD bitand 0x1 ) ) or ( ( col0 >= col1 ) and not ( bestD bitand 0x1 ) ) ) {
		rgb4_t tmpC = bestC0;
		bestC0 = bestC1;
		bestC1 = tmpC;
	}
	
	*out_c0 = bestC0;
	*out_c1 = bestC1;
	*out_d = bestD;
	
	convert444to888( &col8[0], bestC0 );
	convert444to888( &col8[1], bestC1 );
	computeRGBColorPaletteH( &palette[0], col8[0], col8[1], bestD );
	return computeBlockError( out_modulation, in_BLOCK_RGB, palette );
}



#pragma mark - exposed interface



uint32_t compressH( ETCBlockColor_t * out_block, const rgb8_t in_BLOCK_RGB[4][4], const Strategy_t in_STRATEGY ) {
	rgb4_t c0, c1;
	int d;
	uint8_t modulation[4][4];
	uint32_t blockError = 0xFFFFFFFF;
	
	blockError = quick( &c0, &c1, &d, modulation, in_BLOCK_RGB );
	//blockError = brute( &c0, &c1, &d, modulation, in_BLOCK_RGB );
	buildBlock( out_block, c0, c1, d, (const uint8_t(*)[4])modulation );
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
