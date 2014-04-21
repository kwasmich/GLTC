//
//  ETC_Compress.h
//  GLTC
//
//  Created by Michael Kwasnicki on 10.01.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#ifndef GLTC_ETC_Compress_h
#define GLTC_ETC_Compress_h


#include "ETC_Common.h"


void compressETC1BlockRGB( ETCBlockColor_t * out_block, const rgb8_t in_BLOCK_RGB[4][4], const Strategy_t in_STRATEGY );
void compressETC2BlockRGB( ETCBlockColor_t * out_block, const rgb8_t in_BLOCK_RGB[4][4], const Strategy_t in_STRATEGY );
void compressETC2BlockRGBA( ETC2BlockRGBA_t * out_block, const rgba8_t in_BLOCK_RGBA[4][4], const Strategy_t in_STRATEGY );

void computeUniformColorLUT( void );

#endif
