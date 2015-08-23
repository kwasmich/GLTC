//
//  DXTC_CompressAlpha.h
//  GLTC
//
//  Created by Michael Kwasnicki on 27.02.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#ifndef GLTC_DXTC_CompressAlpha_h
#define GLTC_DXTC_CompressAlpha_h

#include "DXTC_Common.h"

void compressDXT3BlockA(DXT3AlphaBlock_t *in_out_block, const rgba8_t in_BLOCK_RGBA[4][4]);
void compressDXT5BlockA(DXT5AlphaBlock_t *in_out_block, const rgba8_t in_BLOCK_RGBA[4][4]);

#endif
