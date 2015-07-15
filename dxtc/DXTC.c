//
//  DXTC.c
//  GLTC
//
//  Created by Michael Kwasnicki on 14.02.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#include "DXTC.h"

#include "DXTC_Common.h"
#include "DXTC_CompressAlpha.h"
#include "DXTC_Decompress.h"
#include "../lib.h"

#include <assert.h>
#include <math.h>
#include <iso646.h>
#include <stdio.h>



#pragma mark - block compression helper



typedef enum {
    kZERO_THIRDS, kTHREE_THIRDS, kONE_THIRD, kTWO_THIRDS, kZERO_HALFS, kTWO_HALFS, kONE_HALF, kINVALID
} colorCompositionCase_t;



typedef struct {
    uint8_t c0;
    uint8_t c1;
    uint8_t error;
    uint8_t delta;
} colorComposition_t;



colorComposition_t COLOR_LUT_5[kINVALID][256];
colorComposition_t COLOR_LUT_6[kINVALID][256];



static void computeUniformColorsLUT() {
    uint8_t c0 = 0;
    uint8_t c1 = 0;
    uint8_t c2 = 0;
    uint8_t c3 = 0;
    uint8_t delta = 0;
    
    colorComposition_t tmp = { 0, 0, 255, 255 };
    
    for ( int j = 0; j < kINVALID; j++ ) {
        for ( int i = 0; i < 256; i++ ) {
            COLOR_LUT_5[j][i] = tmp;
            COLOR_LUT_6[j][i] = tmp;
        }
    }
    
    for ( int c5a = 1; c5a < 32; c5a++ ) {
        for ( int c5b = 0; c5b < c5a; c5b++ ) {
            c0 = ( c5a << 3 ) bitor ( c5a >> 2 );
            c1 = ( c5b << 3 ) bitor ( c5b >> 2 );
            delta = c0 - c1;

            if ( not kFloatingPoint ) {
                c2 = ( 2 * c0 + c1 ) / 3;
                c3 = ( c0 + 2 * c1 ) / 3;
            } else {
                // using ANSI round-to-floor to get round-to-nearest
                c2 = (uint8_t)( ( (float)( 2 * c0 + c1 ) / 3.0f ) + 0.5f );
                c3 = (uint8_t)( ( (float)( c0 + 2 * c1 ) / 3.0f ) + 0.5f );
            }
            
            if ( delta < COLOR_LUT_5[kZERO_THIRDS][c0].delta ) {
                COLOR_LUT_5[kZERO_THIRDS][c0].c0 = c5a;
                COLOR_LUT_5[kZERO_THIRDS][c0].c1 = c5b;
                COLOR_LUT_5[kZERO_THIRDS][c0].error = 0;
                COLOR_LUT_5[kZERO_THIRDS][c0].delta = delta;
            }
            
            if ( delta < COLOR_LUT_5[kONE_THIRD][c2].delta ) {
                COLOR_LUT_5[kONE_THIRD][c2].c0 = c5a;
                COLOR_LUT_5[kONE_THIRD][c2].c1 = c5b;
                COLOR_LUT_5[kONE_THIRD][c2].error = 0;
                COLOR_LUT_5[kONE_THIRD][c2].delta = delta;
            }
            
            if ( delta < COLOR_LUT_5[kTWO_THIRDS][c3].delta ) {
                COLOR_LUT_5[kTWO_THIRDS][c3].c0 = c5a;
                COLOR_LUT_5[kTWO_THIRDS][c3].c1 = c5b;
                COLOR_LUT_5[kTWO_THIRDS][c3].error = 0;
                COLOR_LUT_5[kTWO_THIRDS][c3].delta = delta;
            }
            
            if ( delta < COLOR_LUT_5[kTHREE_THIRDS][c1].delta ) {
                COLOR_LUT_5[kTHREE_THIRDS][c1].c0 = c5a;
                COLOR_LUT_5[kTHREE_THIRDS][c1].c1 = c5b;
                COLOR_LUT_5[kTHREE_THIRDS][c1].error = 0;
                COLOR_LUT_5[kTHREE_THIRDS][c1].delta = delta;
            }
        }
    }
    
    for ( int c5a = 0; c5a < 32; c5a++ ) {
        for ( int c5b = c5a; c5b < 32; c5b++ ) {
            c0 = ( c5a << 3 ) bitor ( c5a >> 2 );
            c1 = ( c5b << 3 ) bitor ( c5b >> 2 );
            delta = c1 - c0;
            
            if ( not kFloatingPoint ) {
                c2 = ( c0 + c1 ) / 2;
            } else {
                // using ANSI round-to-floor to get round-to-nearest
                c2 = (uint8_t)( ( (float)( c0 + c1 ) * 0.5f ) + 0.5f );
            }
            
            if ( delta < COLOR_LUT_5[kZERO_HALFS][c0].delta ) {
                COLOR_LUT_5[kZERO_HALFS][c0].c0 = c5a;
                COLOR_LUT_5[kZERO_HALFS][c0].c1 = c5a;
                COLOR_LUT_5[kZERO_HALFS][c0].error = 0;
                COLOR_LUT_5[kZERO_HALFS][c0].delta = delta;
            }
            
            if ( delta < COLOR_LUT_5[kONE_HALF][c2].delta ) {
                COLOR_LUT_5[kONE_HALF][c2].c0 = c5a;
                COLOR_LUT_5[kONE_HALF][c2].c1 = c5b;
                COLOR_LUT_5[kONE_HALF][c2].error = 0;
                COLOR_LUT_5[kONE_HALF][c2].delta = delta;
            }
            
            if ( delta < COLOR_LUT_5[kTWO_HALFS][c1].delta ) {
                COLOR_LUT_5[kTWO_HALFS][c1].c0 = c5b;
                COLOR_LUT_5[kTWO_HALFS][c1].c1 = c5b;
                COLOR_LUT_5[kTWO_HALFS][c1].error = 0;
                COLOR_LUT_5[kTWO_HALFS][c1].delta = delta;
            }
        }
    }
    
    for ( int c6a = 1; c6a < 64; c6a++ ) {
        for ( int c6b = 0; c6b < c6a; c6b++ ) {
            c0 = ( c6a << 2 ) bitor ( c6a >> 4 );
            c1 = ( c6b << 2 ) bitor ( c6b >> 4 );
            delta = c0 - c1;
            
            if ( not kFloatingPoint ) {
                c2 = ( 2 * c0 + c1 ) / 3;
                c3 = ( c0 + 2 * c1 ) / 3;
            } else {
                // using ANSI round-to-floor to get round-to-nearest
                c2 = (uint8_t)( ( (float)( 2 * c0 + c1 ) / 3.0f ) + 0.5f );
                c3 = (uint8_t)( ( (float)( c0 + 2 * c1 ) / 3.0f ) + 0.5f );
            }
            
            if ( delta < COLOR_LUT_6[kZERO_THIRDS][c0].delta ) {
                COLOR_LUT_6[kZERO_THIRDS][c0].c0 = c6a;
                COLOR_LUT_6[kZERO_THIRDS][c0].c1 = c6b;
                COLOR_LUT_6[kZERO_THIRDS][c0].error = 0;
                COLOR_LUT_6[kZERO_THIRDS][c0].delta = delta;
            }
            
            if ( delta < COLOR_LUT_6[kONE_THIRD][c2].delta ) {
                COLOR_LUT_6[kONE_THIRD][c2].c0 = c6a;
                COLOR_LUT_6[kONE_THIRD][c2].c1 = c6b;
                COLOR_LUT_6[kONE_THIRD][c2].error = 0;
                COLOR_LUT_6[kONE_THIRD][c2].delta = delta;
            }
            
            if ( delta < COLOR_LUT_6[kTWO_THIRDS][c3].delta ) {
                COLOR_LUT_6[kTWO_THIRDS][c3].c0 = c6a;
                COLOR_LUT_6[kTWO_THIRDS][c3].c1 = c6b;
                COLOR_LUT_6[kTWO_THIRDS][c3].error = 0;
                COLOR_LUT_6[kTWO_THIRDS][c3].delta = delta;
            }
            
            if ( delta < COLOR_LUT_6[kTHREE_THIRDS][c1].delta ) {
                COLOR_LUT_6[kTHREE_THIRDS][c1].c0 = c6a;
                COLOR_LUT_6[kTHREE_THIRDS][c1].c1 = c6b;
                COLOR_LUT_6[kTHREE_THIRDS][c1].error = 0;
                COLOR_LUT_6[kTHREE_THIRDS][c1].delta = delta;
            }
        }
    }
    
    for ( int c6a = 0; c6a < 64; c6a++ ) {
        for ( int c6b = c6a; c6b < 64; c6b++ ) {
            c0 = ( c6a << 2 ) bitor ( c6a >> 4 );
            c1 = ( c6b << 2 ) bitor ( c6b >> 4 );
            delta = c1 - c0;
            
            if ( not kFloatingPoint ) {
                c2 = ( c0 + c1 ) / 2;
            } else {
                // using ANSI round-to-floor to get round-to-nearest
                c2 = (uint8_t)( ( (float)( c0 + c1 ) * 0.5f ) + 0.5f );
            }
            
            if ( delta < COLOR_LUT_6[kZERO_HALFS][c0].delta ) {
                COLOR_LUT_6[kZERO_HALFS][c0].c0 = c6a;
                COLOR_LUT_6[kZERO_HALFS][c0].c1 = c6a;
                COLOR_LUT_6[kZERO_HALFS][c0].error = 0;
                COLOR_LUT_6[kZERO_HALFS][c0].delta = delta;
            }
            
            if ( delta < COLOR_LUT_6[kONE_HALF][c2].delta ) {
                COLOR_LUT_6[kONE_HALF][c2].c0 = c6a;
                COLOR_LUT_6[kONE_HALF][c2].c1 = c6b;
                COLOR_LUT_6[kONE_HALF][c2].error = 0;
                COLOR_LUT_6[kONE_HALF][c2].delta = delta;
            }
            
            if ( delta < COLOR_LUT_6[kTWO_HALFS][c1].delta ) {
                COLOR_LUT_6[kTWO_HALFS][c1].c0 = c6b;
                COLOR_LUT_6[kTWO_HALFS][c1].c1 = c6b;
                COLOR_LUT_6[kTWO_HALFS][c1].error = 0;
                COLOR_LUT_6[kTWO_HALFS][c1].delta = delta;
            }
        }
    }

    // fill the gaps
    for ( int j = 0; j < kINVALID; j++ ) {
        int distance = 128;
        tmp.error = 255;
        
        // sweep backward
        for ( int i = 255; i >= 0; i-- ) {
            if ( COLOR_LUT_5[j][i].error == 0 ) {
                tmp = COLOR_LUT_5[j][i];
                distance = 0;
            } else {
                distance++;
                
                if ( tmp.error != 255 ) {
                    tmp.error = distance * distance;
                }
                
                if ( tmp.error < COLOR_LUT_5[j][i].error ) {
                    COLOR_LUT_5[j][i] = tmp;
                }
            }
        }
        
        distance = 128;
        tmp.error = 255;
        
        // sweep backward
        for ( int i = 255; i >= 0; i-- ) {
            if ( COLOR_LUT_6[j][i].error == 0 ) {
                tmp = COLOR_LUT_6[j][i];
                distance = 0;
            } else {
                distance++;
                
                if ( tmp.error != 255 ) {
                    tmp.error = distance * distance;
                }
                
                if ( tmp.error < COLOR_LUT_6[j][i].error ) {
                    COLOR_LUT_6[j][i] = tmp;
                }
            }
        }
        
        distance = 128;
        tmp.error = 255;
        
        // sweep forward
        for ( int i = 0; i < 256; i++ ) {
            if ( COLOR_LUT_5[j][i].error == 0 ) {
                tmp = COLOR_LUT_5[j][i];
                distance = 0;
            } else {
                distance++;
                
                if ( tmp.error != 255 ) {
                    tmp.error = distance * distance;
                }
                
                if ( tmp.error < COLOR_LUT_5[j][i].error ) {
                    COLOR_LUT_5[j][i] = tmp;
                }
            }
        }
        
        distance = 128;
        tmp.error = 255;
        
        // sweep forward
        for ( int i = 0; i < 256; i++ ) {
            if ( COLOR_LUT_6[j][i].error == 0 ) {
                tmp = COLOR_LUT_6[j][i];
                distance = 0;
            } else {
                distance++;
                
                if ( tmp.error != 255 ) {
                    tmp.error = distance * distance;
                }
                
                if ( tmp.error < COLOR_LUT_6[j][i].error ) {
                    COLOR_LUT_6[j][i] = tmp;
                }
            }
        }
    }
    
    /*
    printf( "        0   1   ⅓   ⅔   0   1   ½ |   0   1   ⅓   ⅔   0   1   ½\n" );
    
    for ( int i = 0; i < 256; i++ ) {
        printf( "%3i : ", i );
        
        for ( int j = 0; j < kINVALID; j++ ) {
            printf( "%3i ", COLOR_LUT_5[j][i].error );
        }
        
        printf( "| " );
        
        for ( int j = 0; j < kINVALID; j++ ) {
            printf( "%3i ", COLOR_LUT_6[j][i].error );
        }
        
        printf( "\n" );
    }
    
    for ( int i = 0; i < 256; i++ ) {
        printf( "%3i : ", i );
        const int j = kTWO_THIRDS; 
        printf( "%3i (%2i %2i) %3i", COLOR_LUT_5[j][i].error, COLOR_LUT_5[j][i].c0, COLOR_LUT_5[j][i].c1, COLOR_LUT_5[j][i].delta );
        
        printf( "\n" );
    }
    
    exit( -1 );
     */
}


