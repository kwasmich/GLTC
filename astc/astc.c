//
//  astc.c
//  GLTC
//
//  Created by Michael Kwasnicki on 20.08.15.
//  Copyright (c) 2015 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

// https://github.com/ARM-software/astc-encoder.git

#include <assert.h>
#include <iso646.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>


typedef struct uint128_t {
    uint64_t lo;
    uint64_t hi;
} uint128_t;


static uint128_t shr128(const uint128_t in_V, const uint8_t in_S) {
    assert(in_S <= 128);
    uint128_t result = in_V;

    if (in_S >= 64) {
        result.lo = result.hi;
        result.hi = 0;
        result = shr128(result, in_S - 64);
    } else {
        result.lo = (result.hi << (64 - in_S)) bitor (result.lo >> in_S);
        result.hi = (result.hi >> in_S);
    }

    return result;
}



typedef union Octet_s {
    uint8_t u8;

    struct {
        uint8_t b0 : 1;
        uint8_t b1 : 1;
        uint8_t b2 : 1;
        uint8_t b3 : 1;
        uint8_t b4 : 1;
        uint8_t b5 : 1;
        uint8_t b6 : 1;
        uint8_t b7 : 1;
    };
} Octet_s;

typedef struct ASTCBlock_s {
    uint64_t blockMode  : 11;
    uint64_t part       :  2;
    uint64_t            : 51;
    uint64_t weights;
} ASTCBlock_s;

// ?
//typedef struct ASTCBlockMultiPart_s {
//    uint64_t weights;
//    uint64_t blockMode  : 11;
//    uint64_t part       :  2; // non-zero
//    uint64_t cem        :  2;
//    uint64_t cem1       :  4;
//    uint64_t cem2       :  4;
//    uint64_t cem3       :  4;
//    uint64_t extra      :  1;
//    uint64_t colorEnd   : 12;
//    uint64_t fill       : 13;
//    uint64_t more       :  6;
//    uint64_t a          :  5;
//} ASTCBlockMultiPart_s;


typedef union ASTCBlockMode_s {
    struct {
        uint16_t R1a : 1;
        uint16_t R2a : 1;
        uint16_t R1b : 1;
        uint16_t R2b : 1;
        uint16_t R0  : 1;
        uint16_t A   : 2;
        uint16_t B   : 2;
        uint16_t H   : 1;
        uint16_t D   : 1;
    };
    
    uint16_t u16;
} ASTCBlockMode_s;


typedef struct BlockMode_s {
    bool dualPlane;
    bool highPrecisionRange;
    uint8_t r;
    uint8_t dimX;
    uint8_t dimY;
    uint8_t dimZ;
    bool isVoidExtent;
} BlockMode_s;


typedef struct BlockInfo_s {
    uint8_t dimX;                       // [2...12]
    uint8_t dimY;                       // [2...12]
    uint8_t dimZ;                       // [1...6]
    uint8_t range;                      // [2...32]
    uint8_t numBits;                    // [0...8]
    uint8_t numTrits;                   // [0...1]
    uint8_t numQuints;                  // [0...1]
    uint8_t numPartitions;              // [1...4]
    uint8_t partitionPatternIndex;      // [0...1023] ???
    uint8_t colorEndpointMode[4];       // [0...15]
    void *colorEndpointData;
    uint8_t numPlanes;                  // [1...2]
    uint8_t planeToChannelAssignment;   // [0...3]
    bool isVoidExtent;
} BlockInfo_s;





typedef union bla_s {
    struct {
        uint32_t a : 10;
        uint32_t   : 10;
        uint32_t c : 10;
    };

    uint32_t d;
} bla_s;



/*
    -------------------------------------------------------------------------
 m  10  9   8   7   6   5   4   3   2   1   0   Width Height Notes
    -------------------------------------------------------------------------
 0  D   H     B       A     R0  0   0   R2  R1  B+4   A+2
 1  D   H     B       A     R0  0   1   R2  R1  B+8   A+2
 2  D   H     B       A     R0  1   0   R2  R1  A+2   B+8
 3  D   H   0   B     A     R0  1   1   R2  R1  A+2   B+6
 4  D   H   1   B     A     R0  1   1   R2  R1  B+2   A+2
 5  D   H   0   0     A     R0  R2  R1  0   0   12    A+2
 6  D   H   0   1     A     R0  R2  R1  0   0   A+2   12
 9  D   H   1   1   0   0   R0  R2  R1  0   0   6     10
10  D   H   1   1   0   1   R0  R2  R1  0   0   10    6
 7    B     1   0     A     R0  R2  R1  0   0   A+6   B+6   D=0, H=0
11  x   x   1   1   1   1   1   1   1   0   0   -     -     Void-extent
 8  x   x   1   1   1   x   x   x   x   0   0   -     -     Reserved*
12  x   x   x   x   x   x   x   0   0   0   0   -     -     Reserved
    -------------------------------------------------------------------------
*/


//H:[0,1]
//D:[0,1]
//R:[2...7] (special case R:0 for Void-extent)
//2x[2...12]
//3x[2...12]
//4x[2...12]
//5x[2...12]
//6x[2...5,10] ([6...9] only with D=0, H=0)
//7x[2...5]    ([6...9] only with D=0, H=0)
//8x[2...5]    ([6...9] only with D=0, H=0)
//9x[2...5]    ([6...9] only with D=0, H=0)
//10x[2...5,6]
//11x[2...5]
//12x[2...5]
//      regular              + D=0,H=0 + void-extent
//2*2*6*(4 * 11 + 7 * 4 + 2) + 6*(4*4) + 4


static const uint16_t ASTC_BLOCK_MODE_COMPARE[]  = { 0b00000000000, 0b00000000100, 0b00000001000, 0b00000001100, 0b00100001100, 0b00000000000, 0b00010000000, 0b00100000000, 0b00111000000, 0b00110000000, 0b00110100000, 0b00111111100, 0b00000000000 };
static const uint16_t ASTC_BLOCK_MODE_MASK[]     = { 0b00000001100, 0b00000001100, 0b00000001100, 0b00100001100, 0b00100001100, 0b00110000011, 0b00110000011, 0b00110000011, 0b00111000011, 0b00111100011, 0b00111100011, 0b00111111111, 0b00000001111 };


static BlockMode_s astcParseBlockMode(const ASTCBlockMode_s in_MODE) {
    bool d = true;
    bool h = false;
    int r = 2;
    int a = 0;
    int b = 0;
    int ww = 0;
    int wh = 0;

    int ra = ( in_MODE.R2a << 2 ) bitor ( in_MODE.R1a << 1 ) bitor in_MODE.R0;
    int rb = ( in_MODE.R2b << 2 ) bitor ( in_MODE.R1b << 1 ) bitor in_MODE.R0;
    int m = 0;
    bool isVoidExtent = false;

    for ( int i = 0; i < 13; i++ ) {
        if ( ( in_MODE.u16 bitand ASTC_BLOCK_MODE_MASK[i] ) == ASTC_BLOCK_MODE_COMPARE[i] ) {
            m = i;
        }
    }

    h = in_MODE.H;
    d = in_MODE.D;
    a = in_MODE.A;
    b = in_MODE.B;

    switch ( m ) {
        case 0:
            r = ra;
            ww = b + 4;
            wh = a + 2;
            break;

        case 1:
            r = ra;
            ww = b + 8;
            wh = a + 2;
            break;

        case 2:
            r = ra;
            ww = a + 2;
            wh = b + 8;
            break;

        case 3:
            r = ra;
            b = b bitand 0x1;
            ww = a + 2;
            wh = b + 6;
            break;

        case 4:
            r = ra;
            b = b bitand 0x1;
            ww = b + 2;
            wh = a + 2;
            break;

        case 5:
            r = rb;
            ww = 12;
            wh = a + 2;
            break;

        case 6:
            r = rb;
            ww = a + 2;
            wh = 12;
            break;

        case 9:
            r = rb;
            ww = 6;
            wh = 10;
            break;

        case 10:
            r = rb;
            ww = 10;
            wh = 6;
            break;

        case 7:
            r = rb;
            b = ( d << 1 ) bitor h;
            d = 0;
            h = 0;
            ww = a + 6;
            wh = b + 6;
            break;

        case 11:
            r = 0;
            d = 0;
            h = 0;
            ww = 0;
            wh = 0;
            isVoidExtent = true;
            break;

        case 8:
        case 12:
            r = 0;
            d = 0;
            h = 0;
            ww = 0;
            wh = 0;
            assert(false);
            break;
    }

    //printf( "m:%2d D:%d H:%d R:%d Width:%2d Height:%2d\n", m, d, h, r, ww, wh );
    //printf( "H:%d R:%d dim:%2dx%-2d D:%d m:%2d %d\n", h, r, ww, wh, d, m, in_MODE );

    BlockMode_s result = { .dualPlane = d, .highPrecisionRange = h, .r = r, .dimX = ww, .dimY = wh, .dimZ = 1, .isVoidExtent = isVoidExtent };
    return result;
}


