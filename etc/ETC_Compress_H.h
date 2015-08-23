//
//  ETC_Compress_H.h
//  GLTC
//
//  Created by Michael Kwasnicki on 04.05.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#ifndef GLTC_ETC_Compress_H_h
#define GLTC_ETC_Compress_H_h


#include "ETC_Common.h"


uint32_t compressH(ETCBlockColor_t *out_block, const rgba8_t in_BLOCK_RGBA[4][4], const Strategy_t in_STRATEGY, const bool in_OPAQUE);

void printInfoH(ETCBlockColor_t *in_BLOCK);

#endif
