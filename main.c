//
//  main.c
//  GLTC
//
//  Created by Michael Kwasnicki on 19.03.11.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#include "PNG.h"
#include "colorSpaceReduction.h"
#include "DXTC.h"
#include "ETC.h"
#include "PVRTC.h"
#include "lib.h"


#include <assert.h>
#include <float.h>
#include <iso646.h> //or and xor...
#include <math.h>
#include <unistd.h> //getopt
#include <stdbool.h> //bool true false
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



/*
// set FLIP_Y of read_png to TRUE to use image output in OpenGL
void testColorSpaceReduction() {
    fillLUT();
    prepareBayer();
    
    rgb565_t color565 = { 0, 0, 0 };
    rgba4444_t color4444 = { 0, 0, 0, 0 };
    rgba5551_t color5551 = { 0, 0, 0, 0 };
    rgb8_t * imageRGB8 = NULL;
    rgb8_t * imageRGB8Ptr = NULL;
    rgba8_t * imageRGBA8 = NULL;
    rgba8_t * imageRGBA8Ptr = NULL;
    rgb565_t * imageRGB565 = NULL;
    rgb565_t * imageRGB565Ptr = NULL;
    rgba4444_t * imageRGBA4444 = NULL;
    rgba4444_t * imageRGBA4444Ptr = NULL;
    rgba5551_t * imageRGBA5551 = NULL;
    rgba5551_t * imageRGBA5551Ptr = NULL;
	unsigned long w = 0;
	unsigned long h = 0;
	int c = 0;
    FILE * outputFileStream = NULL;
    
    
    pngRead( "PNG512A.png", false, (png_byte **)&imageRGBA8, &w, &h, &c );
    imageRGBA8Ptr = imageRGBA8;
    imageRGBA4444 = malloc( w * h * sizeof( rgba4444_t ) );
    imageRGBA4444Ptr = imageRGBA4444;
    
    for ( int y = 0; y < h; y++ ) {
        for ( int x = 0; x < w; x++ ) {
            ditherRGBA( imageRGBA8Ptr, kRGBA4444, x, y );
            convert8888to4444( &color4444, *imageRGBA8Ptr );
            convert4444to8888( imageRGBA8Ptr, color4444 );
            *imageRGBA4444Ptr = color4444;
            imageRGBA8Ptr++;
            imageRGBA4444Ptr++;
        }
    }
    
    pngWrite( "PNG512A.4444.png", (png_byte *)imageRGBA8, w, h, c );
	free( imageRGBA8 );
	imageRGBA8 = NULL;
    imageRGBA8Ptr = NULL;
    
    outputFileStream = fopen( "PNG512A.4444", "wb" );
    fwrite( imageRGBA4444, sizeof( rgba4444_t ), w * h, outputFileStream );
    fclose( outputFileStream );
    outputFileStream = NULL;
    free( imageRGBA4444 );
    imageRGBA4444 = NULL;
    imageRGBA4444Ptr = NULL;
    
    
    
    pngRead( "PNG512A.png", false, (png_byte **)&imageRGBA8, &w, &h, &c );
    imageRGBA8Ptr = imageRGBA8;
    imageRGBA5551 = malloc( w * h * sizeof( rgba5551_t ) );
    imageRGBA5551Ptr = imageRGBA5551;
    
    for ( int y = 0; y < h; y++ ) {
        for ( int x = 0; x < w; x++ ) {
            ditherRGBA( imageRGBA8Ptr, kRGBA5551A, x, y );
            convert8888to5551( &color5551, *imageRGBA8Ptr );
            convert5551to8888( imageRGBA8Ptr, color5551 );
            *imageRGBA5551Ptr = color5551;
            imageRGBA8Ptr++;
            imageRGBA5551Ptr++;
        }
    }
    
    pngWrite( "PNG512A.5551.png", (png_byte *)imageRGBA8, w, h, c );
	free( imageRGBA8 );
	imageRGBA8 = NULL;
    imageRGBA8Ptr = NULL;
    
    outputFileStream = fopen( "PNG512A.5551", "wb" );
    fwrite( imageRGBA5551, sizeof( rgba5551_t ), w * h, outputFileStream );
    fclose( outputFileStream );
    outputFileStream = NULL;
    free( imageRGBA5551 );
    imageRGBA5551 = NULL;
    imageRGBA5551Ptr = NULL;
    
    
    
    pngRead( "PNG512.png", false, (png_byte **)&imageRGB8, &w, &h, &c );
    imageRGB8Ptr = imageRGB8;
    imageRGB565 = malloc( w * h * sizeof( rgb565_t ) );
    imageRGB565Ptr = imageRGB565;
    
    for ( int y = 0; y < h; y++ ) {
        for ( int x = 0; x < w; x++ ) {
            ditherRGB( imageRGB8Ptr, kRGB565, x, y );
            convert888to565( &color565, *imageRGB8Ptr );
            convert565to888( imageRGB8Ptr, color565 );
            *imageRGB565Ptr = color565;
            imageRGB8Ptr++;
            imageRGB565Ptr++;
        }
    }
    
    pngWrite( "PNG512.565.png", (png_byte *)imageRGB8, w, h, c );
	free( imageRGB8 );
	imageRGB8 = NULL;
    imageRGB8Ptr = NULL;
    
    outputFileStream = fopen( "PNG512.565", "wb" );
    fwrite( imageRGB565, sizeof( rgb565_t ), w * h, outputFileStream );
    fclose( outputFileStream );
    outputFileStream = NULL;
    free( imageRGB565 );
    imageRGB565 = NULL;
    imageRGB565Ptr = NULL;
}
*/

