//
//  PVRTC_Compress.c
//  GLTC
//
//  Created by Michael Kwasnicki on 16.03.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#include "PVRTC_Compress.h"

#include <iso646.h>
#include <stdlib.h>


void bilinearFilter3_4x4( rgba8_t * out_rgba, const int in_X, const int in_Y, const rgba8_t in_BLOCK[3][3] ) {
    int blockX = in_X >> 2;
    int blockY = in_Y >> 2;
    int x = in_X bitand 0x3;
    int y = in_Y bitand 0x3;
    
    for ( int i = 0; i < 4; i++ ) {
        uint32_t filtered = 0;
        filtered += in_BLOCK[blockY + 0][blockX + 0].array[i] * ( 4 - x ) * ( 4 - y );
        filtered += in_BLOCK[blockY + 0][blockX + 1].array[i] * x * ( 4 - y );
        filtered += in_BLOCK[blockY + 1][blockX + 0].array[i] * ( 4 - x ) * y;
        filtered += in_BLOCK[blockY + 1][blockX + 1].array[i] * x * y;
        filtered >>= 4;
        out_rgba->array[i] = filtered;
    }
}


// assuming 0/8, 3/8, 5/8, 8/8 colors
uint32_t computeError( const rgba8_t in_RGBA[8][8], const rgba8_t in_BASE_A[3][3], const rgba8_t in_BASE_B[3][3] ) {
    uint32_t error = 0;
    rgba8_t baseA;
    rgba8_t baseB;
    rgba8_t blend;
    uint32_t temp[4];
    
    const int modulation[4] = { 0, 3, 5, 8 };

    for ( int by = 1; by < 8; by++ ) {        
        for ( int bx = 1; bx < 8; bx++ ) {
            rgba8_t pixel = in_RGBA[by][bx];
            bilinearFilter3_4x4( &baseA, bx, by, in_BASE_A );
            bilinearFilter3_4x4( &baseB, bx, by, in_BASE_B );

            uint32_t leastPixelError = 0xFFFFFFFF;
            int bestM = 0;
            
            for ( int m = 0; m < 4; m++ ) {
                int mod = modulation[m];
                
                temp[0] = ( baseA.r * mod + baseB.r * ( 8 - mod ) ) >> 3;
                temp[1] = ( baseA.g * mod + baseB.g * ( 8 - mod ) ) >> 3;
                temp[2] = ( baseA.b * mod + baseB.b * ( 8 - mod ) ) >> 3;
                temp[3] = ( baseA.a * mod + baseB.a * ( 8 - mod ) ) >> 3;
                temp[0] = pixel.r - temp[0];
                temp[1] = pixel.g - temp[1];
                temp[2] = pixel.b - temp[2];
                temp[3] = pixel.a - temp[3];
                
                uint32_t pixelError = temp[0] * temp[0] + temp[1] * temp[1] + temp[2] * temp[2]; // + temp[3] * temp[3];
                
                if ( pixelError < leastPixelError ) {
                    leastPixelError = pixelError;
                    bestM = mod;
                }
            }
            
            error += leastPixelError;
        }
    }
    
    return error;
}
