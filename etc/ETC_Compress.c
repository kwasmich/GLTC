//
//  ETC_Compress.c
//  GLTC
//
//  Created by Michael Kwasnicki on 10.01.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#include "ETC_Compress.h"

#include "ETC_Compress_Common.h"
#include "ETC_Compress_I.h"
#include "ETC_Compress_D.h"
#include "ETC_Compress_T.h"
#include "ETC_Compress_H.h"
#include "ETC_Compress_P.h"
#include "ETC_Decompress.h"
#include "lib.h"

#include "PNG.h"

#include <assert.h>
#include <iso646.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h> // memcpy




#pragma mark - exposed interface

//  flip          no flip
// +---------+   +-----+-----+
// | a e i m |   | a e | i m |
// | b f j n |   | b f | j n |
// +---------+   | c g | k o |
// | c g k o |   | d h | l p |
// | d h l p |   +-----+-----+
// +---------+

void compressETC1BlockRGB( ETCBlockColor_t * out_block, const rgb8_t in_BLOCK_RGB[4][4] ) {
	static int counter = 0;
	uint32_t error;
	bool flip = out_block->flip;
	int table0 = out_block->table0;
	int table1 = out_block->table1;
	
    //printf( "\nNEW BLOCK %i\n", counter );
    
	if ( counter == -136 ) {
		out_block->b64 = 0x0ULL;
	} else {
		//error = compressI( out_block, in_BLOCK_RGB );
		error = compressD( out_block, in_BLOCK_RGB );
		//error = compressT( out_block, in_BLOCK_RGB );
		//error = compressH( out_block, in_BLOCK_RGB );
		//error = compressP( out_block, in_BLOCK_RGB );
	}
	
	//printf( "t %u\n", error );
	
//	if ( counter == 2 )
//		compressI( out_block, in_BLOCK_RGB );
//	else
//		out_block->b64 = 0;
	
//	bruteP( out_block, in_BLOCK_RGB );
	
//	rgb8_t blockRGB[4][4];
//	rgb8_t blockRGBFlip[4][4];
//	rgb8_t minC[4], maxC[4], acmC[4][11];
//	int t0[4], t1[4];
//	
//	for ( int by = 0; by < 4; by++ ) {
//		for ( int bx = 0; bx < 4; bx++ ) {
//			blockRGB[by][bx] = in_BLOCK_RGB[bx][by];
//			blockRGBFlip[by][bx] = in_BLOCK_RGB[by][bx];
//		}
//	}
//	
//	computeMinMaxAvgCenterMedian( &minC[0], &maxC[0], &acmC[0][0], &t0[0], &t1[0], &blockRGB[0] );		// left
//	computeMinMaxAvgCenterMedian( &minC[1], &maxC[1], &acmC[1][0], &t0[1], &t1[1], &blockRGB[2] );		// right
//	computeMinMaxAvgCenterMedian( &minC[2], &maxC[2], &acmC[2][0], &t0[2], &t1[2], &blockRGBFlip[0] );	// top
//	computeMinMaxAvgCenterMedian( &minC[3], &maxC[3], &acmC[3][0], &t0[3], &t1[3], &blockRGBFlip[1] );	// bottom
//	
//	printf( "%2i %2i %2i vs. %2i %2i %2i\n", reduce8to4bits( acmC[0][1].r ), reduce8to4bits( acmC[0][1].g ), reduce8to4bits( acmC[0][1].b ), reduce8to4bits( acmC[0][3].r ), reduce8to4bits( acmC[0][3].g ), reduce8to4bits( acmC[0][3].b ) );
//	printf( "%2i %2i %2i vs. %2i %2i %2i\n", reduce8to4bits( acmC[1][1].r ), reduce8to4bits( acmC[1][1].g ), reduce8to4bits( acmC[1][1].b ), reduce8to4bits( acmC[1][3].r ), reduce8to4bits( acmC[1][3].g ), reduce8to4bits( acmC[1][3].b ) );
//	printf( "%2i %2i %2i vs. %2i %2i %2i\n", reduce8to4bits( acmC[2][1].r ), reduce8to4bits( acmC[2][1].g ), reduce8to4bits( acmC[2][1].b ), reduce8to4bits( acmC[2][3].r ), reduce8to4bits( acmC[2][3].g ), reduce8to4bits( acmC[2][3].b ) );
//	printf( "%2i %2i %2i vs. %2i %2i %2i\n", reduce8to4bits( acmC[3][1].r ), reduce8to4bits( acmC[3][1].g ), reduce8to4bits( acmC[3][1].b ), reduce8to4bits( acmC[3][3].r ), reduce8to4bits( acmC[3][3].g ), reduce8to4bits( acmC[3][3].b ) );
//	
//	uint32_t volume[4];
//	
//	for ( int i = 0; i < 4; i++ )
//		volume[i] = ( maxC[i].r - minC[i].r + 1 ) * ( maxC[i].g - minC[i].g + 1 ) * ( maxC[i].b - minC[i].b + 1 );
	
	//printf( "%8i %8i, %8i %8i; %i %i, %i %i; ", volume[0], volume[1], volume[2], volume[3], t0[0], t0[1], t1[0], t1[1]  );
	
//	printf( "%i %i\n", table0, table1 );
//	
//	if ( flip ) {
//		printf( "%i %i;\n", t0[0], t0[1] );
//	} else {
//		printf( "%i %i;\n", t1[0], t1[1] );
//	}
	
//	if ( volume[0] + volume[1] <= volume[2] + volume[3] ) {
//		// no flip
//		
//		printf( "%s %s  -  \n", ( volume[0] == 1 ) ? "const" : "  -  ", ( volume[1] == 1 ) ? "const" : "  -  " );
//	} else {
//		// flip
//		
//		printf( "%s %s flip\n", ( volume[2] == 1 ) ? "const" : "  -  ", ( volume[3] == 1 ) ? "const" : "  -  " );
//	}
	
	//	ETC1BlockIndividual_t blockI;
	//	ETC1BlockDifferential_t blockD;
	//	uint32_t errorI = compressI( &blockI, in_BLOCK_RGB );
	//	uint32_t errorD = compressD( &blockD, in_BLOCK_RGB );
	//	
	//	if ( errorI < errorD ) {
	//		//printf( "%i\n", errorI );
	//		out_block->b64 = blockI.b64;
	//	} else {
	//		//printf( "%i\n", errorD );
	//		out_block->b64 = blockD.b64;
	//	}
	
	//if ( bestFlip )
	//	block->b64 = 0LL;
	
	
	//printf( "%4i\n", counter );
	//fputs( ".", stdout );
	//fflush( stdout );
	counter++;
}



void computeUniformColorLUT() {
	computeUniformColorLUTI();
	computeUniformColorLUTD();
	computeUniformColorLUTT();
	computeUniformColorLUTP();
}


