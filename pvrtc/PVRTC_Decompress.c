//
//  PVRTC_Decompress.c
//  GLTC
//
//  Created by Michael Kwasnicki on 04.03.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#include "PVRTC_Decompress.h"

#include <assert.h>
#include <iso646.h>
#include <stdbool.h>


void decodeBaseColorA( rgba8_t * out_rgba, const uint16_t in_COLOR_A ) {
    if ( in_COLOR_A bitand 0x8000 ) {   // opaque color mode
        rgb554_t colorA;
        colorA.b16 = in_COLOR_A;
        out_rgba->r = ( colorA.r << 3 ) bitor ( colorA.r >> 2 );
        out_rgba->g = ( colorA.g << 3 ) bitor ( colorA.g >> 2 );
        out_rgba->b = ( colorA.b << 4 ) bitor colorA.b;
        out_rgba->a = 255;
    } else {                            // translucent color mode
        argb3443_t colorA;
        colorA.b16 = in_COLOR_A;
        out_rgba->r = ( colorA.r << 4 ) bitor colorA.r;
        out_rgba->g = ( colorA.g << 4 ) bitor colorA.g;
        out_rgba->b = ( colorA.b << 5 ) bitor ( colorA.b << 2 ) bitor ( colorA.b >> 1 );
        out_rgba->a = ( colorA.a << 5 ) bitor ( colorA.a << 2 ) bitor ( colorA.a >> 1 );
    }
}



void decodeBaseColorB( rgba8_t * out_rgba, const uint16_t in_COLOR_B ) {
    if ( in_COLOR_B bitand 0x8000 ) {   // opaque color mode
        rgb555_t colorB;
        colorB.b16 = in_COLOR_B;
        out_rgba->r = ( colorB.r << 3 ) bitor ( colorB.r >> 2 );
        out_rgba->g = ( colorB.g << 3 ) bitor ( colorB.g >> 2 );
        out_rgba->b = ( colorB.b << 3 ) bitor ( colorB.b >> 2 );
        out_rgba->a = 255;
    } else {                            // translucent color mode
        argb3444_t colorB;
        colorB.b16 = in_COLOR_B;
        out_rgba->r = ( colorB.r << 4 ) bitor colorB.r;
        out_rgba->g = ( colorB.g << 4 ) bitor colorB.g;
        out_rgba->b = ( colorB.b << 4 ) bitor colorB.b;
        out_rgba->a = ( colorB.a << 5 ) bitor ( colorB.a << 2 ) bitor ( colorB.a >> 1 );
    }
}



void bilinearFilter4x4( rgba8_t * out_rgba, const int in_X, const int in_Y, const rgba8_t in_BLOCK[2][2] ) {
    for ( int i = 0; i < 4; i++ ) {
        uint32_t filtered = 0;
        filtered += in_BLOCK[0][0].array[i] * ( 4 - in_X ) * ( 4 - in_Y );
        filtered += in_BLOCK[0][1].array[i] * in_X * ( 4 - in_Y );
        filtered += in_BLOCK[1][0].array[i] * ( 4 - in_X ) * in_Y;
        filtered += in_BLOCK[1][1].array[i] * in_X * in_Y;
        filtered >>= 4;
        out_rgba->array[i] = filtered;
    }
}



typedef enum {
    kZERO_EIGHT  = 0,
    kTHREE_EIGHT = 3,
    kFOUR_EIGHT  = 4,
    kFOUR_EIGHT_ZERO_ALPHA = 16,
    kFIVE_EIGHT  = 5,
    kEIGHT_EIGHT = 8
} ModulationMode_t;



