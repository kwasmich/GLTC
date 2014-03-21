//
//  ETC_Compress.h
//  GLTC
//
//  Created by Michael Kwasnicki on 10.01.13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#ifndef GLTC_ETC_Compress_h
#define GLTC_ETC_Compress_h

#include "ETC_Common.h"

#include "colorSpaceReduction.h"

void compressETC1BlockRGB( ETCBlockColor_t * out_block, const rgb8_t in_BLOCK_RGB[4][4] );
void compressETC2BlockRGB( ETCBlockColor_t * out_block, const rgb8_t in_BLOCK_RGB[4][4] );

void computeUniformColorLUT();

#endif
