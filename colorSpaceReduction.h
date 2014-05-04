//
//  colorSpaceReduction.h
//  GLTC
//
//  Created by Michael Kwasnicki on 24.01.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#ifndef GLTC_colorSpaceReduction_h
#define GLTC_colorSpaceReduction_h

#include <iso646.h>
#include <stdint.h>

typedef union {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
    
    uint8_t array[3];
} rgb8_t;

typedef struct {
	uint16_t b : 5;
	uint16_t g : 5;
	uint16_t r : 5;
} rgb5_t;

typedef struct {
	uint16_t b : 4;
	uint16_t g : 4;
	uint16_t r : 4;
} rgb4_t;

typedef struct {
	int16_t b : 3;
	int16_t g : 3;
	int16_t r : 3;
} rgb3_t;

typedef struct {
	uint32_t b : 6;
	uint32_t g : 7;
	uint32_t r : 6;
} rgb676_t;




typedef union {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
    
    uint8_t array[4];
} rgba8_t;

// WARNING: bitfields are filled from lowest to highest bit
// 565 : rrrrrggg gggbbbbb
// 4444: rrrrggggbbbbaaaa
// 5551: rrrrrgggggbbbbba

typedef union {
    struct {
        uint16_t b : 5;
        uint16_t g : 6;
        uint16_t r : 5;
    };
    
    uint16_t b16;
} rgb565_t;

typedef union {
    struct {
        uint16_t a : 4;
        uint16_t b : 4;
        uint16_t g : 4;
        uint16_t r : 4;
    };
    
    uint16_t b16;
} rgba4444_t;

typedef union {
    struct {
        uint16_t a : 1;
        uint16_t b : 5;
        uint16_t g : 5;
        uint16_t r : 5;
    };
    
    uint16_t b16;
} rgba5551_t;

void fillLUT( void );

inline static uint8_t extend4to8bits( const int in_C4 ) {
	return (uint8_t)( ( in_C4 << 4 ) bitor in_C4 );
}

inline static uint8_t extend5to8bits( const int in_C5 ) {
	return (uint8_t)( ( in_C5 << 3 ) bitor ( in_C5 >> 2 ) );
}

inline static uint8_t extend6to8bits( const int in_C6 ) {
	return (uint8_t)( ( in_C6 << 2 ) bitor ( in_C6 >> 4 ) );
}

inline static uint8_t extend7to8bits( const int in_C7 ) {
	return (uint8_t)( ( in_C7 << 1 ) bitor ( in_C7 >> 6 ) );
}

int reduce8to7bits( const int in_c8 );
int reduce8to6bits( const int in_c8 );
int reduce8to5bits( const int in_c8 );
int reduce8to4bits( const int in_c8 );

void convert888to565( rgb565_t * out_rgb, const rgb8_t in_RGB );
void convert565to888( rgb8_t * out_rgb, const rgb565_t in_RGB );
void convert888to444( rgb4_t * out_rgb, const rgb8_t in_RGB );
void convert444to888( rgb8_t * out_rgb, const rgb4_t in_RGB );
void convert888to555( rgb5_t * out_rgb, const rgb8_t in_RGB );
void convert555to888( rgb8_t * out_rgb, const rgb5_t in_RGB );
void convert676to888( rgb8_t * out_rgb, const rgb676_t in_RGB );
void convert888to676( rgb676_t * out_rgb, const rgb8_t in_RGB );
void convert8888to4444( rgba4444_t * out_rgba, const rgba8_t in_RGBA );
void convert4444to8888( rgba8_t * out_rgba, const rgba4444_t in_RGBA );
void convert8888to5551( rgba5551_t * out_rgba, const rgba8_t in_RGBA );
void convert5551to8888( rgba8_t * out_rgba, const rgba5551_t in_RGBA );

typedef enum { kRGB565, kRGBA4444, kRGBA5551, kRGBA5551A } format_t;

void prepareBayer( void );
void ditherRGB( rgb8_t * in_out_rgb, const format_t in_FORMAT, const int in_X, const int in_Y );
void ditherRGBA( rgba8_t * in_out_rgba, const format_t in_FORMAT, const int in_X, const int in_Y );

#endif
