//
//  ETC_Compress_Alpha.c
//  GLTC
//
//  Created by Michael Kwasnicki on 20.04.14.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#include "ETC_Compress_Alpha.h"

#include "ETC_Compress_Common.h"
#include "ETC_Decompress.h"

#include <stdio.h>



static void buildBlock( ETCBlockAlpha_t * out_block, const uint8_t in_BASE, const int in_T, const int in_MUL, const uint8_t in_BLOCK_A[4][4] ) {
	uint8_t palette[ETC_ALPHA_PALETTE_SIZE];
	uint8_t modulation[4][4];
	computeAlphaPalette( palette, in_BASE, in_T, in_MUL );
	computeAlphaBlockError( modulation, in_BLOCK_A, palette );
	uint64_t bitField = generateAlphaBitField( modulation );
	
	ETCBlockAlpha_t block;
	block.base = in_BASE;
	block.table = in_T;
	block.mul = in_MUL;
	block.aBitField = bitField;
	out_block->b64 = block.b64;
}



static uint32_t uniformAlpha( uint8_t * out_base, int * out_t, int * out_mul, const uint8_t in_BLOCK_A[4][4] ) {
	uint8_t base;
	
	computeAlphaBlockCenter( &base, in_BLOCK_A );
	
	*out_base = base;
	*out_t = 13;
	*out_mul = 1;
	return 0;
}



static uint32_t quick( uint8_t * out_base, int * out_t, int * out_mul, const uint8_t in_BLOCK_A[4][4] ) {
	uint8_t base, palette[ETC_ALPHA_PALETTE_SIZE];
	int t, mul;
	computeAlphaBlockCenter( &base, in_BLOCK_A );
	computeAlphaBlockWidth( &t, &mul, in_BLOCK_A );
	computeAlphaPalette( palette, base, t, mul );
	
	*out_base = base;
	*out_t = t;
	*out_mul = mul;
	return computeAlphaBlockError( NULL, in_BLOCK_A, palette );
}



static uint32_t brute( uint8_t * out_base, int * out_t, int * out_mul, const uint8_t in_BLOCK_A[4][4] ) {
	uint8_t palette[ETC_ALPHA_PALETTE_SIZE];
	uint8_t center;
	uint32_t error = 0xFFFFFFFF;
	uint32_t bestError = 0xFFFFFFFF;
	uint32_t bestDistance = 0xFFFFFFFF;
	uint8_t bestB;
	int bestT, bestMul;
	int distance, dA;
	
	computeAlphaBlockCenter( &center, in_BLOCK_A );
	
	for ( int b = 0; b < 256; b++ ) {
		dA = center - b;
		distance = dA * dA;
		
		for ( int t = 0; t < ETC_ALPHA_TABLE_COUNT; t++ ) {
			for ( int mul = 0; mul < ETC_ALPHA_MULTIPLIER_COUNT; mul++ ) {
				computeAlphaPalette( palette, b, t, mul );
				error = computeAlphaBlockError( NULL, in_BLOCK_A, palette );
				
				if ( ( error < bestError ) or ( ( error == bestError ) and ( distance < bestDistance ) ) ) {
					bestError = error;
					bestDistance = distance;
					bestB = b;
					bestT = t;
					bestMul = mul;
				}
				
				if ( ( bestError == 0 ) and ( bestDistance == 0) )
					goto earlyExit;
			}
		}
	}
	
earlyExit:
	
	*out_base = bestB;
	*out_t = bestT;
	*out_mul = bestMul;
	return bestError;
}



#pragma mark - exposed interface



uint32_t compressAlpha( ETCBlockAlpha_t * out_block, const uint8_t in_BLOCK_A[4][4], const Strategy_t in_STRATEGY ) {
	uint8_t base;
	int t, mul;
	uint32_t blockError = 0xFFFFFFFF;
	
	if ( isUniformAlphaBlock( in_BLOCK_A ) ) {
		blockError = uniformAlpha( &base, &t, &mul, in_BLOCK_A );
	} else {
		switch ( in_STRATEGY ) {
			case kFAST:
				blockError = quick( &base, &t, &mul, in_BLOCK_A );
				break;
				
			case kBEST:
				blockError = brute( &base, &t, &mul, in_BLOCK_A );
				break;
		}
	}
	
	buildBlock( out_block, base, t, mul, in_BLOCK_A );
	return blockError;
}



void printInfoAlpha( ETCBlockAlpha_t * in_BLOCK ) {
	printf( "A: b:%3i t:%3i mul:%3i\n", in_BLOCK->base, in_BLOCK->table, in_BLOCK->mul );
}
