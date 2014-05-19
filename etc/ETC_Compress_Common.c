//
//  ETC_Compress_Common.c
//  GLTC
//
//  Created by Michael Kwasnicki on 04.04.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#include "ETC_Compress_Common.h"

#include "ETC_Decompress.h"

#include "../lib.h"

#include <assert.h>
#include <iso646.h>
#include <math.h>
#include <stdio.h>



uint32_t g_counter = 0;



static int mySort( const void * in_A, const void * in_B ) {
	uint8_t a = *(uint8_t*)in_A;
	uint8_t b = *(uint8_t*)in_B;
	return a - b;
}



void fillUniformColorLUTGaps( ETCUniformColorComposition_t in_out_lut[256] ) {
	ETCUniformColorComposition_t tmp = { 0, 65535 };
	int dist = 255;
	bool found = false;
	
	// sweep forward
	for ( int c = 0; c < 256; c++ ) {
		if ( in_out_lut[c].error == 0 ) {
			tmp = in_out_lut[c];
			dist = 0;
			found = true;
		}
		
		if ( found ) {
			tmp.error = dist * dist;
			in_out_lut[c] = tmp;
			dist++;
		}
	}
	
	dist = 255;
	found = false;
	
	// sweep backward
	for ( int c = 255; c >= 0; c-- ) {
		if ( in_out_lut[c].error == 0 ) {
			tmp = in_out_lut[c];
			dist = 0;
			found = true;
		}
		
		if ( found ) {
			tmp.error = dist * dist;
			
			if ( tmp.error < in_out_lut[c].error )
				in_out_lut[c] = tmp;
			
			dist++;
		}
	}
}



void fillUniformColorPaletteLUTGaps( ETCUniformColorComposition_t in_out_lut[8][4][256] ) {
	for ( int t = 0; t < ETC_TABLE_COUNT; t++ ) {
		for ( int p = 0; p < ETC_PALETTE_SIZE; p++ ) {
			fillUniformColorLUTGaps( in_out_lut[t][p] );
		}
	}
}



#warning compute error separately for each color comoponent



// computes the error for a 4x2 RGB sub-block
uint32_t computeSubBlockError( uint8_t out_modulation[2][4], const rgba8_t in_SUB_BLOCK_RGBA[2][4], const rgba8_t in_PALETTE[4] ) {
	uint32_t subBlockError = 0;
	uint32_t pixelError = 0;
	uint32_t lowestPixelError = 0;
	int32_t dR, dG, dB;
	
	for ( int sby = 0; sby < 2; sby++ ) {
		for ( int sbx = 0; sbx < 4; sbx++ ) {
			rgba8_t pixel = in_SUB_BLOCK_RGBA[sby][sbx];
			lowestPixelError = 0xFFFFFFFF;
			
			for ( int p = 0; p < 4; p++ ) {
				dR = pixel.r - in_PALETTE[p].r;
				dG = pixel.g - in_PALETTE[p].g;
				dB = pixel.b - in_PALETTE[p].b;
				pixelError = dR * dR + dG * dG + dB * dB;
				
				if ( pixelError < lowestPixelError ) {
					lowestPixelError = pixelError;
					
					if ( out_modulation )
						out_modulation[sby][sbx] = p;
					
					if ( lowestPixelError == 0 )
						break;
				}
			}
			
			subBlockError += lowestPixelError;
		}
	}
	
	g_counter++;
	return subBlockError;
}



// computes the error for a 4x4 RGB block
uint32_t computeBlockError( uint8_t out_modulation[4][4], const rgba8_t in_BLOCK_RGBA[4][4], const rgba8_t in_PALETTE[4], const bool in_OPAQUE ) {
	uint32_t blockError = 0;
	uint32_t pixelError = 0;
	uint32_t lowestPixelError = 0;
	int32_t dR, dG, dB;
	
	for ( int by = 0; by < 4; by++ ) {
		for ( int bx = 0; bx < 4; bx++ ) {
			rgba8_t pixel = in_BLOCK_RGBA[by][bx];
			lowestPixelError = 0xFFFFFFFF;
			
			for ( int p = 0; p < 4; p++ ) {
				if ( !in_OPAQUE and p == 2 )
					continue;
				
				dR = pixel.r - in_PALETTE[p].r;
				dG = pixel.g - in_PALETTE[p].g;
				dB = pixel.b - in_PALETTE[p].b;
				pixelError = dR * dR + dG * dG + dB * dB;
				
				if ( pixelError < lowestPixelError ) {
					lowestPixelError = pixelError;
					
					if ( out_modulation )
						out_modulation[by][bx] = p;
					
					if ( lowestPixelError == 0 )
						break;
				}
			}
			
			blockError += lowestPixelError;
		}
	}
	
	g_counter += 2;
	return blockError;
}