static void astcDecodeTritSequence(uint8_t (*out_values)[5], const uint8_t in_SEQUENCE) {
    uint8_t T = in_SEQUENCE;
    uint8_t C = 0;

    if ((T bitand 0b00011100) == 0b00011100) {
        C = ((T bitand 0b11100000) >> 3) bitor (T bitand 0b00000011);
        (*out_values)[3] = 2;
        (*out_values)[4] = 2;
    } else {
        C = T bitand 0b00011111;

        if ((T bitand 0b01100000) == 0b01100000) {
            (*out_values)[3] = T >> 7;
            (*out_values)[4] = 2;
        } else {
            (*out_values)[3] = (T >> 5) bitand 0b00000011;
            (*out_values)[4] = T >> 7;
        }
    }

    if ((C bitand 0b00000011) == 0b00000011) {
        (*out_values)[0] = ((C >> 2) bitand 0b00000010) bitor (((C >> 2) bitand 0b00000001) bitand compl ((C >> 3) bitand 0b00000001));
        (*out_values)[1] = (C >> 4) bitand 0b00000001 ;
        (*out_values)[2] = 2;
    } else if ((C bitand 0b00001100) == 0b00001100) {
        (*out_values)[0] = C bitand 0b00000011;
        (*out_values)[1] = 2;
        (*out_values)[2] = 2;
    } else {
        (*out_values)[0] = (C bitand 0b00000010) bitor ((C bitand 0b00000001) bitand compl ((C >> 1) bitand 0b00000001));
        (*out_values)[1] = (C >> 2) bitand 0b00000011;
        (*out_values)[2] = (C >> 4) bitand 0b00000001;
    }

//    printf( "%3i > ", in_SEQUENCE );
//
//    for ( int i = 4; i >= 0; i-- ) {
//        printf( "%d ", (*out_values)[i] );
//    }
//
//    puts("");
}


static void astcDecodeQuintSequence(uint8_t (*out_values)[3], const uint8_t in_SEQUENCE) {
    uint8_t Q = in_SEQUENCE;
    uint8_t C = 0;

    if (((Q bitand 0b00000110) == 0b00000110) and ((Q bitand 0b01100000) == 0b00000000)) {
        (*out_values)[0] = 4;
        (*out_values)[1] = 4;
        (*out_values)[2] = ((Q bitand 0b00000001) << 2) bitor ((((Q >> 4) bitand 0b00000001) bitand compl (Q bitand 0b00000001)) << 1) bitor (((Q >> 3) bitand 0b00000001) bitand compl (Q bitand 0b00000001));
    } else {
        if ((Q bitand 0b00000110) == 0b00000110) {
            C = (Q bitand 0b00011000) bitor ((compl (Q >> 4)) bitand 0b00000110) bitor (Q bitand 0b00000001);
            (*out_values)[2] = 4;
        } else {
            C = (Q bitand 0b00011111);
            (*out_values)[2] = (Q >> 5) bitand 0b00000011;
        }

        if ((C bitand 0b00000111) == 0b00000101) {
            (*out_values)[1] = 4;
            (*out_values)[0] = (C >> 3) bitand 0b00000011;
        } else {
            (*out_values)[1] = (C >> 3) bitand 0b00000011;
            (*out_values)[0] = C bitand 0b00000111;
        }
    }

//    printf( "%3i > ", in_SEQUENCE );
//
//    for ( int i = 2; i >= 0; i-- ) {
//        printf( "%d ", (*out_values)[i] );
//    }
//    
//    puts("");
}


typedef struct ASTCTextureInfo_s {
    uint16_t textureWidth;
    uint16_t textureHeight;
    uint8_t dims; // [2, 3]
    uint8_t blockDimX;
    uint8_t blockDimY;
    uint8_t blockDimZ;
    bool hdr;  // LDR otherwise
    bool sRGB; // only for unorm8
    bool fp16; // or unorm8?
} ASTCTextureInfo_s;


// block sizes
//4x4
//5x[4,5]
//6x[5,6]
//8x[5,6,8]
//10x[5,6,8,10]
//12x[10,12]

// > different quality achievable when image is rotated by 90, 180, 270 deg
// endpoint reversal -> ambiguity?


//static void integerSequenceEncode(const uint8_t in_SEQUENCE) {
//    int i = 0;
//    for ( int a = 0; a < 5; a++ ) {
//        for ( int b = 0; b < 5; b++ ) {
//            for ( int c = 0; c < 5; c++ ) {
//                i = a * 25 + b * 5 + c;
//
//                printf( "%i %i %i > %3i\n", a, b, c, i );
//            }
//        }
//    }
//}
//
//
//static void integerSequenceDecode(const uint8_t in_SEQUENCE, const uint8_t in_BASE) {
//    int out[8];
//    div_t d;
//    d.quot = in_SEQUENCE;
//    int i = 0;
//
//    while (d.quot > 0) {
//        d = div(d.quot, in_BASE);
//        out[i] = d.rem;
//        i++;
//    }
//
//    printf( "%3i > ", in_SEQUENCE );
//
//    for ( i = 7; i >= 0; i-- ) {
//        printf( "%d ", out[i] );
//    }
//
//    puts("");
//}


//static void getValueFromStreamBits(const uint32_t in_STREAM, const uint8_t in_WIDTH, const uint8_t in_INDEX) {
//    assert(in_WIDTH <= 8);
//    int b1 = in_INDEX * in_WIDTH + in_WIDTH - 1;
//    int b2 = in_INDEX * in_WIDTH;
//
//    printf("[%i:%i]\n", b1, b2);
//}
//
//
//static void getValueFromStreamTrits(const uint32_t in_STREAM, const uint8_t in_WIDTH, const uint8_t in_INDEX) {
//    assert(in_WIDTH <= 6);
//    int packedBlockStart = (in_INDEX / 5) * (8 + 5 * in_WIDTH);
//
//    printf("[%i]\n", packedBlockStart);
//}


static void decodeTritBlock(uint8_t (*out_values)[5], const uint64_t in_BLOCK, const uint8_t in_BITS) {
    const uint8_t n = in_BITS;
    const uint8_t bitsMask = (1 << n) - 1;
    uint64_t block = in_BLOCK;
    uint8_t bits[5];
    uint8_t trits[5];
    uint8_t tritSequence = 0;

    bits[0] = block bitand bitsMask;
    block >>= n;
    tritSequence |= block bitand 0b11;
    block >>= 2;
    bits[1] = block bitand bitsMask;
    block >>= n;
    tritSequence |= (block bitand 0b11) << 2;
    block >>= 2;
    bits[2] = block bitand bitsMask;
    block >>= n;
    tritSequence |= (block bitand 0b1) << 4;
    block >>= 1;
    bits[3] = block bitand bitsMask;
    block >>= n;
    tritSequence |= (block bitand 0b11) << 5;
    block >>= 2;
    bits[4] = block bitand bitsMask;
    block >>= n;
    tritSequence |= (block bitand 0b1) << 7;
    block >>= 1;

    astcDecodeTritSequence(&trits, tritSequence);

    for (int i = 0; i < 5; i++) {
        (*out_values)[i] = (trits[i] << n) bitor bits[i];
    }
}


static void decodeQuintBlock(uint8_t (*out_values)[3], const uint64_t in_BLOCK, const uint8_t in_BITS) {
    const uint8_t n = in_BITS;
    const uint8_t bitsMask = (1 << n) - 1;
    uint64_t block = in_BLOCK;
    uint8_t bits[3];
    uint8_t quints[3];
    uint8_t quintSequence = 0;

    bits[0] = block bitand bitsMask;
    block >>= n;
    quintSequence |= block bitand 0b111;
    block >>= 3;
    bits[1] = block bitand bitsMask;
    block >>= n;
    quintSequence |= (block bitand 0b11) << 3;
    block >>= 2;
    bits[2] = block bitand bitsMask;
    block >>= n;
    quintSequence |= (block bitand 0b11) << 5;
    block >>= 2;

    astcDecodeQuintSequence(&quints, quintSequence);

    for (int i = 0; i < 3; i++) {
        (*out_values)[i] = (quints[i] << n) bitor bits[i];
    }
}


typedef struct QuantizationMode_s {
    uint16_t range;
    uint8_t numBits;
    uint8_t numTrits;
    uint8_t numQuints;
} QuantizationMode_s;


static void decodeStream(uint8_t (* const out_values)[64], const uint8_t in_NUM_VALUES, const uint128_t in_VALUE_STREAM, const QuantizationMode_s in_QUANT_MODE) {
    assert(not((in_QUANT_MODE.numTrits > 0) and (in_QUANT_MODE.numQuints > 0)));
    assert((in_QUANT_MODE.numTrits > 0) or (in_QUANT_MODE.numQuints > 0) or (in_QUANT_MODE.numBits > 0));

    const uint8_t n = in_QUANT_MODE.numBits;
    uint16_t bitsToProcess = 0;
    uint128_t packedWeights = in_VALUE_STREAM;
    uint8_t numWeights = 0;
    uint8_t packedBlockSize = 0;

    if (in_QUANT_MODE.numTrits) {
        assert(n <= 6);
        packedBlockSize = 8 + 5 * n;
        bitsToProcess = packedBlockSize * ((in_NUM_VALUES + 4) / 5);
    } else if (in_QUANT_MODE.numQuints) {
        assert(n <= 5);
        packedBlockSize = 7 + 3 * n;
        bitsToProcess = packedBlockSize * ((in_NUM_VALUES + 2) / 3);
    } else {
        assert(n <= 8);
        packedBlockSize = n;
        bitsToProcess = packedBlockSize * in_NUM_VALUES;
    }

    assert(bitsToProcess <= 128);

    const uint64_t blockMask = (1ULL << packedBlockSize) - 1;
    uint8_t trits[5];
    uint8_t quints[3];

    while (bitsToProcess >= packedBlockSize) {
        uint64_t block = packedWeights.lo bitand blockMask;

        if (in_QUANT_MODE.numTrits) {
            decodeTritBlock(&trits, block, n);

            for (int i = 0; i < 5; i++) {
                (*out_values)[numWeights] = trits[i];
                numWeights++;
            }
        } else if (in_QUANT_MODE.numQuints) {
            decodeQuintBlock(&quints, block, n);

            for (int i = 0; i < 3; i++) {
                (*out_values)[numWeights] = quints[i];
                numWeights++;
            }
        } else {
            (*out_values)[numWeights] = block;
            numWeights++;
        }

        packedWeights = shr128(packedWeights, packedBlockSize);
        bitsToProcess -= packedBlockSize;
    }

    if (bitsToProcess > 0) {
        printf("remaining %d bits\n", bitsToProcess);
    }

//    for (int i = 0; i < numWeights; i++) {
//        printf("%d\n", (*out_values)[i]);
//    }
}



