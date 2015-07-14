//
//  ETC.c
//  GLTC
//
//  Created by Michael Kwasnicki on 04.01.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#include "ETC.h"

#include "ETC_Compress.h"
#include "ETC_Compress_Common.h"
#include "ETC_Compress_I.h"
#include "ETC_Compress_D.h"
#include "ETC_Compress_T.h"
#include "ETC_Compress_H.h"
#include "ETC_Compress_P.h"
#include "ETC_Decompress.h"
#include "../endianness.h"
#include "../parallelWorker.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>



static void rgb2rgba( rgba8_t *out_rgba, const rgb8_t in_RGB ) {
	out_rgba->r = in_RGB.r;
	out_rgba->g = in_RGB.g;
	out_rgba->b = in_RGB.b;
	out_rgba->a = 255;
}



static void rgba2rgb( rgb8_t *out_rgb, const rgba8_t in_RGBA ) {
	out_rgb->r = in_RGBA.r;
	out_rgb->g = in_RGBA.g;
	out_rgb->b = in_RGBA.b;
}



static bool readRGB( const char in_FILE[], rgb8_t ** out_image, uint32_t * out_width, uint32_t * out_height, void (*decompressBlockRGB)( rgba8_t[4][4], ETCBlockColor_t ) ) {
    uint32_t w = 0;
    uint32_t h = 0;
    uint32_t blockCount = 0;
    size_t itemsRead = 0;
    ETCBlockColor_t * block = NULL;
    ETCBlockColor_t * blockPtr = NULL;
    rgba8_t * imageRGBA = NULL;
    LinearBlock4x4RGBA_t * imageRGBA_ptr = NULL;
    
    FILE * inputETCFileStream = fopen( in_FILE, "rb" );
    assert( inputETCFileStream != NULL );
    itemsRead = fread( &w, sizeof( uint32_t ), 1, inputETCFileStream );
    assert( itemsRead == 1 );
    assert( w > 0 );
    itemsRead = fread( &h, sizeof( uint32_t ), 1, inputETCFileStream );
    assert( itemsRead == 1 );
    assert( h > 0 );
    blockCount = w * h / 16;                                                                                            // FIXME : we have to round up w and h to multiple of 4
    // w >> 2 + ( w & 0x3 ) ? 1 : 0;
    block = malloc( blockCount * sizeof( ETCBlockColor_t ) );
    blockPtr = block;
    imageRGBA = malloc( w * h * sizeof( rgba8_t ) );
    imageRGBA_ptr = REINTERPRET(LinearBlock4x4RGBA_t*)imageRGBA;
    itemsRead = fread( block, sizeof( ETCBlockColor_t ), blockCount, inputETCFileStream );
    assert( itemsRead == blockCount );
    fclose( inputETCFileStream );
    
    for ( uint32_t y = 0; y < h; y += 4 ) {
        for ( uint32_t x = 0; x < w; x += 4 ) {
			betoh64( blockPtr->b64 );
			ETCMode_t mode = etcGetBlockMode( *blockPtr, false );
			
			switch ( mode ) {
				case kETC_I:
					fputs( "I ", stdout );
					break;
					
				case kETC_D:
					fputs( "D ", stdout );
					break;
					
				case kETC_T:
					fputs( "T ", stdout );
					break;
					
				case kETC_H:
					fputs( "H ", stdout );
					break;
					
				case kETC_P:
					fputs( "P ", stdout );
					break;
					
				case kETC_INVALID:
				default:
					fputs( "? ", stdout );
			}
			
            decompressBlockRGB( imageRGBA_ptr->block, *blockPtr );
            blockPtr++;
            imageRGBA_ptr++;
        }
		
		puts( "" );
    }
    
    free_s( block );
    
    twiddleBlocksRGBA( imageRGBA, w, h, true );
    
	
	rgb8_t * imageRGB = malloc( w * h * sizeof( rgb8_t ) );
	
	for ( int xy = 0; xy < w * h; xy++ ) {
		rgba2rgb( &imageRGB[xy], imageRGBA[xy] );
	}
	
	free_s( imageRGBA );
	
    *out_image = imageRGB;
    *out_width = w;
    *out_height = h;
	return true;
}


