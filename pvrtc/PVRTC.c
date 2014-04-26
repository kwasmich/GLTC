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
#include "../PNG.h"

#include <assert.h>
#include <iso646.h>
#include <stdio.h>


typedef struct {
    uint32_t headerLength;
    uint32_t height;
    uint32_t width;
    uint32_t numMipmaps;
    uint32_t flags;
    uint32_t dataLength;
    uint32_t bpp;
    uint32_t bitmaskRed;
    uint32_t bitmaskGreen;
    uint32_t bitmaskBlue;
    uint32_t bitmaskAlpha;
    uint32_t pvrTag;
    uint32_t numSurfs;
} PVRTexHeader_t;



unsigned int spread( const unsigned short in_X ) {
	unsigned int x = in_X;
	x = ( x bitor ( x << 8 ) ) bitand 0x00FF00FF;
	x = ( x bitor ( x << 4 ) ) bitand 0x0F0F0F0F;
	x = ( x bitor ( x << 2 ) ) bitand 0x33333333;
	x = ( x bitor ( x << 1 ) ) bitand 0x55555555;
	return x;
}

unsigned short unspread( const unsigned int in_X ) {
	unsigned int x = in_X;
	x = ( x bitor ( x >> 1 ) ) bitand 0x33333333;
	x = ( x bitor ( x >> 2 ) ) bitand 0x0F0F0F0F;
	x = ( x bitor ( x >> 4 ) ) bitand 0x00FF00FF;
	x = ( x bitor ( x >> 8 ) ) bitand 0x0000FFFF;
	return x;
}

unsigned int zOrder( const unsigned short in_X, const unsigned short in_Y ) {
	unsigned int x = spread( in_X );
	unsigned int y = spread( in_Y );
	return x | ( y << 1 );
}

void zUnorder( unsigned short * out_X, unsigned short * out_Y, const unsigned int in_Z ) {
	unsigned int x = unspread( in_Z bitand 0x55555555 );
	unsigned int y = unspread( ( in_Z >> 1 ) bitand 0x55555555 );
	*out_X = x;
	*out_Y = y;
}



