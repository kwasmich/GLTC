//
//  ETC_Compress_Common.h
//  GLTC
//
//  Created by Michael Kwasnicki on 04.04.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#ifndef GLTC_ETC_Compress_Common_h
#define GLTC_ETC_Compress_Common_h


#include "ETC_Common.h"

#include <stdbool.h>


typedef struct {
    uint8_t c;
    uint16_t error;
} ETCUniformColorComposition_t;



void changeCounter( const int32_t in_INC );
void printCounter( void );

void fillUniformColorLUTGaps( ETCUniformColorComposition_t in_out_lut[256] );
void fillUniformColorPaletteLUTGaps( ETCUniformColorComposition_t in_out_lut[8][4][256] );

uint32_t computeSubBlockError( uint8_t out_modulation[2][4], const rgb8_t in_SUB_BLOCK_RGB[2][4], const rgb8_t in_PALETTE[ETC_PALETTE_SIZE] );
uint32_t computeBlockError( uint8_t out_modulation[4][4], const rgb8_t in_BLOCK_RGB[4][4], const rgb8_t in_PALETTE[ETC_PALETTE_SIZE] );
uint32_t computeBlockErrorP( const rgb8_t in_C0, const rgb8_t in_CH, const rgb8_t in_CV, const rgb8_t in_BLOCK_RGB[4][4] );
uint32_t computeAlphaBlockError( uint8_t out_modulation[4][4], const uint8_t in_BLOCK_A[4][4], const uint8_t in_PALETTE[ETC_ALPHA_PALETTE_SIZE] );

uint32_t generateBitField( const uint8_t in_MODULATION[4][4] );
uint64_t generateAlphaBitField( const uint8_t in_MODULATION[4][4] );
void computeSubBlockMinMax( rgb8_t * out_min, rgb8_t * out_max, const rgb8_t in_SUB_BLOCK_RGB[2][4] );
void computeSubBlockCenter( rgb8_t * out_center, const rgb8_t in_SUB_BLOCK_RGB[2][4] );
void computeSubBlockMedian( rgb8_t * out_median, const rgb8_t in_SUB_BLOCK_RGB[2][4] );
void computeSubBlockWidth( int * out_t, const rgb8_t in_SUB_BLOCK_RGB[2][4] );
void computeAlphaBlockWidth( int * out_t, int * out_mul, const uint8_t in_BLOCK_A[4][4] );
void computeBlockMinMax( rgb8_t * out_min, rgb8_t * out_max, const rgb8_t in_BLOCK_RGB[4][4] );
void computeBlockCenter( rgb8_t * out_center, const rgb8_t in_BLOCK_RGB[4][4] );
void computeBlockChromas( rgb8_t out_c0[8], rgb8_t out_c1[8], const rgb8_t in_BLOCK_RGB[4][4] );
void computeAlphaBlockMinMax( uint8_t * out_min, uint8_t * out_max, const uint8_t in_BLOCK_A[4][4] );
void computeAlphaBlockCenter( uint8_t * out_center, const uint8_t in_BLOCK_A[4][4] );

void computeMinMaxAvgCenterMedian( rgb8_t * out_min, rgb8_t * out_max, rgb8_t out_acm[3], int * out_t0, int * out_t1, const rgb8_t in_SUB_BLOCK_RGB[2][4] );

bool isUniformColorBlock( const rgb8_t in_BLOCK_RGB[4][4] );
bool isUniformAlphaBlock( const uint8_t in_BLOCK_A[4][4] );
bool isUniformColorSubBlock( const rgb8_t in_SUB_BLOCK_RGB[2][4] );

#endif