// computes the error for a 4x4 RGB block in planar mode
uint32_t computeBlockErrorP( const rgba8_t in_C0, const rgba8_t in_CH, const rgba8_t in_CV, const rgba8_t in_BLOCK_RGBA[4][4] ) {
	uint32_t blockError = 0;
	int32_t dR, dG, dB;
	rgba8_t col8;
	
	for ( int by = 0; by < 4; by++ ) {
		for ( int bx = 0; bx < 4; bx++ ) {
			rgba8_t pixel = in_BLOCK_RGBA[by][bx];
			col8.r = computePlaneColor( bx, by, in_C0.r, in_CH.r, in_CV.r );
			col8.g = computePlaneColor( bx, by, in_C0.g, in_CH.g, in_CV.g );
			col8.b = computePlaneColor( bx, by, in_C0.b, in_CH.b, in_CV.b );
			dR = pixel.r - col8.r;
			dG = pixel.g - col8.g;
			dB = pixel.b - col8.b;
			blockError += dR * dR + dG * dG + dB * dB;
		}
	}
	
	g_counter += 2;
	return blockError;
}



// computes the error for a 4x4 block
uint32_t computeAlphaBlockError( uint8_t out_modulation[4][4], const uint8_t in_BLOCK_A[4][4], const uint8_t in_PALETTE[ETC_ALPHA_PALETTE_SIZE] ) {
	uint32_t blockError = 0;
	uint32_t pixelError = 0;
	uint32_t lowestPixelError = 0;
	int32_t dA;
	
	for ( int by = 0; by < 4; by++ ) {
		for ( int bx = 0; bx < 4; bx++ ) {
			uint8_t pixel = in_BLOCK_A[by][bx];
			lowestPixelError = 0xFFFFFFFF;
			
			for ( int p = 0; p < ETC_ALPHA_PALETTE_SIZE; p++ ) {
				dA = pixel - in_PALETTE[p];
				pixelError = dA * dA;
				
				if ( pixelError < lowestPixelError ) {
					lowestPixelError = pixelError;
					
					if ( out_modulation )
						out_modulation[by][bx] = p;
					
					if ( lowestPixelError == 0 )
						break;
				}
			}
			
			blockError += lowestPixelError;
		}
	}
	
	g_counter += 2;
	return blockError;
}



void printCounter() {
	printf( "Calls for Error : %u\n", g_counter );
}



uint32_t generateBitField( const uint8_t in_MODULATION[4][4] ) {
	uint32_t bitField = 0x0;
	
	for ( int bx = 3; bx >= 0; bx-- ) {
		for ( int by = 3; by >= 0; by-- ) {
			bitField <<= 1;
			bitField |= ( in_MODULATION[by][bx] bitand 0x1 ) bitor ( ( in_MODULATION[by][bx] bitand 0x2 ) << 15 );
		}
	}
	
	return bitField;
}



uint64_t generateAlphaBitField( const uint8_t in_MODULATION[4][4] ) {
	uint64_t bitField = 0x0;
	
	for ( int bx = 0; bx < 4; bx++ ) {
		for ( int by = 0; by < 4; by++ ) {
			bitField <<= 3;
			bitField |= ( in_MODULATION[by][bx] bitand 0x7 );
		}
	}
	
	return bitField;
}



void computeSubBlockMinMax( rgba8_t * out_min, rgba8_t * out_max, const rgba8_t in_SUB_BLOCK_RGBA[2][4] ) {
    rgba8_t pixel;
	rgba8_t cMin = {{ 255, 255, 255, 255 }};
	rgba8_t cMax = {{ 0, 0, 0, 0 }};
	
	for ( int sby = 0; sby < 2; sby++ ) {
		for ( int sbx = 0; sbx < 4; sbx++ ) {
			pixel = in_SUB_BLOCK_RGBA[sby][sbx];
			
			cMin.r = ( pixel.r < cMin.r ) ? pixel.r : cMin.r;
			cMin.g = ( pixel.g < cMin.g ) ? pixel.g : cMin.g;
			cMin.b = ( pixel.b < cMin.b ) ? pixel.b : cMin.b;
			cMin.a = ( pixel.a < cMin.a ) ? pixel.a : cMin.a;
			
			cMax.r = ( pixel.r > cMax.r ) ? pixel.r : cMax.r;
			cMax.g = ( pixel.g > cMax.g ) ? pixel.g : cMax.g;
			cMax.b = ( pixel.b > cMax.b ) ? pixel.b : cMax.b;
			cMax.a = ( pixel.a > cMax.a ) ? pixel.a : cMax.a;
		}
	}
    
    *out_min = cMin;
    *out_max = cMax;
}



void computeSubBlockCenter( rgba8_t * out_center, const rgba8_t in_SUB_BLOCK_RGBA[2][4] ) {
	rgba8_t cMin = {{ 255, 255, 255, 255 }};
	rgba8_t cMax = {{ 0, 0, 0, 0 }};
	computeSubBlockMinMax( &cMin, &cMax, in_SUB_BLOCK_RGBA );
	out_center->r = ( cMin.r + cMax.r + 1 ) >> 1;
	out_center->g = ( cMin.g + cMax.g + 1 ) >> 1;
	out_center->b = ( cMin.b + cMax.b + 1 ) >> 1;
	out_center->a = ( cMin.a + cMax.a + 1 ) >> 1;
}