void bilinearFilter2( rgba8_t * out_RGBA, const int in_X, const int in_Y, const rgba8_t in_TL, const rgba8_t in_TR, const rgba8_t in_BL, const rgba8_t in_BR ) {
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



bool pvrtcRead2BPPRGBA( const char in_FILE[], rgba8_t ** out_image, uint32_t * out_width, uint32_t * out_height ) {
    uint32_t w = 0;
    uint32_t h = 0;
    uint32_t blockCount = 0;
    size_t itemsRead = 0;
    PVRTC4Block_t * block = NULL;
    PVRTC4Block_t * blockPtr = NULL;
    rgba8_t blockRGBA[4][4];
    rgba8_t * imageRGBA = NULL;
    PVRTexHeader_t header;
    
    FILE * inputPVRFileStream = fopen( in_FILE, "rb" );
    assert( inputPVRFileStream != NULL );
    itemsRead = fread( &header, sizeof( PVRTexHeader_t ), 1, inputPVRFileStream);
    assert( itemsRead == 1 );
    w = header.width;
    h = header.height;
    blockCount = w * h / 32;                                                                                            // FIXME : we have to round up w and h to multiple of 4
    int stride = w / 8;
    // w >> 2 + ( w & 0x3 ) ? 1 : 0;
    block = malloc( blockCount * sizeof( PVRTC4Block_t ) );
    blockPtr = block;
    imageRGBA = malloc( w * h * sizeof( rgba8_t ) );
    itemsRead = fread( block, sizeof( PVRTC4Block_t ), blockCount, inputPVRFileStream );
    assert( itemsRead == blockCount );
    fclose( inputPVRFileStream );
    
    rgba8_t * baseColorA = malloc( blockCount * sizeof( rgba8_t) );
    rgba8_t * baseColorB = malloc( blockCount * sizeof( rgba8_t) );
    
    for ( int z = 0; z < blockCount; z++ ) {
        uint16_t x;
        uint16_t y;
        zUnorder( &y, &x, z );
        int pos = y * stride + x;
        decodeBaseColorA( &baseColorA[pos], blockPtr->a );
        decodeBaseColorB( &baseColorB[pos], blockPtr->b );
        blockPtr++;
    }
    
    rgba8_t * filteredA = malloc( w * h * sizeof( rgba8_t ) );
    rgba8_t * filteredB = malloc( w * h * sizeof( rgba8_t ) );
    uint8_t * filteredC = malloc( w * h * sizeof( uint8_t ) );
    memset( filteredC, 0, w * h * sizeof( uint8_t ) );
    
    int repeatMaskX = stride - 1;
    int repeatMaskY = ( stride * 2 ) - 1;
    int repeatMask = w - 1;
    
    for ( int y = 0; y < h; y++ ) {
        int baseT = ( y - 2 ) >> 2;
        int baseB = baseT + 1;
        baseT &= repeatMaskY;            // repeating instead of clamping
        baseB &= repeatMaskY;            // repeating instead of clamping
        
        int bY = ( y - 2 ) bitand 0x3;
        
        for ( int x = 0; x < w; x++ ) {
            int baseL = ( x - 4 ) >> 3;
            int baseR = baseL + 1;
            baseL &= repeatMaskX;        // repeating instead of clamping
            baseR &= repeatMaskX;        // repeating instead of clamping
            int bX = ( x - 4 ) bitand 0x7;
            int posTL = baseT * stride + baseL;
            int posTR = baseT * stride + baseR;
            int posBL = baseB * stride + baseL;
            int posBR = baseB * stride + baseR;
            
            bilinearFilter2( &filteredA[y * w + x], bX, bY, baseColorA[posTL], baseColorA[posTR], baseColorA[posBL], baseColorA[posBR] );
            bilinearFilter2( &filteredB[y * w + x], bX, bY, baseColorB[posTL], baseColorB[posTR], baseColorB[posBL], baseColorB[posBR] );
        }
    }
    
    pngWrite( "/Users/kwasmich/Desktop/Test/GLTC/__A.png", (uint8_t *)baseColorA, w / 8, h / 4, 4 );
    pngWrite( "/Users/kwasmich/Desktop/Test/GLTC/__B.png", (uint8_t *)baseColorB, w / 8, h / 4, 4 );
    
    pngWrite( "/Users/kwasmich/Desktop/Test/GLTC/_A.png", (uint8_t *)filteredA, w, h, 4 );
    pngWrite( "/Users/kwasmich/Desktop/Test/GLTC/_B.png", (uint8_t *)filteredB, w, h, 4 );
    
    
    blockPtr = block;
    
    for ( int z = 0; z < blockCount; z++ ) {
        uint16_t x;
        uint16_t y;
        zUnorder( &y, &x, z );
        uint32_t modulationMask = blockPtr->mod;
        bool modulationMode = blockPtr->a bitand 0x1;
        int8_t modulation[4][8];
        
        if ( modulationMode ) {
            PVRInterpolation_t interpolationMode = HV;            // H & V
            
            // pixel (0, 0) has 1 bit accuracy it's least significant bit is used to tell that we are only using interpolation in one dimension
            if ( modulationMask bitand 0x1 ) {          // H or V only
                // pixel (4, 2) has 1 bit accuracy it's least significant bit is used to tell in which direction to interpolate ( 0 = H, 1 = V )
                if ( modulationMask bitand 0x100000 ) { // V only
                    interpolationMode = V;
                } else {                                // H only
                    interpolationMode = H;
                }
                
                // convert 1 bit accuracy to two bit accuracy;
                if ( modulationMask bitand 0x200000 ) {
                    modulationMask = modulationMask bitor 0x100000;
                } else {
                    modulationMask = modulationMask bitand 0xFFEFFFFF;
                }
            }
            
            // convert 1 bit accuracy to two bit accuracy;
            if ( modulationMask bitand 0x2 ) {
                modulationMask = modulationMask bitor 0x1;
            } else {
                modulationMask = modulationMask bitand 0xFFFFFFFE;
            }
            
            // fill in the checkerboard modulation
            for ( int by = 0; by < 4; by++ ) {
                const int start = by bitand 0x1;
                
                for ( int bx = start; bx < 8; bx += 2 ) {
                    switch ( modulationMask bitand 0x3 ) {
                        case 0:
                            modulation[by][bx] = 8;
                            break;
                            
                        case 1:
                            modulation[by][bx] = 5;
                            break;
                            
                        case 2:
                            modulation[by][bx] = 3;
                            break;
                            
                        case 3:
                            modulation[by][bx] = 0;
                            break;
                    }
                    
                    modulationMask >>= 2;
                }
                
                for ( int bx = (not start); bx < 8; bx += 2 ) {
                    modulation[by][bx] = 0;
                }
            }
        } else {
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 8; bx++ ) {
                    if ( modulationMask bitand 0x1 ) {
                        modulation[by][bx] = 0;
                    } else {
                        modulation[by][bx] = 8;
                    }

                    modulationMask >>= 1;
                }
            }
        }
        
        for ( int by = 0; by < 4; by++ ) {
            for ( int bx = 0; bx < 8; bx++ ) {
                int arrayPosition = ( y * 4 + by ) * w + x * 8 + bx;
                int m = modulation[by][bx];
                filteredC[arrayPosition] = m;
            }
        }
        
        blockPtr++;
    }
        
    blockPtr = block;
    
    for ( int z = 0; z < blockCount; z++ ) {
        uint16_t x;
        uint16_t y;
        zUnorder( &y, &x, z );
        uint32_t modulationMask = blockPtr->mod;
        bool modulationMode = blockPtr->a bitand 0x1;
        
        if ( modulationMode ) {
            PVRInterpolation_t interpolationMode = HV;            // H & V
            
            // pixel (0, 0) has 1 bit accuracy it's least significant bit is used to tell that we are only using interpolation in one dimension
            if ( modulationMask bitand 0x1 ) {          // H or V only
                // pixel (4, 2) has 1 bit accuracy it's least significant bit is used to tell in which direction to interpolate ( 0 = H, 1 = V )
                if ( modulationMask bitand 0x100000 ) { // V only
                    interpolationMode = V;
                } else {                                // H only
                    interpolationMode = H;
                }
            }
            
            // fill the gaps
            for ( int by = 0; by < 4; by++ ) {
                int t  = ( y * 4 + by - 1 ) bitand repeatMask;
                int tb = ( y * 4 + by )     bitand repeatMask;
                int b  = ( y * 4 + by + 1 ) bitand repeatMask;
                
                for ( int bx = 0; bx < 8; bx++ ) {
                    int l  = ( x * 8 + bx - 1 ) bitand repeatMask;
                    int lr = ( x * 8 + bx )     bitand repeatMask;
                    int r  = ( x * 8 + bx + 1 ) bitand repeatMask;
                    
                    if ( ( bx xor by ) bitand 0x1 ) {   // the gabs in the checkboard
                        int avg = 0;
                        
                        switch ( interpolationMode ) {
                            case HV:
                                avg += filteredC[tb * w + l];
                                avg += filteredC[tb * w + r];
                                avg += filteredC[t * w + lr];
                                avg += filteredC[b * w + lr];
                                avg = ( avg + 2 ) >> 2;
                                break;
                                
                            case H:
                                avg += filteredC[tb * w + l];
                                avg += filteredC[tb * w + r];
                                avg = ( avg + 1 ) >> 1;
                                break;
                                
                            case V:
                                avg += filteredC[t * w + lr];
                                avg += filteredC[b * w + lr];
                                avg = ( avg + 1 ) >> 1;
                                break;
                        }
                        
                        filteredC[tb * w + lr] = avg;
                    }
                }
            }
        }
        
        for ( int by = 0; by < 4; by++ ) {
            for ( int bx = 0; bx < 8; bx++ ) {
                int arrayPosition = ( y * 4 + by ) * w + x * 8 + bx;
                int m = filteredC[arrayPosition];
                //filteredC[arrayPosition] = filteredC[arrayPosition] * 31;
                imageRGBA[arrayPosition].r = ( filteredA[arrayPosition].r * m + filteredB[arrayPosition].r * ( 8 - m ) ) >> 3;
                imageRGBA[arrayPosition].g = ( filteredA[arrayPosition].g * m + filteredB[arrayPosition].g * ( 8 - m ) ) >> 3;
                imageRGBA[arrayPosition].b = ( filteredA[arrayPosition].b * m + filteredB[arrayPosition].b * ( 8 - m ) ) >> 3;
                imageRGBA[arrayPosition].a = ( filteredA[arrayPosition].a * m + filteredB[arrayPosition].a * ( 8 - m ) ) >> 3;
            }
        }
        
        blockPtr++;
    }
    
    blockPtr = block;
    
    for ( int z = 0; z < blockCount; z++ ) {
        uint16_t x;
        uint16_t y;
        zUnorder( &y, &x, z );
        
        for ( int by = 0; by < 4; by++ ) {
            for ( int bx = 0; bx < 8; bx++ ) {
                int arrayPosition = ( y * 4 + by ) * w + x * 8 + bx;
                filteredC[arrayPosition] = filteredC[arrayPosition] * 31;
            }
        }
        
        blockPtr++;
    }
    
    pngWrite( "/Users/kwasmich/Desktop/Test/GLTC/_C.png", (uint8_t *)filteredC, w, h, 1 );
    
    free( filteredA );
    filteredA = NULL;
    free( filteredB );
    filteredB = NULL;
    
    free( baseColorA );
    baseColorA = NULL;
    free( baseColorB );
    baseColorB = NULL;
    
    free( block );
    block = NULL;
    
    *out_image = imageRGBA;
    *out_width = w;
    *out_height = h;
}



