//
//  PVRTC.c
//  GLTC
//
//  Created by Michael Kwasnicki on 04.03.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#include "PVRTC.h"

#include "PVRTC_Common.h"
#include "PVRTC_Compress.h"
#include "PVRTC_Decompress.h"
#include "../blockCompressionCommon.h"
#include "../lib.h"
#include "../simplePNG.h"

#include <assert.h>
#include <iso646.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



typedef struct {
    uint32_t version;
    uint32_t flags;
    uint32_t pixelFormat[2];    // actually this is a uint64_t but this fixes the alignment issue with this format
    uint32_t colorSpace;
    uint32_t channelType;
    uint32_t height;
    uint32_t width;
    uint32_t depth;
    uint32_t numSurfaces;
    uint32_t numFaces;
    uint32_t numMIPMaps;
    uint32_t metaDataSize;
} PVRTexHeader_t;



static unsigned int spread( const unsigned short in_X ) {
	unsigned int x = in_X;
	x = ( x bitor ( x << 8 ) ) bitand 0x00FF00FF;
	x = ( x bitor ( x << 4 ) ) bitand 0x0F0F0F0F;
	x = ( x bitor ( x << 2 ) ) bitand 0x33333333;
	x = ( x bitor ( x << 1 ) ) bitand 0x55555555;
	return x;
}

static unsigned short unspread( const unsigned int in_X ) {
	unsigned int x = in_X;
	x = ( x bitor ( x >> 1 ) ) bitand 0x33333333;
	x = ( x bitor ( x >> 2 ) ) bitand 0x0F0F0F0F;
	x = ( x bitor ( x >> 4 ) ) bitand 0x00FF00FF;
	x = ( x bitor ( x >> 8 ) ) bitand 0x0000FFFF;
	return x;
}

static unsigned int zOrder( const unsigned short in_X, const unsigned short in_Y ) {
	unsigned int x = spread( in_X );
	unsigned int y = spread( in_Y );
	return x | ( y << 1 );
}

static void zUnorder( unsigned short * out_X, unsigned short * out_Y, const unsigned int in_Z ) {
	unsigned int x = unspread( in_Z bitand 0x55555555 );
	unsigned int y = unspread( ( in_Z >> 1 ) bitand 0x55555555 );
	*out_X = x;
	*out_Y = y;
}



static void bilinearFilter2( rgba8_t * out_RGBA, const int in_X, const int in_Y, const rgba8_t in_TL, const rgba8_t in_TR, const rgba8_t in_BL, const rgba8_t in_BR ) {
    for ( int i = 0; i < 4; i++ ) {
        uint32_t filtered = 0;
        filtered += in_TL.array[i] * ( 8 - in_X ) * ( 4 - in_Y );
        filtered += in_TR.array[i] * in_X * ( 4 - in_Y );
        filtered += in_BL.array[i] * ( 8 - in_X ) * in_Y;
        filtered += in_BR.array[i] * in_X * in_Y;
        filtered >>= 5;
        out_RGBA->array[i] = filtered;
    }
}



typedef enum {
    HV,
    H,
    V
} PVRInterpolation_t;



#pragma mark - exposed interface



