//
//  PVRTC.h
//  GLTC
//
//  Created by Michael Kwasnicki on 04.03.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#ifndef GLTC_PVRTC_h
#define GLTC_PVRTC_h


#include "../colorSpaceReduction.h"

#include <stdbool.h>


bool pvrtcRead4BPPRGBA(const char in_FILE[], rgba8_t **out_image, uint32_t *out_width, uint32_t *out_height);
bool pvrtcRead2BPPRGBA(const char in_FILE[], rgba8_t **out_image, uint32_t *out_width, uint32_t *out_height);

bool pvrtcWrite4BPPRGBA(const char in_FILE[], const rgba8_t *in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT);
bool pvrtcWrite2BPPRGBA(const char in_FILE[], const rgba8_t *in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT);

void pvrtcFreeRGBA(rgba8_t **in_out_image);


#endif
