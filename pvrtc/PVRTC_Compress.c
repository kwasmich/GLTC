//
//  PVRTC_Compress.c
//  GLTC
//
//  Created by Michael Kwasnicki on 16.03.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#include "PVRTC_Compress.h"

#include "../lib.h"
#include "../simplePNG.h"

#include <assert.h>
#include <iso646.h>
#include <stdbool.h>
#include <stdlib.h>


//static void buildBlock( ETCBlockColor_t * out_block, const rgb5_t in_C, const rgb3_t in_DC, const int in_T0, const int in_T1, const bool in_FLIP, const uint8_t in_MODULATION[4][4], const bool in_OPAQUE ) {
//    uint32_t bitField = generateBitField( in_MODULATION );
//    
//    ETCBlockD_t block;
//    block.differential = in_OPAQUE;
//    block.r = in_C.r;
//    block.g = in_C.g;
//    block.b = in_C.b;
//    block.dR = in_DC.r;
//    block.dG = in_DC.g;
//    block.dB = in_DC.b;
//    block.table0 = in_T0;
//    block.table1 = in_T1;
//    block.flip = in_FLIP;
//    block.cBitField = bitField;
//    out_block->b64 = block.b64;
//}



// assuming 0/8, 3/8, 5/8, 8/8 colors
uint32_t computeMetaBlockError( uint8_t out_modulation[4][4], rgba8_t out_rgba[7][7], const rgba8_t in_RGBA[7][7], const rgba8_t in_BASE_A[7][7], const rgba8_t in_BASE_B[7][7] ) {
    uint32_t blockError = 0;
    rgba8_t pixel;
    rgba8_t baseA;
    rgba8_t baseB;
    rgba8_t blend;
    uint32_t temp[4];
    
    static const int modulation[4] = { 0, 3, 5, 8 };

    for ( int mby = 0; mby < 7; mby++ ) {
        for ( int mbx = 0; mbx < 7; mbx++ ) {
            pixel = in_RGBA[mby][mbx];
            baseA = in_BASE_A[mby][mbx];
            baseB = in_BASE_B[mby][mbx];
            
            uint32_t lowestPixelError = 0xFFFFFFFF;
            
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
                
                if ( pixelError < lowestPixelError ) {
                    lowestPixelError = pixelError;
                    blend.r = ( baseA.r * mod + baseB.r * ( 8 - mod ) ) >> 3;
                    blend.g = ( baseA.g * mod + baseB.g * ( 8 - mod ) ) >> 3;
                    blend.b = ( baseA.b * mod + baseB.b * ( 8 - mod ) ) >> 3;
                    blend.a = ( baseA.a * mod + baseB.a * ( 8 - mod ) ) >> 3;
                    
                    if ( out_modulation )
                        out_modulation[mby][mbx] = m;
                    
                    if ( lowestPixelError == 0 )
                        break;
                }
            }
            
            if ( out_rgba )
                out_rgba[mby][mbx] = blend;
            
            blockError += lowestPixelError;
        }
    }
    
    return blockError;
}



static void bilinearFilter7x7( rgba8_t * out_rgba, const int in_X, const int in_Y, const rgba8_t in_BLOCK[3][3] ) {
    static const int w[3][7] = {
        { 3, 2, 1, 0, 0, 0, 0 },
        { 1, 2, 3, 4, 3, 2, 1 },
        { 0, 0, 0, 0, 1, 2, 3 }
    };
    
    for ( int i = 0; i < 4; i++ ) {
        uint32_t filtered = 0;
        filtered += in_BLOCK[0][0].array[i] * w[0][in_X] * w[0][in_Y];
        filtered += in_BLOCK[0][1].array[i] * w[1][in_X] * w[0][in_Y];
        filtered += in_BLOCK[0][2].array[i] * w[2][in_X] * w[0][in_Y];
        filtered += in_BLOCK[1][0].array[i] * w[0][in_X] * w[1][in_Y];
        filtered += in_BLOCK[1][1].array[i] * w[1][in_X] * w[1][in_Y];
        filtered += in_BLOCK[1][2].array[i] * w[2][in_X] * w[1][in_Y];
        filtered += in_BLOCK[2][0].array[i] * w[0][in_X] * w[2][in_Y];
        filtered += in_BLOCK[2][1].array[i] * w[1][in_X] * w[2][in_Y];
        filtered += in_BLOCK[2][2].array[i] * w[2][in_X] * w[2][in_Y];
        filtered >>= 4;
        out_rgba->array[i] = filtered;
    }
}



#pragma mark - exposed interface