static uint32_t computeColorPaletteErrorDXT1RGB( const rgb565_t in_C0, const rgb565_t in_C1, const rgb8_t in_BLOCK_RGB[4][4] ) {
    rgb8_t palette[4];
    uint32_t error = 0xFFFFFFFF;
    uint32_t lowestPixelError = 0xFFFFFFFF;
    uint32_t pixelError = 0;
    uint32_t temp[3] = { 0, 0, 0 };
    computeDXT1RGBPalette( palette, in_C0, in_C1 );
    
    for ( int by = 0; by < 4; by++ ) {
        for ( int bx = 0; bx < 4; bx++ ) {
            rgb8_t pixel = in_BLOCK_RGB[by][bx];
            lowestPixelError = 0xFFFFFFFF;
            
            for ( int p = 0; p < 4; p++ ) {
                temp[0] = pixel.r - palette[p].r;
                temp[1] = pixel.g - palette[p].g;
                temp[2] = pixel.b - palette[p].b;
                pixelError = temp[0] * temp[0] + temp[1] * temp[1] + temp[2] * temp[2];
                
                if ( pixelError < lowestPixelError ) {
                    lowestPixelError = pixelError;
                }
            }
            
            error += lowestPixelError;
        }
    }
    
    return error;
}



static uint32_t computeColorPaletteErrorDXT1RGBAOpaque( const rgb565_t in_C0, const rgb565_t in_C1, const rgba8_t in_BLOCK_RGBA[4][4] ) {
    rgba8_t palette[4];
    uint32_t error = 0;
    uint32_t lowestPixelError = 0xFFFFFFFF;
    uint32_t pixelError = 0;
    uint32_t temp[3];
    const int range = ( in_C0.b16 <= in_C1.b16 ) ? 3 : 4;
    
    computeDXT1RGBAPalette( palette, in_C0, in_C1 );
    
    for ( int by = 0; by < 4; by++ ) {
        for ( int bx = 0; bx < 4; bx++ ) {
            rgba8_t pixel = in_BLOCK_RGBA[by][bx];
            lowestPixelError = 0xFFFFFFFF;
            
            for ( int p = 0; p < range; p++ ) {
                temp[0] = pixel.r - palette[p].r;
                temp[1] = pixel.g - palette[p].g;
                temp[2] = pixel.b - palette[p].b;
                pixelError = temp[0] * temp[0] + temp[1] * temp[1] + temp[2] * temp[2];
                
                if ( pixelError < lowestPixelError ) {
                    lowestPixelError = pixelError;
                }
            }
            
            error += lowestPixelError;
        }
    }
    
    return error;
}



static uint32_t computeColorPaletteErrorDXT1RGBATransparent( const rgb565_t in_C0, const rgb565_t in_C1, const rgba8_t in_BLOCK_RGBA[4][4] ) {
    rgba8_t palette[4];
    uint32_t error = 0;
    uint32_t lowestPixelError = 0xFFFFFFFF;
    uint32_t pixelError = 0;
    uint32_t temp[3];
    
    if ( in_C0.b16 > in_C1.b16 ) {
        return 0xFFFFFFFF;
    }
    
    computeDXT1RGBAPalette( palette, in_C0, in_C1 );
    
    for ( int by = 0; by < 4; by++ ) {
        for ( int bx = 0; bx < 4; bx++ ) {
            rgba8_t pixel = in_BLOCK_RGBA[by][bx];
            lowestPixelError = 0xFFFFFFFF;
            
            if ( pixel.a == 0 ) {
                lowestPixelError = 0;
            } else {
                for ( int p = 0; p < 3; p++ ) {
                    temp[0] = pixel.r - palette[p].r;
                    temp[1] = pixel.g - palette[p].g;
                    temp[2] = pixel.b - palette[p].b;
                    pixelError = temp[0] * temp[0] + temp[1] * temp[1] + temp[2] * temp[2];
                    
                    if ( pixelError < lowestPixelError ) {
                        lowestPixelError = pixelError;
                    }
                }
            }
            
            error += lowestPixelError;
        }
    }
    
    return error;
}



static uint32_t computeColorPaletteErrorDXT3( const rgb565_t in_C0, const rgb565_t in_C1, const rgba8_t in_BLOCK_RGBA[4][4] ) {
    rgba8_t palette[4];
    uint32_t error = 0;
    uint32_t lowestPixelError = 0xFFFFFFFF;
    uint32_t pixelError = 0;
    uint32_t temp[3];
    
    if ( in_C0.b16 <= in_C1.b16 ) {
        return 0xFFFFFFFF;
    }
    
    computeDXT3RGBPalette( palette, in_C0, in_C1 );
    
    for ( int by = 0; by < 4; by++ ) {
        for ( int bx = 0; bx < 4; bx++ ) {
            rgba8_t pixel = in_BLOCK_RGBA[by][bx];
            lowestPixelError = 0xFFFFFFFF;
            
            for ( int p = 0; p < 4; p++ ) {
                // Lets weight pixels with low alpha less (div 8 to avoid overflow (256 * 256)^2 / 64 * 3 * 16)
                temp[0] = ( pixel.r - palette[p].r ) * pixel.a;
                temp[1] = ( pixel.g - palette[p].g ) * pixel.a;
                temp[2] = ( pixel.b - palette[p].b ) * pixel.a;
                temp[0] = ( temp[0] * temp[0] ) >> 6;
                temp[1] = ( temp[1] * temp[1] ) >> 6;
                temp[2] = ( temp[2] * temp[2] ) >> 6;
                pixelError = temp[0] + temp[1] + temp[2];
                
                if ( pixelError < lowestPixelError ) {
                    lowestPixelError = pixelError;
                }
            }
            
            error += lowestPixelError;
        }
    }
    
    return error;
}



// stupid linear force (r g b separately but in fact there are only 4 colors with rgb combined)
static void findColorPaletteBrutePerColorDXT1RGB( rgb565_t * in_out_bestC0, rgb565_t * in_out_bestC1, uint32_t * in_out_bestError, const rgb8_t in_BLOCK_RGB[4][4] ) {
    uint32_t error;
    uint32_t bruteError = 0xFFFFFFFF;
    rgb565_t bruteC0 = *in_out_bestC0;
    rgb565_t bruteC1 = *in_out_bestC1;
    rgb565_t c0;
    rgb565_t c1;
    
    bool improved = true;
    
    while ( improved ) {
        improved = false;
        c0 = bruteC0;
        c1 = bruteC1;
        bruteError = computeColorPaletteErrorDXT1RGB( bruteC0, bruteC1, in_BLOCK_RGB );
        
        for ( int r0 = 0; r0 < 32; r0++ ) {
            for ( int r1 = 0; r1 < 32; r1++ ) {
                c0.r = r0;
                c1.r = r1;
                
                error = computeColorPaletteErrorDXT1RGB( c0, c1, in_BLOCK_RGB );
                
                if ( error < bruteError ) {
                    bruteError = error;
                    bruteC0.r = r0;
                    bruteC1.r = r1;
                    improved = true;
                }
            }
        }
        
        c0 = bruteC0;
        c1 = bruteC1;
        bruteError = computeColorPaletteErrorDXT1RGB( bruteC0, bruteC1, in_BLOCK_RGB );
        
        for ( int g0 = 0; g0 < 64; g0++ ) {
            for ( int g1 = 0; g1 < 64; g1++ ) {
                c0.g = g0;
                c1.g = g1;
                
                error = computeColorPaletteErrorDXT1RGB( c0, c1, in_BLOCK_RGB );
                
                if ( error < bruteError ) {
                    bruteError = error;
                    bruteC0.g = g0;
                    bruteC1.g = g1;
                    improved = true;
                }
            }
        }
        
        c0 = bruteC0;
        c1 = bruteC1;
        bruteError = computeColorPaletteErrorDXT1RGB( bruteC0, bruteC1, in_BLOCK_RGB );
        
        for ( int b0 = 0; b0 < 32; b0++ ) {
            for ( int b1 = 0; b1 < 32; b1++ ) {
                c0.b = b0;
                c1.b = b1;
                
                error = computeColorPaletteErrorDXT1RGB( c0, c1, in_BLOCK_RGB );
                
                if ( error < bruteError ) {
                    bruteError = error;
                    bruteC0.b = b0;
                    bruteC1.b = b1;
                    improved = true;
                }
            }
        }
    }
    
    if ( bruteError < *in_out_bestError ) {
        *in_out_bestError = bruteError;
        *in_out_bestC0 = bruteC0;
        *in_out_bestC1 = bruteC1;
    }
}



// stupid linear force (r g b separately but in fact there are only 4 colors with rgb combined)
static void findColorPaletteBrutePerColorDXT1RGBAOpaque( rgb565_t * in_out_bestC0, rgb565_t * in_out_bestC1, uint32_t * in_out_bestError, const rgba8_t in_BLOCK_RGBA[4][4] ) {
    uint32_t error;
    uint32_t bruteError = 0xFFFFFFFF;
    rgb565_t bruteC0 = *in_out_bestC0;
    rgb565_t bruteC1 = *in_out_bestC1;
    rgb565_t c0;
    rgb565_t c1;
    
    bool improved = true;
    
    while ( improved ) {
        improved = false;
        c0 = bruteC0;
        c1 = bruteC1;
        bruteError = computeColorPaletteErrorDXT1RGBAOpaque( bruteC0, bruteC1, in_BLOCK_RGBA );
        
        for ( int r0 = 0; r0 < 32; r0++ ) {
            for ( int r1 = 0; r1 < 32; r1++ ) {
                c0.r = r0;
                c1.r = r1;
                
                error = computeColorPaletteErrorDXT1RGBAOpaque( c0, c1, in_BLOCK_RGBA );
                
                if ( error < bruteError ) {
                    bruteError = error;
                    bruteC0.r = r0;
                    bruteC1.r = r1;
                    improved = true;
                }
            }
        }
        
        c0 = bruteC0;
        c1 = bruteC1;
        bruteError = computeColorPaletteErrorDXT1RGBAOpaque( bruteC0, bruteC1, in_BLOCK_RGBA );
        
        for ( int g0 = 0; g0 < 64; g0++ ) {
            for ( int g1 = 0; g1 < 64; g1++ ) {
                c0.g = g0;
                c1.g = g1;
                
                error = computeColorPaletteErrorDXT1RGBAOpaque( c0, c1, in_BLOCK_RGBA );
                
                if ( error < bruteError ) {
                    bruteError = error;
                    bruteC0.g = g0;
                    bruteC1.g = g1;
                    improved = true;
                }
            }
        }
        
        c0 = bruteC0;
        c1 = bruteC1;
        bruteError = computeColorPaletteErrorDXT1RGBAOpaque( bruteC0, bruteC1, in_BLOCK_RGBA );
        
        for ( int b0 = 0; b0 < 32; b0++ ) {
            for ( int b1 = 0; b1 < 32; b1++ ) {
                c0.b = b0;
                c1.b = b1;
                
                error = computeColorPaletteErrorDXT1RGBAOpaque( c0, c1, in_BLOCK_RGBA );
                
                if ( error < bruteError ) {
                    bruteError = error;
                    bruteC0.b = b0;
                    bruteC1.b = b1;
                    improved = true;
                }
            }
        }
    }
    
    if ( bruteError < *in_out_bestError ) {
        *in_out_bestError = bruteError;
        *in_out_bestC0 = bruteC0;
        *in_out_bestC1 = bruteC1;
    }
}