typedef struct {
    ETCBlockColor_t *out;
    LinearBlock4x4RGBA_t *in;
    Strategy_t strategy;
    void (*compressBlockRGB)( ETCBlockColor_t *, const rgba8_t[4][4], const Strategy_t );
} WorkArgs_s;


void func( void *in_args ) {
    WorkArgs_s *args = (WorkArgs_s*)in_args;
    args->compressBlockRGB( args->out, args->in->block, args->strategy );
    htobe64( args->out->b64 );
}



static bool writeRGB( const char in_FILE[], const rgb8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const Strategy_t in_STRATEGY, void (*compressBlockRGB)( ETCBlockColor_t *, const rgba8_t[4][4], const Strategy_t ) ) {
	computeUniformColorLUT();
    const uint32_t blockCount = in_WIDTH * in_HEIGHT >> 4;    // FIXME : we have to round up w and h to multiple of 4
    ETCBlockColor_t * block = NULL;
    ETCBlockColor_t * blockPtr = NULL;
    rgba8_t * imageRGBA = malloc( in_WIDTH * in_HEIGHT * sizeof( rgba8_t ) );
    LinearBlock4x4RGBA_t * imageRGBA_ptr = REINTERPRET(LinearBlock4x4RGBA_t*)imageRGBA;
	
	for ( int xy = 0; xy < in_WIDTH * in_HEIGHT; xy++ ) {
		rgb2rgba( &imageRGBA[xy], in_IMAGE[xy] );
	}

    twiddleBlocksRGBA( imageRGBA, in_WIDTH, in_HEIGHT, false );
    
    block = malloc( blockCount * sizeof( ETCBlockColor_t ) );
    blockPtr = block;
    
//    for ( uint32_t b = 0; b < blockCount; b++ ) {
//        compressBlockRGB( blockPtr, imageRGBA_ptr->block, in_STRATEGY );
//        htobe64( blockPtr->b64 );
//        blockPtr++;
//        imageRGBA_ptr++;
//    }

    WorkArgs_s *args = malloc( blockCount * sizeof( WorkArgs_s ) );
    WorkItem_s *queue = malloc( blockCount * sizeof( WorkItem_s ) );
    
    for ( uint32_t b = 0; b < blockCount; b++ ) {
        args[b].out = &blockPtr[b];
        args[b].in = &imageRGBA_ptr[b];
        args[b].strategy = in_STRATEGY;
        args[b].compressBlockRGB = compressBlockRGB;
        queue[b].args = &args[b];
        queue[b].func = func;
    }
    
    pwForEach( queue, blockCount, 4 );
    
    //free( queue );
    free( args );
    
    
	printCounter();
    
    FILE * outputETCFileStream = fopen( in_FILE, "wb" );
    fwrite( &in_WIDTH, sizeof( uint32_t ), 1, outputETCFileStream );
    fwrite( &in_HEIGHT, sizeof( uint32_t ), 1, outputETCFileStream );
    fwrite( block, sizeof( ETCBlockColor_t ), blockCount, outputETCFileStream );
    fclose( outputETCFileStream );
    
    free_s( imageRGBA );
    free_s( block );
    
	return true;
}



#pragma mark - exposed interface



bool etcReadETC1RGB( const char in_FILE[], rgb8_t ** out_image, uint32_t * out_width, uint32_t * out_height ) {
	return readRGB( in_FILE, out_image, out_width, out_height, decompressETC1BlockRGB );
}



bool etcReadETC2RGB( const char in_FILE[], rgb8_t ** out_image, uint32_t * out_width, uint32_t * out_height ) {
    return readRGB( in_FILE, out_image, out_width, out_height, decompressETC2BlockRGB );
}



bool etcWriteETC1RGB( const char in_FILE[], const rgb8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const Strategy_t in_STRATEGY ) {
    return writeRGB( in_FILE, in_IMAGE, in_WIDTH, in_HEIGHT, in_STRATEGY, compressETC1BlockRGB );
}



