//
//  ETC_Compress.c
//  GLTC
//
//  Created by Michael Kwasnicki on 10.01.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#include "ETC_Compress.h"

#include "ETC_Compress_Common.h"
#include "ETC_Compress_I.h"
#include "ETC_Compress_D.h"
#include "ETC_Compress_T.h"
#include "ETC_Compress_H.h"
#include "ETC_Compress_P.h"
#include "ETC_Compress_Alpha.h"
#include "ETC_Decompress.h"

#include "../lib.h"

#include <iso646.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>



static void compressETCBlockRGB(ETCBlockColor_t *out_block, const rgba8_t in_BLOCK_RGBA[4][4], const Strategy_t in_STRATEGY, const bool in_OPAQUE, uint32_t (*in_COMPRESSION_MODE[])(ETCBlockColor_t *,
                                const rgba8_t[4][4], const Strategy_t, const bool), const int in_COMPRESSION_MODE_COUNT) {
    uint32_t error;
    uint32_t bestError = 0xFFFFFFFF;
    ETCBlockColor_t block;
    ETCBlockColor_t bestBlock;

    for (int m = 0; m < in_COMPRESSION_MODE_COUNT; m++) {
        error = in_COMPRESSION_MODE[m](&block, in_BLOCK_RGBA, in_STRATEGY, in_OPAQUE);

        if (error < bestError) {
            bestError = error;
            bestBlock = block;
        }
    }

    *out_block = bestBlock;
}



#pragma mark - exposed interface

// TODO: The H-Mode is state of the art. make all other modes comply with it
// TODO: the H-Mode and T-Mode have much code in common. reduce it!

void compressETC1BlockRGB(ETCBlockColor_t *out_block, const rgba8_t in_BLOCK_RGBA[4][4], const Strategy_t in_STRATEGY) {
    uint32_t (*compressionMode[])(ETCBlockColor_t *, const rgba8_t[4][4], const Strategy_t, const bool) = {
        &compressI,
        &compressD
    };
    const int compressionModeCount = sizeof(compressionMode) / sizeof(void *);
    compressETCBlockRGB(out_block, in_BLOCK_RGBA, in_STRATEGY, true, compressionMode, compressionModeCount);
}



void compressETC2BlockRGB(ETCBlockColor_t *out_block, const rgba8_t in_BLOCK_RGBA[4][4], const Strategy_t in_STRATEGY) {
    uint32_t (*compressionMode[])(ETCBlockColor_t *, const rgba8_t[4][4], const Strategy_t, const bool) = {
        &compressI,
        &compressD,
        &compressT,
        &compressH,
        &compressP
    };
    const int compressionModeCount = sizeof(compressionMode) / sizeof(void *);
    compressETCBlockRGB(out_block, in_BLOCK_RGBA, in_STRATEGY, true, compressionMode, compressionModeCount);
}



static void compressETC2BlockRGBA(ETCBlockColor_t *out_block, const rgba8_t in_BLOCK_RGBA[4][4], const Strategy_t in_STRATEGY, const bool in_OPAQUE) {
    uint32_t (*compressionMode[])(ETCBlockColor_t *, const rgba8_t[4][4], const Strategy_t, const bool) = {
        &compressD,
        &compressT,
        &compressH,
        &compressP	// to be omitted if not opaque
    };
    const int compressionModeCount = sizeof(compressionMode) / sizeof(void *) - ((in_OPAQUE) ? 0 : 1);
    compressETCBlockRGB(out_block, in_BLOCK_RGBA, in_STRATEGY, in_OPAQUE, compressionMode, compressionModeCount);
}



void compressETC2BlockRGBA8(ETC2BlockRGBA_t *out_block, const rgba8_t in_BLOCK_RGBA[4][4], const Strategy_t in_STRATEGY) {
    uint8_t blockA[4][4];

    for (int by = 0; by < 4; by++) {
        for (int bx = 0; bx < 4; bx++) {
            blockA[by][bx] = in_BLOCK_RGBA[by][bx].a;
        }
    }

    compressETC2BlockRGB(&out_block->color, in_BLOCK_RGBA, in_STRATEGY);
    compressAlpha(&out_block->alpha, blockA, in_STRATEGY);
}



void compressETC2BlockRGB8A1(ETCBlockColor_t *out_block, const rgba8_t in_BLOCK_RGBA[4][4], const Strategy_t in_STRATEGY) {
    rgba8_t min, max;
    computeBlockMinMax(&min, &max, in_BLOCK_RGBA);

    if (min.a < 128 and max.a < 128) {
        // completely transparent
        out_block->b64 = 0x00000000FFFF0000ULL;
    } else if (min.a >= 128 and max.a >= 128) {
        // completely opaque
        //out_block->b64 = 0x0000000000000000ULL;
        compressETC2BlockRGBA(out_block, in_BLOCK_RGBA, in_STRATEGY, true);
    } else {
        // something in between
        //out_block->b64 = 0xFFFFFFFFFFFFFFFFULL;
        compressETC2BlockRGBA(out_block, in_BLOCK_RGBA, in_STRATEGY, false);
        //out_block->differential = false;
    }
}



void computeUniformColorLUT() {
    computeUniformColorLUTI();
    computeUniformColorLUTD();
    computeUniformColorLUTT(); // H is a subset of T, so there is no use
    computeUniformColorLUTP();
}