// stupid linear force (r g b separately but in fact there are only 4 colors with rgb combined)
static void findColorPaletteBrutePerColorDXT1RGBATransparent( rgb565_t * in_out_bestC0, rgb565_t * in_out_bestC1, uint32_t * in_out_bestError, const rgba8_t in_BLOCK_RGBA[4][4] ) {
    uint32_t error;
    uint32_t bruteError = 0xFFFFFFFF;
    rgb565_t bruteC0 = *in_out_bestC0;
    rgb565_t bruteC1 = *in_out_bestC1;
    rgb565_t c0;
    rgb565_t c1;
    
    bool improved = true;
    
    while ( improved ) {
        improved = false;
        c0 = bruteC0;
        c1 = bruteC1;
        bruteError = computeColorPaletteErrorDXT1RGBATransparent( bruteC0, bruteC1, in_BLOCK_RGBA );
        
        for ( int r0 = 0; r0 < 32; r0++ ) {
            for ( int r1 = 0; r1 < 32; r1++ ) {
                c0.r = r0;
                c1.r = r1;
                
                error = computeColorPaletteErrorDXT1RGBATransparent( c0, c1, in_BLOCK_RGBA );
                
                if ( error < bruteError ) {
                    bruteError = error;
                    bruteC0.r = r0;
                    bruteC1.r = r1;
                    improved = true;
                }
            }
        }
        
        c0 = bruteC0;
        c1 = bruteC1;
        bruteError = computeColorPaletteErrorDXT1RGBATransparent( bruteC0, bruteC1, in_BLOCK_RGBA );
        
        for ( int g0 = 0; g0 < 64; g0++ ) {
            for ( int g1 = 0; g1 < 64; g1++ ) {
                c0.g = g0;
                c1.g = g1;
                
                error = computeColorPaletteErrorDXT1RGBATransparent( c0, c1, in_BLOCK_RGBA );
                
                if ( error < bruteError ) {
                    bruteError = error;
                    bruteC0.g = g0;
                    bruteC1.g = g1;
                    improved = true;
                }
            }
        }
        
        c0 = bruteC0;
        c1 = bruteC1;
        bruteError = computeColorPaletteErrorDXT1RGBATransparent( bruteC0, bruteC1, in_BLOCK_RGBA );
        
        for ( int b0 = 0; b0 < 32; b0++ ) {
            for ( int b1 = 0; b1 < 32; b1++ ) {
                c0.b = b0;
                c1.b = b1;
                
                error = computeColorPaletteErrorDXT1RGBATransparent( c0, c1, in_BLOCK_RGBA );
                
                if ( error < bruteError ) {
                    bruteError = error;
                    bruteC0.b = b0;
                    bruteC1.b = b1;
                    improved = true;
                }
            }
        }
    }
    
    if ( bruteError < *in_out_bestError ) {
        *in_out_bestError = bruteError;
        *in_out_bestC0 = bruteC0;
        *in_out_bestC1 = bruteC1;
    }
}



// stupid linear force (r g b separately but in fact there are only 4 colors with rgb combined)
static void findColorPaletteBrutePerColorDXT3( rgb565_t * in_out_bestC0, rgb565_t * in_out_bestC1, uint32_t * in_out_bestError, const rgba8_t in_BLOCK_RGBA[4][4] ) {
    uint32_t error;
    uint32_t bruteError = 0xFFFFFFFF;
    rgb565_t bruteC0 = *in_out_bestC0;
    rgb565_t bruteC1 = *in_out_bestC1;
    rgb565_t c0;
    rgb565_t c1;
    
    bool improved = true;
    
    while ( improved ) {
        improved = false;
        c0 = bruteC0;
        c1 = bruteC1;
        bruteError = computeColorPaletteErrorDXT3( bruteC0, bruteC1, in_BLOCK_RGBA );
        
        for ( int r0 = 0; r0 < 32; r0++ ) {
            for ( int r1 = 0; r1 < 32; r1++ ) {
                c0.r = r0;
                c1.r = r1;
                
                error = computeColorPaletteErrorDXT3( c0, c1, in_BLOCK_RGBA );
                
                if ( error < bruteError ) {
                    bruteError = error;
                    bruteC0.r = r0;
                    bruteC1.r = r1;
                    improved = true;
                }
            }
        }
        
        c0 = bruteC0;
        c1 = bruteC1;
        bruteError = computeColorPaletteErrorDXT3( bruteC0, bruteC1, in_BLOCK_RGBA );
        
        for ( int g0 = 0; g0 < 64; g0++ ) {
            for ( int g1 = 0; g1 < 64; g1++ ) {
                c0.g = g0;
                c1.g = g1;
                
                error = computeColorPaletteErrorDXT3( c0, c1, in_BLOCK_RGBA );
                
                if ( error < bruteError ) {
                    bruteError = error;
                    bruteC0.g = g0;
                    bruteC1.g = g1;
                    improved = true;
                }
            }
        }
        
        c0 = bruteC0;
        c1 = bruteC1;
        bruteError = computeColorPaletteErrorDXT3( bruteC0, bruteC1, in_BLOCK_RGBA );
        
        for ( int b0 = 0; b0 < 32; b0++ ) {
            for ( int b1 = 0; b1 < 32; b1++ ) {
                c0.b = b0;
                c1.b = b1;
                
                error = computeColorPaletteErrorDXT3( c0, c1, in_BLOCK_RGBA );
                
                if ( error < bruteError ) {
                    bruteError = error;
                    bruteC0.b = b0;
                    bruteC1.b = b1;
                    improved = true;
                }
            }
        }
    }
    
    if ( bruteError < *in_out_bestError ) {
        *in_out_bestError = bruteError;
        *in_out_bestC0 = bruteC0;
        *in_out_bestC1 = bruteC1;
        assert( in_out_bestC0->b16 > in_out_bestC1->b16 );
    }
}



// iterative method (shift one end or pinch/expand both ends)
static void findColorPaletteIterativeDXT1RGB( rgb565_t * in_out_bestC0, rgb565_t * in_out_bestC1, uint32_t * in_out_bestError, const rgb8_t in_BLOCK_RGB[4][4], const uint32_t in_MAX_RANGE, const bool in_MORE_AGGRESSIVE ) {
    uint32_t bestError = *in_out_bestError;
    rgb565_t bestC0 = *in_out_bestC0;
    rgb565_t bestC1 = *in_out_bestC1;
    rgb565_t c0 = bestC0;
    rgb565_t c1 = bestC1;
    rgb565_t c0tmp = RGB( 0, 0, 0 );
    rgb565_t c1tmp = RGB( 0, 0, 0 );
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
    uint32_t currentColor = 0;
    
    
    while ( true ) {
        c0tmp = c0;
        c1tmp = c1;
        
        switch ( currentColor ) {
            case 0:
                c0tmp.r = clampi( c0.r + dirs[currentDir * 2 + 0] * stepSize, 0, 31 );
                c1tmp.r = clampi( c1.r + dirs[currentDir * 2 + 1] * stepSize, 0, 31 );
                break;
                
            case 1:
                c0tmp.g = clampi( c0.g + dirs[currentDir * 2 + 0] * stepSize, 0, 63 );
                c1tmp.g = clampi( c1.g + dirs[currentDir * 2 + 1] * stepSize, 0, 63 );
                break;
                
            case 2:
                c0tmp.b = clampi( c0.b + dirs[currentDir * 2 + 0] * stepSize, 0, 31 );
                c1tmp.b = clampi( c1.b + dirs[currentDir * 2 + 1] * stepSize, 0, 31 );
                break;
                
            default:
                assert( false );
                break;
        }
        
        error = computeColorPaletteErrorDXT1RGB( c0tmp, c1tmp, in_BLOCK_RGB );
        
        if ( error < bestError ) {
            c0 = c0tmp;
            c1 = c1tmp;
            failCount = 0;
            bestError = error;
            bestC0 = c0;
            bestC1 = c1;
            stepSize = 1;
            currentDir &= 0xFE; // set direction to be positive
            
            if ( bestError == 0 ) {
                break;
            }
        } else {
            failCount++;
            
            if ( failCount >= 3 * maxDir ) {
                stepSize++;
                failCount = 0;
                
                if ( stepSize > in_MAX_RANGE ) {
                    break;
                }
            }
            
            currentColor++;
            currentColor = ( currentColor > 2 ) ? 0 : currentColor;
            
            if ( currentColor == 0 ) {
                currentDir++;
                currentDir = ( currentDir >= maxDir ) ? 0 : currentDir;
            }
        }
        
        if ( bestError == 0 ) {
            break;
        }
    }
    
    *in_out_bestC0 = bestC0;
    *in_out_bestC1 = bestC1;
    *in_out_bestError = bestError;
}



// iterative method (shift one end or pinch/expand both ends)
static void findColorPaletteIterativeDXT1RGBAOpaque( rgb565_t * in_out_bestC0, rgb565_t * in_out_bestC1, uint32_t * in_out_bestError, const rgba8_t in_BLOCK_RGBA[4][4], const uint32_t in_MAX_RANGE, const bool in_MORE_AGGRESSIVE ) {
    uint32_t bestError = *in_out_bestError;
    rgb565_t bestC0 = *in_out_bestC0;
    rgb565_t bestC1 = *in_out_bestC1;
    rgb565_t c0 = bestC0;
    rgb565_t c1 = bestC1;
    rgb565_t c0tmp = RGB( 0, 0, 0 );
    rgb565_t c1tmp = RGB( 0, 0, 0 );
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
    uint32_t currentColor = 0;
    
    
    while ( true ) {
        c0tmp = c0;
        c1tmp = c1;
        
        switch ( currentColor ) {
            case 0:
                c0tmp.r = clampi( c0.r + dirs[currentDir * 2 + 0] * stepSize, 0, 31 );
                c1tmp.r = clampi( c1.r + dirs[currentDir * 2 + 1] * stepSize, 0, 31 );
                break;
                
            case 1:
                c0tmp.g = clampi( c0.g + dirs[currentDir * 2 + 0] * stepSize, 0, 63 );
                c1tmp.g = clampi( c1.g + dirs[currentDir * 2 + 1] * stepSize, 0, 63 );
                break;
                
            case 2:
                c0tmp.b = clampi( c0.b + dirs[currentDir * 2 + 0] * stepSize, 0, 31 );
                c1tmp.b = clampi( c1.b + dirs[currentDir * 2 + 1] * stepSize, 0, 31 );
                break;
                
            default:
                assert( false );
                break;
        }
        
        error = computeColorPaletteErrorDXT1RGBAOpaque( c0tmp, c1tmp, in_BLOCK_RGBA );
        
        if ( error < bestError ) {
            c0 = c0tmp;
            c1 = c1tmp;
            failCount = 0;
            bestError = error;
            bestC0 = c0;
            bestC1 = c1;
            stepSize = 1;
            currentDir &= 0xFE; // set direction to be positive
            
            if ( bestError == 0 ) {
                break;
            }
        } else {
            failCount++;
            
            if ( failCount >= 3 * maxDir ) {
                stepSize++;
                failCount = 0;
                
                if ( stepSize > in_MAX_RANGE ) {
                    break;
                }
            }
            
            currentColor++;
            currentColor = ( currentColor > 2 ) ? 0 : currentColor;
            
            if ( currentColor == 0 ) {
                currentDir++;
                currentDir = ( currentDir >= maxDir ) ? 0 : currentDir;
            }
        }
        
        if ( bestError == 0 ) {
            break;
        }
    }
    
    *in_out_bestC0 = bestC0;
    *in_out_bestC1 = bestC1;
    *in_out_bestError = bestError;
}



