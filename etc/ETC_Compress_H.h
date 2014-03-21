//
//  ETC_Compress_H.h
//  GLTC
//
//  Created by Michael Kwasnicki on 04.05.13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#ifndef GLTC_ETC_Compress_H_h
#define GLTC_ETC_Compress_H_h


#include "colorSpaceReduction.h"
#include "ETC_Common.h"

#include <stdlib.h>



uint32_t compressH( ETCBlockColor_t * out_block, const rgb8_t in_BLOCK_RGB[4][4] );

void printInfoH( ETCBlockColor_t * in_BLOCK );

#endif