bool etcWriteETC2RGB( const char in_FILE[], const rgb8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const Strategy_t in_STRATEGY ) {
    return writeRGB( in_FILE, in_IMAGE, in_WIDTH, in_HEIGHT, in_STRATEGY, compressETC2BlockRGB );
}



void etcFreeRGB( rgb8_t ** in_out_image ) {
    free_s( *in_out_image );
}



bool etcReadETC2RGBA8( const char in_FILE[], rgba8_t ** out_image, uint32_t * out_width, uint32_t * out_height ) {
	uint32_t w = 0;
    uint32_t h = 0;
    uint32_t blockCount = 0;
    size_t itemsRead = 0;
    ETC2BlockRGBA_t * block = NULL;
    ETC2BlockRGBA_t * blockPtr = NULL;
    rgba8_t * imageRGBA = NULL;
    LinearBlock4x4RGBA_t * imageRGBA_ptr = NULL;
    
    FILE * inputETCFileStream = fopen( in_FILE, "rb" );
    assert( inputETCFileStream != NULL );
    itemsRead = fread( &w, sizeof( uint32_t ), 1, inputETCFileStream );
    assert( itemsRead == 1 );
    itemsRead = fread( &h, sizeof( uint32_t ), 1, inputETCFileStream );
    assert( itemsRead == 1 );
    blockCount = w * h / 16;                                                                                            // FIXME : we have to round up w and h to multiple of 4
    // w >> 2 + ( w & 0x3 ) ? 1 : 0;
    block = malloc( blockCount * sizeof( ETC2BlockRGBA_t ) );
    blockPtr = block;
    imageRGBA = malloc( w * h * sizeof( rgba8_t ) );
    imageRGBA_ptr = REINTERPRET(LinearBlock4x4RGBA_t*)imageRGBA;
    itemsRead = fread( block, sizeof( ETC2BlockRGBA_t ), blockCount, inputETCFileStream );
    assert( itemsRead == blockCount );
    fclose( inputETCFileStream );
    
    for ( int y = 0; y < h; y += 4 ) {
        for ( int x = 0; x < w; x += 4 ) {
			ETCBlockAlpha_t *alpha = &(blockPtr->alpha);
			ETCBlockColor_t *color = &(blockPtr->color);
            betoh64( alpha->b64 );
            betoh64( color->b64 );
			
			ETCMode_t mode = etcGetBlockMode( blockPtr->color, false );
			
			switch ( mode ) {
				case kETC_I:
					fputs( "I ", stdout );
					break;
					
				case kETC_D:
					fputs( "D ", stdout );
					break;
					
				case kETC_T:
					fputs( "T ", stdout );
					break;
					
				case kETC_H:
					fputs( "H ", stdout );
					break;
					
				case kETC_P:
					fputs( "P ", stdout );
					break;
					
				case kETC_INVALID:
				default:
					fputs( "? ", stdout );
			}
			
            decompressETC2BlockRGBA8( imageRGBA_ptr->block, *blockPtr );
            blockPtr++;
            imageRGBA_ptr++;
        }
		
		puts( "" );
    }
    
    free_s( block );
    
    twiddleBlocksRGBA( imageRGBA, w, h, true );
    
    *out_image = imageRGBA;
    *out_width = w;
    *out_height = h;
	return true;
}