static QuantizationMode_s decodeWeightQuantizationMode(const BlockMode_s in_BLOCK_MODE) {
    static const uint8_t RANGES[16] = { 0xFF, 0xFF, 2, 3, 4, 5, 6, 8, 0xFF, 0xFF, 10, 12, 16, 20, 24, 32 };
    static const uint8_t BITS[16]   = { 0, 0, 1, 0, 2, 0, 1, 3, 0, 0, 1, 2, 4, 2, 3, 5 };
    static const bool TRITS[16]     = { 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0 };
    static const bool QUINTS[16]    = { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0 };
    uint8_t rangeIndex = (in_BLOCK_MODE.highPrecisionRange << 3) bitor in_BLOCK_MODE.r;
    return (QuantizationMode_s){ .range = RANGES[rangeIndex], .numBits = BITS[rangeIndex], .numTrits = TRITS[rangeIndex], .numQuints = QUINTS[rangeIndex] };
}


//static void decodeWeights(uint8_t (*out_weights)[162], const uint8_t in_NUM_WEIGHTS, const uint64_t in_WEIGHT_STREAM, const Range_s in_RANGE) {
//    decodeStream(in_WEIGHT_STREAM, in_RANGE.bits, in_RANGE.trits, in_RANGE.quints);
//}


static uint8_t expand8(const uint16_t in_VALUE, const uint8_t in_BITS) {
    uint8_t result = 0;

    switch (in_BITS) {
        case 1:
            assert(in_VALUE < 2);
            result = (in_VALUE << 1) bitor in_VALUE;
            result = (result << 2) bitor result;
            result = (result << 4) bitor result;
            break;

        case 2:
            assert(in_VALUE < 4);
            result = (in_VALUE << 2) bitor in_VALUE;
            result = (result << 4) bitor result;
            break;

        case 3:
            assert(in_VALUE < 8);
            result = (in_VALUE << 5) bitor (in_VALUE << 2) bitor (in_VALUE >> 1);
            break;

        case 4:
            assert(in_VALUE < 16);
            result = (in_VALUE << 4) bitor in_VALUE;
            break;

        case 5:
            assert(in_VALUE < 32);
            result = (in_VALUE << 3) bitor (in_VALUE >> 2);
            break;

        case 6:
            assert(in_VALUE < 64);
            result = (in_VALUE << 2) bitor (in_VALUE >> 4);
            break;

        case 7:
            assert(in_VALUE < 128);
            result = (in_VALUE << 1) bitor (in_VALUE >> 6);
            break;

        case 8:
            assert(in_VALUE < 256);
            result = in_VALUE;
            break;

        default:
            assert(false);
    }
    
    return result;
}



// C.2.13  Endpoint Unquantization
// -------------------------------

static void unquantizeColorValues(uint8_t (* const out_unquantizedColorEndpointValues)[18], const uint8_t in_RAW_COLOR_ENDPOINT_VALUES[static const 18], const uint8_t in_NUM_COLOR_ENDPOINT_VALUES, const QuantizationMode_s in_QUANT_MODE) {
    assert(not((in_QUANT_MODE.numTrits > 0) and (in_QUANT_MODE.numQuints > 0)));
    assert((in_QUANT_MODE.numTrits > 0) or (in_QUANT_MODE.numQuints > 0) or (in_QUANT_MODE.numBits > 0));

    static const uint8_t TRIT[3] = { 0, 128, 255 };
    static const uint8_t QUINT[5] = { 0, 64, 128, 191, 255 };
    uint16_t a = 0;
    uint16_t b = 0;
    uint16_t c = 0;
    uint16_t t = 0;

    for (size_t i = 0; i < in_NUM_COLOR_ENDPOINT_VALUES; i++) {
        if (in_QUANT_MODE.numTrits > 0) {
            if (in_QUANT_MODE.numBits == 0) {
                assert(in_RAW_COLOR_ENDPOINT_VALUES[i] < 3);
                t = TRIT[in_RAW_COLOR_ENDPOINT_VALUES[i]];
            } else {
                uint8_t bits = in_RAW_COLOR_ENDPOINT_VALUES[i] bitand ((1 << in_QUANT_MODE.numBits) - 1);
                uint8_t trit = in_RAW_COLOR_ENDPOINT_VALUES[i] >> in_QUANT_MODE.numBits;
                assert(trit < 3);

                a = bits bitand 0x1;
                a = (a << 1) bitor a;
                a = (a << 2) bitor a;
                a = (a << 4) bitor a;
                a = (a << 8) bitor a;
                a = a bitand 0x1FF;
                bits >>= 1;

                switch (in_QUANT_MODE.numBits) {
                    case 1:
                        b = 0;
                        c = 204;
                        break;

                    case 2:
                        b = (bits << 8) bitor (bits << 4) bitor (bits << 2) bitor (bits << 1);
                        c = 93;
                        break;

                    case 3:
                        b = (bits << 7) bitor (bits << 2) bitor bits;
                        c = 44;
                        break;

                    case 4:
                        b = (bits << 6) bitor bits;
                        c = 22;
                        break;

                    case 5:
                        b = (bits << 5) bitor (bits >> 2);
                        c = 11;
                        break;

                    case 6:
                        b = (bits << 4) bitor (bits >> 4);
                        c = 11;
                        break;

                    default:
                        assert(false);
                }

                t = trit * c + b;
                t = t xor a;
                t = (a bitand 0x80) bitor (t >> 2);
            }
        } else if (in_QUANT_MODE.numQuints > 0) {
            if (in_QUANT_MODE.numBits == 0) {
                assert(in_RAW_COLOR_ENDPOINT_VALUES[i] < 5);
                t = QUINT[in_RAW_COLOR_ENDPOINT_VALUES[i]];
            } else {
                uint8_t bits = in_RAW_COLOR_ENDPOINT_VALUES[i] bitand ((1 << in_QUANT_MODE.numBits) - 1);
                uint8_t quint = in_RAW_COLOR_ENDPOINT_VALUES[i] >> in_QUANT_MODE.numBits;
                assert(quint < 5);

                a = bits bitand 0x1;
                a = (a << 1) bitor a;
                a = (a << 2) bitor a;
                a = (a << 4) bitor a;
                a = (a << 8) bitor a;
                a = a bitand 0x1FF;
                bits >>= 1;

                switch (in_QUANT_MODE.numBits) {
                    case 1:
                        b = 0;
                        c = 113;
                        break;

                    case 2:
                        b = (bits << 8) bitor (bits << 3) bitor (bits << 2);
                        c = 54;
                        break;

                    case 3:
                        b = (bits << 7) bitor (bits << 1) bitor (bits >> 1);
                        c = 26;
                        break;

                    case 4:
                        b = (bits << 6) bitor (bits >> 1);
                        c = 13;
                        break;

                    case 5:
                        b = (bits << 5) bitor (bits >> 3);
                        c = 6;
                        break;
                        
                    default:
                        assert(false);
                }

                t = quint * c + b;
                t = t xor a;
                t = (a bitand 0x80) bitor (t >> 2);
            }
        } else {
            t = expand8(in_RAW_COLOR_ENDPOINT_VALUES[i], in_QUANT_MODE.numBits);
        }

        (*out_unquantizedColorEndpointValues)[i] = t;
    }
}


// decode to RGBA8, sRGBA8, RGBA_FP16

// CEM:
// one partition: 4bit
// multiple partitions:
// 00 + 4bit
// or
// 2bit for endpoint class + 3 bits per partition (split at CEM and before weights)

typedef union rgba8_s {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    uint8_t u8[4];
} rgba8_s;

#define RGBA8(R, G, B, A) (rgba8_s){ .r = clamp8(R), .g = clamp8(G), .b = clamp8(B), .a = clamp8(A) }

typedef struct rgba16_s {
    struct {
        uint16_t r;
        uint16_t g;
        uint16_t b;
        uint16_t a;
    };

    uint16_t u16[4];
} rgba16_s;

#define RGBA16(R, G, B, A) (rgba16_s){ .r = R, .g = G, .b = B, .a = A }


static uint8_t clamp8(const uint16_t in_VALUE) {
    return (in_VALUE > 0xFF) ? 0xFF : in_VALUE;
}


typedef struct color_s {
    int r;
    int g;
    int b;
    int a;
} color_s;

#define COLOR(R, G, B, A) (color_s){ .r = R, .g = G, .b = B, .a = A }


void clamp_unorm8(color_s * const c) {
    if(c->r < 0) {c->r=0;} else if(c->r > 255) {c->r=255;}
    if(c->g < 0) {c->g=0;} else if(c->g > 255) {c->g=255;}
    if(c->b < 0) {c->b=0;} else if(c->b > 255) {c->b=255;}
    if(c->a < 0) {c->a=0;} else if(c->a > 255) {c->a=255;}
}



static void bit_transfer_signed(uint8_t * const a, uint8_t * const b) {
    *b >>= 1;
    *b |= *a bitand 0x80;
    *a >>= 1;
    *a &= 0x3F;

    if ((*a bitand 0x20) != 0) {
        *a -= 0x40;
    }
}


