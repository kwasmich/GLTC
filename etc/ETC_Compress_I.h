//
//  ETC_Compress_I.h
//  GLTC
//
//  Created by Michael Kwasnicki on 04.04.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#ifndef GLTC_ETC_Compress_I_h
#define GLTC_ETC_Compress_I_h


#include "colorSpaceReduction.h"
#include "ETC_Common.h"

#include <stdlib.h>



void computeUniformColorLUTI( void );

uint32_t compressI( ETCBlockColor_t * out_block, const rgb8_t in_BLOCK_RGB[4][4] );

void printInfoI( ETCBlockColor_t * in_BLOCK );

#endif