void computeSubBlockMedian( rgba8_t * out_median, const rgba8_t in_SUB_BLOCK_RGBA[2][4] ) {
    rgba8_t pixel;
    uint8_t mR[8];
    uint8_t mG[8];
    uint8_t mB[8];
    int index = 0;
    
    for ( int sby = 0; sby < 2; sby++ ) {
        for ( int sbx = 0; sbx < 4; sbx++ ) {
            pixel = in_SUB_BLOCK_RGBA[sby][sbx];
            
            mR[index] = pixel.r;
            mG[index] = pixel.g;
            mB[index] = pixel.b;
            index++;
        }
    }
    
    qsort( mR, 8, sizeof( uint8_t ), mySort );
    qsort( mG, 8, sizeof( uint8_t ), mySort );
    qsort( mB, 8, sizeof( uint8_t ), mySort );
    
    (*out_median).r = ( mR[3] + mR[4] + 1 ) >> 1;
    (*out_median).g = ( mG[3] + mG[4] + 1 ) >> 1;
    (*out_median).b = ( mB[3] + mB[4] + 1 ) >> 1;
}



// Project all pixels onto the diagonal ( 1, 1, 1 )
// Get the maximum and minimum to compute the width
// Find a table index that matches the span
void computeSubBlockWidth( int * out_t, const rgba8_t in_SUB_BLOCK_RGBA[2][4] ) {
	rgba8_t pixel;
	uint32_t dot;
	uint32_t min = 0xFFFFFFFF;
	uint32_t max = 0;
	
	for ( int sby = 0; sby < 2; sby++ ) {
		for ( int sbx = 0; sbx < 4; sbx++ ) {
			pixel = in_SUB_BLOCK_RGBA[sby][sbx];
			
			dot = pixel.r + pixel.g + pixel.b;
			min = ( dot < min ) ? dot : min;
			max = ( dot > max ) ? dot : max;
		}
	}
	
	int halfWidth = ( max - min ) / 6; // intentionally round twards zero
	int t0 = 0;
	int t1 = 7;
	
	for ( int t = 0; t < ETC_TABLE_COUNT; t++ ) {
		if ( ETC_MODIFIER_TABLE[t][1] < halfWidth )
			t0 = t;
	}
	
	for ( int t = ETC_TABLE_COUNT - 1; t >= 0; t-- ) {
		if ( ETC_MODIFIER_TABLE[t][1] > halfWidth )
			t1 = t;
	}
	
    *out_t = t1;
	*out_t = t0;
}


//#error 0 and 255 needs to be represented exactly!!!
void computeAlphaBlockWidth( int * out_t, int * out_mul, const uint8_t in_BLOCK_A[4][4] ) {
	uint8_t aMin, aMax;
    computeAlphaBlockMinMax( &aMin, &aMax, in_BLOCK_A );
	int center = ( aMin + aMax + 1 ) >> 1;
	int mul, t;
	const int table[8] = { 13,  3, 15,  9,  7,  4,  1,  0 };
	
	for ( mul = 1; mul < 16; mul++ ) {
		for ( t = 0; t < 8; t++ ) {
			if ( ( aMin >= center + ETC_ALPHA_MODIFIER_TABLE[table[t]][3] * mul ) and ( aMax <= center + ETC_ALPHA_MODIFIER_TABLE[table[t]][7] * mul ) )
				goto exit;
			
		}
	}
	
exit:
	
	*out_t = table[t];
	*out_mul = mul;
}



void computeBlockMinMax( rgba8_t * out_min, rgba8_t * out_max, const rgba8_t in_BLOCK_RGBA[4][4] ) {
    rgba8_t pixel;
	rgba8_t cMin = {{ 255, 255, 255, 255 }};
	rgba8_t cMax = {{ 0, 0, 0, 0 }};
	
	for ( int by = 0; by < 4; by++ ) {
		for ( int bx = 0; bx < 4; bx++ ) {
			pixel = in_BLOCK_RGBA[by][bx];
			
			cMin.r = ( pixel.r < cMin.r ) ? pixel.r : cMin.r;
			cMin.g = ( pixel.g < cMin.g ) ? pixel.g : cMin.g;
			cMin.b = ( pixel.b < cMin.b ) ? pixel.b : cMin.b;
			cMin.a = ( pixel.a < cMin.a ) ? pixel.a : cMin.a;
			
			cMax.r = ( pixel.r > cMax.r ) ? pixel.r : cMax.r;
			cMax.g = ( pixel.g > cMax.g ) ? pixel.g : cMax.g;
			cMax.b = ( pixel.b > cMax.b ) ? pixel.b : cMax.b;
			cMax.a = ( pixel.a > cMax.a ) ? pixel.a : cMax.a;
		}
	}
    
    *out_min = cMin;
    *out_max = cMax;
}



void computeAlphaBlockMinMax( uint8_t * out_min, uint8_t * out_max, const uint8_t in_BLOCK_A[4][4] ) {
    uint8_t pixel;
	uint8_t aMin = 255;
	uint8_t aMax = 0;
	
	for ( int by = 0; by < 4; by++ ) {
		for ( int bx = 0; bx < 4; bx++ ) {
			pixel = in_BLOCK_A[by][bx];
			
			aMin = ( pixel < aMin ) ? pixel : aMin;
			aMax = ( pixel > aMax ) ? pixel : aMax;
		}
	}
    
    *out_min = aMin;
    *out_max = aMax;
}