static rgba8_s blue_contract(int r, int g, int b, int a) {
    rgba8_s c;
    c.r = (r+b) >> 1;
    c.g = (g+b) >> 1;
    c.b = b;
    c.a = a;
    return c;
}


static void decodeColorEndpoint(rgba8_s (* const out_colorEndpoints)[2], const uint8_t in_UNQUANTIZED_COLOR_ENDPOINT_VALUES[static const 8], const uint8_t in_CEM) {
    uint8_t v[8];
    rgba8_s e0, e1;
    uint16_t L0, L1;
    uint16_t s0, s1;

    for (int i = 0; i < 8; i++) {
        v[i] = in_UNQUANTIZED_COLOR_ENDPOINT_VALUES[i];
    }

    switch (in_CEM) {
        case 0:
            e0 = RGBA8(v[0], v[0], v[0], 0xFF);
            e1 = RGBA8(v[1], v[1], v[1], 0xFF);
            break;

        case 1:
            L0 = (v[0] >> 2) bitor (v[1] & 0xC0);
            L1 = L0 + (v[1] & 0x3F);

            if (L1 > 0xFF) {
                L1 = 0xFF;
            }

            e0 = RGBA8(L0, L0, L0, 0xFF);
            e1 = RGBA8(L1, L1, L1, 0xFF);
            break;

        case 4:
            e0 = RGBA8(v[0], v[0], v[0], v[2]);
            e1 = RGBA8(v[1], v[1], v[1], v[3]);
            break;

        case 5:
            bit_transfer_signed(&v[1],&v[0]);
            bit_transfer_signed(&v[3],&v[2]);
            e0 = RGBA8(v[0], v[0], v[0], v[2]);
            e1 = RGBA8(v[0] + v[1], v[0] + v[1], v[0] + v[1], v[2] + v[3]);
            //clamp_unorm8(e0);
            //clamp_unorm8(e1);
            break;

        case 6:
            e0 = RGBA8(v[0] * v[3] >> 8, v[1] * v[3] >> 8, v[2] * v[3] >> 8, 0xFF);
            e1 = RGBA8(v[0], v[1], v[2], 0xFF);
            break;

        case 8:
            s0 = v[0] + v[2] + v[4];
            s1 = v[1] + v[3] + v[5];

            if (s1 >= s0) {
                e0 = RGBA8(v[0], v[2], v[4], 0xFF);
                e1 = RGBA8(v[1], v[3], v[5], 0xFF);
            } else {
                e0=blue_contract(v[1], v[3], v[5], 0xFF);
                e1=blue_contract(v[0], v[2], v[4], 0xFF);
            }

            break;

        case 9:
            bit_transfer_signed(&v[1],&v[0]);
            bit_transfer_signed(&v[3],&v[2]);
            bit_transfer_signed(&v[5],&v[4]);

            if (v[1] + v[3] + v[5] >= 0) {
                e0 = RGBA8(v[0], v[2], v[4], 0xFF);
                e1 = RGBA8(v[0] + v[1], v[2] + v[3], v[4] + v[5], 0xFF);
            } else {
                e0 = blue_contract(v[0] + v[1], v[2] + v[3], v[4] + v[5], 0xFF);
                e1 = blue_contract(v[0], v[2], v[4], 0xFF);
            }

            //clamp_unorm8(e0);
            //clamp_unorm8(e1);
            break;

        case 10:
            e0 = RGBA8(v[0] * v[3] >> 8, v[1] * v[3] >> 8, v[2] * v[3] >> 8, v[4]);
            e1 = RGBA8(v[0], v[1], v[2], v[5]);
            break;

        case 12:
            s0 = v[0] + v[2] + v[4];
            s1 = v[1] + v[3] + v[5];

            if (s1 >= s0) {
                e0 = RGBA8(v[0], v[2], v[4], v[6]);
                e1 = RGBA8(v[1], v[3], v[5], v[7]);
            } else {
                e0 = blue_contract(v[1], v[3], v[5], v[7]);
                e1 = blue_contract(v[0], v[2], v[4], v[6]);
            }

            break;

        case 13:
            bit_transfer_signed(&v[1], &v[0]);
            bit_transfer_signed(&v[3], &v[2]);
            bit_transfer_signed(&v[5], &v[4]);
            bit_transfer_signed(&v[7], &v[6]);

            if (v[1] + v[3] + v[5] >= 0) {
                e0 = RGBA8(v[0], v[2], v[4], v[6]);
                e1 = RGBA8(v[0] + v[1], v[2] + v[3], v[4] + v[5], v[6] + v[7]);
            } else {
                e0 = blue_contract(v[0] + v[1], v[2] + v[3], v[4] + v[5], v[6] + v[7]);
                e1 = blue_contract(v[0], v[2], v[4], v[6]);
            }

            //clamp_unorm8(e0);
            //clamp_unorm8(e1);
            break;
    }

    (*out_colorEndpoints)[0] = e0;
    (*out_colorEndpoints)[1] = e1;
}



static uint64_t reverseBitOrder(const uint64_t in_V) {
    uint64_t v = in_V;
    uint64_t r = v;
    size_t s = sizeof(v) * CHAR_BIT - 1;

    for (v >>= 1; v; v >>= 1) {
        r <<= 1;
        r |= v & 1;
        s--;
    }

    r <<= s;
    return r;
}


static uint8_t expand6(const uint8_t in_VALUE, const uint8_t in_BITS) {
    uint8_t result = 0;

    switch (in_BITS) {
        case 1:
            assert(in_VALUE < 2);
            result = (in_VALUE << 2) bitor (in_VALUE << 1) bitor in_VALUE;
            result = (result << 3) bitor result;
            break;

        case 2:
            assert(in_VALUE < 4);
            result = (in_VALUE << 4) bitor (in_VALUE << 2) bitor in_VALUE;
            break;

        case 3:
            assert(in_VALUE < 8);
            result = (in_VALUE << 3) bitor in_VALUE;
            break;

        case 4:
            assert(in_VALUE < 16);
            result = (in_VALUE << 2) bitor (in_VALUE >> 2);
            break;

        case 5:
            assert(in_VALUE < 32);
            result = (in_VALUE << 1) bitor (in_VALUE >> 4);
            break;

        default:
            assert(false);
    }

    return result;
}


// actually performs something like the following:
// roundf((float)weights[i] * 64.0f / maxValueInRange);
// but only for plain bit, trit and quint values but not for mixed
// all the others are scrambled up but basically do the same
static void unquantizeWeights(uint8_t (* const out_unquantizedWeights)[64], const uint8_t in_RAW_WEIGHTS[static const 64], const uint8_t in_NUM_WEIGHTS, const QuantizationMode_s in_QUANT_MODE) {
    assert(not((in_QUANT_MODE.numTrits > 0) and (in_QUANT_MODE.numQuints > 0)));
    assert((in_QUANT_MODE.numTrits > 0) or (in_QUANT_MODE.numQuints > 0) or (in_QUANT_MODE.numBits > 0));

    static const uint8_t TRIT[3] = { 0, 32, 63 };
    static const uint8_t QUINT[5] = { 0, 16, 32, 47, 63 };
    uint8_t a = 0;
    uint8_t b = 0;
    uint8_t c = 0;
    uint8_t t = 0;

    for (size_t i = 0; i < in_NUM_WEIGHTS; i++) {
        if (in_QUANT_MODE.numTrits > 0) {
            if (in_QUANT_MODE.numBits == 0) {
                assert(in_RAW_WEIGHTS[i] < 3);
                t = TRIT[in_RAW_WEIGHTS[i]];
            } else {
                uint8_t bits = in_RAW_WEIGHTS[i] bitand ((1 << in_QUANT_MODE.numBits) - 1);
                uint8_t trit = in_RAW_WEIGHTS[i] >> in_QUANT_MODE.numBits;
                assert(trit < 3);

                a = bits bitand 0x1;
                a = (a << 1) bitor a;
                a = (a << 2) bitor a;
                a = (a << 4) bitor a;
                a = a bitand 0x7F;
                bits >>= 1;

                switch (in_QUANT_MODE.numBits) {
                    case 1:
                        b = 0;
                        c = 50;
                        break;

                    case 2:
                        b = (bits << 6) bitor (bits << 2) bitor bits;
                        c = 23;
                        break;

                    case 3:
                        b = (bits << 5) bitor bits;
                        c = 11;
                        break;

                    default:
                        assert(false);
                }

                t = trit * c + b;
                t = t xor a;
                t = (a bitand 0x20) bitor (t >> 2);
            }
        } else if (in_QUANT_MODE.numQuints > 0) {
            if (in_QUANT_MODE.numBits == 0) {
                assert(in_RAW_WEIGHTS[i] < 5);
                t = QUINT[in_RAW_WEIGHTS[i]];
            } else {
                uint8_t bits = in_RAW_WEIGHTS[i] bitand ((1 << in_QUANT_MODE.numBits) - 1);
                uint8_t quint = in_RAW_WEIGHTS[i] >> in_QUANT_MODE.numBits;
                assert(quint < 5);

                a = bits bitand 0x1;
                a = (a << 1) bitor a;
                a = (a << 2) bitor a;
                a = (a << 4) bitor a;
                a = a bitand 0x7F;
                bits >>= 1;

                switch (in_QUANT_MODE.numBits) {
                    case 1:
                        b = 0;
                        c = 28;
                        break;

                    case 2:
                        b = (bits << 6) bitor (bits << 1);
                        c = 13;
                        break;

                    default:
                        assert(false);
                }

                t = quint * c + b;
                t = t xor a;
                t = (a bitand 0x20) bitor (t >> 2);
            }
        } else {
            t = expand6(in_RAW_WEIGHTS[i], in_QUANT_MODE.numBits);
        }

        if (t > 32) {
            t += 1;
        }

        (*out_unquantizedWeights)[i] = t;
    }
}


