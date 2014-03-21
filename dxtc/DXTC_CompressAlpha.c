//
//  DXTC_CompressAlpha.c
//  GLTC
//
//  Created by Michael Kwasnicki on 27.02.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#include "DXTC_CompressAlpha.h"

#include "DXTC_Decompress.h"
#include "lib.h"

#include <iso646.h>
#include <string.h>



//FIXME: calling convert8888to4444 is unnecessary | make dithering optional
void compressDXT3BlockA( DXT3AlphaBlock_t * in_out_block, const rgba8_t in_BLOCK_RGBA[4][4] ) {
    DXT3AlphaBlock_t aBitField = 0x0L;
    rgba4444_t pixel;
    
    for ( int by = 0; by < 4; by++ ) {
        for ( int bx = 0; bx < 4; bx++ ) {
            if ( kDitherDXT3Alpha ) {
                rgba8_t dithered = in_BLOCK_RGBA[3-by][3-bx];
                ditherRGBA( &dithered, kRGBA4444, 3-by, 3-bx );
                convert8888to4444( &pixel, dithered );
            } else {
                convert8888to4444( &pixel, in_BLOCK_RGBA[3-by][3-bx] );
            }
            
            aBitField <<= 4;
            aBitField |= pixel.a;
        }
    }
    
    *in_out_block = aBitField;
}



uint32_t computeAlphaPaletteErrorDXT5( const uint8_t in_A0, const uint8_t in_A1, const rgba8_t in_BLOCK_RGBA[4][4] ) {
    uint32_t error = 0;
    uint8_t alphaPalette[8];
    
    computeDXT5APalette( alphaPalette, in_A0, in_A1 );
    
    for ( int by = 0; by < 4; by++ ) {
        for ( int bx = 0; bx < 4; bx++ ) {
            const uint8_t a = in_BLOCK_RGBA[by][bx].a;
            uint32_t bestPixelError = 0xFFFF;
            uint32_t pixelError[8];
            
            pixelError[0] = a - alphaPalette[0];
            pixelError[1] = a - alphaPalette[1];
            pixelError[2] = a - alphaPalette[2];
            pixelError[3] = a - alphaPalette[3];
            pixelError[4] = a - alphaPalette[4];
            pixelError[5] = a - alphaPalette[5];
            pixelError[6] = a - alphaPalette[6];
            pixelError[7] = a - alphaPalette[7];
            
            pixelError[0] *= pixelError[0];
            pixelError[1] *= pixelError[1];
            pixelError[2] *= pixelError[2];
            pixelError[3] *= pixelError[3];
            pixelError[4] *= pixelError[4];
            pixelError[5] *= pixelError[5];
            pixelError[6] *= pixelError[6];
            pixelError[7] *= pixelError[7];
            
            for ( int j = 0; j < 8; j++ ) {
                if ( pixelError[j] < bestPixelError ) {
                    bestPixelError = pixelError[j];
                }
            }
            
            error += bestPixelError;
        }
    }
    
    return error;
}



// iterative method (shift one end or pinch/expand both ends)
void findAlphaPaletteIterativeDXT5( uint8_t * in_out_bestA0, uint8_t * in_out_bestA1, uint32_t * in_out_bestError, const rgba8_t in_BLOCK_RGBA[4][4], const uint32_t in_MAX_RANGE, const bool in_MORE_AGGRESSIVE ) {
    uint32_t bestError = *in_out_bestError;
    uint8_t bestA0 = *in_out_bestA0;
    uint8_t bestA1 = *in_out_bestA1;
    uint8_t a0 = bestA0;
    uint8_t a1 = bestA1;
    uint8_t a0tmp = 0;
    uint8_t a1tmp = 0;
    uint32_t error = 0;
    uint8_t failCount = 0;
    int stepSize = 1;
    int8_t dirs[16] = {
         1,  0,
        -1,  0,
         0,  1,
         0, -1,
         1,  1,
        -1, -1,
        -1,  1,
         1, -1,
    };
    uint32_t currentDir = 0;
    uint32_t maxDir = ( in_MORE_AGGRESSIVE ) ? 8 : 4;
    
    // actually this is an array of bools but to reduce impact of memset we use separate bits
    uint32_t visited[2048]; // (256 * 256)entries / 32bits
    memset( visited, 0, 8192 );
    
    while ( true ) {
        a0tmp = clampi( a0 + dirs[currentDir * 2 + 0] * stepSize, 0, 255 );
        a1tmp = clampi( a1 + dirs[currentDir * 2 + 1] * stepSize, 0, 255 );
        uint16_t pos = a0tmp * 256 + a1tmp;
        
        if ( not ( visited[pos >> 5] bitand ( 1 << ( pos bitand 0x1F ) ) ) ) {
            visited[pos >> 5] |= ( 1 << ( pos bitand 0x1F ) );
            error = computeAlphaPaletteErrorDXT5( a0tmp, a1tmp, in_BLOCK_RGBA );
        }
        
        if ( error < bestError ) {
            a0 = a0tmp;
            a1 = a1tmp;
            failCount = 0;
            bestError = error;
            bestA0 = a0;
            bestA1 = a1;
            stepSize = 1;
            
            if ( bestError == 0 ) {
                break;
            }
        } else {
            failCount++;
            
            if ( failCount >= maxDir ) {
                stepSize++;
                failCount = 0;
                
                if ( stepSize > in_MAX_RANGE ) {
                    break;
                }
            }
            
            currentDir++;
            currentDir = ( currentDir >= maxDir ) ? 0 : currentDir;
        }
    }
    
    *in_out_bestA0 = bestA0;
    *in_out_bestA1 = bestA1;
    *in_out_bestError = bestError;
}