void computeBlockCenter( rgba8_t * out_center, const rgba8_t in_BLOCK_RGBA[4][4] ) {
	rgba8_t cMin = {{ 255, 255, 255, 255 }};
	rgba8_t cMax = {{ 0, 0, 0, 0 }};
	computeBlockMinMax( &cMin, &cMax, in_BLOCK_RGBA );
	out_center->r = ( cMin.r + cMax.r + 1 ) >> 1;
	out_center->g = ( cMin.g + cMax.g + 1 ) >> 1;
	out_center->b = ( cMin.b + cMax.b + 1 ) >> 1;
	out_center->a = ( cMin.a + cMax.a + 1 ) >> 1;
}



void computeAlphaBlockCenter( uint8_t * out_center, const uint8_t in_BLOCK_A[4][4] ) {
	uint8_t aMin = 255;
	uint8_t aMax = 0;
	computeAlphaBlockMinMax( &aMin, &aMax, in_BLOCK_A );
	*out_center = ( aMin + aMax + 1 ) >> 1;
}



// project RGB onto HSL
static const float M[3][3] = {
	{ 0.788675f, -0.211325f, 0.577350f },
	{-0.211325f,  0.788675f, 0.577350f },
	{-0.577350f, -0.577350f, 0.577350f }
};



struct edge_t {
	uint32_t dist;
	uint8_t a;
	uint8_t b;
};



struct cluster_t {
	uint8_t cluster;
	uint8_t size;
};



static int edgeSort( const void * in_A, const void * in_B ) {
	struct edge_t a = *(struct edge_t*)in_A;
	struct edge_t b = *(struct edge_t*)in_B;
	return a.dist - b.dist;
}



static void _computeBlockChromas( rgb8_t out_c0[8], rgb8_t out_c1[8], const rgb8_t in_BLOCK_RGB[4][4] ) {
	int clusterPartition[16];
	struct cluster_t clusterSize[16];
	struct edge_t edge[120];
	const rgb8_t * data = &in_BLOCK_RGB[0][0];
	int dX, dY, dZ;
	int maxClusterSize = 0;
	
	for ( int i = 0; i < 16; i++ ) {
		clusterPartition[i] = i;
		clusterSize[i].cluster = i;
		clusterSize[i].size = 1;
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
	
	qsort( edge, 120, sizeof( struct edge_t ), edgeSort );

	for ( int i = 0; i < 120; i++ ) {
		int old = clusterPartition[edge[i].b];
		int new = clusterPartition[edge[i].a];
		
		if ( old == new )
			continue;
		
		for ( int j = 0; j < 16; j++ ) {
			if ( ( clusterPartition[j] == old ) and ( edge[i].a != j ) ) {
				clusterPartition[j] = new;
				clusterSize[clusterPartition[j]].size += clusterSize[j].size;
				clusterSize[j].size = 0;
			}
		}
		
		bool done = true;
		
		for ( int j = 0; j < 16; j++ ) {
			if ( clusterSize[j].size == 1 ) {
				done = false;
				break;
			}
		}
		
		if ( done )
			break;
	}
	
	for ( int i = 0; i < 16; i++ ) {
		if ( maxClusterSize < clusterSize[i].size )
			maxClusterSize = clusterSize[i].size;
	}
	
	for ( int i = 0; i < 16; i++ ) {
		if ( maxClusterSize == clusterSize[i].size ) {
			int cluster = clusterPartition[clusterSize[i].cluster];
			int c0[3] = { 0, 0, 0 };
			int c1[3] = { 0, 0, 0 };
			int c0Min[3] = { 255, 255, 255 };
			int c1Min[3] = { 255, 255, 255 };
			int c0Max[3] = { 0, 0, 0 };
			int c1Max[3] = { 0, 0, 0 };
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
					
					c0Min[0] = ( data[j].r < c0Min[0] ) ? data[j].r : c0Min[0];
					c0Min[1] = ( data[j].r < c0Min[1] ) ? data[j].r : c0Min[1];
					c0Min[2] = ( data[j].r < c0Min[2] ) ? data[j].r : c0Min[2];
					c0Max[0] = ( data[j].r > c0Max[0] ) ? data[j].r : c0Max[0];
					c0Max[1] = ( data[j].r > c0Max[1] ) ? data[j].r : c0Max[1];
					c0Max[2] = ( data[j].r > c0Max[2] ) ? data[j].r : c0Max[2];
					
					dot0 = data[j].r + data[j].g + data[j].b;
					min0 = ( dot0 < min0 ) ? dot0 : min0;
					max0 = ( dot0 > max0 ) ? dot0 : max0;
				} else {
					c1[0] += data[j].r;
					c1[1] += data[j].g;
					c1[2] += data[j].b;
					
					c1Min[0] = ( data[j].r < c1Min[0] ) ? data[j].r : c1Min[0];
					c1Min[1] = ( data[j].r < c1Min[1] ) ? data[j].r : c1Min[1];
					c1Min[2] = ( data[j].r < c1Min[2] ) ? data[j].r : c1Min[2];
					c1Max[0] = ( data[j].r > c1Max[0] ) ? data[j].r : c1Max[0];
					c1Max[1] = ( data[j].r > c1Max[1] ) ? data[j].r : c1Max[1];
					c1Max[2] = ( data[j].r > c1Max[2] ) ? data[j].r : c1Max[2];
					
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
			}
			
			assert( c0[0] < 256 and c0[0] >= 0 );
			assert( c0[1] < 256 and c0[1] >= 0 );
			assert( c0[2] < 256 and c0[2] >= 0 );
			assert( c1[0] < 256 and c1[0] >= 0 );
			assert( c1[1] < 256 and c1[1] >= 0 );
			assert( c1[2] < 256 and c1[2] >= 0 );
			
			c0[0] = ( c0Min[0] + c0Max[0] + 1 ) >> 1;
			c0[1] = ( c0Min[1] + c0Max[1] + 1 ) >> 1;
			c0[2] = ( c0Min[2] + c0Max[2] + 1 ) >> 1;
			c1[0] = ( c1Min[0] + c1Max[0] + 1 ) >> 1;
			c1[1] = ( c1Min[1] + c1Max[1] + 1 ) >> 1;
			c1[2] = ( c1Min[2] + c1Max[2] + 1 ) >> 1;
			
			printf( "C%2i %3i %3i %3i - %3i %3i %3i\n", i, c0[0], c0[1], c0[2], c1[0], c1[1], c1[2] );
			out_c0->r = c0[0];
			out_c0->g = c0[1];
			out_c0->b = c0[2];
			out_c1->r = c1[0];
			out_c1->g = c1[1];
			out_c1->b = c1[2];
			return;
		}
	}
}



