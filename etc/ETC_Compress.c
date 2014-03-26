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



static void compressETCBlockRGB( ETCBlockColor_t * out_block, const rgb8_t in_BLOCK_RGB[4][4], const Strategy_t in_STRATEGY, uint32_t (*in_COMPRESSION_MODE[])( ETCBlockColor_t *, const rgb8_t[4][4], const Strategy_t), const int in_COMPRESSION_MODE_COUNT ) {
    uint32_t error;
	uint32_t bestError = 0xFFFFFFFF;
    ETCBlockColor_t block;
    ETCBlockColor_t bestBlock;
    
    for ( int m = 0; m < in_COMPRESSION_MODE_COUNT; m++ ) {
        error = in_COMPRESSION_MODE[m]( &block, in_BLOCK_RGB, in_STRATEGY );
        
        if ( error < bestError ) {
            bestError = error;
            bestBlock = block;
        }
    }
	
    *out_block = bestBlock;
}



#pragma mark - exposed interface



void compressETC1BlockRGB( ETCBlockColor_t * out_block, const rgb8_t in_BLOCK_RGB[4][4], const Strategy_t in_STRATEGY ) {
    uint32_t (*compressionMode[])( ETCBlockColor_t *, const rgb8_t[4][4], const Strategy_t) = {
        &compressI,
        &compressD
    };
    const int compressionModeCount = sizeof( compressionMode ) / sizeof( void * );
    compressETCBlockRGB( out_block, in_BLOCK_RGB, in_STRATEGY, compressionMode, compressionModeCount );
}



void compressETC2BlockRGB( ETCBlockColor_t * out_block, const rgb8_t in_BLOCK_RGB[4][4], const Strategy_t in_STRATEGY ) {
    uint32_t (*compressionMode[])( ETCBlockColor_t *, const rgb8_t[4][4], const Strategy_t) = {
        &compressI,
        &compressD,
        &compressT,
        &compressH,
        &compressP
    };
    const int compressionModeCount = sizeof( compressionMode ) / sizeof( void * );
    compressETCBlockRGB( out_block, in_BLOCK_RGB, in_STRATEGY, compressionMode, compressionModeCount );
}



void computeUniformColorLUT() {
	computeUniformColorLUTI();
	computeUniformColorLUTD();
	computeUniformColorLUTT(); // H is a subset of T, so there is no use
	computeUniformColorLUTP();
}