// iterative method (shift one end or pinch/expand both ends)
static void findColorPaletteIterativeDXT1RGBATransparent( rgb565_t * in_out_bestC0, rgb565_t * in_out_bestC1, uint32_t * in_out_bestError, const rgba8_t in_BLOCK_RGBA[4][4], const uint32_t in_MAX_RANGE, const bool in_MORE_AGGRESSIVE ) {
    uint32_t bestError = *in_out_bestError;
    rgb565_t bestC0 = *in_out_bestC0;
    rgb565_t bestC1 = *in_out_bestC1;
    rgb565_t c0 = bestC0;
    rgb565_t c1 = bestC1;
    rgb565_t c0tmp = RGB( 0, 0, 0 );
    rgb565_t c1tmp = RGB( 0, 0, 0 );
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
    uint32_t currentColor = 0;
    
    
    while ( true ) {
        c0tmp = c0;
        c1tmp = c1;
        
        switch ( currentColor ) {
            case 0:
                c0tmp.r = clampi( c0.r + dirs[currentDir * 2 + 0] * stepSize, 0, 31 );
                c1tmp.r = clampi( c1.r + dirs[currentDir * 2 + 1] * stepSize, 0, 31 );
                break;
                
            case 1:
                c0tmp.g = clampi( c0.g + dirs[currentDir * 2 + 0] * stepSize, 0, 63 );
                c1tmp.g = clampi( c1.g + dirs[currentDir * 2 + 1] * stepSize, 0, 63 );
                break;
                
            case 2:
                c0tmp.b = clampi( c0.b + dirs[currentDir * 2 + 0] * stepSize, 0, 31 );
                c1tmp.b = clampi( c1.b + dirs[currentDir * 2 + 1] * stepSize, 0, 31 );
                break;
                
            default:
                assert( false );
                break;
        }
        
        if ( c0tmp.b16 > c1tmp.b16 ) {
            rgb565_t tmp = c0tmp;
            c0tmp = c1tmp;
            c1tmp = tmp;
        }
        
        //printf( "%u %u\n", c0tmp.b16, c1tmp.b16 );
        error = computeColorPaletteErrorDXT1RGBATransparent( c0tmp, c1tmp, in_BLOCK_RGBA );
        
        if ( error < bestError ) {
            c0 = c0tmp;
            c1 = c1tmp;
            failCount = 0;
            bestError = error;
            bestC0 = c0;
            bestC1 = c1;
            stepSize = 1;
            currentDir &= 0xFE; // set direction to be positive
            
            if ( bestError == 0 ) {
                break;
            }
        } else {
            failCount++;
            
            if ( failCount >= 3 * maxDir ) {
                stepSize++;
                failCount = 0;
                
                if ( stepSize > in_MAX_RANGE ) {
                    break;
                }
            }
            
            currentColor++;
            currentColor = ( currentColor > 2 ) ? 0 : currentColor;
            
            if ( currentColor == 0 ) {
                currentDir++;
                currentDir = ( currentDir >= maxDir ) ? 0 : currentDir;
            }
        }
        
        if ( bestError == 0 ) {
            break;
        }
    }
    
    *in_out_bestC0 = bestC0;
    *in_out_bestC1 = bestC1;
    *in_out_bestError = bestError;
}



// iterative method (shift one end or pinch/expand both ends)
static void findColorPaletteIterativeDXT3( rgb565_t * in_out_bestC0, rgb565_t * in_out_bestC1, uint32_t * in_out_bestError, const rgba8_t in_BLOCK_RGBA[4][4], const uint32_t in_MAX_RANGE, const bool in_MORE_AGGRESSIVE ) {
    uint32_t bestError = *in_out_bestError;
    rgb565_t bestC0 = *in_out_bestC0;
    rgb565_t bestC1 = *in_out_bestC1;
    rgb565_t c0 = bestC0;
    rgb565_t c1 = bestC1;
    rgb565_t c0tmp = RGB( 0, 0, 0 );
    rgb565_t c1tmp = RGB( 0, 0, 0 );
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
    uint32_t currentColor = 0;
    
    
    while ( true ) {
        c0tmp = c0;
        c1tmp = c1;
        
        switch ( currentColor ) {
            case 0:
                c0tmp.r = clampi( c0.r + dirs[currentDir * 2 + 0] * stepSize, 0, 31 );
                c1tmp.r = clampi( c1.r + dirs[currentDir * 2 + 1] * stepSize, 0, 31 );
                break;
                
            case 1:
                c0tmp.g = clampi( c0.g + dirs[currentDir * 2 + 0] * stepSize, 0, 63 );
                c1tmp.g = clampi( c1.g + dirs[currentDir * 2 + 1] * stepSize, 0, 63 );
                break;
                
            case 2:
                c0tmp.b = clampi( c0.b + dirs[currentDir * 2 + 0] * stepSize, 0, 31 );
                c1tmp.b = clampi( c1.b + dirs[currentDir * 2 + 1] * stepSize, 0, 31 );
                break;
                
            default:
                assert( false );
                break;
        }
        
        //printf( "%u %u\n", c0tmp.b16, c1tmp.b16 );
        error = computeColorPaletteErrorDXT3( c0tmp, c1tmp, in_BLOCK_RGBA );
        
        if ( error < bestError ) {
            c0 = c0tmp;
            c1 = c1tmp;
            failCount = 0;
            bestError = error;
            bestC0 = c0;
            bestC1 = c1;
            stepSize = 1;
            currentDir &= 0xFE; // set direction to be positive
            
            if ( bestError == 0 ) {
                break;
            }
        } else {
            failCount++;
            
            if ( failCount >= 3 * maxDir ) {
                stepSize++;
                failCount = 0;
                
                if ( stepSize > in_MAX_RANGE ) {
                    break;
                }
            }
            
            currentColor++;
            currentColor = ( currentColor > 2 ) ? 0 : currentColor;
            
            if ( currentColor == 0 ) {
                currentDir++;
                currentDir = ( currentDir >= maxDir ) ? 0 : currentDir;
            }
        }
        
        if ( bestError == 0 ) {
            break;
        }
    }
    
    *in_out_bestC0 = bestC0;
    *in_out_bestC1 = bestC1;
    *in_out_bestError = bestError;
    
    assert( in_out_bestC0->b16 > in_out_bestC1->b16 );
}



static void findColorPaletteDXT1RGB( rgb565_t * out_c0, rgb565_t * out_c1, const rgb8_t in_BLOCK_RGB[4][4] ) {
    rgb565_t c0;
    rgb565_t c1;
    rgb8_t cMin = RGB( 255, 255, 255 );
    rgb8_t cMax = RGB( 0, 0, 0 );
    
    for ( int by = 0; by < 4; by++ ) {
        for ( int bx = 0; bx < 4; bx++ ) {
            rgb8_t pixel = in_BLOCK_RGB[by][bx];
            cMin.r = ( pixel.r < cMin.r ) ? pixel.r : cMin.r;
            cMin.g = ( pixel.g < cMin.g ) ? pixel.g : cMin.g;
            cMin.b = ( pixel.b < cMin.b ) ? pixel.b : cMin.b;
            cMax.r = ( pixel.r > cMax.r ) ? pixel.r : cMax.r;
            cMax.g = ( pixel.g > cMax.g ) ? pixel.g : cMax.g;
            cMax.b = ( pixel.b > cMax.b ) ? pixel.b : cMax.b;
        }
    }
    
    uint32_t bestError = 0xFFFFFFFF;
    rgb565_t bestC0 = RGB( 0, 0, 0 );
    rgb565_t bestC1 = RGB( 0, 0, 0 );
    
    // initial guess : straight forward diagonal across the min-max-color-cube
    {
        rgb8_t a = RGB( 0, 0, 0 );
        rgb8_t b = RGB( 0, 0, 0 );
        
        for ( int i = 0; i < 8; i++ ) {
            a.r = ( i bitand 0x4 ) ? cMax.r : cMin.r;
            a.g = ( i bitand 0x2 ) ? cMax.g : cMin.g;
            a.b = ( i bitand 0x1 ) ? cMax.b : cMin.b;
            b.r = ( i bitand 0x4 ) ? cMin.r : cMax.r;
            b.g = ( i bitand 0x2 ) ? cMin.g : cMax.g;
            b.b = ( i bitand 0x1 ) ? cMin.b : cMax.b;
            
            c0.r = COLOR_LUT_5[kZERO_THIRDS][a.r].c0;
            c0.g = COLOR_LUT_6[kZERO_THIRDS][a.g].c0;
            c0.b = COLOR_LUT_5[kZERO_THIRDS][a.b].c0;
            c1.r = COLOR_LUT_5[kTHREE_THIRDS][b.r].c1;
            c1.g = COLOR_LUT_6[kTHREE_THIRDS][b.g].c1;
            c1.b = COLOR_LUT_5[kTHREE_THIRDS][b.b].c1;
            
            uint32_t error = computeColorPaletteErrorDXT1RGB( c0, c1, in_BLOCK_RGB );
            
            if ( error < bestError ) {
                bestError = error;
                bestC0 = c0;
                bestC1 = c1;
            }
        }
    }
    
    // brute force per color
    if ( false ) {
        findColorPaletteBrutePerColorDXT1RGB( &bestC0, &bestC1, &bestError, in_BLOCK_RGB );
    } else {
        uint32_t bestErrorTmpA = 0xFFFFFFFF;
        rgb565_t bestC0TmpA = bestC0;
        rgb565_t bestC1TmpA = bestC1;
        uint32_t bestErrorTmpB = 0xFFFFFFFF;
        rgb565_t bestC0TmpB = bestC1;
        rgb565_t bestC1TmpB = bestC0;
        findColorPaletteIterativeDXT1RGB( &bestC0TmpA, &bestC1TmpA, &bestErrorTmpA, in_BLOCK_RGB, 11, true );
        findColorPaletteIterativeDXT1RGB( &bestC0TmpB, &bestC1TmpB, &bestErrorTmpB, in_BLOCK_RGB, 11, true );
        
        if ( bestErrorTmpA < bestErrorTmpB ) {
            if ( bestErrorTmpA < bestError ) {
                bestError = bestErrorTmpA;
                bestC0 = bestC0TmpA;
                bestC1 = bestC1TmpA;
            }
        } else {
            if ( bestErrorTmpB < bestError ) {
                bestError = bestErrorTmpB;
                bestC0 = bestC0TmpB;
                bestC1 = bestC1TmpB;
            }
        }
    }
    
    //straightError = bestError;
    
    // brute force is too hard with its 2^32 combinations per block
    if ( false ) {
        uint16_t c0h;
        uint16_t c1h;
        
        for ( int co0 = 0; co0 < 0x10000; co0++ ) {
            c0h = co0;
            c0.b16 = c0h;
            
            for ( int co1 = 0; co1 < 0x10000; co1++ ) {
                c1h = co1;
                c1.b16 = co1;
                
                uint32_t error = computeColorPaletteErrorDXT1RGB( c0, c1, in_BLOCK_RGB );
                
                if ( error < bestError ) {
                    printf( "%i\n", error );
                    bestError = error;
                    bestC0 = c0;
                    bestC1 = c1;
                }
            }
        }
    }
    
    *out_c0 = bestC0;
    *out_c1 = bestC1;

    //printf( "(%3i %3i %3i) (%3i %3i %3i) <-> (%2i %2i %2i) (%2i %2i %2i) (%9u < %9u ∆ = %9u)\n",
    //       cMin.r, cMin.g, cMin.b, cMax.r, cMax.g, cMax.b, bestC0.r, bestC0.g, bestC0.b, bestC1.r, bestC1.g, bestC1.b, bestError, straightError, straightError - bestError );
}