bool etcReadETC2RGB8A1( const char in_FILE[], rgba8_t ** out_image, uint32_t * out_width, uint32_t * out_height ) {
	uint32_t w = 0;
    uint32_t h = 0;
    uint32_t blockCount = 0;
    size_t itemsRead = 0;
    ETCBlockColor_t * block = NULL;
    ETCBlockColor_t * blockPtr = NULL;
    rgba8_t * imageRGBA = NULL;
    LinearBlock4x4RGBA_t * imageRGBA_ptr = NULL;
    
    FILE * inputETCFileStream = fopen( in_FILE, "rb" );
    assert( inputETCFileStream != NULL );
    itemsRead = fread( &w, sizeof( uint32_t ), 1, inputETCFileStream );
    assert( itemsRead == 1 );
    itemsRead = fread( &h, sizeof( uint32_t ), 1, inputETCFileStream );
    assert( itemsRead == 1 );
    blockCount = w * h / 16;                                                                                            // FIXME : we have to round up w and h to multiple of 4
    // w >> 2 + ( w & 0x3 ) ? 1 : 0;
    block = malloc( blockCount * sizeof( ETCBlockColor_t ) );
    blockPtr = block;
    imageRGBA = malloc( w * h * sizeof( rgba8_t ) );
    imageRGBA_ptr = REINTERPRET(LinearBlock4x4RGBA_t*)imageRGBA;
    itemsRead = fread( block, sizeof( ETCBlockColor_t ), blockCount, inputETCFileStream );
    assert( itemsRead == blockCount );
    fclose( inputETCFileStream );
    
    for ( int y = 0; y < h; y += 4 ) {
        for ( int x = 0; x < w; x += 4 ) {
            betoh64( blockPtr->b64 );
            decompressETC2BlockRGB8A1( imageRGBA_ptr->block, *blockPtr );
            blockPtr++;
            imageRGBA_ptr++;
        }
    }
    
    free_s( block );
    
    twiddleBlocksRGBA( imageRGBA, w, h, true );
    
    *out_image = imageRGBA;
    *out_width = w;
    *out_height = h;
	return true;
}



bool etcWriteETC2RGBA8( const char in_FILE[], const rgba8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const Strategy_t in_STRATEGY ) {
	computeUniformColorLUT();
    const uint32_t blockCount = in_WIDTH * in_HEIGHT >> 4;    // FIXME : we have to round up w and h to multiple of 4
    ETC2BlockRGBA_t * block = NULL;
    ETC2BlockRGBA_t * blockPtr = NULL;
    rgba8_t * imageRGBA = malloc( in_WIDTH * in_HEIGHT * sizeof( rgba8_t ) );
    LinearBlock4x4RGBA_t * imageRGBA_ptr = REINTERPRET(LinearBlock4x4RGBA_t*)imageRGBA;
    memcpy( imageRGBA, in_IMAGE, in_WIDTH * in_HEIGHT * sizeof( rgba8_t ) );
    twiddleBlocksRGBA( imageRGBA, in_WIDTH, in_HEIGHT, false );
    
    block = malloc( blockCount * sizeof( ETC2BlockRGBA_t ) );
    blockPtr = block;
    
    for ( uint32_t b = 0; b < blockCount; b++ ) {
        compressETC2BlockRGBA8( blockPtr, imageRGBA_ptr->block, in_STRATEGY );
		ETCBlockAlpha_t *alpha = &(blockPtr->alpha);
		ETCBlockColor_t *color = &(blockPtr->color);
        htobe64( alpha->b64 );
        htobe64( color->b64 );
        blockPtr++;
        imageRGBA_ptr++;
    }
	
	printCounter();
    
    FILE * outputETCFileStream = fopen( in_FILE, "wb" );
    fwrite( &in_WIDTH, sizeof( uint32_t ), 1, outputETCFileStream );
    fwrite( &in_HEIGHT, sizeof( uint32_t ), 1, outputETCFileStream );
    fwrite( block, sizeof( ETC2BlockRGBA_t ), blockCount, outputETCFileStream );
    fclose( outputETCFileStream );
    
    free_s( imageRGBA );
    free_s( block );
    
	return true;
}



