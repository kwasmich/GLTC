//
//  ETC_Compress_D.h
//  GLTC
//
//  Created by Michael Kwasnicki on 04.04.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#ifndef GLTC_ETC_Compress_D_h
#define GLTC_ETC_Compress_D_h


#include "ETC_Common.h"


void computeUniformColorLUTD( void );

uint32_t compressD( ETCBlockColor_t * out_block, const rgb8_t in_BLOCK_RGB[4][4], const Strategy_t in_STRATEGY );

void printInfoD( ETCBlockColor_t * in_BLOCK );

#endif