static void findColorPaletteDXT1RGBAOpaque( rgb565_t * out_c0, rgb565_t * out_c1, const rgba8_t in_BLOCK_RGBA[4][4] ) {
    rgb565_t c0;
    rgb565_t c1;
    rgb8_t cMin = RGB( 255, 255, 255 );
    rgb8_t cMax = RGB( 0, 0, 0 );
    
    for ( int by = 0; by < 4; by++ ) {
        for ( int bx = 0; bx < 4; bx++ ) {
            rgba8_t pixel = in_BLOCK_RGBA[by][bx];
            cMin.r = ( pixel.r < cMin.r ) ? pixel.r : cMin.r;
            cMin.g = ( pixel.g < cMin.g ) ? pixel.g : cMin.g;
            cMin.b = ( pixel.b < cMin.b ) ? pixel.b : cMin.b;
            cMax.r = ( pixel.r > cMax.r ) ? pixel.r : cMax.r;
            cMax.g = ( pixel.g > cMax.g ) ? pixel.g : cMax.g;
            cMax.b = ( pixel.b > cMax.b ) ? pixel.b : cMax.b;
        }
    }
    
    uint32_t bestError = 0xFFFFFFFF;
    rgb565_t bestC0 = RGB( 0, 0, 0 );
    rgb565_t bestC1 = RGB( 0, 0, 0 );
    
    // initial guess : straight forward diagonal across the min-max-color-cube
    {
        rgb8_t a = RGB( 0, 0, 0 );
        rgb8_t b = RGB( 0, 0, 0 );
        
        for ( int i = 0; i < 8; i++ ) {
            a.r = ( i bitand 0x4 ) ? cMax.r : cMin.r;
            a.g = ( i bitand 0x2 ) ? cMax.g : cMin.g;
            a.b = ( i bitand 0x1 ) ? cMax.b : cMin.b;
            b.r = ( i bitand 0x4 ) ? cMin.r : cMax.r;
            b.g = ( i bitand 0x2 ) ? cMin.g : cMax.g;
            b.b = ( i bitand 0x1 ) ? cMin.b : cMax.b;
            
            c0.r = COLOR_LUT_5[kZERO_THIRDS][a.r].c0;
            c0.g = COLOR_LUT_6[kZERO_THIRDS][a.g].c0;
            c0.b = COLOR_LUT_5[kZERO_THIRDS][a.b].c0;
            c1.r = COLOR_LUT_5[kTHREE_THIRDS][b.r].c1;
            c1.g = COLOR_LUT_6[kTHREE_THIRDS][b.g].c1;
            c1.b = COLOR_LUT_5[kTHREE_THIRDS][b.b].c1;
            
            uint32_t error = computeColorPaletteErrorDXT1RGBAOpaque( c0, c1, in_BLOCK_RGBA );
            
            if ( error < bestError ) {
                bestError = error;
                bestC0 = c0;
                bestC1 = c1;
            }
        }
    }
    
    uint32_t straightError = bestError;
    
    // brute force per color
    if ( false ) {
        findColorPaletteBrutePerColorDXT1RGBAOpaque( &bestC0, &bestC1, &bestError, in_BLOCK_RGBA );
    } else {
        uint32_t bestErrorTmpA = 0xFFFFFFFF;
        rgb565_t bestC0TmpA = bestC0;
        rgb565_t bestC1TmpA = bestC1;
        uint32_t bestErrorTmpB = 0xFFFFFFFF;
        rgb565_t bestC0TmpB = bestC1;
        rgb565_t bestC1TmpB = bestC0;
        findColorPaletteIterativeDXT1RGBAOpaque( &bestC0TmpA, &bestC1TmpA, &bestErrorTmpA, in_BLOCK_RGBA, 11, true );
        findColorPaletteIterativeDXT1RGBAOpaque( &bestC0TmpB, &bestC1TmpB, &bestErrorTmpB, in_BLOCK_RGBA, 11, true );
        
        if ( bestErrorTmpA < bestErrorTmpB ) {
            if ( bestErrorTmpA < bestError ) {
                bestError = bestErrorTmpA;
                bestC0 = bestC0TmpA;
                bestC1 = bestC1TmpA;
            }
        } else {
            if ( bestErrorTmpB < bestError ) {
                bestError = bestErrorTmpB;
                bestC0 = bestC0TmpB;
                bestC1 = bestC1TmpB;
            }
        }
    }
    
    //straightError = bestError;
    
    // brute force is too hard with its 2^32 combinations per block
    if ( false ) {
        uint16_t c0h;
        uint16_t c1h;
        
        for ( int co0 = 0; co0 < 0x10000; co0++ ) {
            c0h = co0;
            c0.b16 = c0h;
            
            for ( int co1 = 0; co1 < 0x10000; co1++ ) {
                c1h = co1;
                c1.b16 = co1;
                
                uint32_t error = computeColorPaletteErrorDXT1RGBAOpaque( c0, c1, in_BLOCK_RGBA );
                
                if ( error < bestError ) {
                    printf( "%i\n", error );
                    bestError = error;
                    bestC0 = c0;
                    bestC1 = c1;
                }
            }
        }
    }
    
    *out_c0 = bestC0;
    *out_c1 = bestC1;
    
    printf( "(%3i %3i %3i) (%3i %3i %3i) <-> (%2i %2i %2i) (%2i %2i %2i) (%9u < %9u ∆ = %9u)\n",
           cMin.r, cMin.g, cMin.b, cMax.r, cMax.g, cMax.b, bestC0.r, bestC0.g, bestC0.b, bestC1.r, bestC1.g, bestC1.b, bestError, straightError, straightError - bestError );
}



static void findColorPaletteDXT1RGBATransparent( rgb565_t * out_c0, rgb565_t * out_c1, const rgba8_t in_BLOCK_RGBA[4][4] ) {
    rgb8_t cMin = RGB( 255, 255, 255 );
    rgb8_t cMax = RGB( 0, 0, 0 );
    rgb565_t c0 = RGB( 0, 0, 0 );
    rgb565_t c1 = RGB( 0, 0, 0 );
    
    for ( int by = 0; by < 4; by++ ) {
        for ( int bx = 0; bx < 4; bx++ ) {
            rgba8_t pixel = in_BLOCK_RGBA[by][bx];
            bool opaquePixel = ( pixel.a != 0 );
            cMin.r = ( opaquePixel and ( pixel.r < cMin.r ) ) ? pixel.r : cMin.r;
            cMin.g = ( opaquePixel and ( pixel.g < cMin.g ) ) ? pixel.g : cMin.g;
            cMin.b = ( opaquePixel and ( pixel.b < cMin.b ) ) ? pixel.b : cMin.b;
            cMax.r = ( opaquePixel and ( pixel.r > cMax.r ) ) ? pixel.r : cMax.r;
            cMax.g = ( opaquePixel and ( pixel.g > cMax.g ) ) ? pixel.g : cMax.g;
            cMax.b = ( opaquePixel and ( pixel.b > cMax.b ) ) ? pixel.b : cMax.b;
        }
    }
    
    uint32_t bestError = 0xFFFFFFFF;
    rgb565_t bestC0 = RGB( 0, 0, 0 );
    rgb565_t bestC1 = RGB( 0, 0, 0 );
    
    // initial guess : straight forward diagonal across the min-max-color-cube
    {
        rgb8_t a = RGB( 0, 0, 0 );
        rgb8_t b = RGB( 0, 0, 0 );
        
        for ( int i = 0; i < 8; i++ ) {
            a.r = ( i bitand 0x4 ) ? cMax.r : cMin.r;
            a.g = ( i bitand 0x2 ) ? cMax.g : cMin.g;
            a.b = ( i bitand 0x1 ) ? cMax.b : cMin.b;
            b.r = ( i bitand 0x4 ) ? cMin.r : cMax.r;
            b.g = ( i bitand 0x2 ) ? cMin.g : cMax.g;
            b.b = ( i bitand 0x1 ) ? cMin.b : cMax.b;
            
            c0.r = COLOR_LUT_5[kZERO_HALFS][a.r].c0;
            c0.g = COLOR_LUT_6[kZERO_HALFS][a.g].c0;
            c0.b = COLOR_LUT_5[kZERO_HALFS][a.b].c0;
            c1.r = COLOR_LUT_5[kTWO_HALFS][b.r].c1;
            c1.g = COLOR_LUT_6[kTWO_HALFS][b.g].c1;
            c1.b = COLOR_LUT_5[kTWO_HALFS][b.b].c1;
            
            uint32_t error = computeColorPaletteErrorDXT1RGBATransparent( c0, c1, in_BLOCK_RGBA );
            
            if ( error < bestError ) {
                bestError = error;
                bestC0 = c0;
                bestC1 = c1;
            }
        }
    }
    
    // brute force per color
    if ( false ) {
        findColorPaletteBrutePerColorDXT1RGBATransparent( &bestC0, &bestC1, &bestError, in_BLOCK_RGBA );
    } else {
        findColorPaletteIterativeDXT1RGBATransparent( &bestC0, &bestC1, &bestError, in_BLOCK_RGBA, 11, false );
    }
    
    //straightError = bestError;
    
    // brute force is too hard with its 2^31 combinations per block
    if ( false ) {
        uint16_t c0h;
        uint16_t c1h;
        
        for ( int co0 = 0; co0 < 0x10000; co0++ ) {
            c0h = co0;
            c0.b16 = c0h;
            
            for ( int co1 = co0; co1 < 0x10000; co1++ ) {
                c1h = co1;
                c1.b16 = co1;
                
                if ( c0.b16 <= c1.b16 ) {
                    
                    uint32_t error = computeColorPaletteErrorDXT1RGBATransparent( c0, c1, in_BLOCK_RGBA );
                    
                    if ( error < bestError ) {
                        printf( "%i\n", error );
                        bestError = error;
                        bestC0 = c0;
                        bestC1 = c1;
                    }
                }
            }
        }
    }
    
    assert( bestC0.b16 <= bestC1.b16 );
    *out_c0 = bestC0;
    *out_c1 = bestC1;
    
    //printf( "(%3i %3i %3i) (%3i %3i %3i) <-> (%2i %2i %2i) (%2i %2i %2i) (%9u < %9u ∆ = %9u)\n",
    //       cMin.r, cMin.g, cMin.b, cMax.r, cMax.g, cMax.b, bestC0.r, bestC0.g, bestC0.b, bestC1.r, bestC1.g, bestC1.b, bestError, straightError, straightError - bestError );
}