bool etcWriteETC2RGB8A1( const char in_FILE[], const rgba8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const Strategy_t in_STRATEGY ) {
	computeUniformColorLUT();
    const uint32_t blockCount = in_WIDTH * in_HEIGHT >> 4;    // FIXME : we have to round up w and h to multiple of 4
    ETCBlockColor_t * block = NULL;
    ETCBlockColor_t * blockPtr = NULL;
    rgba8_t * imageRGBA = malloc( in_WIDTH * in_HEIGHT * sizeof( rgba8_t ) );
    LinearBlock4x4RGBA_t * imageRGBA_ptr = REINTERPRET(LinearBlock4x4RGBA_t*)imageRGBA;
    memcpy( imageRGBA, in_IMAGE, in_WIDTH * in_HEIGHT * sizeof( rgba8_t ) );
    twiddleBlocksRGBA( imageRGBA, in_WIDTH, in_HEIGHT, false );
    
    block = malloc( blockCount * sizeof( ETCBlockColor_t ) );
    blockPtr = block;
    
    for ( uint32_t b = 0; b < blockCount; b++ ) {
        compressETC2BlockRGB8A1( blockPtr, imageRGBA_ptr->block, in_STRATEGY );
        htobe64( blockPtr->b64 );
        blockPtr++;
        imageRGBA_ptr++;
    }
	
	printCounter();
    
    FILE * outputETCFileStream = fopen( in_FILE, "wb" );
    fwrite( &in_WIDTH, sizeof( uint32_t ), 1, outputETCFileStream );
    fwrite( &in_HEIGHT, sizeof( uint32_t ), 1, outputETCFileStream );
    fwrite( block, sizeof( ETCBlockColor_t ), blockCount, outputETCFileStream );
    fclose( outputETCFileStream );
    
    free_s( imageRGBA );
    free_s( block );
    
	return true;
}



void etcFreeRGBA( rgba8_t ** in_out_image ) {
    free_s( *in_out_image );
}