bool pvrtcRead2BPPRGBA( const char in_FILE[], rgba8_t ** out_image, uint32_t * out_width, uint32_t * out_height ) {
    uint32_t w = 0;
    uint32_t h = 0;
    uint32_t blockCount = 0;
    size_t itemsRead = 0;
    PVRTC4Block_t * block = NULL;
    PVRTC4Block_t * blockPtr = NULL;
    rgba8_t blockRGBA[4][8];
    rgba8_t * imageRGBA = NULL;
    PVRTexHeader_t header;
    uint8_t dummy[1024];
    
    FILE * inputPVRFileStream = fopen( in_FILE, "rb" );
    assert( inputPVRFileStream != NULL );
    itemsRead = fread( &header, sizeof( PVRTexHeader_t ), 1, inputPVRFileStream);
    assert( itemsRead == 1 );
    assert( header.version == 0x03525650 );
    itemsRead = fread( &dummy[0], sizeof( uint8_t ), header.metaDataSize, inputPVRFileStream);
    assert( itemsRead == header.metaDataSize );
    w = header.width;
    h = header.height;
    blockCount = w * h / 32;                                                                                            // FIXME : we have to round up w and h to multiple of 4
    int strideY = w / 4;
    int strideX = w / 8;
    // w >> 2 + ( w & 0x3 ) ? 1 : 0;
    block = malloc( blockCount * sizeof( PVRTC4Block_t ) );
    blockPtr = block;
    imageRGBA = malloc( w * h * sizeof( rgba8_t ) );
    itemsRead = fread( block, sizeof( PVRTC4Block_t ), blockCount, inputPVRFileStream );
    assert( itemsRead == blockCount );
    fclose( inputPVRFileStream );
    const int repeatMask = header.width - 1;
    const int repeatMaskBlockW = header.width / 8 - 1;
    const int repeatMaskBlockH = header.height / 4 - 1;
    
    for ( int y = 0; y < strideY; y++ ) {
        for ( int x = 0; x < strideX; x++ ) {
            PVRTC4Block_t blockPVR[3][3];
            int xx, yy, zz;
            
            for ( int ny = 0; ny < 3; ny++ ) {
                for ( int nx = 0; nx < 3; nx++ ) {
                    xx = ( x + nx - 1 ) bitand repeatMaskBlockW;
                    yy = ( y + ny - 1 ) bitand repeatMaskBlockH;
                    zz = zOrder( yy, xx );
                    blockPVR[ny][nx] = block[zz];
                }
            }
            
            if ( true or x == 29 and y == 39 ) {
            pvrtcDecodeBlock2BPP( blockRGBA, blockPVR );
            
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 8; bx++ ) {
                    int posY = ( y * 4 + by ) bitand repeatMask;
                    int posX = ( x * 8 + bx ) bitand repeatMask;
                    int pos = posY * header.width + posX;
                    imageRGBA[pos] = blockRGBA[by][bx];
                }
            }
            }
        }
    }
    
    free_s( block );
    
    *out_image = imageRGBA;
    *out_width = header.width;
    *out_height = header.height;
    return true;
}



bool pvrtcRead4BPPRGBA( const char in_FILE[], rgba8_t ** out_image, uint32_t * out_width, uint32_t * out_height ) {
    uint32_t blockCount = 0;
    size_t itemsRead = 0;
    PVRTC4Block_t * block = NULL;
    PVRTC4Block_t * blockPtr = NULL;
    rgba8_t blockRGBA[4][4];
    rgba8_t * imageRGBA = NULL;
    PVRTexHeader_t header;
    uint8_t dummy[1024];
    
    FILE * inputPVRFileStream = fopen( in_FILE, "rb" );
    assert( inputPVRFileStream != NULL );
    itemsRead = fread( &header, sizeof( PVRTexHeader_t ), 1, inputPVRFileStream);
    assert( itemsRead == 1 );
    assert( header.version == 0x03525650 );
    itemsRead = fread( &dummy[0], sizeof( uint8_t ), header.metaDataSize, inputPVRFileStream);
    assert( itemsRead == header.metaDataSize );
    blockCount = header.width * header.height / 16;                                                                     // FIXME : we have to round up w and h to multiple of 4
    const int stride = header.width / 4;
    // w >> 2 + ( w & 0x3 ) ? 1 : 0;
    block = malloc( blockCount * sizeof( PVRTC4Block_t ) );
    blockPtr = block;
    imageRGBA = malloc( header.width * header.height * sizeof( rgba8_t ) );
    itemsRead = fread( block, sizeof( PVRTC4Block_t ), blockCount, inputPVRFileStream );
    assert( itemsRead == blockCount );
    fclose( inputPVRFileStream );
    const int repeatMask = header.width - 1;
    const int repeatMaskBlock = header.width / 4 - 1;
    
    for ( int y = 0; y < stride; y++ ) {
        for ( int x = 0; x < stride; x++ ) {
            PVRTC4Block_t blockPVR[3][3];
            int xx, yy, zz;
            
            for ( int ny = 0; ny < 3; ny++ ) {
                for ( int nx = 0; nx < 3; nx++ ) {
                    xx = ( x + nx - 1 ) bitand repeatMaskBlock;
                    yy = ( y + ny - 1 ) bitand repeatMaskBlock;
                    zz = zOrder( yy, xx );
                    blockPVR[ny][nx] = block[zz];
                }
            }
            
            pvrtcDecodeBlock4BPP( blockRGBA, blockPVR );
            
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    int posY = ( y * 4 + by ) bitand repeatMask;
                    int posX = ( x * 4 + bx ) bitand repeatMask;
                    int pos = posY * header.width + posX;
                    imageRGBA[pos] = blockRGBA[by][bx];
                }
            }
        }    
    }
    
    free_s( block );
 
    *out_image = imageRGBA;
    *out_width = header.width;
    *out_height = header.height;
    return true;
}