void packMetaBlocksRGBA( LinearBlock7x7RGBA_t * out_image, const rgba8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT ) {
    assert( in_WIDTH % 4 == 0 );
    assert( in_HEIGHT % 4 == 0 );
    
    LinearBlock7x7RGBA_t * block_ptr = out_image;
    
    for ( int y = 0; y < in_HEIGHT; y += 4 ) {
        for ( int x = 0; x < in_WIDTH; x += 4 ) {
            for ( int mby = 0; mby < 7; mby++ ) {
                int dy = ( y + mby - 1 + in_HEIGHT ) % in_HEIGHT;
                
                for ( int mbx = 0; mbx < 7; mbx++ ) {
                    int dx = ( x + mbx - 1 + in_WIDTH ) % in_WIDTH;
                    block_ptr->block[mby][mbx] = in_IMAGE[dy * in_WIDTH + dx];
                }
            }
            
            block_ptr++;
        }
    }
}



void initialGuess( PVRTCIntermediateBlock_t * out_block, const rgba8_t in_RGBA[7][7] ) {
    static const int w[7] = { 1, 2, 3, 4, 3, 2, 1 };
    uint32_t avg4[4] = { 0, 0, 0, 0 };
    uint32_t min4[4] = { 0, 0, 0, 0 };
    uint32_t max4[4] = { 0, 0, 0, 0 };
    rgba8_t avg;
    rgba8_t min = { { 255, 255, 255, 255 } };
    rgba8_t max = { { 0, 0, 0, 0 } };
    
    for ( int mby = 0; mby < 7; mby++ ) {
        for ( int mbx = 0; mbx < 7; mbx++ ) {
            for ( int i = 0; i < 4; i++ ) {
                avg4[i] += in_RGBA[mby][mbx].array[i] * w[mby] * w[mbx];
                min.array[i] = ( in_RGBA[mby][mbx].array[i] < min.array[i] ) ? in_RGBA[mby][mbx].array[i] : min.array[i];
                max.array[i] = ( in_RGBA[mby][mbx].array[i] > max.array[i] ) ? in_RGBA[mby][mbx].array[i] : max.array[i];
            }
        }
    }
    
    for ( int i = 0; i < 4; i++ ) {
        //avg.array[i] = avg4[i] / 49;
        avg.array[i] = avg4[i] >> 8;
    }
    
    uint32_t avgLength = avg.r * avg.r + avg.g * avg.g + avg.b * avg.b;
    uint32_t numMin = 0;
    uint32_t numMax = 0;
    
    for ( int mby = 0; mby < 7; mby++ ) {
        for ( int mbx = 0; mbx < 7; mbx++ ) {
            rgba8_t c = in_RGBA[mby][mbx];
            uint32_t pixelLength = c.r * c.r + c.g * c.g + c.b * c.b;
            
            if ( pixelLength < avgLength ) {
                for ( int i = 0; i < 4; i++ ) {
                    min4[i] += c.array[i];
                }
                
                numMin++;
            } else {
                for ( int i = 0; i < 4; i++ ) {
                    max4[i] += c.array[i];
                }
                
                numMax++;
            }
        }
    }
    
    if ( numMin ) {
        for ( int i = 0; i < 4; i++ ) {
            min.array[i] = min4[i] / numMin;
        }
    } else {
        min = avg;
    }
    
    if ( numMax ) {
        for ( int i = 0; i < 4; i++ ) {
            max.array[i] = max4[i] / numMax;
        }
    } else {
        max = avg;
    }
    
    out_block->a = max;
    out_block->b = min;
}


void fakeCompress4BPP( rgba8_t out_rgba[4][4], const rgba8_t in_RGBA[7][7], const PVRTCIntermediateBlock_t in_BLOCKS[3][3] ) {
    rgba8_t a[3][3];
    rgba8_t b[3][3];
    rgba8_t baseA[7][7];
    rgba8_t baseB[7][7];
    rgba8_t out[7][7];
    
    for ( int by = 0; by < 3; by++ ) {
        for ( int bx = 0; bx < 3; bx++ ) {
            a[by][bx] = in_BLOCKS[by][bx].a;
            b[by][bx] = in_BLOCKS[by][bx].b;
        }
    }
    
    for ( int mby = 0; mby < 7; mby++ ) {
        for ( int mbx = 0; mbx < 7; mbx++ ) {
            bilinearFilter7x7( &baseA[mby][mbx], mbx, mby, a );
            bilinearFilter7x7( &baseB[mby][mbx], mbx, mby, b );
        }
    }
    
    computeMetaBlockError( NULL, out, in_RGBA, baseA, baseB );

    for ( int by = 0; by < 4; by++ ) {
        for ( int bx = 0; bx < 4; bx++ ) {
            out_rgba[by][bx] = out[by+1][bx+1];
        }
    }
}
