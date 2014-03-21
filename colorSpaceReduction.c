//
//  colorSpaceReduction.c
//  GLTC
//
//  Created by Michael Kwasnicki on 24.01.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#include "colorSpaceReduction.h"

#include "lib.h"

#include <math.h>
#include <iso646.h>

static uint8_t LUT_1[256];
static uint8_t LUT_2[256];
static uint8_t LUT_3[256];
static uint8_t LUT_4[256];
static uint8_t LUT_5[256];
static uint8_t LUT_6[256];
static uint8_t LUT_7[256];


// this function computes Look-Up-Tables with round to nearest values when reducing the bit depth
// in respect to the decompressed values.
void fillLUT() {
	for ( int i = 0; i < 256; i++ ) {
		LUT_1[i] = i >> 7;
		LUT_2[i] = ( ( i - ( i >> 2 ) + ( i >> 7 ) + 31 ) >> 6 );
		LUT_3[i] = ( ( i - ( i >> 3 ) + ( i >> 7 ) + 15 ) >> 5 );
		LUT_4[i] = ( ( 2 * i + 17 ) * 15  ) / ( 2 * 255 ); //( ( i - ( i >> 4 ) + ( i >> 7 ) + 7 ) >> 4 );
		LUT_5[i] = ( ( i - ( i >> 5 ) + 3 ) >> 3 );
		LUT_6[i] = ( ( i - ( i >> 6 ) + 1 ) >> 2 );
		LUT_7[i] = ( ( i - ( i >> 7 ) ) >> 1 );
	}
	
//	for ( int i = 0; i < 65536; i++ ) {
//		printf( "%i %i %i\n", i, ( ( 2 * i + 17 ) * 15  ) / ( 2 * 255 ), ( ( i - ( i >> 4 ) + ( i >> 7 ) + 7 ) >> 4 ) );
//	}
//	
//	exit( -1 );
}


int reduce8to7bits( const int in_C8 ) {
	return LUT_7[in_C8];
}

int reduce8to6bits( const int in_C8 ) {
	return LUT_6[in_C8];
}

int reduce8to5bits( const int in_C8 ) {
	return LUT_5[in_C8];
}

int reduce8to4bits( const int in_C8 ) {
	return LUT_4[in_C8];
}


void convert888to565( rgb565_t * out_rgb, const rgb8_t in_RGB ) {
    out_rgb->r = LUT_5[in_RGB.r];
    out_rgb->g = LUT_6[in_RGB.g];
    out_rgb->b = LUT_5[in_RGB.b];
}



void convert565to888( rgb8_t * out_rgb, const rgb565_t in_RGB ) {
    out_rgb->r = extend5to8bits( in_RGB.r );
    out_rgb->g = extend6to8bits( in_RGB.g );
    out_rgb->b = extend5to8bits( in_RGB.b );
}

void convert888to444( rgb4_t * out_rgb, const rgb8_t in_RGB ) {
    out_rgb->r = LUT_4[in_RGB.r];
    out_rgb->g = LUT_4[in_RGB.g];
    out_rgb->b = LUT_4[in_RGB.b];
}

void convert444to888( rgb8_t * out_rgb, const rgb4_t in_RGB ) {
    out_rgb->r = extend4to8bits( in_RGB.r );
    out_rgb->g = extend4to8bits( in_RGB.g );
    out_rgb->b = extend4to8bits( in_RGB.b );
}

void convert888to555( rgb5_t * out_rgb, const rgb8_t in_RGB ) {
    out_rgb->r = LUT_5[in_RGB.r];
    out_rgb->g = LUT_5[in_RGB.g];
    out_rgb->b = LUT_5[in_RGB.b];
}

void convert555to888( rgb8_t * out_rgb, const rgb5_t in_RGB ) {
    out_rgb->r = extend5to8bits( in_RGB.r );
    out_rgb->g = extend5to8bits( in_RGB.g );
    out_rgb->b = extend5to8bits( in_RGB.b );
}



void convert888to676( rgb676_t * out_rgb, const rgb8_t in_RGB ) {
    out_rgb->r = LUT_6[in_RGB.r];
    out_rgb->g = LUT_7[in_RGB.g];
    out_rgb->b = LUT_6[in_RGB.b];
}