static void _computeBlockChromas2( rgb8_t * out_c0, rgb8_t * out_c1, const rgb8_t in_BLOCK_RGB[4][4] ) {
	float blocks[4][4][3];
	float cf0[3], cf1[3];
	float dX, dY, dZ, dist, maxDist = -MAXFLOAT;
	
	int clusterPartition[16];
	struct cluster_t clusterSize[16];
	struct edge_t edge[120];
	
	const rgb8_t * data = &in_BLOCK_RGB[0][0];
	
	for ( int i = 0; i < 16; i++ ) {
		clusterPartition[i] = i;
		clusterSize[i].cluster = i;
		clusterSize[i].size = 1;
		printf( "%2i %3i %3i %3i\n", i, data[i].r, data[i].g, data[i].b );
	}
	
	int k = 0;
	int xx, yy, zz;
	
	for ( int i = 0; i < 16; i++ ) {
		for ( int j = i + 1; j < 16; j++ ) {
			xx = data[i].r - data[j].r;
			yy = data[i].g - data[j].g;
			zz = data[i].b - data[j].b;
			edge[k].a = i;
			edge[k].b = j;
			edge[k].dist = xx * xx + yy * yy + zz * zz;
			k++;
		}
	}
	
	qsort( edge, 120, sizeof( struct edge_t ), edgeSort );
	
//	for ( int l = 0; l < 120; l++ ) {
//		printf( "%3i %2i %2i %i\n", l, edge[l].a, edge[l].b, edge[l].dist );
//	}
	
	for ( int i = 0; i < 120; i++ ) {
		int old = clusterPartition[edge[i].b];
		int new = clusterPartition[edge[i].a];
		
		for ( int j = 0; j < 16; j++ ) {
			if ( ( clusterPartition[j] == old ) and ( edge[i].a != j ) ) {
				clusterPartition[j] = new;
				clusterSize[edge[i].a].size += clusterSize[j].size;
				clusterSize[j].size = 0;
			}
		}
		
		bool done = true;
		
		for ( int j = 0; j < 16; j++ ) {
			if ( clusterSize[j].size == 1 )
				done = false;
		}
		
		if ( done )
			break;
	}
	
	for ( int j = 0; j < 16; j++ ) {
		printf( "%2i %2i %2i %2i\n", j, clusterPartition[j], clusterSize[j].size, clusterSize[j].cluster );
	}

	int maxClusterSize = 0;
	
	for ( int i = 0; i < 16; i++ ) {
		if ( maxClusterSize < clusterSize[i].size )
			maxClusterSize = clusterSize[i].size;
	}
	
	for ( int i = 0; i < 16; i++ ) {
		if ( maxClusterSize == clusterSize[i].size ) {
			int cluster = clusterPartition[clusterSize[i].cluster];
			int c0i[3] = { 0, 0, 0 };
			int c1i[3] = { 0, 0, 0 };
			
			for ( int j = 0; j < 16; j++ ) {
				if ( clusterPartition[j] == cluster ) {
					c0i[0] += data[j].r;
					c0i[1] += data[j].g;
					c0i[2] += data[j].b;
				} else {
					c1i[0] += data[j].r;
					c1i[1] += data[j].g;
					c1i[2] += data[j].b;
				}
			}
			
			c0i[0] /= maxClusterSize;
			c0i[1] /= maxClusterSize;
			c0i[2] /= maxClusterSize;
			
			if ( maxClusterSize < 16 ) {
				c1i[0] /= 16 - maxClusterSize;
				c1i[1] /= 16 - maxClusterSize;
				c1i[2] /= 16 - maxClusterSize;
			}
			
			printf( "C%2i %3i %3i %3i - %3i %3i %3i\n", i, c0i[0], c0i[1], c0i[2], c1i[0], c1i[1], c1i[2] );
		}
	}
	
	return;
	
	
	for ( int by = 0; by < 4; by++ ) {
		for ( int bx = 0; bx < 4; bx++ ) {
			blocks[by][bx][0] = M[0][0] * in_BLOCK_RGB[by][bx].r + M[1][0] * in_BLOCK_RGB[by][bx].g + M[2][0] * in_BLOCK_RGB[by][bx].b;
			blocks[by][bx][1] = M[0][1] * in_BLOCK_RGB[by][bx].r + M[1][1] * in_BLOCK_RGB[by][bx].g + M[2][1] * in_BLOCK_RGB[by][bx].b;
			blocks[by][bx][2] = M[0][2] * in_BLOCK_RGB[by][bx].r + M[1][2] * in_BLOCK_RGB[by][bx].g + M[2][2] * in_BLOCK_RGB[by][bx].b;
//			printf( "%c %3i %3i %3i\n", count, in_BLOCK_RGB[by][bx].r, in_BLOCK_RGB[by][bx].g, in_BLOCK_RGB[by][bx].b );
//			count++;
		}
	}
	
//	int count0 = 'a';
//	
//	
//	for ( int by0 = 0; by0 < 4; by0++ ) {
//		for ( int bx0 = 0; bx0 < 4; bx0++ ) {
//			int count1 = 'a';
//			
//			printf( "%c\t", count0 );
//			
//			for ( int by1 = 0; by1 < 4; by1++ ) {
//				for ( int bx1 = 0; bx1 < 4; bx1++ ) {
//					dX = blocks[by1][bx1][0] - blocks[by0][bx0][0];
//					dY = blocks[by1][bx1][1] - blocks[by0][bx0][1];
//					dZ = blocks[by1][bx1][2] - blocks[by0][bx0][2];
//					dist = dX * dX + dY * dY; // + dZ * dZ;
//					
//					printf( "%7.3f\t", sqrtf( dist ) );
//					count1++;
//				}
//			}
//			
//			puts( "" );
//			
//			count0++;
//		}
//	}
	
	for ( int by0 = 0; by0 < 4; by0++ ) {
		for ( int bx0 = 0; bx0 < 4; bx0++ ) {
			for ( int by1 = 0; by1 < 4; by1++ ) {
				for ( int bx1 = 0; bx1 < 4; bx1++ ) {
					dX = blocks[by1][bx1][0] - blocks[by0][bx0][0];
					dY = blocks[by1][bx1][1] - blocks[by0][bx0][1];
					dZ = blocks[by1][bx1][2] - blocks[by0][bx0][2];
					dist = dX * dX + dY * dY + dZ * dZ;
					
					if ( dist > maxDist ) {
						maxDist = dist;
						cf0[0] = blocks[by0][bx0][0];
						cf0[1] = blocks[by0][bx0][1];
						cf0[2] = blocks[by0][bx0][2];
						cf1[0] = blocks[by1][bx1][0];
						cf1[1] = blocks[by1][bx1][1];
						cf1[2] = blocks[by1][bx1][2];
					}
				}
			}
		}
	}
	
	int cnt0 = 0;
	int cnt1 = 0;
	int dist0, dist1;
	float df0[3] = { 0, 0, 0 };
	float df1[3] = { 0, 0, 0 };
	
	for ( int by = 0; by < 4; by++ ) {
		for ( int bx = 0; bx < 4; bx++ ) {
			dX = blocks[by][bx][0] - cf0[0];
			dY = blocks[by][bx][1] - cf0[1];
			dZ = blocks[by][bx][2] - cf0[2];
			dist0 = dX * dX + dY * dY + dZ * dZ;
			
			dX = blocks[by][bx][0] - cf1[0];
			dY = blocks[by][bx][1] - cf1[1];
			dZ = blocks[by][bx][2] - cf1[2];
			dist1 = dX * dX + dY * dY + dZ * dZ;
			
			if ( dist0 < dist1 ) {
				cnt0++;
				df0[0] += blocks[by][bx][0];
				df0[1] += blocks[by][bx][1];
				df0[2] += blocks[by][bx][2];
			} else {
				cnt1++;
				df1[0] += blocks[by][bx][0];
				df1[1] += blocks[by][bx][1];
				df1[2] += blocks[by][bx][2];
			}
		}
	}
	
	df0[0] /= cnt0;
	df0[1] /= cnt0;
	df0[2] /= cnt0;
	df1[0] /= cnt1;
	df1[1] /= cnt1;
	df1[2] /= cnt1;
	
	if ( cnt0 > cnt1 ) {
		for ( int i = 0; i < 3; i++ ) {
			float tmp = df0[i];
			df0[i] = df1[i];
			df1[i] = tmp;
		}
	}
	
	out_c0->r = roundf( M[0][0] * df0[0] + M[0][1] * df0[1] + M[0][2] * df0[2] );
	out_c0->g = roundf( M[1][0] * df0[0] + M[1][1] * df0[1] + M[1][2] * df0[2] );
	out_c0->b = roundf( M[2][0] * df0[0] + M[2][1] * df0[1] + M[2][2] * df0[2] );
	out_c1->r = roundf( M[0][0] * df1[0] + M[0][1] * df1[1] + M[0][2] * df1[2] );
	out_c1->g = roundf( M[1][0] * df1[0] + M[1][1] * df1[1] + M[1][2] * df1[2] );
	out_c1->b = roundf( M[2][0] * df1[0] + M[2][1] * df1[1] + M[2][2] * df1[2] );
	
//	out_c0->r = roundf( M[0][0] * cf0[0] + M[0][1] * cf0[1] + M[0][2] * cf0[2] );
//	out_c0->g = roundf( M[1][0] * cf0[0] + M[1][1] * cf0[1] + M[1][2] * cf0[2] );
//	out_c0->b = roundf( M[2][0] * cf0[0] + M[2][1] * cf0[1] + M[2][2] * cf0[2] );
//	out_c1->r = roundf( M[0][0] * cf1[0] + M[0][1] * cf1[1] + M[0][2] * cf1[2] );
//	out_c1->g = roundf( M[1][0] * cf1[0] + M[1][1] * cf1[1] + M[1][2] * cf1[2] );
//	out_c1->b = roundf( M[2][0] * cf1[0] + M[2][1] * cf1[1] + M[2][2] * cf1[2] );
}