static void findColorPaletteDXT3( rgb565_t * out_c0, rgb565_t * out_c1, const rgba8_t in_BLOCK_RGBA[4][4] ) {
    rgb565_t c0;
    rgb565_t c1;
    rgb8_t cMin = RGB( 255, 255, 255 );
    rgb8_t cMax = RGB( 0, 0, 0 );
    
    for ( int by = 0; by < 4; by++ ) {
        for ( int bx = 0; bx < 4; bx++ ) {
            rgba8_t pixel = in_BLOCK_RGBA[by][bx];
            cMin.r = ( pixel.r < cMin.r ) ? pixel.r : cMin.r;
            cMin.g = ( pixel.g < cMin.g ) ? pixel.g : cMin.g;
            cMin.b = ( pixel.b < cMin.b ) ? pixel.b : cMin.b;
            cMax.r = ( pixel.r > cMax.r ) ? pixel.r : cMax.r;
            cMax.g = ( pixel.g > cMax.g ) ? pixel.g : cMax.g;
            cMax.b = ( pixel.b > cMax.b ) ? pixel.b : cMax.b;
        }
    }
    
    uint32_t bestError = 0xFFFFFFFF;
    rgb565_t bestC0 = RGB( 0, 0, 0 );
    rgb565_t bestC1 = RGB( 0, 0, 0 );
    
    // initial guess : straight forward diagonal across the min-max-color-cube
    {
        rgb8_t a = RGB( 0, 0, 0 );
        rgb8_t b = RGB( 0, 0, 0 );

        for ( int i = 0; i < 8; i++ ) {
            a.r = ( i bitand 0x4 ) ? cMax.r : cMin.r;
            a.g = ( i bitand 0x2 ) ? cMax.g : cMin.g;
            a.b = ( i bitand 0x1 ) ? cMax.b : cMin.b;
            b.r = ( i bitand 0x4 ) ? cMin.r : cMax.r;
            b.g = ( i bitand 0x2 ) ? cMin.g : cMax.g;
            b.b = ( i bitand 0x1 ) ? cMin.b : cMax.b;
            
            c0.r = COLOR_LUT_5[kTHREE_THIRDS][a.r].c0;
            c0.g = COLOR_LUT_6[kTHREE_THIRDS][a.g].c0;
            c0.b = COLOR_LUT_5[kTHREE_THIRDS][a.b].c0;
            c1.r = COLOR_LUT_5[kZERO_THIRDS][b.r].c1;
            c1.g = COLOR_LUT_6[kZERO_THIRDS][b.g].c1;
            c1.b = COLOR_LUT_5[kZERO_THIRDS][b.b].c1;
            
            uint32_t error = computeColorPaletteErrorDXT3( c0, c1, in_BLOCK_RGBA );
            
            if ( error < bestError ) {
                bestError = error;
                bestC0 = c0;
                bestC1 = c1;
            }
        }

        assert( bestC0.b16 > bestC1.b16 );
    }
    
    // brute force per color
    if ( false ) {
        findColorPaletteBrutePerColorDXT3( &bestC0, &bestC1, &bestError, in_BLOCK_RGBA );
    } else {
        findColorPaletteIterativeDXT3( &bestC0, &bestC1, &bestError, in_BLOCK_RGBA, 11, true );
    }
    
    //straightError = bestError;
    
    // brute force is too hard with its 2^31 combinations per block
    if ( false ) {
        uint16_t c0h;
        uint16_t c1h;
        
        for ( int co0 = 0; co0 < 0x10000; co0++ ) {
            c0h = co0;
            c0.b16 = c0h;
            
            for ( int co1 = co0; co1 < 0x10000; co1++ ) {
                c1h = co1;
                c1.b16 = co1;
                
                uint32_t error = computeColorPaletteErrorDXT3( c0, c1, in_BLOCK_RGBA );
                
                if ( error < bestError ) {
                    printf( "%i\n", error );
                    bestError = error;
                    bestC0 = c0;
                    bestC1 = c1;
                }
            }
        }
    }
    
    if ( false ) {
        uint16_t c0h;
        uint16_t c1h;
        rgb8_t cc0;
        rgb8_t cc1;
        rgb8_t ccMin;
        rgb8_t ccMax;
        
        for ( int ci1 = 0; ci1 < 0x10000; ci1++ ) {
            c1h = ci1;
            c1.b16 = c1h;
            convert565to888( &cc1, c1 );
            
            for ( int ci0 = ci1; ci0 < 0x10000; ci0++ ) {
                c0h = ci0;
                c0.b16 = c0h;
                convert565to888( &cc0, c0 );
                
                //check that the bounds intesect the color cube
                ccMin.r = ( cc0.r < cc1.r ) ? cc0.r : cc1.r;
                ccMin.g = ( cc0.g < cc1.g ) ? cc0.g : cc1.g;
                ccMin.b = ( cc0.b < cc1.b ) ? cc0.b : cc1.b;
                ccMax.r = ( cc0.r > cc1.r ) ? cc0.r : cc1.r;
                ccMax.g = ( cc0.g > cc1.g ) ? cc0.g : cc1.g;
                ccMax.b = ( cc0.b > cc1.b ) ? cc0.b : cc1.b;
                
                if ( ccMax.r < cMin.r ) {
                    ci0 += 0x7FF;   // skip ahead to the next red color
                    continue;
                }
                
                if ( ccMax.g < cMin.g ) {
                    ci0 += 0x1F;   // skip ahead to the next green color
                    continue;
                }
                
                if ( ccMax.b < cMin.b ) {
                    continue;
                }
                
                if ( ccMin.r > cMax.r ) {
                    break;
                }
                
                if ( ccMin.g > cMax.g ) {
                    ci0 += 0x1F;   // skip ahead to the next green color
                    continue;
                }
                
                if ( ccMin.b > cMax.b ) {
                    continue;
                }
                
                
                uint32_t error = computeColorPaletteErrorDXT3( c0, c1, in_BLOCK_RGBA );
                
                if ( error < bestError ) {
                    printf( "%i\n", error );
                    bestError = error;
                    bestC0 = c0;
                    bestC1 = c1;
                }
            }
            
            if ( ccMin.r > cMax.r ) {
                break;
            }
        }
    }
    
    *out_c0 = bestC0;
    *out_c1 = bestC1;
    
    assert( out_c0->b16 > out_c1->b16 );
    
    //printf( "(%3i %3i %3i) (%3i %3i %3i) <-> (%2i %2i %2i) (%2i %2i %2i) (%9u < %9u ∆ = %9u)\n",
    //       cMin.r, cMin.g, cMin.b, cMax.r, cMax.g, cMax.b, bestC0.r, bestC0.g, bestC0.b, bestC1.r, bestC1.g, bestC1.b, bestError, straightError, straightError - bestError );
}



#pragma mark - block compression



static void compressDXT1BlockRGB( DXT1Block_t * in_out_block, const rgb8_t in_BLOCK_RGB[4][4] ) {
    rgb8_t cMin = RGB( 255, 255, 255 );
    rgb8_t cMax = RGB( 0, 0, 0 );
    rgb565_t c0 = RGB( 0, 0, 0 );
    rgb565_t c1 = RGB( 0, 0, 0 );
    
    for ( int by = 0; by < 4; by++ ) {
        for ( int bx = 0; bx < 4; bx++ ) {
            rgb8_t pixel = in_BLOCK_RGB[by][bx];
            cMin.r = ( pixel.r < cMin.r ) ? pixel.r : cMin.r;
            cMin.g = ( pixel.g < cMin.g ) ? pixel.g : cMin.g;
            cMin.b = ( pixel.b < cMin.b ) ? pixel.b : cMin.b;
            cMax.r = ( pixel.r > cMax.r ) ? pixel.r : cMax.r;
            cMax.g = ( pixel.g > cMax.g ) ? pixel.g : cMax.g;
            cMax.b = ( pixel.b > cMax.b ) ? pixel.b : cMax.b;
        }
    }
    
    // Uniform Color
    if ( ( cMin.r == cMax.r ) and ( cMin.g == cMax.g ) and ( cMin.b == cMax.b ) ) {
        uint32_t bestError = 0xFFFFFFFF;
        rgb565_t bestC0 = RGB( 0, 0, 0 );
        rgb565_t bestC1 = RGB( 0, 0, 0 );
        colorCompositionCase_t bestType = kINVALID;
        
        for ( int j = 0; j < kINVALID; j++ ) {
            uint32_t error = 0;
            error += COLOR_LUT_5[j][cMin.r].error;
            error += COLOR_LUT_6[j][cMin.g].error;
            error += COLOR_LUT_5[j][cMin.b].error;
            
            if ( error < bestError ) {
                bestError = error;
                
                c0.r = COLOR_LUT_5[j][cMin.r].c0;
                c1.r = COLOR_LUT_5[j][cMin.r].c1;
                c0.g = COLOR_LUT_6[j][cMin.g].c0;
                c1.g = COLOR_LUT_6[j][cMin.g].c1;
                c0.b = COLOR_LUT_5[j][cMin.b].c0;
                c1.b = COLOR_LUT_5[j][cMin.b].c1;
                
                bestC0 = c0;
                bestC1 = c1;
                bestType = j;
            }
        }
        
        switch ( bestType ) {
            case kZERO_THIRDS:
            case kZERO_HALFS:
                in_out_block->cBitField = 0x0;
                break;
                
            case kTHREE_THIRDS:
            case kTWO_HALFS:
                in_out_block->cBitField = 0x55555555;
                break;
                
            case kONE_THIRD:
            case kONE_HALF:
                in_out_block->cBitField = 0xAAAAAAAA;
                break;
                
            case kTWO_THIRDS:
                in_out_block->cBitField = 0xFFFFFFFF;
                break;
                
            default:
                break;
        }
        
        in_out_block->c0 = bestC0;
        in_out_block->c1 = bestC1;
    } else {
        // do the hard work
        rgb8_t palette[4];
        findColorPaletteDXT1RGB( &c0, &c1, in_BLOCK_RGB );
        computeDXT1RGBPalette( palette, c0, c1 );
        
        uint32_t cBitField = 0;
        uint32_t error = 0;
        uint32_t temp[3] = { 0, 0, 0 };
        
        // TODO : maybe introduce some dithering?
        
        for ( int by = 0; by < 4; by++ ) {
            for ( int bx = 0; bx < 4; bx++ ) {
                uint32_t bestError = 0xFFFFFFFF;
                uint32_t bestIndex = 0;
                rgb8_t pixel = in_BLOCK_RGB[3-by][3-bx];
                
                for ( int p = 0; p < 4; p++ ) {
                    temp[0] = pixel.r - palette[p].r;
                    temp[1] = pixel.g - palette[p].g;
                    temp[2] = pixel.b - palette[p].b;
                    error = temp[0] * temp[0] + temp[1] * temp[1] + temp[2] * temp[2];
                    
                    if ( error < bestError ) {
                        bestError = error;
                        bestIndex = p;
                    }
                }
                
                cBitField <<= 2;
                cBitField |= bestIndex bitand 0x3;
            }
        }
        
        in_out_block->c0 = c0;
        in_out_block->c1 = c1;
        in_out_block->cBitField = cBitField;
    }
}