void convert676to888( rgb8_t * out_rgb, const rgb676_t in_RGB ) {
    out_rgb->r = extend6to8bits( in_RGB.r );
    out_rgb->g = extend7to8bits( in_RGB.g );
    out_rgb->b = extend6to8bits( in_RGB.b );
}



void convert8888to4444( rgba4444_t * out_rgba, const rgba8_t in_RGBA ) {
    out_rgba->r = LUT_4[in_RGBA.r];
    out_rgba->g = LUT_4[in_RGBA.g];
    out_rgba->b = LUT_4[in_RGBA.b];
    out_rgba->a = LUT_4[in_RGBA.a];
}



void convert4444to8888( rgba8_t * out_rgba, const rgba4444_t in_RGBA ) {
    out_rgba->r = extend4to8bits( in_RGBA.r );
    out_rgba->g = extend4to8bits( in_RGBA.g );
    out_rgba->b = extend4to8bits( in_RGBA.b );
    out_rgba->a = extend4to8bits( in_RGBA.a );
}



void convert8888to5551( rgba5551_t * out_rgba, const rgba8_t in_RGBA ) {
    out_rgba->r = LUT_5[in_RGBA.r];
    out_rgba->g = LUT_5[in_RGBA.g];
    out_rgba->b = LUT_5[in_RGBA.b];
    out_rgba->a = LUT_1[in_RGBA.a];
}



void convert5551to8888( rgba8_t * out_rgba, const rgba5551_t in_RGBA ) {
    out_rgba->r = extend5to8bits( in_RGBA.r );
    out_rgba->g = extend5to8bits( in_RGBA.g );
    out_rgba->b = extend5to8bits( in_RGBA.b );
    out_rgba->a = ( in_RGBA.a ) ? 255 : 0;
}



//const uint8_t BAYER_2X2[2][2] = { 0, 2, 3, 1 };
const uint8_t BAYER_4X4[4][4] = {
	{  0, 12,  2, 14 },
	{  8,  4, 10,  6 },
	{  3, 15,  1, 13 },
	{ 11,  7,  9,  5 }
};
//const uint8_t BAYER_8X8[8][8] = { 0, 32, 12, 44, 2, 34, 14, 46, 48, 16, 60, 28, 50, 18, 62, 30, 8, 40, 4, 36, 10, 42, 6, 38, 56, 24, 52, 20, 58, 26, 54, 22, 3, 35, 15, 47, 1, 33, 13, 45, 51, 19, 63, 31, 49, 17, 61, 29, 11, 43, 7, 39, 9, 41, 5, 37, 59, 27, 55, 23, 57, 25, 53, 21 };
//signed char LUT_BAYER_2X2[2][2];
signed char LUT_BAYER_4X4_1[4][4];
signed char LUT_BAYER_4X4_4[4][4];
signed char LUT_BAYER_4X4_5[4][4];
signed char LUT_BAYER_4X4_6[4][4];
//signed char LUT_BAYER_8X8[8][8];