static void usage( const char in_EXEC[] ) {
    printf( "Usage:\n" );
    printf( "%s\n -qwertz\n", in_EXEC );
    exit( EXIT_FAILURE );
}

typedef enum {
    //kRGB565, kRGBA4444, kRGBA5551, 
    kRGB_DXT1,
	kRGBA_DXT1,
	kRGBA_DXT3,
	kRGBA_DXT5,
	kRGB_ETC1,
	kRGB_ETC2,
	kRGBA_ETC2,
	kRGBA_ETC2_PUNCH_THROUGH,
	kRGBA_PVR4BPP,
	kRGBA_PVR2BPP,
	kINVALID
} tcType_t;

/*
static int main1( int argc, char * argv[] ) {
    uint32_t w = 0;
    uint32_t h = 0;
    uint32_t c = 0;
    rgba8_t * a = NULL;
    rgba8_t * b = NULL;
    rgb8_t * ab = NULL;
    uint8_t * m = NULL;
    
    pngRead( "/Users/kwasmich/Desktop/Test/GLTC/_A.png", false, (uint8_t **)&a, (uint32_t *)&w, (uint32_t *)&h, &c );
    assert( c == 4 );
    pngRead( "/Users/kwasmich/Desktop/Test/GLTC/_B.png", false, (uint8_t **)&b, (uint32_t *)&w, (uint32_t *)&h, &c );
    assert( c == 4 );
    pngRead( "/Users/kwasmich/Desktop/Test/TextureTool/TestRGB256.pvrtc2.ref.png", false, (uint8_t **)&ab, (uint32_t *)&w, (uint32_t *)&h, &c );
    assert( c == 3 );
    m = malloc( w * h * sizeof(uint8_t) );
    
    for ( int xy = 0; xy < w * h; xy++ ) {
        uint32_t bestError = 0xFFFFFFFF;
        uint8_t bestM = 0;
//#warning sdf
//#error        finde m/8, sodass    m * a + (1 - m) * b = ab
        
        for ( int m = 0; m <= 8; m++ ) {
            uint32_t error = 0;
            
            for ( int i = 0; i < 3; i++ ) {
                int tmp = ( a[xy].array[i] * m + b[xy].array[i] * ( 8 - m ) ) >> 3;
                int tmp2 = ab[xy].array[i] - tmp;
                error += tmp2 * tmp2;
            }
            
            if ( error < bestError ) {
                bestError = error;
                bestM = m;
            }
        }
        
        printf( "%i (%i)\n", bestM, bestError );
        
        m[xy] = bestM * 31;
    }
    
    pngWrite( "/Users/kwasmich/Desktop/Test/GLTC/_M.ref.png", m, w, h, 1 );
    
    return 0;
}

static int main2( int argc, char * argv[] ) {
	int w = 4096 * 4;
	int h = 4096 * 4;
	int c = 3;
	rgb8_t * imageData = malloc( w * h * sizeof(rgb8_t) );
	
	for ( int b = 0; b < 256; b++ ) {
		for ( int g = 0; g < 256; g++ ) {
			for ( int r = 0; r < 256; r++ ) {
				int x = r + ( b % 16 ) * 256;
				int y = g + ( b / 16 ) * 256;
				int pos = x + y * 4096;
				
				for ( int dy = 0; dy < 4; dy++ ) {
					for ( int dx = 0; dx < 4; dx++ ) {
						int newPos = x * 4 + dx + ( y * 4 + dy ) * 4 * 4096;
						imageData[newPos].r = r;
						imageData[newPos].g = g;
						imageData[newPos].b = b;
					}
				}
			}
		}
	}
	
	pngWrite( "trueColor4.png", REINTERPRET(uint8_t *)imageData, w, h, c );
	
	return 0;
}
*/


