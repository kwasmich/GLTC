//
//  main.c
//  GLTC
//
//  Created by Michael Kwasnicki on 19.03.11.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#include "simplePNG.h"
#include "colorSpaceReduction.h"
#include "dxtc/DXTC.h"
#include "etc/ETC.h"
#include "pvrtc/PVRTC.h"
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



static void usage( const char in_EXEC[], const char in_FALLACY[] ) {
	puts( in_FALLACY );
    printf("\nUsage:\n"
		   "%s -cd <mode> -0123456789h? -f <input_image> [-o <output_image>]\n"
		   "\n"
		   "the following options are available:\n"
		   "-c <mode>\n"
		   "       Compress the input image and store the compressed result in output image.\n"
		   "       This option is mutual exclusive with -d.\n"
		   "\n"
		   "-d <mode>\n"
		   "       Decompress a previously compressed input image and store the uncompressed result in output image.\n"
		   "       This option is mutual exclusive with -c.\n"
		   "\n"
		   "-0, -1, -2, -3, -4, -5, -6, -7, -8, -9\n"
		   "       When compressing an image these options tell the compressor how exhaustive the search for the best image quality should be.\n"
		   "       -0 is the fastest possible compression and -9 provides least the same quality as a brute force search.\n"
		   "\n"
		   "-h, -?\n"
		   "       Prints this help.\n"
		   "\n"
		   "-f <input_image>\n"
		   "       The image to be processed.\n"
		   "\n"
		   "-o <output_image>\n"
		   "       Path where the output image will be stored. If not specified the output will be stored in /dev/null.\n"
		   "\n"
		   "List of valid values for <mode>:\n"
		   "\n"
		   "   ETC1\n"
		   "   ETC2RGB\n"
		   "   ETC2RGBA\n"
		   "   ETC2RGBAPUNCH\n"
		   "   DXT1RGB\n"
		   "   DXT1RGBA\n"
		   "   DXT3\n"
		   "   DXT5\n"
		   "   PVR2BPP\n"
		   "   PVR4BPP\n"
		   "   \n"
		   "List of    supported file formats:\n"
		   "   \n"
		   "   KTX\n"
		   "   PNG\n"
		   "\n", in_EXEC );
    exit( EXIT_FAILURE );
}



int main( int argc, char * argv[] ) {
    printf( "use KTX as container format! - visit http://www.khronos.org/opengles/sdk/tools/KTX/\n" );
    fillLUT();
	
	
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
				usage( execName, "" );
		}
	}
	
    argc -= optind;
	argv += optind;

	// either compress or decompress
	if ( not ( optC xor optD ) ) {
		usage( execName, "You must specify if you want to compress (-c) or decompress (-d)." );
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
    
	if ( type == kINVALID ) {
		usage( execName, "Invalid argument provided for <mode>." );
	}
	
	if ( not optF ) {
        if ( argc > 0 ) {
            optFArg = argv[0];
        } else {
            usage( execName, "Missing input image file" );
        }
    }
	
	optFArg = expandTilde( optFArg );
	
	if ( not optO ) {
		optOArg = "/dev/null";
	}
	
	optOArg = expandTilde( optOArg );
	
	printf( "c: %i \"%s\"\n", optC, optCDArg );
    printf( "d: %i \"%s\"\n", optD, optCDArg );
    printf( "-%i\n", optStrategy );
    printf( "f: %i \"%s\" -> \"%s\"\n", optF, optFArg, expandTilde( optFArg ) );
    printf( "o: %i \"%s\" -> \"%s\"\n", optO, optOArg, expandTilde( optOArg ) );
	
    for ( int i = 0; i < argc; i++ ) {
        printf( "%i: \"%s\"\n", i, argv[i] );
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
				etcWriteETC2RGBA8( optOArg, imageData, w, h, optStrategy );
                pngFree( (uint8_t **)&imageData );
            }
                break;
				
			case kRGBA_ETC2_PUNCH_THROUGH:
			{
                rgba8_t * imageData;
                pngRead( optFArg, false, (uint8_t **)&imageData, (uint32_t *)&w, (uint32_t *)&h, &c );
                assert( c == 4 );
				etcWriteETC2RGB8A1( optOArg, imageData, w, h, optStrategy );
                pngFree( (uint8_t **)&imageData );
            }
				break;
                
            case kRGBA_PVR4BPP:
            {
                rgba8_t * imageData;
                pngRead( optFArg, false, (uint8_t **)&imageData, (uint32_t *)&w, (uint32_t *)&h, &c );
                assert( c == 4 );
                pvrtcWrite4BPPRGBA( optOArg, imageData, w, h );
                pngFree( (uint8_t **)&imageData );
            }
                break;

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
				etcReadETC2RGBA8( optFArg, &imageData, &w, &h );
                pngWrite( optOArg, REINTERPRET(uint8_t *)imageData, w, h, 4 );
                etcFreeRGBA( &imageData );
            }
                break;
                
			case kRGBA_ETC2_PUNCH_THROUGH:
            {
                rgba8_t * imageData;
				etcReadETC2RGB8A1( optFArg, &imageData, &w, &h );
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
