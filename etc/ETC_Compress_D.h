//
//  ETC_Compress_D.h
//  GLTC
//
//  Created by Michael Kwasnicki on 04.04.13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#ifndef GLTC_ETC_Compress_D_h
#define GLTC_ETC_Compress_D_h


#include "colorSpaceReduction.h"
#include "ETC_Common.h"

#include <stdlib.h>



void computeUniformColorLUTD( void );

uint32_t compressD( ETCBlockColor_t * out_block, const rgb8_t in_BLOCK_RGB[4][4] );

void printInfoD( ETCBlockColor_t * in_BLOCK );

#endif