void findAlphaPaletteDXT5( uint8_t * out_a0, uint8_t * out_a1, const rgba8_t in_BLOCK_RGBA[4][4] ) {
    uint8_t aMin = 255;
    uint8_t aMinNonBorder = 255;
    uint8_t aMaxNonBorder = 0;
    uint8_t aMax = 0;
    
    for ( int by = 0; by < 4; by++ ) {
        for ( int bx = 0; bx < 4; bx++ ) {
            uint8_t a = in_BLOCK_RGBA[by][bx].a;
            aMin = ( a < aMin ) ? a : aMin;
            aMinNonBorder = ( ( a < aMinNonBorder ) and ( a !=   0 ) ) ? a : aMinNonBorder;
            aMaxNonBorder = ( ( a > aMaxNonBorder ) and ( a != 255 ) ) ? a : aMaxNonBorder;
            aMax = ( a > aMax ) ? a : aMax;
        }
    }
    
    uint32_t error = 0xFFFFFFFF;
    uint8_t a0 = 0;
    uint8_t a1 = 0;
    uint32_t bestError = 0xFFFFFFFF;
    uint8_t bestA0 = 0;
    uint8_t bestA1 = 0;
    
    // initial guess
    {
        // find closest colors a0 > a1
        error = computeAlphaPaletteErrorDXT5( aMax, aMin, in_BLOCK_RGBA );
        
        if ( error < bestError ) {
            bestError = error;
            bestA0 = aMax;
            bestA1 = aMin;
        }
        
        // find closest colors a0 <= a1
        error = computeAlphaPaletteErrorDXT5( aMinNonBorder, aMaxNonBorder, in_BLOCK_RGBA );
        
        if ( error < bestError ) {
            bestError = error;
            bestA0 = aMinNonBorder;
            bestA1 = aMaxNonBorder;
        }
    }
    
    uint32_t straightError = bestError;
    
    // iterative Method
    if ( true ) {
        uint32_t bestErrorTmp7 = 0xFFFFFFFF;
        uint8_t bestA0Tmp7 = aMax;
        uint8_t bestA1Tmp7 = aMin;
        uint32_t bestErrorTmp5 = 0xFFFFFFFF;
        uint8_t bestA0Tmp5 = aMinNonBorder;
        uint8_t bestA1Tmp5 = aMaxNonBorder;
        findAlphaPaletteIterativeDXT5( &bestA0Tmp7, &bestA1Tmp7, &bestErrorTmp7, in_BLOCK_RGBA, ( aMax - aMin + 7 ) / 7, false );
        findAlphaPaletteIterativeDXT5( &bestA0Tmp5, &bestA1Tmp5, &bestErrorTmp5, in_BLOCK_RGBA, ( aMaxNonBorder - aMinNonBorder + 5 ) / 5, false );
        
        if ( bestErrorTmp7 < bestErrorTmp5 ) {
            if ( bestErrorTmp7 < bestError ) {
                bestError = bestErrorTmp7;
                bestA0 = bestA0Tmp7;
                bestA1 = bestA1Tmp7;
            }
        } else {
            if ( bestErrorTmp5 < bestError ) {
                bestError = bestErrorTmp5;
                bestA0 = bestA0Tmp5;
                bestA1 = bestA1Tmp5;
            }
        }
    }
    
    straightError = bestError;
    
    // brute force
    if ( false ) {
        for ( int a1 = 0; a1 < 256; a1++ ) {
            for ( int a0 = 0; a0 < 256; a0++ ) {
                if ( ( ( a0 > a1 ) and ( ( a0 - a1 ) < 7 ) ) or ( a0 <= a1 ) and ( ( a1 - a0 ) < 5 ) ) {
                    continue;
                }
                
                uint32_t error = computeAlphaPaletteErrorDXT5( a0, a1, in_BLOCK_RGBA );
                
                if ( error < bestError ) {
                    bestError = error;
                    bestA0 = a0;
                    bestA1 = a1;
                }
                
                if ( bestError == 0 ) {
                    break;
                }
            }
            
            if ( bestError == 0 ) {
                break;
            }
        }
    }
    
    *out_a0 = bestA0;
    *out_a1 = bestA1;
    
    //    printf( "%3i %3i (%3i %3i) <-> %3i %3i (%6u < %6u ∆ = %6u)\n",
    //           aMin, aMax, aMinNonBorder, aMaxNonBorder, bestA0, bestA1, bestError, straightError, straightError - bestError );
    
}



