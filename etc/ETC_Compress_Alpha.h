//
//  ETC_Compress_Alpha.h
//  GLTC
//
//  Created by Michael Kwasnicki on 20.04.14.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#ifndef GLTC_ETC_Compress_Alpha_h
#define GLTC_ETC_Compress_Alpha_h


#include "ETC_Common.h"


uint32_t compressAlpha( ETCBlockAlpha_t * out_block, const uint8_t in_BLOCK_A[4][4], const Strategy_t in_STRATEGY );

void printInfoAlpha( ETCBlockAlpha_t * in_BLOCK );

#endif