static void compressDXT1BlockRGBA( DXT1Block_t * in_out_block, const rgba8_t in_BLOCK_RGBA[4][4] ) {
    rgba8_t cMin = RGBA( 255, 255, 255, 255 );
    rgba8_t cMax = RGBA( 0, 0, 0, 0 );
    rgb565_t c0 = RGB( 0, 0, 0 );
    rgb565_t c1 = RGB( 0, 0, 0 );
    
    for ( int by = 0; by < 4; by++ ) {
        for ( int bx = 0; bx < 4; bx++ ) {
            rgba8_t pixel = in_BLOCK_RGBA[by][bx];
            bool opaquePixel = ( pixel.a != 0 );
            cMin.r = ( opaquePixel and ( pixel.r < cMin.r ) ) ? pixel.r : cMin.r;
            cMin.g = ( opaquePixel and ( pixel.g < cMin.g ) ) ? pixel.g : cMin.g;
            cMin.b = ( opaquePixel and ( pixel.b < cMin.b ) ) ? pixel.b : cMin.b;
            cMin.a = ( pixel.a < cMin.a ) ? pixel.a : cMin.a;
            cMax.r = ( opaquePixel and ( pixel.r > cMax.r ) ) ? pixel.r : cMax.r;
            cMax.g = ( opaquePixel and ( pixel.g > cMax.g ) ) ? pixel.g : cMax.g;
            cMax.b = ( opaquePixel and ( pixel.b > cMax.b ) ) ? pixel.b : cMax.b;
            cMax.a = ( pixel.a > cMax.a ) ? pixel.a : cMax.a;
        }
    }
    
    if ( cMax.a == 0 ) {            // Everything is transparent
        in_out_block->b64 = 0xFFFFFFFFFFFFFFFFLL;
    } else if ( cMin.a == 255 ) {   // Everything is opaque
        // Uniform Color
        if ( ( cMin.r == cMax.r ) and ( cMin.g == cMax.g ) and ( cMin.b == cMax.b ) ) {
            uint32_t bestError = 0xFFFFFFFF;
            rgb565_t bestC0 = RGB( 0, 0, 0 );
            rgb565_t bestC1 = RGB( 0, 0, 0 );
            colorCompositionCase_t bestType = kINVALID;
            
            for ( int j = 0; j < kINVALID; j++ ) {
                uint32_t error = 0;
                error += COLOR_LUT_5[j][cMin.r].error;
                error += COLOR_LUT_6[j][cMin.g].error;
                error += COLOR_LUT_5[j][cMin.b].error;
                
                if ( error < bestError ) {
                    bestError = error;
                    
                    c0.r = COLOR_LUT_5[j][cMin.r].c0;
                    c1.r = COLOR_LUT_5[j][cMin.r].c1;
                    c0.g = COLOR_LUT_6[j][cMin.g].c0;
                    c1.g = COLOR_LUT_6[j][cMin.g].c1;
                    c0.b = COLOR_LUT_5[j][cMin.b].c0;
                    c1.b = COLOR_LUT_5[j][cMin.b].c1;
                    
                    bestC0 = c0;
                    bestC1 = c1;
                    bestType = j;
                }
            }
            
            switch ( bestType ) {
                case kZERO_THIRDS:
                case kZERO_HALFS:
                    in_out_block->cBitField = 0x0;
                    break;
                    
                case kTHREE_THIRDS:
                case kTWO_HALFS:
                    in_out_block->cBitField = 0x55555555;
                    break;
                    
                case kONE_THIRD:
                case kONE_HALF:
                    in_out_block->cBitField = 0xAAAAAAAA;
                    break;
                    
                case kTWO_THIRDS:
                    in_out_block->cBitField = 0xFFFFFFFF;
                    break;
                    
                default:
                    assert( false );
                    break;
            }
            
            in_out_block->c0 = bestC0;
            in_out_block->c1 = bestC1;
        } else {
            // do the hard work
            rgb8_t palette[4];
            findColorPaletteDXT1RGBAOpaque( &c0, &c1, in_BLOCK_RGBA );
            computeDXT1RGBPalette( palette, c0, c1 );
            
            uint32_t cBitField = 0;
            uint32_t error = 0;
            uint32_t temp[3] = { 0, 0, 0 };
            const int range = ( c0.b16 <= c1.b16 ) ? 3 : 4;
            
            // TODO : maybe introduce some dithering?
            
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    uint32_t bestError = 0xFFFFFFFF;
                    uint32_t bestIndex = 0;
                    rgba8_t pixel = in_BLOCK_RGBA[3-by][3-bx];
                    
                    for ( int p = 0; p < range; p++ ) {
                        temp[0] = pixel.r - palette[p].r;
                        temp[1] = pixel.g - palette[p].g;
                        temp[2] = pixel.b - palette[p].b;
                        error = temp[0] * temp[0] + temp[1] * temp[1] + temp[2] * temp[2];
                        
                        if ( error < bestError ) {
                            bestError = error;
                            bestIndex = p;
                        }
                    }
                    
                    cBitField <<= 2;
                    cBitField |= bestIndex bitand 0x3;
                    assert( not ( ( c0.b16 <= c1.b16 ) and ( bestIndex == 3 ) ) );
                }
            }
            
            in_out_block->c0 = c0;
            in_out_block->c1 = c1;
            in_out_block->cBitField = cBitField;
        }
    } else {                        // This is the border between opaque and transparent
        in_out_block->c0.b16 = 0x07E0;
        in_out_block->c1.b16 = 0x07E0;
        in_out_block->cBitField = 0x0;
        
        if ( ( cMin.r == cMax.r ) and ( cMin.g == cMax.g ) and ( cMin.b == cMax.b ) ) {
            uint32_t bestError = 0xFFFFFFFF;
            rgb565_t bestC0 = RGB( 0, 0, 0 );
            rgb565_t bestC1 = RGB( 0, 0, 0 );
            colorCompositionCase_t bestType = kINVALID;
            
            for ( int j = kZERO_HALFS; j < kINVALID; j++ ) {
                uint32_t error = 0;
                error += COLOR_LUT_5[j][cMin.r].error;
                error += COLOR_LUT_6[j][cMin.g].error;
                error += COLOR_LUT_5[j][cMin.b].error;
                
                if ( error < bestError ) {
                    bestError = error;
                    
                    c0.r = COLOR_LUT_5[j][cMin.r].c0;
                    c1.r = COLOR_LUT_5[j][cMin.r].c1;
                    c0.g = COLOR_LUT_6[j][cMin.g].c0;
                    c1.g = COLOR_LUT_6[j][cMin.g].c1;
                    c0.b = COLOR_LUT_5[j][cMin.b].c0;
                    c1.b = COLOR_LUT_5[j][cMin.b].c1;
                    
                    bestC0 = c0;
                    bestC1 = c1;
                    bestType = j;
                }
            }
                        
            uint32_t cBitField = 0;
            
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    rgba8_t pixel = in_BLOCK_RGBA[3-by][3-bx];
                    uint32_t bestIndex = ( pixel.a ==  0 ) ? 3 : 0;
                    cBitField <<= 2;
                    cBitField |= bestIndex bitand 0x3;
                }
            }
            
            in_out_block->c0 = bestC0;
            in_out_block->c1 = bestC1;
            in_out_block->cBitField = cBitField;
        } else {
            // do the hard work
            rgba8_t palette[4];
            findColorPaletteDXT1RGBATransparent( &c0, &c1, in_BLOCK_RGBA );
            computeDXT1RGBAPalette( palette, c0, c1 );
            
            uint32_t cBitField = 0;
            uint32_t error = 0;
            uint32_t temp[3] = { 0, 0, 0 };
            
            // TODO : maybe introduce some dithering?
            
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    uint32_t bestError = 0xFFFFFFFF;
                    uint32_t bestIndex = 0;
                    rgba8_t pixel = in_BLOCK_RGBA[3-by][3-bx];
                    
                    if ( pixel.a == 0 ) {
                        bestIndex = 3;
                    } else {
                        for ( int p = 0; p < 3; p++ ) {
                            temp[0] = pixel.r - palette[p].r;
                            temp[1] = pixel.g - palette[p].g;
                            temp[2] = pixel.b - palette[p].b;
                            error = temp[0] * temp[0] + temp[1] * temp[1] + temp[2] * temp[2];
                            
                            if ( error < bestError ) {
                                bestError = error;
                                bestIndex = p;
                            }
                        }
                    }
                    
                    cBitField <<= 2;
                    cBitField |= bestIndex bitand 0x3;
                }
            }
            
            assert( c0.b16 <= c1.b16 );
            in_out_block->c0 = c0;
            in_out_block->c1 = c1;
            in_out_block->cBitField = cBitField;
        }
    }
}



static void compressDXT3BlockRGB( DXT1Block_t * in_out_block, const rgba8_t in_BLOCK_RGBA[4][4] ) {
    rgb8_t cMin = RGB( 255, 255, 255 );
    rgb8_t cMax = RGB( 0, 0, 0 );
    rgb565_t c0 = RGB( 0, 0, 0 );
    rgb565_t c1 = RGB( 0, 0, 0 );
    
    for ( int by = 0; by < 4; by++ ) {
        for ( int bx = 0; bx < 4; bx++ ) {
            cMin.r = ( in_BLOCK_RGBA[by][bx].r < cMin.r ) ? in_BLOCK_RGBA[by][bx].r : cMin.r;
            cMin.g = ( in_BLOCK_RGBA[by][bx].g < cMin.g ) ? in_BLOCK_RGBA[by][bx].g : cMin.g;
            cMin.b = ( in_BLOCK_RGBA[by][bx].b < cMin.b ) ? in_BLOCK_RGBA[by][bx].b : cMin.b;
            cMax.r = ( in_BLOCK_RGBA[by][bx].r > cMax.r ) ? in_BLOCK_RGBA[by][bx].r : cMax.r;
            cMax.g = ( in_BLOCK_RGBA[by][bx].g > cMax.g ) ? in_BLOCK_RGBA[by][bx].g : cMax.g;
            cMax.b = ( in_BLOCK_RGBA[by][bx].b > cMax.b ) ? in_BLOCK_RGBA[by][bx].b : cMax.b;
        }
    }
    
    // Uniform Color
    if ( ( cMin.r == cMax.r ) and ( cMin.g == cMax.g ) and ( cMin.b == cMax.b ) ) {
        uint32_t bestError = 0xFFFFFFFF;
        rgb565_t bestC0 = RGB( 0, 0, 0 );
        rgb565_t bestC1 = RGB( 0, 0, 0 );
        colorCompositionCase_t bestType = kINVALID;
        
        for ( int j = 0; j < kZERO_HALFS; j++ ) {
            uint32_t error = 0;
            error += COLOR_LUT_5[j][cMin.r].error;
            error += COLOR_LUT_6[j][cMin.g].error;
            error += COLOR_LUT_5[j][cMin.b].error;
            
            if ( error < bestError ) {
                bestError = error;
                
                c0.r = COLOR_LUT_5[j][cMin.r].c0;
                c1.r = COLOR_LUT_5[j][cMin.r].c1;
                c0.g = COLOR_LUT_6[j][cMin.g].c0;
                c1.g = COLOR_LUT_6[j][cMin.g].c1;
                c0.b = COLOR_LUT_5[j][cMin.b].c0;
                c1.b = COLOR_LUT_5[j][cMin.b].c1;
                
                bestC0 = c0;
                bestC1 = c1;
                bestType = j;
            }
        }
        
        switch ( bestType ) {
            case kZERO_THIRDS:
                in_out_block->cBitField = 0x0;
                break;
                
            case kTHREE_THIRDS:
                in_out_block->cBitField = 0x55555555;
                break;
                
            case kONE_THIRD:
                in_out_block->cBitField = 0xAAAAAAAA;
                break;
                
            case kTWO_THIRDS:
                in_out_block->cBitField = 0xFFFFFFFF;
                break;
                
            default:
                break;
        }
        
        in_out_block->c0 = bestC0;
        in_out_block->c1 = bestC1;
        assert( in_out_block->c0.b16 > in_out_block->c1.b16 );
    } else {
        // do the hard work
        rgba8_t palette[4];
        findColorPaletteDXT3( &c0, &c1, in_BLOCK_RGBA );
        computeDXT3RGBPalette( palette, c0, c1 );
        
        uint32_t cBitField = 0;
        uint32_t error = 0;
        uint32_t temp[3] = { 0, 0, 0 };
        
        // TODO : maybe introduce some dithering?
        
        for ( int by = 0; by < 4; by++ ) {
            for ( int bx = 0; bx < 4; bx++ ) {
                uint32_t bestError = 0xFFFFFFFF;
                uint32_t bestIndex = 0;
                rgba8_t pixel = in_BLOCK_RGBA[3-by][3-bx];
                
                for ( int p = 0; p < 4; p++ ) {
                    temp[0] = pixel.r - palette[p].r;
                    temp[1] = pixel.g - palette[p].g;
                    temp[2] = pixel.b - palette[p].b;
                    error = temp[0] * temp[0] + temp[1] * temp[1] + temp[2] * temp[2];
                    
                    if ( error < bestError ) {
                        bestError = error;
                        bestIndex = p;
                    }
                }
                
                cBitField <<= 2;
                cBitField |= bestIndex bitand 0x3;
            }
        }
        
        in_out_block->c0 = c0;
        in_out_block->c1 = c1;
        in_out_block->cBitField = cBitField;
        assert( in_out_block->c0.b16 > in_out_block->c1.b16 );
    }
}



#pragma mark - exposed interface



bool dxtcReadDXT1RGB( const char in_FILE[], rgb8_t ** out_image, uint32_t * out_width, uint32_t * out_height ) {
    uint32_t w = 0;
    uint32_t h = 0;
    uint32_t blockCount = 0;
    size_t itemsRead = 0;
    DXT1Block_t * block = NULL;
    DXT1Block_t * blockPtr = NULL;
    rgb8_t blockRGB[4][4];
    rgb8_t * imageRGB = NULL;
    
    FILE * inputDXTFileStream = fopen( in_FILE, "rb" );
    assert( inputDXTFileStream != NULL );
    itemsRead = fread( &w, sizeof( uint32_t ), 1, inputDXTFileStream );
    assert( itemsRead == 1 );
    itemsRead = fread( &h, sizeof( uint32_t ), 1, inputDXTFileStream );
    assert( itemsRead == 1 );
    blockCount = w * h / 16;                                                                                            // FIXME : we have to round up w and h to multiple of 4
    // w >> 2 + ( w & 0x3 ) ? 1 : 0;
    block = malloc( blockCount * sizeof( DXT1Block_t ) );
    blockPtr = block;
    imageRGB = malloc( w * h * sizeof( rgb8_t ) );
    itemsRead = fread( block, sizeof( DXT1Block_t ), blockCount, inputDXTFileStream );
    assert( itemsRead == blockCount );
    fclose( inputDXTFileStream );
    
    for ( int y = 0; y < h; y += 4 ) {
        for ( int x = 0; x < w; x += 4 ) {
            decompressDXT1BlockRGB( blockRGB, *blockPtr );
            blockPtr++;
            
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    int arrayPosition = ( y + by ) * w + x + bx;
                    imageRGB[arrayPosition] = blockRGB[by][bx];
                }
            }
        }
    }
    
    free_s( block );
    
    *out_image = imageRGB;
    *out_width = w;
    *out_height = h;
    
    return true;
}



void dxtcFreeRGB( rgb8_t ** in_out_image ) {
    free_s( *in_out_image );
}