// bilinear interpolation
static void infillWeightsSingle(uint8_t (* const out_infilledWeights)[288], const uint8_t in_BLOCK_WIDTH, const uint8_t in_BLOCK_HEIGHT, const uint8_t in_WEIGHTS[static const 64], const uint8_t in_WEIGHTS_WIDTH, const uint8_t in_WEIGHTS_HEIGHT) {
    uint8_t bs = in_BLOCK_WIDTH;
    uint8_t bt = in_BLOCK_HEIGHT;
    float ds = floorf((1024.0f + floorf(bs * 0.5f)) / (bs - 1));
    float dt = floorf((1024.0f + floorf(bt * 0.5f)) / (bt - 1));
    uint16_t cs;
    uint16_t ct;
    uint16_t gs;
    uint16_t gt;
    uint16_t js, fs, jt, ft;
    uint16_t v0;
    uint8_t p00, p01, p10, p11;
    uint16_t w00, w01, w10, w11;
    uint8_t w;

    for (int t = 0; t < in_WEIGHTS_HEIGHT; t++) {
        for (int s = 0; s < in_WEIGHTS_WIDTH; s++) {
            printf("%2u ", in_WEIGHTS[t * in_WEIGHTS_WIDTH + s]);
        }

        puts("");
    }

    puts("");

    for (int t = 0; t < in_BLOCK_HEIGHT; t++) {
        ct = dt * t;
        gt = (ct * (in_WEIGHTS_HEIGHT - 1) + 32) >> 6;
        assert(gt < 177);
        jt = gt >> 4;
        ft = gt bitand 0xF;


        for (int s = 0; s < in_BLOCK_WIDTH; s++) {
            cs = ds * s;
            gs = (cs * (in_WEIGHTS_WIDTH - 1) + 32) >> 6;
            assert(gs < 177);
            js = gs >> 4;
            fs = gs & 0x0F;

            v0 = js + jt * in_WEIGHTS_WIDTH;
            p00 = in_WEIGHTS[v0];
            p01 = in_WEIGHTS[v0 + 1];
            p10 = in_WEIGHTS[v0 + in_WEIGHTS_WIDTH];
            p11 = in_WEIGHTS[v0 + in_WEIGHTS_WIDTH + 1];

            w11 = (fs * ft + 8) >> 4;
            w10 = ft - w11;
            w01 = fs - w11;
            w00 = 16 - fs - ft + w11;

            w = (p00*w00 + p01*w01 + p10*w10 + p11*w11 + 8) >> 4;
            (*out_infilledWeights)[t * in_BLOCK_WIDTH + s] = w;
            printf("%2u ", w);
        }

        puts("");
    }

    puts("");
}


static void infillWeights(uint8_t (* const out_infilledWeights)[288], const uint8_t in_BLOCK_WIDTH, const uint8_t in_BLOCK_HEIGHT, const uint8_t in_WEIGHTS[static const 64], const uint8_t in_WEIGHTS_WIDTH, const uint8_t in_WEIGHTS_HEIGHT, const bool in_DUAL_PLANE) {
    if (in_DUAL_PLANE) {
        uint8_t splittedWeights[2][64];
        uint8_t numWeights = in_WEIGHTS_WIDTH * in_WEIGHTS_HEIGHT;
        assert(numWeights <= 32);

        for (int i = 0; i < numWeights; i++) {
            splittedWeights[0][i] = in_WEIGHTS[i * 2 + 0];
            splittedWeights[1][i] = in_WEIGHTS[i * 2 + 1];
        }

        uint8_t splittedInfilledWeights[2][288];
        infillWeightsSingle(&splittedInfilledWeights[0], in_BLOCK_WIDTH, in_BLOCK_HEIGHT, splittedWeights[0], in_WEIGHTS_WIDTH, in_WEIGHTS_HEIGHT);
        infillWeightsSingle(&splittedInfilledWeights[1], in_BLOCK_WIDTH, in_BLOCK_HEIGHT, splittedWeights[1], in_WEIGHTS_WIDTH, in_WEIGHTS_HEIGHT);

        uint8_t numTexels = in_BLOCK_WIDTH * in_BLOCK_HEIGHT;

        for (int i = 0; i < numTexels; i++) {
            (*out_infilledWeights)[i * 2 + 0] = splittedInfilledWeights[0][i];
            (*out_infilledWeights)[i * 2 + 1] = splittedInfilledWeights[1][i];
        }
    } else {
        infillWeightsSingle(out_infilledWeights, in_BLOCK_WIDTH, in_BLOCK_HEIGHT, in_WEIGHTS, in_WEIGHTS_WIDTH, in_WEIGHTS_HEIGHT);
    }
}


static rgba8_s convertRGBA16to8(const rgba16_s in_RGBA) {
    return RGBA8(in_RGBA.r >> 8, in_RGBA.g >> 8, in_RGBA.b >> 8, in_RGBA.a >> 8);
}


// output as FP16
static void computePaletteLDR(rgba8_s (* const out_palette)[65], const rgba8_s in_C0, const rgba8_s in_C1, const bool in_S_RGB) {
    rgba16_s c0, c1, c;

    if (in_S_RGB) {
        c0.r = (in_C0.r << 8) bitor 0x80;
        c0.g = (in_C0.g << 8) bitor 0x80;
        c0.b = (in_C0.b << 8) bitor 0x80;
        c0.a = (in_C0.a << 8) bitor 0x80;
        c1.r = (in_C1.r << 8) bitor 0x80;
        c1.g = (in_C1.g << 8) bitor 0x80;
        c1.b = (in_C1.b << 8) bitor 0x80;
        c1.a = (in_C1.a << 8) bitor 0x80;
    } else {
        c0.r = (in_C0.r << 8) bitor in_C0.r;
        c0.g = (in_C0.g << 8) bitor in_C0.g;
        c0.b = (in_C0.b << 8) bitor in_C0.b;
        c0.a = (in_C0.a << 8) bitor in_C0.a;
        c1.r = (in_C1.r << 8) bitor in_C1.r;
        c1.g = (in_C1.g << 8) bitor in_C1.g;
        c1.b = (in_C1.b << 8) bitor in_C1.b;
        c1.a = (in_C1.a << 8) bitor in_C1.a;
    }

    for (int i = 0; i <= 64; i++) {
        c.r = (c0.r * (64 - i) + c1.r * i + 32) / 64; // floor?
        c.g = (c0.g * (64 - i) + c1.g * i + 32) / 64; // floor?
        c.b = (c0.b * (64 - i) + c1.b * i + 32) / 64; // floor?
        c.a = (c0.a * (64 - i) + c1.a * i + 32) / 64; // floor?

        if (in_S_RGB) {
            assert(false);
        } else {
            (*out_palette)[i] = convertRGBA16to8(c);
        }
    }
}


static rgba8_s interpolateColorLDR(const rgba8_s in_C0, const rgba8_s in_C1, const uint8_t in_WEIGHT, const bool in_S_RGB) {
    rgba16_s c0, c1, c;
    rgba8_s output;

    if (in_S_RGB) {
        c0.r = (in_C0.r << 8) bitor 0x80;
        c0.g = (in_C0.g << 8) bitor 0x80;
        c0.b = (in_C0.b << 8) bitor 0x80;
        c0.a = (in_C0.a << 8) bitor 0x80;
        c1.r = (in_C1.r << 8) bitor 0x80;
        c1.g = (in_C1.g << 8) bitor 0x80;
        c1.b = (in_C1.b << 8) bitor 0x80;
        c1.a = (in_C1.a << 8) bitor 0x80;
    } else {
        c0.r = (in_C0.r << 8) bitor in_C0.r;
        c0.g = (in_C0.g << 8) bitor in_C0.g;
        c0.b = (in_C0.b << 8) bitor in_C0.b;
        c0.a = (in_C0.a << 8) bitor in_C0.a;
        c1.r = (in_C1.r << 8) bitor in_C1.r;
        c1.g = (in_C1.g << 8) bitor in_C1.g;
        c1.b = (in_C1.b << 8) bitor in_C1.b;
        c1.a = (in_C1.a << 8) bitor in_C1.a;
    }

    c.r = (c0.r * (64 - in_WEIGHT) + c1.r * in_WEIGHT + 32) / 64; // floor?
    c.g = (c0.g * (64 - in_WEIGHT) + c1.g * in_WEIGHT + 32) / 64; // floor?
    c.b = (c0.b * (64 - in_WEIGHT) + c1.b * in_WEIGHT + 32) / 64; // floor?
    c.a = (c0.a * (64 - in_WEIGHT) + c1.a * in_WEIGHT + 32) / 64; // floor?

    if (in_S_RGB) {
        assert(false);
    } else {
        output = convertRGBA16to8(c);
    }

    return output;
}



// C.2.21  Partition Pattern Generation
// ------------------------------------

static uint32_t hash52(uint32_t p) {
    p ^= p >> 15;  p -= p << 17;  p += p << 7; p += p <<  4;
    p ^= p >>  5;  p += p << 16;  p ^= p >> 7; p ^= p >> 3;
    p ^= p <<  6;  p ^= p >> 17;
    return p;
}