static void _computeMinMaxAvgCenterMedian( rgb8_t * out_min, rgb8_t * out_max, rgb8_t out_acm[11], int * out_t0, int * out_t1, const rgb8_t in_SUB_BLOCK_RGB[2][4] ) {
	rgb8_t pixel;
	uint8_t mR[8];
	uint8_t mG[8];
	uint8_t mB[8];
	int index = 0;
	int averageColor[3] = { 4, 4, 4 };
	rgb8_t cMin = {{ 255, 255, 255 }};
	rgb8_t cMax = {{ 0, 0, 0 }};
	uint32_t dot;
	uint32_t min = 0xFFFFFFFF;
	uint32_t max = 0;
	
	static const float m[3][3] = {
		{ 0.788675, -0.211325, 0.577350 },
		{-0.211325,  0.788675, 0.577350 },
		{-0.577350, -0.577350, 0.577350 }
	};
	
	for ( int sby = 0; sby < 2; sby++ ) {
		for ( int sbx = 0; sbx < 4; sbx++ ) {
			pixel = in_SUB_BLOCK_RGB[sby][sbx];
			
			averageColor[0] += pixel.r;
			averageColor[1] += pixel.g;
			averageColor[2] += pixel.b;
			
			cMin.r = ( pixel.r < cMin.r ) ? pixel.r : cMin.r;
			cMin.g = ( pixel.g < cMin.g ) ? pixel.g : cMin.g;
			cMin.b = ( pixel.b < cMin.b ) ? pixel.b : cMin.b;
			
			cMax.r = ( pixel.r > cMax.r ) ? pixel.r : cMax.r;
			cMax.g = ( pixel.g > cMax.g ) ? pixel.g : cMax.g;
			cMax.b = ( pixel.b > cMax.b ) ? pixel.b : cMax.b;
			
			mR[index] = pixel.r;
			mG[index] = pixel.g;
			mB[index] = pixel.b;
			index++;
			
			dot = pixel.r + pixel.g + pixel.b;
			min = ( dot < min ) ? dot : min;
			max = ( dot > max ) ? dot : max;
		}
	}
	
	*out_min = cMin;
	*out_max = cMax;
	
	qsort( mR, 8, sizeof( uint8_t ), mySort );
	qsort( mG, 8, sizeof( uint8_t ), mySort );
	qsort( mB, 8, sizeof( uint8_t ), mySort );
	
	// 65 %
	out_acm[0].r = averageColor[0] >> 3;
	out_acm[0].g = averageColor[1] >> 3;
	out_acm[0].b = averageColor[2] >> 3;
	
	// 25%
	out_acm[1].r = ( cMin.r + cMax.r + 1 ) >> 1;
	out_acm[1].g = ( cMin.g + cMax.g + 1 ) >> 1;
	out_acm[1].b = ( cMin.b + cMax.b + 1 ) >> 1;
	
	// 10%
	out_acm[2].r = ( mR[3] + mR[4] + 1 ) >> 1;
	out_acm[2].g = ( mG[3] + mG[4] + 1 ) >> 1;
	out_acm[2].b = ( mB[3] + mB[4] + 1 ) >> 1;
	
	for ( int i = 0; i < 8; i++ ) {
		out_acm[3 + i].r = ( i bitand 0x4 ) ? cMax.r : cMin.r;
		out_acm[3 + i].g = ( i bitand 0x2 ) ? cMax.g : cMin.g;
		out_acm[3 + i].b = ( i bitand 0x1 ) ? cMax.b : cMin.b;
	}
	
	int halfWidth = ( max - min ) / 6; // intentionally round twards zero
	int t0 = 0;
	int t1 = 7;
	
	for ( int t = 0; t < ETC_TABLE_COUNT; t++ ) {
		if ( ETC_MODIFIER_TABLE[t][1] < halfWidth )
			t0 = t;
	}
	
	for ( int t = ETC_TABLE_COUNT - 1; t >= 0; t-- ) {
		if ( ETC_MODIFIER_TABLE[t][1] > halfWidth )
			t1 = t;
	}
	
	*out_t0 = t0;
	*out_t1 = t1;
	
	float blocks[2][4][3];
	
	
	float dMin[3] = {  1024,  1024,  1024 };
	float dMax[3] = { -1024, -1024, -1024 };
	
	for ( int sby = 0; sby < 2; sby++ ) {
		for ( int sbx = 0; sbx < 4; sbx++ ) {
			blocks[sby][sbx][0] = in_SUB_BLOCK_RGB[sby][sbx].r;
			blocks[sby][sbx][1] = in_SUB_BLOCK_RGB[sby][sbx].g;
			blocks[sby][sbx][2] = in_SUB_BLOCK_RGB[sby][sbx].b;
			float px = m[0][0] * blocks[sby][sbx][0] + m[1][0] * blocks[sby][sbx][1] + m[2][0] * blocks[sby][sbx][2];
			float py = m[0][1] * blocks[sby][sbx][0] + m[1][1] * blocks[sby][sbx][1] + m[2][1] * blocks[sby][sbx][2];
			float pz = m[0][2] * blocks[sby][sbx][0] + m[1][2] * blocks[sby][sbx][1] + m[2][2] * blocks[sby][sbx][2];
			
			dMin[0] = ( px < dMin[0] ) ? px : dMin[0];
			dMin[1] = ( py < dMin[1] ) ? py : dMin[1];
			dMin[2] = ( pz < dMin[2] ) ? pz : dMin[2];
			dMax[0] = ( px > dMax[0] ) ? px : dMax[0];
			dMax[1] = ( py > dMax[1] ) ? py : dMax[1];
			dMax[2] = ( pz > dMax[2] ) ? pz : dMax[2];
		}
	}
		
	float px = m[0][0] * out_acm[1].r + m[1][0] * out_acm[1].g + m[2][0] * out_acm[1].b;
	float py = m[0][1] * out_acm[1].r + m[1][1] * out_acm[1].g + m[2][1] * out_acm[1].b;
	float pz = m[0][2] * out_acm[1].r + m[1][2] * out_acm[1].g + m[2][2] * out_acm[1].b;
	
	float cx = ( dMax[0] + dMin[0] ) * 0.5f;
	float cy = ( dMax[1] + dMin[1] ) * 0.5f;
	float cz = ( dMax[2] + dMin[2] ) * 0.5f;
	
	float qx = m[0][0] * cx + m[0][1] * cy + m[0][2] * cz;
	float qy = m[1][0] * cx + m[1][1] * cy + m[1][2] * cz;
	float qz = m[2][0] * cx + m[2][1] * cy + m[2][2] * cz;
	
	printf( "CTR %.3f %.3f %.3f\n", px, py, pz );
	printf( "Ctr %.3f %.3f %.3f\n", cx, cy, cz );
	printf( "Ctr %.3f %.3f %.3f\n", fabsf( px - cx ), fabsf( py - cy ), fabsf( pz - cz ) );
	printf( "CTR_%3i %3i %3i\n", out_acm[1].r, out_acm[1].g, out_acm[1].b );
	printf( "Ctr_%.0f %.0f %.0f\n", qx, qy, qz );
	
	out_acm[3].r = roundf( qx );
	out_acm[3].g = roundf( qy );
	out_acm[3].b = roundf( qz );
	
//	
//	printf( "Wdt %.3f %.3f %.3f\n", dMax[0] - dMin[0], dMax[1] - dMin[1], dMax[2] - dMin[2] );
}



bool isUniformColorBlock( const rgba8_t in_BLOCK_RGBA[4][4] ) {
    rgba8_t cMin, cMax;
    computeBlockMinMax( &cMin, &cMax, in_BLOCK_RGBA );
    return ( ( cMin.r == cMax.r ) and ( cMin.g == cMax.g ) and ( cMin.b == cMax.b ) );
}



bool isUniformAlphaBlock( const uint8_t in_BLOCK_A[4][4] ) {
    uint8_t aMin, aMax;
    computeAlphaBlockMinMax( &aMin, &aMax, in_BLOCK_A );
    return ( aMin == aMax );
}



bool isUniformColorSubBlock( const rgba8_t in_SUB_BLOCK_RGBA[2][4] ) {
    rgba8_t cMin, cMax;
    computeSubBlockMinMax( &cMin, &cMax, in_SUB_BLOCK_RGBA );
    return ( ( cMin.r == cMax.r ) and ( cMin.g == cMax.g ) and ( cMin.b == cMax.b ) );
}