bool dxtcReadDXT1RGBA( const char in_FILE[], rgba8_t ** out_image, uint32_t * out_width, uint32_t * out_height ) {
    uint32_t w = 0;
    uint32_t h = 0;
    uint32_t blockCount = 0;
    size_t itemsRead = 0;
    DXT1Block_t * block = NULL;
    DXT1Block_t * blockPtr = NULL;
    rgba8_t blockRGBA[4][4];
    rgba8_t * imageRGBA = NULL;
    
    FILE * inputDXTFileStream = fopen( in_FILE, "rb" );
    assert( inputDXTFileStream != NULL );
    itemsRead = fread( &w, sizeof( uint32_t ), 1, inputDXTFileStream );
    assert( itemsRead == 1 );
    itemsRead = fread( &h, sizeof( uint32_t ), 1, inputDXTFileStream );
    assert( itemsRead == 1 );
    blockCount = w * h / 16;                                                                                            // FIXME : we have to round up w and h to multiple of 4
    // w >> 2 + ( w & 0x3 ) ? 1 : 0;
    block = malloc( blockCount * sizeof( DXT1Block_t ) );
    blockPtr = block;
    imageRGBA = malloc( w * h * sizeof( rgba8_t ) );
    itemsRead = fread( block, sizeof( DXT1Block_t ), blockCount, inputDXTFileStream );
    assert( itemsRead == blockCount );
    fclose( inputDXTFileStream );
    
    for ( int y = 0; y < h; y += 4 ) {
        for ( int x = 0; x < w; x += 4 ) {
            decompressDXT1BlockRGBA( blockRGBA, *blockPtr );
            blockPtr++;
            
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    int arrayPosition = ( y + by ) * w + x + bx;
                    imageRGBA[arrayPosition] = blockRGBA[by][bx];
                }
            }
        }
    }
    
    free_s( block );
    
    *out_image = imageRGBA;
    *out_width = w;
    *out_height = h;
    
    return true;
}



bool dxtcReadDXT3RGBA( const char in_FILE[], rgba8_t ** out_image, uint32_t * out_width, uint32_t * out_height ) {
    uint32_t w = 0;
    uint32_t h = 0;
    uint32_t blockCount = 0;
    size_t itemsRead = 0;
    DXT3Block_t * block = NULL;
    DXT3Block_t * blockPtr = NULL;
    rgba8_t blockRGBA[4][4];
    rgba8_t * imageRGBA = NULL;
    
    FILE * inputDXTFileStream = fopen( in_FILE, "rb" );
    assert( inputDXTFileStream != NULL );
    itemsRead = fread( &w, sizeof( uint32_t ), 1, inputDXTFileStream );
    assert( itemsRead == 1 );
    itemsRead = fread( &h, sizeof( uint32_t ), 1, inputDXTFileStream );
    assert( itemsRead == 1 );
    blockCount = w * h / 16;                                                                                            // FIXME : we have to round up w and h to multiple of 4
    // w >> 2 + ( w & 0x3 ) ? 1 : 0;
    block = malloc( blockCount * sizeof( DXT3Block_t ) );
    blockPtr = block;
    imageRGBA = malloc( w * h * sizeof( rgba8_t ) );
    itemsRead = fread( block, sizeof( DXT3Block_t ), blockCount, inputDXTFileStream );
    assert( itemsRead == blockCount );
    fclose( inputDXTFileStream );
    
    for ( int y = 0; y < h; y += 4 ) {
        for ( int x = 0; x < w; x += 4 ) {
            decompressDXT3BlockRGBA( blockRGBA, *blockPtr );
            blockPtr++;
            
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    int arrayPosition = ( y + by ) * w + x + bx;
                    imageRGBA[arrayPosition] = blockRGBA[by][bx];
                }
            }
        }
    }
    
    free_s( block );
    
    *out_image = imageRGBA;
    *out_width = w;
    *out_height = h;
    
    return true;
}



bool dxtcReadDXT5RGBA( const char in_FILE[], rgba8_t ** out_image, uint32_t * out_width, uint32_t * out_height ) {
    uint32_t w = 0;
    uint32_t h = 0;
    uint32_t blockCount = 0;
    size_t itemsRead = 0;
    DXT5Block_t * block = NULL;
    DXT5Block_t * blockPtr = NULL;
    rgba8_t blockRGBA[4][4];
    rgba8_t * imageRGBA = NULL;
    
    FILE * inputDXTFileStream = fopen( in_FILE, "rb" );
    assert( inputDXTFileStream != NULL );
    itemsRead = fread( &w, sizeof( uint32_t ), 1, inputDXTFileStream );
    assert( itemsRead == 1 );
    itemsRead = fread( &h, sizeof( uint32_t ), 1, inputDXTFileStream );
    assert( itemsRead == 1 );
    blockCount = w * h / 16;                                                                                            // FIXME : we have to round up w and h to multiple of 4
    // w >> 2 + ( w & 0x3 ) ? 1 : 0;
    block = malloc( blockCount * sizeof( DXT5Block_t ) );
    blockPtr = block;
    imageRGBA = malloc( w * h * sizeof( rgba8_t ) );
    itemsRead = fread( block, sizeof( DXT5Block_t ), blockCount, inputDXTFileStream );
    assert( itemsRead == blockCount );
    fclose( inputDXTFileStream );
    
    for ( int y = 0; y < h; y += 4 ) {
        for ( int x = 0; x < w; x += 4 ) {
            decompressDXT5BlockRGBA( blockRGBA, *blockPtr );
            blockPtr++;
            
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    int arrayPosition = ( y + by ) * w + x + bx;
                    imageRGBA[arrayPosition] = blockRGBA[by][bx];
                }
            }
        }
    }
    
    free_s( block );
    
    *out_image = imageRGBA;
    *out_width = w;
    *out_height = h;
    
    return true;
}



bool dxtcWriteDXT1RGB( const char in_FILE[], const rgb8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT ) {
    uint32_t w = in_WIDTH;
    uint32_t h = in_HEIGHT;
    uint32_t blockCount = 0;
    DXT1Block_t * block = NULL;
    DXT1Block_t * blockPtr = NULL;
    rgb8_t blockRGB[4][4];
    
    blockCount = in_WIDTH * in_HEIGHT / 16;                                                                             // FIXME : we have to round up w and h to multiple of 4
    block = malloc( blockCount * sizeof( DXT1Block_t ) );
    blockPtr = block;
    
    computeUniformColorsLUT();
    fillLUT();
    prepareBayer();
    
    for ( int y = 0; y < h; y += 4 ) {
        for ( int x = 0; x < w; x += 4 ) {
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    int arrayPosition = ( y + by ) * w + x + bx;
                    blockRGB[by][bx] = in_IMAGE[arrayPosition];
                }
            }
            
            compressDXT1BlockRGB( blockPtr, blockRGB );
            blockPtr++;
        }
    }
    
    FILE * outputDXTFileStream = fopen( in_FILE, "wb" );
    fwrite( &w, sizeof( uint32_t ), 1, outputDXTFileStream );
    fwrite( &h, sizeof( uint32_t ), 1, outputDXTFileStream );
    fwrite( block, sizeof( DXT3Block_t ), blockCount, outputDXTFileStream );
    fclose( outputDXTFileStream );
    
    free_s( block );
    
    return true;
}



bool dxtcWriteDXT1RGBA( const char in_FILE[], const rgba8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT ) {
    uint32_t w = in_WIDTH;
    uint32_t h = in_HEIGHT;
    uint32_t blockCount = 0;
    DXT1Block_t * block = NULL;
    DXT1Block_t * blockPtr = NULL;
    rgba8_t blockRGBA[4][4];
    rgba8_t transparent = RGBA( 0, 0, 0, 0 );
    
    blockCount = in_WIDTH * in_HEIGHT / 16;                                                                             // FIXME : we have to round up w and h to multiple of 4
    block = malloc( blockCount * sizeof( DXT1Block_t ) );
    blockPtr = block;
    
    computeUniformColorsLUT();
    fillLUT();
    prepareBayer();
    
    for ( int y = 0; y < h; y += 4 ) {
        for ( int x = 0; x < w; x += 4 ) {
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    int arrayPosition = ( y + by ) * w + x + bx;
                    
                    if ( in_IMAGE[arrayPosition].a < 128 ) {
                        blockRGBA[by][bx] = transparent;
                    } else {
                        blockRGBA[by][bx] = in_IMAGE[arrayPosition];
                        blockRGBA[by][bx].a = 255;
                    }
                }
            }
            
            compressDXT1BlockRGBA( blockPtr, blockRGBA );
            blockPtr++;
        }
    }
    
    FILE * outputDXTFileStream = fopen( in_FILE, "wb" );
    fwrite( &w, sizeof( uint32_t ), 1, outputDXTFileStream );
    fwrite( &h, sizeof( uint32_t ), 1, outputDXTFileStream );
    fwrite( block, sizeof( DXT3Block_t ), blockCount, outputDXTFileStream );
    fclose( outputDXTFileStream );
    
    free_s( block );
    
    return true;
}



bool dxtcWriteDXT3RGBA( const char in_FILE[], const rgba8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT ) {
    uint32_t w = in_WIDTH;
    uint32_t h = in_HEIGHT;
    uint32_t blockCount = 0;
    DXT3Block_t * block = NULL;
    DXT3Block_t * blockPtr = NULL;
    rgba8_t blockRGBA[4][4];
    
    blockCount = in_WIDTH * in_HEIGHT / 16;                                                                             // FIXME : we have to round up w and h to multiple of 4
    block = malloc( blockCount * sizeof( DXT3Block_t ) );
    blockPtr = block;
    
    computeUniformColorsLUT();
    fillLUT();
    prepareBayer();
    
    for ( int y = 0; y < h; y += 4 ) {
        for ( int x = 0; x < w; x += 4 ) {
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    int arrayPosition = ( y + by ) * w + x + bx;
                    blockRGBA[by][bx] = in_IMAGE[arrayPosition];
                }
            }
            
            compressDXT3BlockRGB( &(blockPtr->rgb), blockRGBA );
            compressDXT3BlockA( &(blockPtr->a), blockRGBA );
            blockPtr++;
        }
    }
    
    FILE * outputDXTFileStream = fopen( in_FILE, "wb" );
    fwrite( &w, sizeof( uint32_t ), 1, outputDXTFileStream );
    fwrite( &h, sizeof( uint32_t ), 1, outputDXTFileStream );
    fwrite( block, sizeof( DXT3Block_t ), blockCount, outputDXTFileStream );
    fclose( outputDXTFileStream );
    
    free_s( block );
    
    return true;
}



bool dxtcWriteDXT5RGBA( const char in_FILE[], const rgba8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT ) {
    uint32_t w = in_WIDTH;
    uint32_t h = in_HEIGHT;
    uint32_t blockCount = 0;
    DXT5Block_t * block = NULL;
    DXT5Block_t * blockPtr = NULL;
    rgba8_t blockRGBA[4][4];
    
    blockCount = in_WIDTH * in_HEIGHT / 16;                                                                             // FIXME : we have to round up w and h to multiple of 4
    block = malloc( blockCount * sizeof( DXT5Block_t ) );
    blockPtr = block;
    
    computeUniformColorsLUT();
    fillLUT();
    prepareBayer();
    
    for ( int y = 0; y < h; y += 4 ) {
        for ( int x = 0; x < w; x += 4 ) {
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    int arrayPosition = ( y + by ) * w + x + bx;
                    blockRGBA[by][bx] = in_IMAGE[arrayPosition];
                }
            }
            
            compressDXT3BlockRGB( &(blockPtr->rgb), blockRGBA );
            compressDXT5BlockA( &(blockPtr->a), blockRGBA );
            blockPtr++;
        }
    }
    
    FILE * outputDXTFileStream = fopen( in_FILE, "wb" );
    fwrite( &w, sizeof( uint32_t ), 1, outputDXTFileStream );
    fwrite( &h, sizeof( uint32_t ), 1, outputDXTFileStream );
    fwrite( block, sizeof( DXT3Block_t ), blockCount, outputDXTFileStream );
    fclose( outputDXTFileStream );
    
    free_s( block );
    
    return true;
}



void dxtcFreeRGBA( rgba8_t ** in_out_image ) {
    free_s( *in_out_image );
}