static int select_partition(uint32_t seed, uint8_t x, uint8_t y, uint8_t z, uint8_t partitioncount, int small_block) {
    assert(seed < 1024);
    assert(partitioncount > 1);
    assert(partitioncount < 5);

    if (x * y >= 31) {
        assert(small_block == 0);
    }

    if (small_block) {
        x <<= 1;
        y <<= 1;
        z <<= 1;
    }

    seed += (partitioncount - 1) * 1024;
    uint32_t rnum = hash52(seed);
    uint8_t seed1  =  rnum        bitand 0xF;
    uint8_t seed2  = (rnum >>  4) bitand 0xF;
    uint8_t seed3  = (rnum >>  8) bitand 0xF;
    uint8_t seed4  = (rnum >> 12) bitand 0xF;
    uint8_t seed5  = (rnum >> 16) bitand 0xF;
    uint8_t seed6  = (rnum >> 20) bitand 0xF;
    uint8_t seed7  = (rnum >> 24) bitand 0xF;
    uint8_t seed8  = (rnum >> 28) bitand 0xF;
    uint8_t seed9  = (rnum >> 18) bitand 0xF;
    uint8_t seed10 = (rnum >> 22) bitand 0xF;
    uint8_t seed11 = (rnum >> 26) bitand 0xF;
    uint8_t seed12 = ((rnum >> 30) bitor (rnum << 2)) bitand 0xF;

    seed1 *= seed1;
    seed2 *= seed2;
    seed3 *= seed3;
    seed4 *= seed4;
    seed5 *= seed5;
    seed6 *= seed6;
    seed7 *= seed7;
    seed8 *= seed8;
    seed9 *= seed9;
    seed10 *= seed10;
    seed11 *= seed11;
    seed12 *= seed12;

    int sh1, sh2, sh3;

    if (seed bitand 1) {
        sh1 = (seed bitand 0x2) ? 4 : 5;
        sh2 = (partitioncount == 3) ? 6 : 5;
    } else {
        sh1 = (partitioncount == 3) ? 6 : 5;
        sh2 = (seed bitand 0x2) ? 4 : 5;
    }

    sh3 = (seed bitand 0x10) ? sh1 : sh2;

    seed1  >>= sh1;
    seed2  >>= sh2;
    seed3  >>= sh1;
    seed4  >>= sh2;
    seed5  >>= sh1;
    seed6  >>= sh2;
    seed7  >>= sh1;
    seed8  >>= sh2;
    seed9  >>= sh3;
    seed10 >>= sh3;
    seed11 >>= sh3;
    seed12 >>= sh3;

    int a = seed1 * x + seed2 * y + seed11 * z + (rnum >> 14);
    int b = seed3 * x + seed4 * y + seed12 * z + (rnum >> 10);
    int c = seed5 * x + seed6 * y + seed9  * z + (rnum >>  6);
    int d = seed7 * x + seed8 * y + seed10 * z + (rnum >>  2);

    a &= 0x3F;
    b &= 0x3F;
    c &= 0x3F;
    d &= 0x3F;

    if (partitioncount < 4) {
        d = 0;
    }

    if (partitioncount < 3) {
        c = 0;
    }

    if (a >= b && a >= c && a >= d) {
        return 0;
    } else if (b >= c && b >= d) {
        return 1;
    } else if (c >= d) {
        return 2;
    } else {
        return 3;
    }
}



// C.2.22  Data Size Determination
// -------------------------------
/*
void computeColorEndpointBits(const BlockMode_s in_BLOCK_MODE, const Range_s in_RANGE) {
    uint8_t num_partitions = 1;
    uint8_t M = in_BLOCK_MODE.width;
    uint8_t N = in_BLOCK_MODE.height;
    uint8_t Q = in_BLOCK_MODE.depth;
    bool dual_plane = in_BLOCK_MODE.dualPlane;
    uint8_t bits_in_weight_range = in_RANGE.bits;
    uint8_t trits_in_weight_range = (in_RANGE.trits) ? 1 : 0;
    uint8_t quints_in_weight_range = (in_RANGE.quints) ? 1 : 0;

    uint8_t config_bits = 17;

    if (num_partitions > 1) {
        if (single_CEM) {
            config_bits = 29;
        } else {
            config_bits = 25 + 3 * num_partitions;
        }
    }

    uint8_t num_weights = M * N * Q; // size of weight grid

    if (dual_plane) {
        config_bits += 2;
    }

    num_weights *= 2;

    uint8_t weight_bits = ceil(num_weights * 8 * trits_in_weight_range / 5)
    + ceil(num_weights * 7 * quints_in_weight_range / 3)
    + num_weights * bits_in_weight_range;

    uint8_t remaining_bits = 128 - config_bits - weight_bits;

    uint8_t num_CEM_pairs = base_CEM_class+1 + count_bits(extra_CEM_bits);

    uint8_t num_CEM_values = num_CEM_pairs*2;

    for(range = each possible CEM range in descending order of size)
    {
        CEM_bits = ceil(num_CEM_values*8*trits_in_CEM_range/5) +
        ceil(num_CEM_values*7*quints_in_CEM_range/3) +
        num_CEM_values*bits_in_CEM_range;

        if(CEM_bits <= remaining_bits)
            break;
    }
    return range;
}
*/



// C.2.23  Void-Extent Blocks
// --------------------------

typedef struct ASTCVoidExtentBlock_s {
    rgba16_s color;

    struct {
        uint64_t _voidExtent  :  9;
        uint64_t dynamicRange :  1;
        uint64_t _three       :  2;
        uint64_t minS         : 13;
        uint64_t maxS         : 13;
        uint64_t minT         : 13;
        uint64_t maxT         : 13;
    };
} ASTCVoidExtentBlock_s;


static void decodeVoidExtentBlock(rgba8_s (* const out_rgba)[144], const uint8_t in_WIDTH, const uint8_t in_HEIGHT, const ASTCVoidExtentBlock_s in_BLOCK, const bool in_S_RGB) {
    assert(in_BLOCK._voidExtent == 0b111111100);
    assert(in_BLOCK._three == 0b11);

    bool constantBlock = (in_BLOCK.minS bitand in_BLOCK.maxS bitand in_BLOCK.minT bitand in_BLOCK.maxT) == 0x1FFF;

    if (!constantBlock) {
        assert(in_BLOCK.minS < in_BLOCK.maxS);
        assert(in_BLOCK.minT < in_BLOCK.maxT);
    }

    if (in_BLOCK.dynamicRange == 0) {
        // LDR
        if (in_S_RGB) {
            assert(false);
        } else {
            rgba8_s c8 = convertRGBA16to8(in_BLOCK.color);
            int outputFill = 0;

            for (int t = 0; t < in_HEIGHT; t++) {
                for (int s = 0; s < in_WIDTH; s++) {
                    (*out_rgba)[outputFill] = c8;
                    outputFill++;
                }
            }
        }
    } else {
        // HDR
        assert(false);
    }
}



// C.2.24  Illegal Encodings
// -------------------------

void checkSanity(const ASTCTextureInfo_s in_TEXTURE, const BlockMode_s in_MODE, const QuantizationMode_s in_QUANT_MODE) {
    uint8_t numWeights = in_MODE.dimX * in_MODE.dimY * ((in_MODE.dualPlane) ? 2 : 1);
    assert(numWeights <= 64);

    uint8_t numWeightBits = ((numWeights * 8 * in_QUANT_MODE.numTrits + 4) / 5) + ((numWeights * 7 * in_QUANT_MODE.numQuints + 2) / 3) + numWeights * in_QUANT_MODE.numBits;

    assert(numWeightBits <= 96);
    assert(numWeightBits >= 24);

    assert(in_TEXTURE.blockDimX >= in_MODE.dimX);
    assert(in_TEXTURE.blockDimY >= in_MODE.dimY);



    // texture wide
    assert(in_TEXTURE.dims >= 2);
    assert(in_TEXTURE.dims <= 3);

    static const uint8_t BLOCK_SIZES[28] = {
        4, 4,
        5, 4,
        5, 5,
        6, 5,
        6, 6,
        8, 5,
        8, 6,
        8, 8,
        10, 5,
        10, 6,
        10, 8,
        10, 10,
        12, 10,
        12, 12
    };


    if (in_TEXTURE.dims == 2) {
        bool isValidBlockSize = false;

        for (int i = 0; i < 14; i++) {
            if ((BLOCK_SIZES[i * 2] == in_TEXTURE.blockDimX) && (BLOCK_SIZES[i * 2 + 1] == in_TEXTURE.blockDimY)) {
                isValidBlockSize = true;
            }
        }

        assert(isValidBlockSize);
    } else {
        assert(false);
    }
}



static const uint32_t MAGIC_FILE_CONSTANT = 0x5CA1AB13;

typedef struct ASTCFileHeader_s {
    uint8_t magic[4];
    uint8_t blockDimX;
    uint8_t blockDimY;
    uint8_t blockDimZ;
    uint8_t sizeX[3];			// x-size = xsize[0] + 256 * xsize[1] + 256 * 256 xsize[2]
    uint8_t sizeY[3];			// x-size, y-size and z-size are given in texels;
    uint8_t sizeZ[3];			// block count is inferred
} ASTCFileHeader_s;


static const uint8_t NUM_WEIGHT_BITS_MIN = 24;
static const uint8_t NUM_WEIGHT_BITS_MAX = 96;