// TODO: maybe introduce some dithering?
void compressDXT5BlockA( DXT5AlphaBlock_t * in_out_block, const rgba8_t in_BLOCK_RGBA[4][4] ) {
    uint8_t aMin = 255;
    uint8_t aMinNonBorder = 255;
    uint8_t aMaxNonBorder = 0;
    uint8_t aMax = 0;
    uint8_t a0 = 255;
    uint8_t a1 = 255;
    
    for ( int by = 0; by < 4; by++ ) {
        for ( int bx = 0; bx < 4; bx++ ) {
            uint8_t a = in_BLOCK_RGBA[by][bx].a;
            aMin = ( a < aMin ) ? a : aMin;
            aMinNonBorder = ( ( a < aMinNonBorder ) and ( a !=   0 ) ) ? a : aMinNonBorder;
            aMaxNonBorder = ( ( a > aMaxNonBorder ) and ( a != 255 ) ) ? a : aMaxNonBorder;
            aMax = ( a > aMax ) ? a : aMax;
        }
    }
    
    // this case is trivial
    if ( aMin == aMax ) {
        switch ( aMin ) {
            case 0:
                in_out_block->aBitField = 0x0LL;
                in_out_block->a0 = 0;
                in_out_block->a1 = 0;
                break;
                
            case 255:
                in_out_block->aBitField = 0xFFFFFFFFFFFFFFFFLL;
                in_out_block->a0 = 255;
                in_out_block->a1 = 255;
                break;
                
            default:
                in_out_block->aBitField = 0x0LL;
                in_out_block->a0 = aMin;
                in_out_block->a1 = 0;
                break;
        }
    } else {
        uint8_t delta7 = aMax - aMin;
        uint8_t delta5 = aMaxNonBorder - aMinNonBorder;
        delta5 = ( aMaxNonBorder ==   0 ) ? 255 : delta5;
        delta5 = ( aMinNonBorder == 255 ) ? 255 : delta5;
        
        // this case can be solved exactly
        if ( delta7 < 8 ) {
            if ( aMin + 7 > 255 ) {
                a1 = aMax - 7;
            } else {
                a1 = aMin;
            }
            
            a0 = a1 + 7;
        } else if ( delta5 < 6 ) {
            if ( aMinNonBorder + 5 > 255 ) {
                a0 = aMaxNonBorder - 5;
            } else {
                a0 = aMinNonBorder;
            }
            
            a1 = a0 + 5;
        } else {
            // do the hard work;
            findAlphaPaletteDXT5( &a0, &a1, in_BLOCK_RGBA );
        }
        
        uint8_t alphaPalette[8];
        computeDXT5APalette( alphaPalette, a0, a1 );
        
        uint64_t aBitField = 0x0LL;
        
        // find closest matching color from the palette
        for ( int by = 0; by < 4; by++ ) {
            for ( int bx = 0; bx < 4; bx++ ) {
                uint32_t bestPixelError = 0xFFFF;
                uint32_t bestIndex = 0;
                
                for ( int j = 0; j < 8; j++ ) {
                    uint32_t pixelError = in_BLOCK_RGBA[3-by][3-bx].a - alphaPalette[j];
                    pixelError *= pixelError;
                    
                    if ( pixelError < bestPixelError ) {
                        bestPixelError = pixelError;
                        bestIndex = j;
                    }
                }
                
                aBitField <<= 3;
                aBitField |= bestIndex;
            }
        }
        
        aBitField <<= 16;
        in_out_block->aBitField = aBitField;
        in_out_block->a0 = a0;
        in_out_block->a1 = a1;
    }
}