/*
bool etcResumeWriteETC1RGB( const char in_FILE[], const rgb8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const Strategy_t in_STRATEGY ) {
	computeUniformColorLUT();
	
	uint32_t w = in_WIDTH;
    uint32_t h = in_HEIGHT;
    uint32_t blockCount = 0;
    ETCBlockColor_t * block = NULL;
    ETCBlockColor_t * blockPtr = NULL;
    rgb8_t blockRGB[4][4];
    
	blockCount = w * h / 16;                                                                                            // FIXME : we have to round up w and h to multiple of 4
	// w >> 2 + ( w & 0x3 ) ? 1 : 0;
	block = malloc( blockCount * sizeof( ETCBlockColor_t ) );
	blockPtr = block;
	
	FILE * inputETCFileStream = fopen( in_FILE, "rb" );
	
	if ( inputETCFileStream ) {
		assert( inputETCFileStream != NULL );
		size_t itemsRead = fread( &w, sizeof( uint32_t ), 1, inputETCFileStream );
		assert( itemsRead == 1 );
		assert( w == in_WIDTH );
		itemsRead = fread( &h, sizeof( uint32_t ), 1, inputETCFileStream );
		assert( itemsRead == 1 );
		assert( h == in_HEIGHT );
		itemsRead = fread( block, sizeof( ETCBlockColor_t ), blockCount, inputETCFileStream );
		assert( itemsRead == blockCount );
		fclose( inputETCFileStream );
	} else {
		memset( block, 0x0, blockCount * sizeof( ETCBlockColor_t ) );
	}
    
	FILE * outputETCFileStream = fopen( in_FILE, "wb" );
    fwrite( &w, sizeof( uint32_t ), 1, outputETCFileStream );
    fwrite( &h, sizeof( uint32_t ), 1, outputETCFileStream );
	fwrite( blockPtr, sizeof( ETCBlockColor_t ), blockCount, outputETCFileStream );
	rewind( outputETCFileStream );
	fwrite( &w, sizeof( uint32_t ), 1, outputETCFileStream );
    fwrite( &h, sizeof( uint32_t ), 1, outputETCFileStream );
	
    for ( int y = 0; y < h; y += 4 ) {
        for ( int x = 0; x < w; x += 4 ) {
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    int arrayPosition = ( y + by ) * w + x + bx;
                    blockRGB[by][bx] = in_IMAGE[arrayPosition];
                }
            }
            
			if ( blockPtr->b64 == 0 ) {
				compressETC1BlockRGB( blockPtr, (const rgb8_t(*)[4])blockRGB, in_STRATEGY );
				htobe64( blockPtr->b64 );
			} else {
				puts( "skipping" );
			}
			
			fwrite( blockPtr, sizeof( ETCBlockColor_t ), 1, outputETCFileStream );
			fflush( outputETCFileStream );
            blockPtr++;
        }
    }
	
	fclose( outputETCFileStream );
	printCounter();
	return true;
}



static ETCBlockColor_t * blockOut( const char in_FILE[], const uint32_t in_WIDTH, const uint32_t in_HEIGHT ) {
	uint32_t w = 0;
    uint32_t h = 0;
	size_t itemsRead = 0;
	uint32_t blockCount = 0;
	ETCBlockColor_t * block;
	
	FILE * inputETCFileStream = fopen( in_FILE, "rb" );
    assert( inputETCFileStream != NULL );
    itemsRead = fread( &w, sizeof( uint32_t ), 1, inputETCFileStream );
    assert( itemsRead == 1 );
    itemsRead = fread( &h, sizeof( uint32_t ), 1, inputETCFileStream );
    assert( itemsRead == 1 );
	assert( w == in_WIDTH );
	assert( h == in_HEIGHT );
    blockCount = w * h / 16;                                                                                            // FIXME : we have to round up w and h to multiple of 4
    // w >> 2 + ( w & 0x3 ) ? 1 : 0;
    block = malloc( blockCount * sizeof( ETCBlockColor_t ) );
    itemsRead = fread( block, sizeof( ETCBlockColor_t ), blockCount, inputETCFileStream );
    assert( itemsRead == blockCount );
    fclose( inputETCFileStream );
	return block;
}







// TODO : Remove those in a future version

bool etcReadRGBCompare( const char in_FILE_I[], const char in_FILE_D[], const char in_FILE_T[], const char in_FILE_H[], const char in_FILE_P[], const rgb8_t * in_IMAGE, const uint32_t in_WIDTH, const uint32_t in_HEIGHT, const Strategy_t in_STRATEGY ) {
    uint32_t w = in_WIDTH;
    uint32_t h = in_HEIGHT;
    
    ETCBlockColor_t * block[5] = { NULL, NULL, NULL, NULL, NULL };
	ETCBlockColor_t * blockPtr;
    uint32_t blockIndex = 0;
	rgb8_t blockRGBReference[4][4];
    rgb8_t blockRGB[4][4];
	int dR, dG, dB;
	uint32_t pixelError, blockError, bestBlockError;
    
    block[0] = blockOut( in_FILE_I, w, h );
	block[1] = blockOut( in_FILE_D, w, h );
	block[2] = blockOut( in_FILE_T, w, h );
	block[3] = blockOut( in_FILE_H, w, h );
	block[4] = blockOut( in_FILE_P, w, h );
    
    for ( int y = 0; y < h; y += 4 ) {
        for ( int x = 0; x < w; x += 4 ) {
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    int arrayPosition = ( y + by ) * w + x + bx;
                    blockRGBReference[by][bx] = in_IMAGE[arrayPosition];
                }
            }
			
			bestBlockError = 0xFFFFFFFF;
			
			for ( int i = 0; i < 5; i++ ) {
				blockPtr = &block[i][blockIndex];
                betoh64( blockPtr->b64 );
				decompressETC2BlockRGB( blockRGB, block[i][blockIndex] );
				
				blockError = 0;
				
				for ( int by = 0; by < 4; by++ ) {
					for ( int bx = 0; bx < 4; bx++ ) {
						dR = blockRGB[by][bx].r - blockRGBReference[by][bx].r;
						dG = blockRGB[by][bx].g - blockRGBReference[by][bx].g;
						dB = blockRGB[by][bx].b - blockRGBReference[by][bx].b;
						pixelError = dR * dR + dG * dG + dB * dB;
						blockError += pixelError;
					}
				}
				
				if ( blockError < bestBlockError ) {
					bestBlockError = blockError;
				}
				
				//printf( "%s %i\n", bla( i ), blockError );
			}
			
            //			printInfoI( &block[0][blockIndex] );
            //			printInfoD( &block[1][blockIndex] );
			printInfoT( &block[2][blockIndex] );
            //			printInfoH( &block[3][blockIndex] );
            //			printInfoP( &block[4][blockIndex] );
			
            //			printf( "%s ", bla( bestBlockType ) );
            //			puts( "" );
			
			compressETC1BlockRGB( &block[0][blockIndex], (const rgb8_t(*)[4])blockRGBReference, in_STRATEGY );
			
			blockIndex++;
        }
		
		puts( "" );
    }
    
	return true;
}
*/

