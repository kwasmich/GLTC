//
//  blockCompressionCommon.c
//  GLTC
//
//  Created by Michael Kwasnicki on 02.03.14.
//
//


#include "blockCompressionCommon.h"

#include "lib.h"

#include <assert.h>
#include <string.h>


static void twiddleBlocks( uint8_t * in_out_image, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const uint8_t in_CHANNELS, const bool in_REVERSE ) {
    assert( in_WIDTH % 4 == 0 );
    assert( in_HEIGHT % 4 == 0 );
    
    uint8_t * copy = malloc( in_WIDTH * in_HEIGHT * in_CHANNELS * sizeof( uint8_t ) );
    memcpy( copy, in_out_image, in_WIDTH * in_HEIGHT * in_CHANNELS * sizeof( uint8_t ) );
    
#pragma mark TODO : parallelize with stripes of 4px height
    for ( uint32_t y = 0; y < in_HEIGHT; y++ ) {
        div_t divY = div( y, 4 );
        
        for ( uint32_t x = 0; x < in_WIDTH; x++ ) {
            div_t divX = div( x, 4 );
            int newPos = x + y * in_WIDTH;
            int pos = divX.rem + divX.quot * 16 + ( divY.rem + divY.quot * in_HEIGHT ) * 4;
            
            if ( in_REVERSE ) {
                //in_out_image[newPos] = copy[pos];
                memcpy( &in_out_image[newPos * in_CHANNELS], &copy[pos * in_CHANNELS], in_CHANNELS * sizeof( uint8_t ) );
            } else {
                //in_out_image[pos] = copy[newPos];
                memcpy( &in_out_image[pos * in_CHANNELS], &copy[newPos * in_CHANNELS], in_CHANNELS * sizeof( uint8_t ) );
            }
        }
    }
    
    free_s( copy );
}

#warning Is there a way to combine this in a clean fashion?

void twiddleBlocksRGB( rgb8_t * in_out_image, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const bool in_REVERSE ) {
    //twiddleBlocks( (uint8_t*)in_out_image, in_WIDTH, in_HEIGHT, 3, in_REVERSE );
    
    assert( in_WIDTH % 4 == 0 );
    assert( in_HEIGHT % 4 == 0 );
    
    rgb8_t * copy = malloc( in_WIDTH * in_HEIGHT * sizeof( rgb8_t ) );
    memcpy( copy, in_out_image, in_WIDTH * in_HEIGHT * sizeof( rgb8_t ) );
    
#pragma mark TODO : parallelize with stripes of 4px height
    for ( uint32_t y = 0; y < in_HEIGHT; y++ ) {
        div_t divY = div( y, 4 );
        
        for ( uint32_t x = 0; x < in_WIDTH; x++ ) {
            div_t divX = div( x, 4 );
            int newPos = x + y * in_WIDTH;
            int pos = divX.rem + divX.quot * 16 + ( divY.rem + divY.quot * in_HEIGHT ) * 4;
            
            if ( in_REVERSE )
                in_out_image[newPos] = copy[pos];
            else
                in_out_image[pos] = copy[newPos];
        }
    }
    
    free_s( copy );
}


void twiddleBlocksRGBA( rgba8_t * in_out_image, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const bool in_REVERSE ) {
    //twiddleBlocks( (uint8_t*)in_out_image, in_WIDTH, in_HEIGHT, 4, in_REVERSE );
    
    assert( in_WIDTH % 4 == 0 );
    assert( in_HEIGHT % 4 == 0 );
    
    rgba8_t * copy = malloc( in_WIDTH * in_HEIGHT * sizeof( rgba8_t ) );
    memcpy( copy, in_out_image, in_WIDTH * in_HEIGHT * sizeof( rgba8_t ) );
    
#pragma mark TODO : parallelize with stripes of 4px height
    for ( uint32_t y = 0; y < in_HEIGHT; y++ ) {
        div_t divY = div( y, 4 );
        
        for ( uint32_t x = 0; x < in_WIDTH; x++ ) {
            div_t divX = div( x, 4 );
            int newPos = x + y * in_WIDTH;
            int pos = divX.rem + divX.quot * 16 + ( divY.rem + divY.quot * in_HEIGHT ) * 4;
            
            if ( in_REVERSE )
                in_out_image[newPos] = copy[pos];
            else
                in_out_image[pos] = copy[newPos];
        }
    }
    
    free_s( copy );
}


