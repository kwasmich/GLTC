//
//  blockCompressionCommon.c
//  GLTC
//
//  Created by Michael Kwasnicki on 02.03.14.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//


#include "blockCompressionCommon.h"

#include "lib.h"

#include <assert.h>
#include <string.h>



void twiddleBlocksRGBA( rgba8_t * in_out_image, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const bool in_REVERSE ) {
    assert( in_WIDTH % 4 == 0 );
    assert( in_HEIGHT % 4 == 0 );
    
    rgba8_t * copy = malloc( in_WIDTH * in_HEIGHT * sizeof( rgba8_t ) );
    assert( copy );
    memcpy( copy, in_out_image, in_WIDTH * in_HEIGHT * sizeof( rgba8_t ) );
    
#pragma mark TODO : parallelize with stripes of 4px height
    for ( uint32_t y = 0; y < in_HEIGHT; y++ ) {
        int divYquot = y >> 2;
        int divYrem  = y bitand 0x3;
        
        for ( uint32_t x = 0; x < in_WIDTH; x++ ) {
            int divXquot = x >> 2;
            int divXrem  = x bitand 0x3;
            
            int newPos = x + y * in_WIDTH;
            int pos = divXrem + divXquot * 16 + ( divYrem + divYquot * in_HEIGHT ) * 4;
            
            if ( in_REVERSE )
                in_out_image[newPos] = copy[pos];
            else
                in_out_image[pos] = copy[newPos];
        }
    }
    
    free_s( copy );
}