bool pvrtcWrite4BPPRGBA( const char in_FILE[], const rgba8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT ) {
    //computeUniformColorLUT();
    const uint32_t numBlocks = in_WIDTH * in_HEIGHT >> 4;    // FIXME : we have to round up w and h to multiple of 4
    LinearBlock7x7RGBA_t * metaBlocksRGBA = malloc( numBlocks * sizeof( LinearBlock7x7RGBA_t ) );
    packMetaBlocksRGBA( metaBlocksRGBA, in_IMAGE, in_WIDTH, in_HEIGHT );
    
    PVRTCIntermediateBlock_t * block = NULL;
    PVRTCIntermediateBlock_t * blockPtr = NULL;
    block = malloc( numBlocks * sizeof( PVRTCIntermediateBlock_t ) );
    blockPtr = block;
    
    int numBX = in_WIDTH >> 2;
    int numBY = in_HEIGHT >> 2;
    PVRTCIntermediateBlock_t pvrBlocks[3][3];

    rgba8_t * loResA = malloc( numBlocks * sizeof( rgba8_t ) );
    rgba8_t * loResB = malloc( numBlocks * sizeof( rgba8_t ) );
    rgba8_t * hiRes = malloc( in_WIDTH * in_HEIGHT * sizeof( rgba8_t ) );
    LinearBlock4x4RGBA_t * hiRes_ptr = REINTERPRET(LinearBlock4x4RGBA_t*)hiRes;
    
    for ( int b = 0; b < numBlocks; b++ ) {
        initialGuess( &block[b], metaBlocksRGBA[b].block );
    }
    
    for ( int by = 0; by < numBY; by++ ) {
        for ( int bx = 0; bx < numBX; bx++ ) {
            for ( int mby = 0; mby < 3; mby++ ) {
                int dy = ( by + mby - 1 + numBY ) % numBY;
                
                for ( int mbx = 0; mbx < 3; mbx++ ) {
                    int dx = ( bx + mbx - 1 + numBX ) % numBX;
                    
                    pvrBlocks[mby][mbx] = block[dy * numBX + dx];
                }
            }
            
            int b = by * numBX + bx;
            loResA[b] = block[b].a;
            loResB[b] = block[b].b;
            fakeCompress4BPP( hiRes_ptr->block, metaBlocksRGBA[b].block, pvrBlocks );
            hiRes_ptr++;
        }
    }
    
    twiddleBlocksRGBA( hiRes, in_WIDTH, in_HEIGHT, true );
    pngWrite( "_loResA.png", (uint8_t *)loResA, numBX, numBY, 4 );
    pngWrite( "_loResB.png", (uint8_t *)loResB, numBX, numBY, 4 );
    pngWrite( "_hiRes.png", (uint8_t *)hiRes, in_WIDTH, in_HEIGHT, 4 );
    
//    char fileName[1024];
//    sprintf( fileName, "_meta %d.png", b );
//    pngWrite( fileName, (uint8_t *)metaBlocksRGBA[b].linear, 7, 7, 4 );
    
    free_s( loResA );
    free_s( loResB );
    free_s( hiRes );
    free_s( metaBlocksRGBA );
    free_s( block );
    
    return true;
}



void pvrtcFreeRGBA( rgba8_t ** in_out_image ) {
    free_s( * in_out_image );
}