bool pvrtcRead4BPPRGBA( const char in_FILE[], rgba8_t ** out_image, uint32_t * out_width, uint32_t * out_height ) {
    uint32_t blockCount = 0;
    size_t itemsRead = 0;
    PVRTC4Block_t * block = NULL;
    PVRTC4Block_t * blockPtr = NULL;
    rgba8_t blockRGBA[4][4];
    rgba8_t * imageRGBA = NULL;
    PVRTexHeader_t header;
    
    FILE * inputPVRFileStream = fopen( in_FILE, "rb" );
    assert( inputPVRFileStream != NULL );
    itemsRead = fread( &header, sizeof( PVRTexHeader_t ), 1, inputPVRFileStream);
    assert( itemsRead == 1 );
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
            PVRTC4Block_t blockPVR[2][2];
            int z = 0;
            int x1 = ( x + 1 ) bitand repeatMaskBlock;
            int y1 = ( y + 1 ) bitand repeatMaskBlock;
            z = zOrder( y, x );
            blockPVR[0][0] = block[z];
            z = zOrder( y, x1 );
            blockPVR[0][1] = block[z];
            z = zOrder( y1, x );
            blockPVR[1][0] = block[z];
            z = zOrder( y1, x1 );
            blockPVR[1][1] = block[z];
            
            pvrtcDecodeBlock4BPP( blockRGBA, blockPVR );
            
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    int posY = ( y * 4 + by + 2 ) bitand repeatMask;
                    int posX = ( x * 4 + bx + 2 ) bitand repeatMask;
                    int pos = posY * header.width + posX;
                    imageRGBA[pos] = blockRGBA[by][bx];
                }
            }
        }    
    }
    
    free( block );
    block = NULL;
 
    *out_image = imageRGBA;
    *out_width = header.width;
    *out_height = header.height;
}