static QuantizationMode_s computeQuantizationMode(const uint8_t in_NUM_VALUES, const uint8_t in_NUM_BITS) {
    static const uint16_t RANGE[21]  = { 2, 3, 4, 5, 6, 8, 10, 12, 16, 20, 24, 32, 40, 48, 64, 80, 96, 128, 160, 192, 256 };
    static const uint8_t  BITS[21]   = { 1, 0, 2, 0, 1, 3, 1, 2, 4, 2, 3, 5, 3, 4, 6, 4, 5, 7, 5, 6, 8 };
    static const uint8_t  TRITS[21]  = { 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0 };
    static const uint8_t  QUINTS[21] = { 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0 };

    QuantizationMode_s quantMode = { .range = 0, .numBits = 0, .numTrits = 0, .numQuints = 0 };

    for (int m = 20; m >= 0; m--) {
        uint8_t numRequiredBits = BITS[m] * in_NUM_VALUES + ((8 * in_NUM_VALUES + 4) * TRITS[m]) / 5 + ((7 * in_NUM_VALUES + 2) * QUINTS[m]) / 3;

        if (numRequiredBits <= in_NUM_BITS) {
            quantMode = (QuantizationMode_s){ .range = RANGE[m], .numBits = BITS[m], .numTrits = TRITS[m], .numQuints = QUINTS[m] };
            break;
        }
    }

    return quantMode;
}


static void decodeASTCBlock(const ASTCTextureInfo_s in_TEXTURE_INFO, const uint128_t in_BLOCK) {
    uint128_t bitField = *(uint128_t *)&in_BLOCK;
    ASTCBlock_s block = *(ASTCBlock_s *)&in_BLOCK;
    ASTCBlockMode_s blockMode;
    blockMode.u16 = block.blockMode;
    BlockMode_s mode = astcParseBlockMode(blockMode);
    printf("%2u %2u %2u %2u %2u %2u\n", mode.dimX, mode.dimY, mode.dimZ, mode.r, mode.highPrecisionRange, mode.dualPlane);
    QuantizationMode_s weightQuantMode = decodeWeightQuantizationMode(mode);

    BlockInfo_s blockInfo;
    blockInfo.dimX = mode.dimX;
    blockInfo.dimY = mode.dimY;
    blockInfo.dimZ = mode.dimZ;

    if (blockInfo.dimX > in_TEXTURE_INFO.blockDimX
        or blockInfo.dimY > in_TEXTURE_INFO.blockDimY
        or blockInfo.dimZ > in_TEXTURE_INFO.blockDimZ) {
        goto error;
    }

    blockInfo.range = weightQuantMode.range;
    blockInfo.numBits = weightQuantMode.numBits;
    blockInfo.numTrits = weightQuantMode.numTrits;
    blockInfo.numQuints = weightQuantMode.numQuints;

    printf("%2u %2u %2u %2u\n", blockInfo.range, blockInfo.numBits, blockInfo.numTrits, blockInfo.numQuints);

    blockInfo.isVoidExtent = mode.isVoidExtent;
    blockInfo.numPlanes = (mode.dualPlane) ? 2 : 1;

    uint8_t numWeights = blockInfo.dimX * blockInfo.dimY * blockInfo.dimZ * blockInfo.numPlanes;
    uint8_t numWeightBits = ((numWeights * 8 * blockInfo.numTrits + 4) / 5) + ((numWeights * 7 * blockInfo.numQuints + 2) / 3) + numWeights * blockInfo.numBits;
    uint8_t numConfigBits = 0;

    if (numWeightBits < NUM_WEIGHT_BITS_MIN or numWeightBits > NUM_WEIGHT_BITS_MAX) {
        goto error;
    }

    bool isSingleCEM = true;

    if (blockInfo.isVoidExtent) {
        assert(false);
    } else {
        blockInfo.numPartitions = block.part + 1;

        if (blockInfo.numPlanes == 2 && blockInfo.numPartitions == 4) {
            goto error;
        }

        bitField = shr128(bitField, 11); // parsed block mode
        numConfigBits += 11;
        bitField = shr128(bitField,  2); // parsed numPartitions
        numConfigBits += 2;

        if (blockInfo.numPartitions == 1) {
            blockInfo.partitionPatternIndex = 0;
            blockInfo.colorEndpointMode[0] = bitField.lo bitand 0xF;
            bitField = shr128(bitField, 4); // parsed CEM
            numConfigBits += 4;
        } else {
            blockInfo.partitionPatternIndex = bitField.lo bitand 0x3FF;
            bitField = shr128(bitField, 10); // parsed partition pattern index
            numConfigBits += 10;

            uint8_t cem = bitField.lo bitand 0x3;
            bitField = shr128(bitField, 2); // parsed CEM sub-set index
            numConfigBits += 2;

            if (cem == 0) {
                isSingleCEM = true;

                for (int p = 0; p < blockInfo.numPartitions; p++) {
                    blockInfo.colorEndpointMode[p] = bitField.lo bitand 0xF;
                }

                bitField = shr128(bitField, 4); // parsed CEM
                numConfigBits += 4;
            } else {
                isSingleCEM = false;
                uint8_t c[4];
                uint8_t m[4];
                uint128_t bitFieldCopy = *(uint128_t *)&in_BLOCK;

                for (int p = 0; p < blockInfo.numPartitions; p++) {
                    c[p] = bitField.lo bitand 0x1;
                    bitField = shr128(bitField, 1); // parsed c sub-set selector
                    numConfigBits += 1;
                }

                switch (blockInfo.numPartitions) {
                    case 2:
                        m[0] = bitField.lo bitand 0x3;
                        bitField = shr128(bitField, 2); // parsed M0
                        numConfigBits += 2;
                        bitFieldCopy = shr128(bitFieldCopy, 128 - numWeightBits - 2);
                        m[1] = bitFieldCopy.lo bitand 0x3;
                        bitFieldCopy = shr128(bitFieldCopy, 2); // parsed M1
                        numConfigBits += 2;
                        break;

                    case 3:
                        m[0] = bitField.lo bitand 0x1;
                        bitField = shr128(bitField, 1); // parsed LSB of M0
                        numConfigBits += 1;
                        bitFieldCopy = shr128(bitFieldCopy, 128 - numWeightBits - 5);
                        m[0] = ((bitFieldCopy.lo bitand 0x1) << 1) bitor m[0];
                        bitFieldCopy = shr128(bitFieldCopy, 1); // parsed MSB of M0
                        numConfigBits += 1;
                        m[1] = bitFieldCopy.lo bitand 0x3;
                        bitFieldCopy = shr128(bitFieldCopy, 2); // parsed M1
                        numConfigBits += 2;
                        m[2] = bitFieldCopy.lo bitand 0x3;
                        bitFieldCopy = shr128(bitFieldCopy, 2); // parsed M2
                        numConfigBits += 2;
                        break;

                    case 4:
                        bitFieldCopy = shr128(bitFieldCopy, 128 - numWeightBits - 8);
                        m[0] = bitFieldCopy.lo bitand 0x3;
                        bitFieldCopy = shr128(bitFieldCopy, 2); // parsed M0
                        numConfigBits += 2;
                        m[1] = bitFieldCopy.lo bitand 0x3;
                        bitFieldCopy = shr128(bitFieldCopy, 2); // parsed M1
                        numConfigBits += 2;
                        m[2] = bitFieldCopy.lo bitand 0x3;
                        bitFieldCopy = shr128(bitFieldCopy, 2); // parsed M2
                        numConfigBits += 2;
                        m[3] = bitFieldCopy.lo bitand 0x3;
                        bitFieldCopy = shr128(bitFieldCopy, 2); // parsed M3
                        numConfigBits += 2;
                        break;

                    default:
                        goto error;
                }

                for (int p = 0; p < blockInfo.numPartitions; p++) {
                    blockInfo.colorEndpointMode[p] = ((cem - 1 + c[p]) << 2) bitor m[p];
                }
            }
        }
    }


    uint8_t config_bits = 17;

    if (blockInfo.numPartitions > 1) {
        if (isSingleCEM) {
            config_bits = 29;
        } else {
            config_bits = 24 + 3 * blockInfo.numPartitions;
        }
    }

    if (blockInfo.numPlanes == 2) {
        config_bits += 2;
    }

    assert(config_bits == numConfigBits);

    uint8_t numColorEndpointBits = 128 - numConfigBits - numWeightBits;

    uint128_t colorEndpointStream = *(uint128_t *)&in_BLOCK;
    colorEndpointStream = shr128(colorEndpointStream, numConfigBits);
    assert(numColorEndpointBits <= 64);
    colorEndpointStream.lo = colorEndpointStream.lo bitand (((uint64_t)1 << numColorEndpointBits) - 1);
    colorEndpointStream.hi = 0;
    uint8_t numColorEndpointValues = 0;

    for (int p = 0; p < blockInfo.numPartitions; p++) {
        numColorEndpointValues += ((blockInfo.colorEndpointMode[p] >> 2) + 1) * 2;
        //printf("CEM: %d (%d)\n", blockInfo.colorEndpointMode[p], ((blockInfo.colorEndpointMode[p] >> 2) + 1) * 2);
    }

    printf("numCEB: %d\n", numColorEndpointBits);
    printf("numCEV: %d\n", numColorEndpointValues);

    QuantizationMode_s colorQuantizationMode = computeQuantizationMode(numColorEndpointValues, numColorEndpointBits);

    printf("q %3d %d %d %d\n", colorQuantizationMode.range, colorQuantizationMode.numBits, colorQuantizationMode.numTrits, colorQuantizationMode.numQuints);

    uint8_t rawColorEndpointValues[64];
    decodeStream(&rawColorEndpointValues, numColorEndpointValues, colorEndpointStream, colorQuantizationMode);
    uint8_t unquantizedColorEndpointValues[18];
    unquantizeColorValues(&unquantizedColorEndpointValues, rawColorEndpointValues, numColorEndpointValues, colorQuantizationMode);
    rgba8_s colorEndpoints[8];
    uint8_t *colorEndpointValues = unquantizedColorEndpointValues;

    for (int p = 0; p < blockInfo.numPartitions; p++) {
        rgba8_s partitionColorEndpoints[2];
        uint8_t partitionColorEndpointValues[8];

        uint8_t numPartitionColorEndpointValues = ((blockInfo.colorEndpointMode[p] >> 2) + 1) * 2;

        for (int i = 0; i < numPartitionColorEndpointValues; i++) {
            partitionColorEndpointValues[i] = *colorEndpointValues;
            colorEndpointValues++;
        }

        decodeColorEndpoint(&partitionColorEndpoints, partitionColorEndpointValues, blockInfo.colorEndpointMode[p]);
        colorEndpoints[p * 2 + 0] = partitionColorEndpoints[0];
        colorEndpoints[p * 2 + 1] = partitionColorEndpoints[1];

        printf("%3d %3d %3d %3d\n", partitionColorEndpoints[0].r, partitionColorEndpoints[0].g, partitionColorEndpoints[0].b, partitionColorEndpoints[0].a);
        printf("%3d %3d %3d %3d\n", partitionColorEndpoints[1].r, partitionColorEndpoints[1].g, partitionColorEndpoints[1].b, partitionColorEndpoints[1].a);

    }

    uint128_t weightStream;
    weightStream.lo = reverseBitOrder(in_BLOCK.hi);
    weightStream.hi = reverseBitOrder(in_BLOCK.lo);
    printf("0x%016llx 0x%016llx\n", weightStream.hi, weightStream.lo);

    uint8_t weights[64];

    checkSanity(in_TEXTURE_INFO, mode, weightQuantMode);

    decodeStream(&weights, numWeights, weightStream, weightQuantMode);
    uint8_t unquantizedWeights[64];
    unquantizeWeights(&unquantizedWeights, weights, numWeights, weightQuantMode);

    // largest count of infilled weights (which will actually never fit into 128 bits) 12 x 12 * dual plane = 288
    uint8_t infilledWeights[288];
    infillWeights(&infilledWeights, in_TEXTURE_INFO.blockDimX, in_TEXTURE_INFO.blockDimY, unquantizedWeights, mode.dimX, mode.dimY, mode.dualPlane);

//    rgba8_s c0 = RGBA8(16, 32, 64, 128);
//    rgba8_s c1 = RGBA8(128, 64, 32, 255);
//    rgba8_s palette0[65];
//    computePaletteLDR(&palette0, c0, c1, false);

    //    for (int i = 0; i < 65; i++) {
    //        printf("%3u %3u %3u %3u\n", palette0[i].r, palette0[i].g, palette0[i].b, palette0[i].a);
    //    }

    rgba8_s texels[144];
    const uint8_t numTexels = in_TEXTURE_INFO.blockDimX * in_TEXTURE_INFO.blockDimY;
    uint8_t ccs = 0;

    if (mode.dualPlane) {
        for (int j = 0; j < numTexels; j++) {
            int i = 2 * j;
            texels[i] = interpolateColorLDR(colorEndpoints[0], colorEndpoints[1], infilledWeights[i], false);
            texels[i].u8[ccs] = interpolateColorLDR(colorEndpoints[0], colorEndpoints[1], infilledWeights[i + 1], false).u8[ccs];
        }
    } else {
        for (int i = 0; i < numTexels; i++) {
            texels[i] = interpolateColorLDR(colorEndpoints[0], colorEndpoints[1], infilledWeights[i], false);
        }
    }

    for (int i = 0; i < numTexels; i++) {
        printf("%02x", texels[i].r);
        printf("%02x", texels[i].g);
        printf("%02x", texels[i].b);
        //printf("%02x", texels[i].a);
    }

    return;

error:
    assert(false);
    return;
}



