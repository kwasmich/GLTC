//
//  ETC.h
//  GLTC
//
//  Created by Michael Kwasnicki on 04.01.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#ifndef GLTC_ETC_h
#define GLTC_ETC_h


#include "ETC_Common.h"

#include <stdbool.h>


bool etcReadETC1RGB( const char in_FILE[], rgb8_t ** out_image, uint32_t * out_width, uint32_t * out_height );
bool etcReadETC2RGB( const char in_FILE[], rgb8_t ** out_image, uint32_t * out_width, uint32_t * out_height );

bool etcWriteETC1RGB( const char in_FILE[], const rgb8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const Strategy_t in_STRATEGY );
bool etcWriteETC2RGB( const char in_FILE[], const rgb8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const Strategy_t in_STRATEGY );

void etcFreeRGB( rgb8_t ** in_out_image );


bool etcReadETC2RGBA( const char in_FILE[], rgba8_t ** out_image, uint32_t * out_width, uint32_t * out_height );
bool etcReadETC2RGBAPunchThrough( const char in_FILE[], rgba8_t ** out_image, uint32_t * out_width, uint32_t * out_height );

bool etcWriteETC2RGBA( const char in_FILE[], const rgba8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const Strategy_t in_STRATEGY );
bool etcWriteETC2RGBAPunchThrough( const char in_FILE[], const rgba8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const Strategy_t in_STRATEGY );

void etcFreeRGBA( rgba8_t ** in_out_image );



bool etcResumeWriteETC1RGB( const char in_FILE[], const rgb8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT );
bool etcReadRGBCompare( const char in_FILE_I[], const char in_FILE_D[], const char in_FILE_T[], const char in_FILE_H[], const char in_FILE_P[], const rgb8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT );

#endif