void prepareBayer() {
    float div;
    float sub;
    
    //    div = 1.0f / 4.0f;
    //    sub = -3.0f / 8.0f;
    //    
    //    for ( int y = 0; y < 2; y++ ) {
    //        for ( int x = 0; x < 2; x++ ) {
    //            //LUT_BAYER_2X2[y][x] = ( 2 * BAYER_2X2[y][x] - 3 ) * in_COLOR_SPACING / 8;
    //            LUT_BAYER_2X2[y][x] = roundf( fmaf( BAYER_2X2[y][x], div, sub ) * in_COLOR_SPACING );
    //            //printf( "%i ", LUT_BAYER_2X2[y][x] );
    //        }
    //    }
    
    div = 1.0f / 16.0f;
    sub = -15.0f / 32.0f;
    
    for ( int y = 0; y < 4; y++ ) {
        for ( int x = 0; x < 4; x++ ) {
            //LUT_BAYER_4X4_1[y][x] = ( 2 * BAYER_4X4[y][x] - 15 ) * 255 / 32;
            //LUT_BAYER_4X4_4[y][x] = ( 2 * BAYER_4X4[y][x] - 15 ) * 17 / 32;
            //LUT_BAYER_4X4_5[y][x] = ( 2 * BAYER_4X4[y][x] - 15 ) * 8 / 32;
            //LUT_BAYER_4X4_6[y][x] = ( 2 * BAYER_4X4[y][x] - 15 ) * 4 / 32;
            LUT_BAYER_4X4_1[y][x] = roundf( fmaf( BAYER_4X4[y][x], div, sub ) * 255.0f );
            LUT_BAYER_4X4_4[y][x] = roundf( fmaf( BAYER_4X4[y][x], div, sub ) * 255.0f / 15.0f );
            LUT_BAYER_4X4_5[y][x] = roundf( fmaf( BAYER_4X4[y][x], div, sub ) * 255.0f / 31.0f );
            LUT_BAYER_4X4_6[y][x] = roundf( fmaf( BAYER_4X4[y][x], div, sub ) * 255.0f / 63.0f );
            //printf( "%i ", LUT_BAYER_4X4[y][x] );
        }
    }
    
    //    div = 1.0f / 64.0f;
    //    sub = -63.0f / 128.0f;
    //    
    //    for ( int y = 0; y < 8; y++ ) {
    //        for ( int x = 0; x < 8; x++ ) {
    //            //LUT_BAYER_8X8[y][x] = ( 2 * BAYER_4X4[y][x] - 63 ) * in_COLOR_SPACING / 128;
    //            LUT_BAYER_8X8[y][x] = roundf( fmaf( BAYER_8X8[y][x], div, sub ) * in_COLOR_SPACING );
    //            //printf( "%i ", LUT_BAYER_8X8[y][x] );
    //        }
    //    }
    
    //printf( "\n" );
}



void ditherRGB( rgb8_t * in_out_rgb, const format_t in_FORMAT, const int in_X, const int in_Y ) {
    int8_t dither[3] = { 0, 0, 0 };
    int x = in_X % 4;
    int y = in_Y % 4;
    
    switch ( in_FORMAT ) {
        case kRGB565:
            dither[0] = LUT_BAYER_4X4_5[y][x];
            dither[1] = LUT_BAYER_4X4_6[y][x];
            dither[2] = LUT_BAYER_4X4_5[y][x];
            break;
        default:
            return;
    }
    
    in_out_rgb->r = clampi( in_out_rgb->r + dither[0], 0, 255 );
    in_out_rgb->g = clampi( in_out_rgb->g + dither[1], 0, 255 );
    in_out_rgb->b = clampi( in_out_rgb->b + dither[2], 0, 255 );
}


void ditherRGBA( rgba8_t * in_out_rgba, const format_t in_FORMAT, const int in_X, const int in_Y ) {
    int8_t dither[4] = { 0, 0, 0, 0 };
    int x = in_X % 4;
    int y = in_Y % 4;
    
    switch ( in_FORMAT ) {
        case kRGBA4444:
            dither[0] = LUT_BAYER_4X4_4[y][x];
            dither[1] = LUT_BAYER_4X4_4[y][x];
            dither[2] = LUT_BAYER_4X4_4[y][x];
            dither[3] = LUT_BAYER_4X4_4[y][x];
            break;
        case kRGBA5551:
            dither[0] = LUT_BAYER_4X4_5[y][x];
            dither[1] = LUT_BAYER_4X4_5[y][x];
            dither[2] = LUT_BAYER_4X4_5[y][x];
            dither[3] = LUT_BAYER_4X4_1[y][x];
            break;
        case kRGBA5551A:
            dither[0] = LUT_BAYER_4X4_5[y][x];
            dither[1] = LUT_BAYER_4X4_5[y][x];
            dither[2] = LUT_BAYER_4X4_5[y][x];
            dither[3] = 0;
            break;
        default:
            break;
    }
    
    in_out_rgba->r = clampi( in_out_rgba->r + dither[0], 0, 255 );
    in_out_rgba->g = clampi( in_out_rgba->g + dither[1], 0, 255 );
    in_out_rgba->b = clampi( in_out_rgba->b + dither[2], 0, 255 );
    in_out_rgba->a = clampi( in_out_rgba->a + dither[3], 0, 255 );
}
