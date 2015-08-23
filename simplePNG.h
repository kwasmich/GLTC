//
//  simplePNG.h
//  GLTC
//
//  Created by Michael Kwasnicki on 19.03.11.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#pragma once

#ifndef __simplePNG_H__
#define __simplePNG_H__


#include <stdbool.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

int pngCheck(const char *in_FILE);

bool pngRead(const char *in_FILE, const bool in_FLIP_Y, uint8_t **out_image, uint32_t *out_width, uint32_t *out_height, uint32_t *out_channels) ;

bool pngWrite(const char *in_FILE_NAME, uint8_t *in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const uint32_t in_CHANNELS);

void pngFree(uint8_t **in_out_image);

#ifdef __cplusplus
}
#endif


#endif //__simplePNG_H__