int main(int argc, const char * argv[]) {
    FILE *inputStream = fopen("/Users/kwasnicki/Desktop/12x12.astc", "rb");

    ASTCFileHeader_s header;
    size_t itemsRead = fread(&header, sizeof(ASTCFileHeader_s), 1, inputStream);
    assert(itemsRead == 1);

    uint32_t sizeX = ((uint32_t)header.sizeX[2] << 16) bitor ((uint32_t)header.sizeX[1] << 8) bitor (uint32_t)header.sizeX[0];
    uint32_t sizeY = ((uint32_t)header.sizeY[2] << 16) bitor ((uint32_t)header.sizeY[1] << 8) bitor (uint32_t)header.sizeY[0];
    uint32_t sizeZ = ((uint32_t)header.sizeZ[2] << 16) bitor ((uint32_t)header.sizeZ[1] << 8) bitor (uint32_t)header.sizeZ[0];
    uint32_t blocksX = (sizeX + header.blockDimX - 1) / header.blockDimX;
    uint32_t blocksY = (sizeY + header.blockDimY - 1) / header.blockDimY;
    uint32_t blocksZ = (sizeZ + header.blockDimZ - 1) / header.blockDimZ;
    uint32_t numBlocks = blocksX * blocksY * blocksZ;

    ASTCTextureInfo_s textureInfo;
    textureInfo.blockDimX = header.blockDimX;
    textureInfo.blockDimY = header.blockDimY;
    textureInfo.blockDimZ = header.blockDimZ;
    textureInfo.textureWidth = sizeX;
    textureInfo.textureHeight = sizeY;
    textureInfo.dims = 2;


    uint128_t *blocks = malloc(numBlocks * sizeof(uint128_t));
    size_t blocksRead = fread(blocks, sizeof(uint128_t), numBlocks, inputStream);
    assert(blocksRead == numBlocks);

    fclose(inputStream);
    inputStream = NULL;

    for (uint32_t i = 0; i < numBlocks; i++) {
        decodeASTCBlock(textureInfo, blocks[i]);
        break;
    }

    free(blocks);
    blocks = NULL;

    return 0;

    ASTCBlock_s block;
    ASTCBlockMode_s blockMode;
    BlockMode_s mode;

    textureInfo.blockDimX = 6;
    textureInfo.blockDimY = 6;
    textureInfo.dims = 2;

    for ( int i = 0; i < 2048; i++ ) {
        ASTCBlockMode_s b = *(ASTCBlockMode_s*)&i;
        mode = astcParseBlockMode(b);
    }

    blockMode.u16 = block.blockMode;
    mode = astcParseBlockMode(blockMode);

    if (mode.dualPlane) {
        assert(block.part < 3); // invalid block
    }

    assert(mode.dimX <= textureInfo.blockDimX);   // invalid block
    assert(mode.dimY <= textureInfo.blockDimY); // invalid block


    for (int i = 0; i < 256; i++) {
        //integerSequenceDecode(i, 3);
        //astcDecodeTrit(i);
        //astcDecodeQuint(i);
    }

    //getValueFromStreamBits(0x12345678, 4, 1);
    //getValueFromStreamTrits(0x12345678, 4, 3);


    //decodeStream(0x0123456789ABCDEF, true, false, 4);

    mode.r = 6;
    mode.dimX = 4;
    mode.dimY = 4;
    mode.highPrecisionRange = false;
    mode.dualPlane = true;

    uint128_t weightStream;
    weightStream.lo = reverseBitOrder(0xF7B3D591E6A2C480);
    weightStream.hi = reverseBitOrder(0xE6A2C48000000000);
    printf("0x%016llx 0x%016llx\n", weightStream.hi, weightStream.lo);

    // largest count of weights (which will actually never fit into 128 bits) 9 x 9 * dual plane = 162
    uint8_t weights[64];
    uint8_t numWeights = mode.dimX * mode.dimY * ((mode.dualPlane) ? 2 : 1);
    QuantizationMode_s weightQuantMode = decodeWeightQuantizationMode(mode);

    checkSanity(textureInfo, mode, weightQuantMode);

    decodeStream(&weights, numWeights, weightStream, weightQuantMode);
    uint8_t unquantizedWeights[64];
    unquantizeWeights(&unquantizedWeights, weights, numWeights, weightQuantMode);

    // largest count of infilled weights (which will actually never fit into 128 bits) 12 x 12 * dual plane = 288
    uint8_t infilledWeights[288];
    infillWeights(&infilledWeights, textureInfo.blockDimX, textureInfo.blockDimY, unquantizedWeights, mode.dimX, mode.dimY, mode.dualPlane);

    rgba8_s c0 = RGBA8(16, 32, 64, 128);
    rgba8_s c1 = RGBA8(128, 64, 32, 255);
    rgba8_s palette0[65];
    computePaletteLDR(&palette0, c0, c1, false);

//    for (int i = 0; i < 65; i++) {
//        printf("%3u %3u %3u %3u\n", palette0[i].r, palette0[i].g, palette0[i].b, palette0[i].a);
//    }

    rgba8_s texels[144];
    const uint8_t numTexels = textureInfo.blockDimX * textureInfo.blockDimY;
    uint8_t ccs = 0;

    if (mode.dualPlane) {
        for (int j = 0; j < numTexels; j++) {
            int i = 2 * j;
            texels[i] = palette0[infilledWeights[i]];
            texels[i].u8[ccs] = palette0[infilledWeights[i + 1]].u8[ccs];
        }
    } else {
        for (int i = 0; i < numTexels; i++) {
            texels[i] = palette0[infilledWeights[i]];
        }
    }

//    for (uint32_t seed = 0; seed < 10; seed++) {
//        for (int y = 0; y < 5; y++) {
//            for (int x = 0; x < 5; x++) {
//                printf("%d ", select_partition(seed, x, y, 0, 2, 1) );
//            }
//
//            puts("");
//        }
//
//        puts("");
//        
//        usleep(100000);
//    }



    // insert code here...
    printf("Hello, World!\n");
    return 0;
}