int main( int argc, char * argv[] ) {
    printf( "use KTX as container format! - visit http://www.khronos.org/opengles/sdk/tools/KTX/\n" );
    fillLUT();
	
	
//	for ( int i = 1; i < 512; i++ ) {
//		printf( "%i\n", (i >> 1) * ( (i bitand 0x1) ? -1 : 1 ) );
//	}
//	
//	return EXIT_SUCCESS;
	
	if ( false ) {
		uint32_t w = 0;
        uint32_t h = 0;
        uint32_t c = 0;
        rgb8_t * imageData;
		pngRead( expandTilde( "~/Desktop/Test/TestRGB64.png" ), false, (uint8_t **)&imageData, (uint32_t *)&w, (uint32_t *)&h, &c );
		assert( c == 3 );
		etcReadRGBCompare( expandTilde( "~/Desktop/ETC/TestRGB64_I.etc" ),
						  expandTilde( "~/Desktop/ETC/TestRGB64_D2.etc" ),
						  expandTilde( "~/Desktop/ETC/TestRGB64_T.etc" ), 
						  expandTilde( "~/Desktop/ETC/TestRGB64_H.etc" ), 
						  expandTilde( "~/Desktop/ETC/TestRGB64_P.etc" ), imageData, w, h, kBEST );
		pngFree( (uint8_t **)&imageData );
		
		return EXIT_SUCCESS;
	}
	
	
    const char* execName = strrchr( argv[0], '/' );
	execName++;
	
	int ch;
    bool optC = false;
    bool optD = false;
    bool optF = false;
    bool optO = false;
	bool optR = false;
    Strategy_t optStrategy = kFAST;
    char * optCDArg = NULL;
    char * optFArg = NULL;
	char * optOArg = NULL;
	
	while ( ( ch = getopt( argc, argv, "c:d:f:o:r0123456789h?" ) ) != -1 ) {
		switch ( ch ) {
			case 'c':
				optC = true;
                optCDArg = optarg;
				break;
                
            case 'd':
				optD = true;
                optCDArg = optarg;
				break;
                
            case 'f':
				optF = true;
                optFArg = optarg;
				break;
                
			case 'o':
				optO = true;
                optOArg = optarg;
				break;
                
			case 'r':
				optR = true;
				break;
            
            case '0':
                optStrategy = kFAST;
                break;
                
            case '1':
                optStrategy = 1;
                break;
                
            case '2':
                optStrategy = 2;
                break;
                
            case '3':
                optStrategy = 3;
                break;
                
            case '4':
                optStrategy = 4;
                break;
                
            case '5':
                optStrategy = 5;
                break;
                
            case '6':
                optStrategy = 6;
                break;
                
            case '7':
                optStrategy = 7;
                break;
                
            case '8':
                optStrategy = 8;
                break;
                
            case '9':
                optStrategy = kBEST;
                break;
                
			case 'h':
			case '?':
			default:
				usage( execName );
		}
	}
	
    argc -= optind;
	argv += optind;

    printf( "c: %i \"%s\"\n", optC, optCDArg );
    printf( "d: %i \"%s\"\n", optD, optCDArg );
    printf( "-%i\n", optStrategy );
    printf( "f: %i \"%s\" -> \"%s\"\n", optF, optFArg, expandTilde( optFArg ) );
    printf( "o: %i \"%s\" -> \"%s\"\n", optO, optOArg, expandTilde( optOArg ) );
	
	optFArg = expandTilde( optFArg );
	optOArg = expandTilde( optOArg );
	
    for ( int i = 0; i < argc; i++ ) {
        printf( "%i: \"%s\"\n", i, argv[i] );
    }
	
    // either compress or decompress
	if ( not ( optC xor optD ) ) {
		usage( execName );
	}
    
    // determine type of compression
    tcType_t type = kINVALID;
    
    type = ( strcmp( optCDArg, "DXT1RGB" ) == 0 ) ? kRGB_DXT1 : type;
    type = ( strcmp( optCDArg, "DXT1RGBA" ) == 0 ) ? kRGBA_DXT1 : type;
    type = ( strcmp( optCDArg, "DXT3" ) == 0 ) ? kRGBA_DXT3 : type;
    type = ( strcmp( optCDArg, "DXT5" ) == 0 ) ? kRGBA_DXT5 : type;
	type = ( strcmp( optCDArg, "ETC1" ) == 0 ) ? kRGB_ETC1 : type;
	type = ( strcmp( optCDArg, "ETC2RGB" ) == 0 ) ? kRGB_ETC2 : type;
	type = ( strcmp( optCDArg, "ETC2RGBA" ) == 0 ) ? kRGBA_ETC2 : type;
	type = ( strcmp( optCDArg, "ETC2RGBAPUNCH" ) == 0 ) ? kRGBA_ETC2_PUNCH_THROUGH : type;
    type = ( strcmp( optCDArg, "PVR4BPP" ) == 0 ) ? kRGBA_PVR4BPP : type;
    type = ( strcmp( optCDArg, "PVR2BPP" ) == 0 ) ? kRGBA_PVR2BPP : type;
    
	if ( not optF ) {
        if ( argc > 0 ) {
            optFArg = argv[0];
        } else {
            usage( execName );
        }
    }
	
    // compress
    if ( optC ) {
        uint32_t w = 0;
        uint32_t h = 0;
        uint32_t c = 0;
        
        switch ( type ) {
            case kRGB_DXT1:
            {
                rgb8_t * imageData;
                pngRead( optFArg, false, (uint8_t **)&imageData, (uint32_t *)&w, (uint32_t *)&h, &c );
                assert( c == 3 );
                dxtcWriteDXT1RGB( optOArg, imageData, w, h );
                pngFree( (uint8_t **)&imageData );
            }
                break;
                
            case kRGBA_DXT1:
            {
                rgba8_t * imageData;
                pngRead( optFArg, false, (uint8_t **)&imageData, (uint32_t *)&w, (uint32_t *)&h, &c );
                assert( c == 4 );
                dxtcWriteDXT1RGBA( optOArg, imageData, w, h );
                pngFree( (uint8_t **)&imageData );
            }
                break;
                
            case kRGBA_DXT3:
            {
                rgba8_t * imageData;
                pngRead( optFArg, false, (uint8_t **)&imageData, (uint32_t *)&w, (uint32_t *)&h, &c );
                assert( c == 4 );
                dxtcWriteDXT3RGBA( optOArg, imageData, w, h );
                pngFree( (uint8_t **)&imageData );
            }
                break;
                
            case kRGBA_DXT5:
            {
                rgba8_t * imageData;
                pngRead( optFArg, false, (uint8_t **)&imageData, (uint32_t *)&w, (uint32_t *)&h, &c );
                assert( c == 4 );
                dxtcWriteDXT5RGBA( optOArg, imageData, w, h );
                pngFree( (uint8_t **)&imageData );
            }
                break;
				
			case kRGB_ETC1:
            {
                rgb8_t * imageData;
                pngRead( optFArg, false, (uint8_t **)&imageData, (uint32_t *)&w, (uint32_t *)&h, &c );
                assert( c == 3 );
				
				if ( optR )
					etcResumeWriteETC1RGB( optOArg, imageData, w, h, optStrategy );
				else
					etcWriteETC1RGB( optOArg, imageData, w, h, optStrategy );
                
                pngFree( (uint8_t **)&imageData );
            }
                break;
				
			case kRGB_ETC2:
			{
                rgb8_t * imageData;
                pngRead( optFArg, false, (uint8_t **)&imageData, (uint32_t *)&w, (uint32_t *)&h, &c );
                assert( c == 3 );
				etcWriteETC2RGB( optOArg, imageData, w, h, optStrategy );
                pngFree( (uint8_t **)&imageData );
            }
                break;
				
			case kRGBA_ETC2:
			{
                rgba8_t * imageData;
                pngRead( optFArg, false, (uint8_t **)&imageData, (uint32_t *)&w, (uint32_t *)&h, &c );
                assert( c == 4 );
				etcWriteETC2RGBA( optOArg, imageData, w, h, optStrategy );
                pngFree( (uint8_t **)&imageData );
            }
                break;
				
//			case kRGBA_ETC2_PUNCH_THROUGH:
//			{
//                rgba8_t * imageData;
//                pngRead( optFArg, false, (uint8_t **)&imageData, (uint32_t *)&w, (uint32_t *)&h, &c );
//                assert( c == 4 );
//				etcWriteETC2RGBAPunchThrough( optOArg, imageData, w, h, optStrategy );
//                pngFree( (uint8_t **)&imageData );
//            }
//				break;
                
            case kRGBA_PVR4BPP:
            {
                rgba8_t * imageData;
                pngRead( optFArg, false, (uint8_t **)&imageData, (uint32_t *)&w, (uint32_t *)&h, &c );
                assert( c == 4 );
                pvrtcWrite4BPPRGBA( optOArg, imageData, w, h );
                pngFree( (uint8_t **)&imageData );
            }
                break;
			
			case kRGBA_ETC2_PUNCH_THROUGH:
            case kRGBA_PVR2BPP:
            case kINVALID:
				fprintf( stderr, "Invalid type %i\n", type );
				exit( EXIT_FAILURE );
        }
    }
    
    // decompress
    if ( optD ) {
        uint32_t w = 0;
        uint32_t h = 0;
        
        switch ( type ) {
            case kRGB_DXT1:
            {
                rgb8_t * imageData;
                dxtcReadDXT1RGB( optFArg, &imageData, &w, &h );
                pngWrite( optOArg, REINTERPRET(uint8_t *)imageData, w, h, 3 );
                dxtcFreeRGB( &imageData );
            }
                break;
                
            case kRGBA_DXT1:
            {
                rgba8_t * imageData;
                dxtcReadDXT1RGBA( optFArg, &imageData, &w, &h );
                pngWrite( optOArg, REINTERPRET(uint8_t *)imageData, w, h, 4 );
                dxtcFreeRGBA( &imageData );
            }
                break;
                
            case kRGBA_DXT3:
            {
                rgba8_t * imageData;
                dxtcReadDXT3RGBA( optFArg, &imageData, &w, &h );
                pngWrite( optOArg, REINTERPRET(uint8_t *)imageData, w, h, 4 );
                dxtcFreeRGBA( &imageData );
            }
                break;
                
            case kRGBA_DXT5:
            {
                rgba8_t * imageData;
                dxtcReadDXT5RGBA( optFArg, &imageData, &w, &h );
                pngWrite( optOArg, REINTERPRET(uint8_t *)imageData, w, h, 4 );
                dxtcFreeRGBA( &imageData );
            }
                break;
				
			case kRGB_ETC1:
            {
                rgb8_t * imageData;
				etcReadETC1RGB( optFArg, &imageData, &w, &h );
                pngWrite( optOArg, REINTERPRET(uint8_t *)imageData, w, h, 3 );
                etcFreeRGB( &imageData );
            }
                break;
				
			case kRGB_ETC2:
            {
                rgb8_t * imageData;
				etcReadETC2RGB( optFArg, &imageData, &w, &h );
                pngWrite( optOArg, REINTERPRET(uint8_t *)imageData, w, h, 3 );
                etcFreeRGB( &imageData );
            }
                break;
				
			case kRGBA_ETC2:
            {
                rgba8_t * imageData;
				etcReadETC2RGBA( optFArg, &imageData, &w, &h );
                pngWrite( optOArg, REINTERPRET(uint8_t *)imageData, w, h, 4 );
                etcFreeRGBA( &imageData );
            }
                break;
                
			case kRGBA_ETC2_PUNCH_THROUGH:
            {
                rgba8_t * imageData;
				etcReadETC2RGBAPunchThrough( optFArg, &imageData, &w, &h );
                pngWrite( optOArg, REINTERPRET(uint8_t *)imageData, w, h, 4 );
                etcFreeRGBA( &imageData );
            }
                break;
				
            case kRGBA_PVR4BPP:
            {
                rgba8_t * imageData;
                pvrtcRead4BPPRGBA( optFArg, &imageData, &w, &h );
                pngWrite( optOArg, REINTERPRET(uint8_t *)imageData, w, h, 4 );
                dxtcFreeRGBA( &imageData );
            }
                break;
                
            case kRGBA_PVR2BPP:
            {
                rgba8_t * imageData;
                pvrtcRead2BPPRGBA( optFArg, &imageData, &w, &h );
                pngWrite( optOArg, REINTERPRET(uint8_t *)imageData, w, h, 4 );
                dxtcFreeRGBA( &imageData );
            }
                break;
			
			default:
				fprintf( stderr, "Invalid type %i\n", type );
				exit( EXIT_FAILURE );
        }
    }
	
    printf( "Hello World!\n" );
    return EXIT_SUCCESS;
}