ModulationMode_t modulation4x4( const int in_X, const int in_Y, const PVRTC4Block_t * in_BLOCK_PVR ) {
    bool mode = in_BLOCK_PVR->a bitand 0x1;
    int shift = ( in_Y * 4 + in_X ) * 2;
    int rawMod = ( in_BLOCK_PVR->mod >> shift ) bitand 0x3;
    ModulationMode_t result;
    
    switch ( rawMod ) {
        case 0:
            result = kZERO_EIGHT;
            break;
            
        case 1:
            result = ( mode ) ? kFOUR_EIGHT : kTHREE_EIGHT;
            break;
            
        case 2:
            result = ( mode ) ? kFOUR_EIGHT_ZERO_ALPHA : kFIVE_EIGHT;
            break;
            
        case 3:
            result = kEIGHT_EIGHT;
            break;
			
		default:
			assert( false );
    }
    
    return result;
}



//  4 PVRTC Blocks
// [] Base Color
// ·· Decoded area
//+--------+--------+
//|        |        |
//|        |        |
//|    []··|····[]  |
//|    ····|····    |
//+--------+--------+
//|    ····|····    |
//|    ····|····    |
//|    []  |    []  |
//|        |        |
//+--------+--------+

void pvrtcDecodeBlock4BPP( rgba8_t out_blockRGBA[4][4], const PVRTC4Block_t in_BLOCK_PVR[2][2] ) {
    rgba8_t blockA[2][2];
    rgba8_t blockB[2][2];
    
    for ( int y = 0; y < 2; y++ ) {
        for ( int x = 0; x < 2; x++ ) {
            decodeBaseColorA( &blockA[y][x], in_BLOCK_PVR[y][x].a );
            decodeBaseColorB( &blockB[y][x], in_BLOCK_PVR[y][x].b );
        }
    }
    
    for ( int by = 0; by < 4; by++ ) {
        int dy = by + 2;
        int y = dy >> 2;
        dy &= 0x3;
        
        for ( int bx = 0; bx < 4; bx++ ) {
            int dx = bx + 2;
            int x = dx >> 2;
            dx &= 0x3;
            
            rgba8_t a;
            rgba8_t b;
            rgba8_t c;
            bilinearFilter4x4( &a, bx, by, blockA );
            bilinearFilter4x4( &b, bx, by, blockB );
            
            ModulationMode_t m = modulation4x4( dx, dy, &in_BLOCK_PVR[y][x] );
            
            switch ( m ) {
                case kZERO_EIGHT:
                    c = a;
                    break;
                    
                case kTHREE_EIGHT:
                    c.r = ( a.r * 5 + b.r * 3 + 4 ) >> 3;
                    c.g = ( a.g * 5 + b.g * 3 + 4 ) >> 3;
                    c.b = ( a.b * 5 + b.b * 3 + 4 ) >> 3;
                    c.a = ( a.a * 5 + b.a * 3 + 4 ) >> 3;
                    break;
                    
                case kFOUR_EIGHT:
                    c.r = ( a.r + b.r + 1 ) >> 1;
                    c.g = ( a.g + b.g + 1 ) >> 1;
                    c.b = ( a.b + b.b + 1 ) >> 1;
                    c.a = ( a.a + b.a + 1 ) >> 1;
                    break;
                    
                case kFOUR_EIGHT_ZERO_ALPHA:
                    c.r = ( a.r + b.r + 1 ) >> 1;
                    c.g = ( a.g + b.g + 1 ) >> 1;
                    c.b = ( a.b + b.b + 1 ) >> 1;
                    c.a = 0;
                    break;
                    
                case kFIVE_EIGHT:
                    c.r = ( a.r * 3 + b.r * 5 + 4 ) >> 3;
                    c.g = ( a.g * 3 + b.g * 5 + 4 ) >> 3;
                    c.b = ( a.b * 3 + b.b * 5 + 4 ) >> 3;
                    c.a = ( a.a * 3 + b.a * 5 + 4 ) >> 3;
                    break;
                    
                case kEIGHT_EIGHT:
                    c = b;
                    break;
                    
            }
            
            out_blockRGBA[by][bx] = c;
        }
    }
}