bool pvrtcWrite4BPPRGBA( const char in_FILE[], const rgba8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT ) {
    const int loW = in_WIDTH / 4;
    const int loH = in_HEIGHT / 4;
    const int repeatMaskLo = loW - 1;
    const int repeatMaskHi = in_WIDTH - 1;
    rgba8_t * loRes = malloc( loW * loH * sizeof( rgba8_t ) );
    rgba8_t * baseA = malloc( loW * loH * sizeof( rgba8_t ) );
    rgba8_t * baseB = malloc( loW * loH * sizeof( rgba8_t ) );
    rgba8_t * baseAHi = malloc( in_WIDTH * in_HEIGHT * sizeof( rgba8_t ) );
    rgba8_t * baseBHi = malloc( in_WIDTH * in_HEIGHT * sizeof( rgba8_t ) );
    rgba8_t * hiRes = malloc( in_WIDTH * in_HEIGHT * sizeof( rgba8_t ) );
    rgba8_t * delta = malloc( in_WIDTH * in_HEIGHT * sizeof( rgba8_t ) );
    
    // create low-resolution image
    for ( int y = 0; y < in_HEIGHT; y+=4 ) {
        for ( int x = 0; x < in_WIDTH; x+=4 ) {
            int avg[4] = { 0, 0, 0, 0 };
            rgba8_t avgPixel;
            
//            for ( int by = 0; by < 4; by++ ) {
//                for ( int bx = 0; bx < 4; bx++ ) {
//                    rgba8_t pixel = in_IMAGE[(y + by) * in_WIDTH + x + bx];
//                    
//                    avg[0] += pixel.r;
//                    avg[1] += pixel.g;
//                    avg[2] += pixel.b;
//                    avg[3] += pixel.a;
//                }
//            }
//            
//            avgPixel.r = avg[0] >> 4;
//            avgPixel.g = avg[1] >> 4;
//            avgPixel.b = avg[2] >> 4;
//            avgPixel.a = avg[3] >> 4;

            for ( int by = 0; by < 5; by++ ) {
                int posY = ( y + by ) bitand repeatMaskHi;
                
                for ( int bx = 0; bx < 5; bx++ ) {
                    int posX = ( x + bx ) bitand repeatMaskHi;
                    rgba8_t pixel = in_IMAGE[posY * in_WIDTH + posX];
                    
                    int factor = 4;
                    
                    if ( ( bx == 0 ) or ( bx == 4 ) ) {
                        factor /= 2;
                    }
                    
                    if ( ( by == 0 ) or ( by == 4 ) ) {
                        factor /= 2;
                    }
                    
                    avg[0] += pixel.r * factor;
                    avg[1] += pixel.g * factor;
                    avg[2] += pixel.b * factor;
                    avg[3] += pixel.a * factor;
                }
            }
            
            avgPixel.r = avg[0] >> 6;
            avgPixel.g = avg[1] >> 6;
            avgPixel.b = avg[2] >> 6;
            avgPixel.a = avg[3] >> 6;
            
            loRes[y/4 * loW + x/4] = in_IMAGE[(y + 2) * in_WIDTH + x + 2];
            loRes[y/4 * loW + x/4] = avgPixel;
        }
    }
    
    rgba8_t block[2][2];
    rgba8_t blockRGBA;
    
    // upscale low-resolution image
    for ( int y = 0; y < loH; y++ ) {
        const int y1 = (y + 1) bitand repeatMaskLo;
        
        for ( int x = 0; x < loW; x++ ) {
            const int x1 = (x + 1) bitand repeatMaskLo;
            block[0][0] = loRes[y * loW + x];
            block[0][1] = loRes[y * loW + x1];
            block[1][0] = loRes[y1 * loW + x];
            block[1][1] = loRes[y1 * loW + x1];
            
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    int posY = ( y * 4 + by + 2 ) bitand repeatMaskHi;
                    int posX = ( x * 4 + bx + 2 ) bitand repeatMaskHi;
                    int pos = posY * in_WIDTH + posX;
                    bilinearFilter4x4( &blockRGBA, bx, by, block );
                    hiRes[pos] = blockRGBA;
                }
            }
        }
    }
    
    // compute delta between original and upscaled image
    for ( int y = 0; y < in_HEIGHT; y++ ) {
        for ( int x = 0; x < in_WIDTH; x++ ) {
            int pos = y * in_WIDTH + x;
            delta[pos].r = in_IMAGE[pos].r - hiRes[pos].r + 127;
            delta[pos].g = in_IMAGE[pos].g - hiRes[pos].g + 127;
            delta[pos].b = in_IMAGE[pos].b - hiRes[pos].b + 127;
            delta[pos].a = in_IMAGE[pos].a - hiRes[pos].a + 127;
            delta[pos].a = 255;
        }
    }
    
    // initial guess
    for ( int y = 0; y < in_HEIGHT; y+=4 ) {
        for ( int x = 0; x < in_WIDTH; x+=4 ) {
            int avg[4] = { 0, 0, 0, 0 };
            int min4[4] = { 0, 0, 0, 0 };
            int max4[4] = { 0, 0, 0, 0 };
            rgba8_t min = { 255, 255, 255, 255 };
            rgba8_t max = { 0, 0, 0, 0 };
            
            for ( int by = -1; by < 6; by++ ) {
                int posY = ( y + by ) bitand repeatMaskHi;
                
                for ( int bx = -1; bx < 6; bx++ ) {
                    int posX = ( x + bx ) bitand repeatMaskHi;
                    int pos = posY * in_WIDTH + posX;
                    
                    avg[0] += in_IMAGE[pos].r;
                    avg[1] += in_IMAGE[pos].g;
                    avg[2] += in_IMAGE[pos].b;
                    avg[3] += in_IMAGE[pos].a;
                }
            }
            
            avg[0] /= 49;
            avg[1] /= 49;
            avg[2] /= 49;
            avg[3] /= 49;
            
            uint32_t avgLength = avg[0] * avg[0] + avg[1] * avg[1] + avg[2] * avg[2];
            uint32_t minCnt = 0;
            uint32_t maxCnt = 0;
            
            for ( int by = -1; by < 6; by++ ) {
                int posY = ( y + by ) bitand repeatMaskHi;
                
                for ( int bx = -1; bx < 6; bx++ ) {
                    int posX = ( x + bx ) bitand repeatMaskHi;
                    int pos = posY * in_WIDTH + posX;
                    rgba8_t pixel = in_IMAGE[pos];
                    
                    uint32_t vectorLength = pixel.r * pixel.r + pixel.g * pixel.g + pixel.b * pixel.b;
                    
                    if ( vectorLength <= avgLength ) {
                        min4[0] += in_IMAGE[pos].r;
                        min4[1] += in_IMAGE[pos].g;
                        min4[2] += in_IMAGE[pos].b;
                        min4[3] += in_IMAGE[pos].a;
                        minCnt++;
                    } else {
                        max4[0] += in_IMAGE[pos].r;
                        max4[1] += in_IMAGE[pos].g;
                        max4[2] += in_IMAGE[pos].b;
                        max4[3] += in_IMAGE[pos].a;
                        maxCnt++;
                    }
                }
            }
            
            if ( minCnt > 0 ) {
                min.r = min4[0] / minCnt;
                min.g = min4[1] / minCnt;
                min.b = min4[2] / minCnt;
                min.a = min4[3] / minCnt;
            }
            
            if ( maxCnt > 0 ) {
                max.r = max4[0] / maxCnt;
                max.g = max4[1] / maxCnt;
                max.b = max4[2] / maxCnt;
                max.a = max4[3] / maxCnt;
            }
            
            if ( minCnt == 0 ) {
                min = max;
            }
            
            if ( maxCnt == 0 ) {
                max = min;
            }

            baseA[y/4 * loW + x/4] = max;
            baseB[y/4 * loW + x/4] = min;
        }
    }
    
    rgba8_t blockA3[3][3];
    rgba8_t blockB3[3][3];
    rgba8_t blockSRC[8][8];
    
    for ( int i = 0; i < 10; i++ )
    for ( int y = 0; y < in_HEIGHT; y += 4 ) {
        const int y1 = y >> 2;
        const int y0 = ( y1 - 1 ) bitand repeatMaskLo;
        const int y2 = ( y1 + 1 ) bitand repeatMaskLo;
        
        for ( int x = 0; x < in_WIDTH; x += 4 ) {
            const int x1 = x >> 2;
            const int x0 = ( x1 - 1 ) bitand repeatMaskLo;
            const int x2 = ( x1 + 1 ) bitand repeatMaskLo;
            
            blockA3[0][0] = baseA[y0 * loW + x0];
            blockA3[0][1] = baseA[y0 * loW + x1];
            blockA3[0][2] = baseA[y0 * loW + x2];
            blockA3[1][0] = baseA[y1 * loW + x0];
            blockA3[1][1] = baseA[y1 * loW + x1];
            blockA3[1][2] = baseA[y1 * loW + x2];
            blockA3[2][0] = baseA[y2 * loW + x0];
            blockA3[2][1] = baseA[y2 * loW + x1];
            blockA3[2][2] = baseA[y2 * loW + x2];
            
            blockB3[0][0] = baseB[y0 * loW + x0];
            blockB3[0][1] = baseB[y0 * loW + x1];
            blockB3[0][2] = baseB[y0 * loW + x2];
            blockB3[1][0] = baseB[y1 * loW + x0];
            blockB3[1][1] = baseB[y1 * loW + x1];
            blockB3[1][2] = baseB[y1 * loW + x2];
            blockB3[2][0] = baseB[y2 * loW + x0];
            blockB3[2][1] = baseB[y2 * loW + x1];
            blockB3[2][2] = baseB[y2 * loW + x2];
            
            for ( int ay = 0; ay < 8; ay++ ) {
                int posY = ( y + ay - 2 ) bitand repeatMaskHi;
                
                for ( int ax = 0; ax < 8; ax++ ) {
                    int posX = ( x + ax - 2 ) bitand repeatMaskHi;
                    int pos = posY * in_WIDTH + posX;
                    blockSRC[ay][ax] = in_IMAGE[pos];
                }
            }
            
            
            rgba8_t pixelA;
            rgba8_t pixelB;
            
            // upscale baseA and baseB
            if ( true ) 
                for ( int by = 0; by < 4; by++ ) {
                    for ( int bx = 0; bx < 4; bx++ ) {
                        int pos = ( y + by ) * in_WIDTH + x + bx;
                        bilinearFilter3_4x4( &pixelA, bx + 2, by + 2, blockA3 );
                        bilinearFilter3_4x4( &pixelB, bx + 2, by + 2, blockB3 );
                        baseAHi[pos] = pixelA;
                        baseBHi[pos] = pixelB;
                    }
                }
            
            int temp[4];
            
            // compute best possible image from the curent basis 
            if ( false )
                for ( int by = 0; by < 4; by++ ) {
                    for ( int bx = 0; bx < 4; bx++ ) {
                        int pos = ( y + by ) * in_WIDTH + x + bx;
                        rgba8_t pixel = in_IMAGE[pos];
                        bilinearFilter3_4x4( &pixelA, bx + 2, by + 2, blockA3 );
                        bilinearFilter3_4x4( &pixelB, bx + 2, by + 2, blockB3 );
                        
                        const int mod[4] = { 0, 3, 5, 8 };
                        uint32_t leastPixelError = 0xFFFFFFFF;
                        int bestM = 0;
                        
                        
                        for ( int m = 0; m < 4; m++ ) {
                            temp[0] = ( pixelA.r * mod[m] + pixelB.r * ( 8 - mod[m] ) ) >> 3;
                            temp[1] = ( pixelA.g * mod[m] + pixelB.g * ( 8 - mod[m] ) ) >> 3;
                            temp[2] = ( pixelA.b * mod[m] + pixelB.b * ( 8 - mod[m] ) ) >> 3;
                            temp[3] = ( pixelA.a * mod[m] + pixelB.a * ( 8 - mod[m] ) ) >> 3;
                            temp[0] = pixel.r - temp[0];
                            temp[1] = pixel.g - temp[1];
                            temp[2] = pixel.b - temp[2];
                            temp[3] = pixel.a - temp[3];
                            
                            uint32_t pixelError = temp[0] * temp[0] + temp[1] * temp[1] + temp[2] * temp[2]; // + temp[3] * temp[3];
                            
                            if ( pixelError < leastPixelError ) {
                                leastPixelError = pixelError;
                                bestM = mod[m];
                            }
                        }
                        
                        hiRes[pos].r = ( pixelA.r * bestM + pixelB.r * ( 8 - bestM ) ) >> 3;
                        hiRes[pos].g = ( pixelA.g * bestM + pixelB.g * ( 8 - bestM ) ) >> 3;
                        hiRes[pos].b = ( pixelA.b * bestM + pixelB.b * ( 8 - bestM ) ) >> 3;
                        hiRes[pos].a = ( pixelA.a * bestM + pixelB.a * ( 8 - bestM ) ) >> 3;
                    }
                }
            
            uint32_t error = computeError( blockSRC, blockA3, blockB3 );
            
            rgba8_t bMax = blockA3[1][1];
            rgba8_t bMin = blockB3[1][1];
            
            // initial guess : straight forward diagonal across the min-max-color-cube Actually we need two sweeps here
            if ( true ){
                rgba8_t a = { 0, 0, 0, 0 };
                rgba8_t b = { 0, 0, 0, 0 };
                uint32_t bestError = 0xFFFFFFFF;
                rgba8_t bestA;
                rgba8_t bestB;
                
                for ( int i = 0; i < 16; i++ ) {
                    a.r = ( i bitand 0x8 ) ? bMax.r : bMin.r;
                    a.g = ( i bitand 0x4 ) ? bMax.g : bMin.g;
                    a.b = ( i bitand 0x2 ) ? bMax.b : bMin.b;
                    a.a = ( i bitand 0x1 ) ? bMax.a : bMin.a;
                    b.r = ( i bitand 0x8 ) ? bMin.r : bMax.r;
                    b.g = ( i bitand 0x4 ) ? bMin.g : bMax.g;
                    b.b = ( i bitand 0x2 ) ? bMin.b : bMax.b;
                    b.a = ( i bitand 0x1 ) ? bMin.a : bMax.a;
                    
                    blockA3[1][1] = a;
                    blockB3[1][1] = b;
                    
                    uint32_t error = computeError( blockSRC, blockA3, blockB3 );
                    
                    if ( error < bestError ) {
                        bestError = error;
                        bestA = a;
                        bestB = b;
                    }
                }
                
                baseA[y1 * loW + x1] = bestA;
                baseB[y1 * loW + x1] = bestB;
            }
            
            
            for ( int by = 1; by < 8; by++ ) {
                int posY = ( y + by - 2 ) bitand repeatMaskHi;
                
                for ( int bx = 1; bx < 8; bx++ ) {
                    int posX = ( x + bx - 2 ) bitand repeatMaskHi;
                    int pos = posY * in_WIDTH + posX;
                    rgba8_t pixel = in_IMAGE[pos];
                    bilinearFilter3_4x4( &pixelA, bx, by, blockA3 );
                    bilinearFilter3_4x4( &pixelB, bx, by, blockB3 );
                    
                    const int modulation[4] = { 0, 3, 5, 8 };
                    uint32_t leastPixelError = 0xFFFFFFFF;
                    int bestM = 0;
                    
                    for ( int m = 0; m < 4; m++ ) {
                        int mod = modulation[m];
                        
                        temp[0] = ( pixelA.r * mod + pixelB.r * ( 8 - mod ) ) >> 3;
                        temp[1] = ( pixelA.g * mod + pixelB.g * ( 8 - mod ) ) >> 3;
                        temp[2] = ( pixelA.b * mod + pixelB.b * ( 8 - mod ) ) >> 3;
                        temp[3] = ( pixelA.a * mod + pixelB.a * ( 8 - mod ) ) >> 3;
                        temp[0] = pixel.r - temp[0];
                        temp[1] = pixel.g - temp[1];
                        temp[2] = pixel.b - temp[2];
                        temp[3] = pixel.a - temp[3];
                        
                        uint32_t pixelError = temp[0] * temp[0] + temp[1] * temp[1] + temp[2] * temp[2]; // + temp[3] * temp[3];
                        
                        if ( pixelError < leastPixelError ) {
                            leastPixelError = pixelError;
                            bestM = mod;
                        }
                    }
                    
                    hiRes[pos].r = ( pixelA.r * bestM + pixelB.r * ( 8 - bestM ) ) >> 3;
                    hiRes[pos].g = ( pixelA.g * bestM + pixelB.g * ( 8 - bestM ) ) >> 3;
                    hiRes[pos].b = ( pixelA.b * bestM + pixelB.b * ( 8 - bestM ) ) >> 3;
                    hiRes[pos].a = ( pixelA.a * bestM + pixelB.a * ( 8 - bestM ) ) >> 3;
                    
                    //printf( "%i ", bestM );
                    error += leastPixelError;
                }
                
                //printf( "\n" );
            }
            
            
            printf( "%u\n", error );
        }
    }
    
    
    pngWrite( "/Users/kwasmich/Desktop/Test/GLTC/__BaseA.png", (uint8_t *)baseA, loW, loH, 4 );
    pngWrite( "/Users/kwasmich/Desktop/Test/GLTC/__BaseB.png", (uint8_t *)baseB, loW, loH, 4 );
    pngWrite( "/Users/kwasmich/Desktop/Test/GLTC/__BaseAHi.png", (uint8_t *)baseAHi, in_WIDTH, in_HEIGHT, 4 );
    pngWrite( "/Users/kwasmich/Desktop/Test/GLTC/__BaseBHi.png", (uint8_t *)baseBHi, in_WIDTH, in_HEIGHT, 4 );
    pngWrite( "/Users/kwasmich/Desktop/Test/GLTC/__LowPass.png", (uint8_t *)loRes, loW, loH, 4 );
    pngWrite( "/Users/kwasmich/Desktop/Test/GLTC/__LowPassHi.png", (uint8_t *)hiRes, in_WIDTH, in_HEIGHT, 4 );
    pngWrite( "/Users/kwasmich/Desktop/Test/GLTC/__DeltaHi.png", (uint8_t *)delta, in_WIDTH, in_HEIGHT, 4 );
    
    free( delta );
    delta = NULL;
    free( hiRes );
    hiRes = NULL;
    free( baseBHi );
    baseBHi = NULL;
    free( baseAHi );
    baseAHi = NULL;
    free( baseB );
    baseB = NULL;
    free( baseA );
    baseA = NULL;
    free( loRes );
    loRes = NULL;
}
