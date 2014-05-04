//
//  ETC_Compress_D.c
//  GLTC
//
//  Created by Michael Kwasnicki on 04.04.13.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//	This content is released under the MIT License.
//

#include "ETC_Compress_D.h"

#include "ETC_Compress_Common.h"
#include "ETC_Decompress.h"

#include <stdio.h>



static ETCUniformColorComposition_t ETC_UNIFORM_COLOR_LUT[ETC_TABLE_COUNT][ETC_PALETTE_SIZE][256];
static ETCUniformColorComposition_t ETC_UNIFORM_COLOR_LUT_NON_OPAQUE[ETC_TABLE_COUNT][ETC_PALETTE_SIZE][256];



static void buildBlock( ETCBlockColor_t * out_block, const rgb5_t in_C, const rgb3_t in_DC, const int in_T0, const int in_T1, const bool in_FLIP, const uint8_t in_MODULATION[4][4] ) {
	uint32_t bitField = generateBitField( in_MODULATION );
	
	ETCBlockD_t block;
	block.differential = 1;
	block.r = in_C.r;
	block.g = in_C.g;
	block.b = in_C.b;
	block.dR = in_DC.r;
	block.dG = in_DC.g;
	block.dB = in_DC.b;
	block.table0 = in_T0;
	block.table1 = in_T1;
	block.flip = in_FLIP;
	block.cBitField = bitField;
	out_block->b64 = block.b64;
}



static uint32_t uniformColor( rgb5_t * out_c, int * out_t, uint8_t out_modulation[2][4], const rgb8_t in_SUB_BLOCK_RGB[2][4], const bool in_OPAQUE ) {
	rgb8_t col8, palette[4];
	rgb5_t col5 = { 0, 0, 0 };
	uint32_t error = 0;
	uint32_t bestError = 0xFFFFFFFF;
	int bestT = 0;
	rgb5_t bestC = { 0, 0, 0 };
	ETCUniformColorComposition_t (* lut)[ETC_PALETTE_SIZE][256] = ( in_OPAQUE ) ? &ETC_UNIFORM_COLOR_LUT[0] : &ETC_UNIFORM_COLOR_LUT_NON_OPAQUE[0];
	
	computeSubBlockCenter( &col8, in_SUB_BLOCK_RGB );
	
	for ( int t = 0; t < ETC_TABLE_COUNT; t++ ) {
		for ( int p = 0; p < ETC_PALETTE_SIZE; p++ ) {
			error  = lut[t][p][col8.r].error;
			error += lut[t][p][col8.g].error;
			error += lut[t][p][col8.b].error;
			col5.r = lut[t][p][col8.r].c;
			col5.g = lut[t][p][col8.g].c;
			col5.b = lut[t][p][col8.b].c;
			
			if ( error < bestError ) {
				bestError = error;
				bestC = col5;
				bestT = t;
			}
			
			if ( bestError == 0 )
				goto earlyExit;
		}
	}
	
earlyExit:
	
	*out_c = bestC;
	*out_t = bestT;
	
	convert555to888( &col8, bestC );
	computeRGBColorPaletteCommonID( palette, col8, bestT, ETC_MODIFIER_TABLE );
	return computeSubBlockError( out_modulation, in_SUB_BLOCK_RGB, palette );
}



static uint32_t quick( rgb5_t * out_c, int * out_t, uint8_t out_modulation[2][4], const rgb8_t in_SUB_BLOCK_RGB[2][4] ) {
	rgb5_t col5;
	rgb8_t center, palette[4];
	int t;
	computeSubBlockMedian( &center, in_SUB_BLOCK_RGB );
	computeSubBlockWidth( &t, in_SUB_BLOCK_RGB );
	convert888to555( &col5, center );
	convert555to888( &center, col5 );
	computeRGBColorPaletteCommonID( palette, center, t, ETC_MODIFIER_TABLE );
	
	*out_c = col5;
	*out_t = t;
	
	return computeSubBlockError( out_modulation, &in_SUB_BLOCK_RGB[0], palette );
}


/*
static uint32_t bruteSubBlock( rgb5_t * out_c, int * out_t, uint8_t out_modulation[2][4], const rgb8_t in_SUB_BLOCK_RGB[2][4] ) {
	rgb8_t palette[4];
	rgb8_t center;
	rgb8_t col8;
	rgb5_t col5, bestC;
	uint32_t error = 0xFFFFFFFF;
	uint32_t bestError = 0xFFFFFFFF;
	uint32_t bestDistance = 0xFFFFFFFF;
	int distance, dR, dG, dB;
	int t, bestT;
	int r, g, b;
	
	computeSubBlockCenter( &center, &in_SUB_BLOCK_RGB[0] );
	
	for ( t = 0; t < ETC_TABLE_COUNT; t++ ) {
		for ( b = 0; b < 32; b++ ) {
			for ( g = 0; g < 32; g++ ) {
				for ( r = 0; r < 32; r++ ) {
					col5 = (rgb5_t){ b, g, r };
					convert555to888( &col8, col5 );
					computeRGBColorPaletteCommonID( palette, col8, t, ETC_MODIFIER_TABLE );
					error = computeSubBlockError( NULL, &in_SUB_BLOCK_RGB[0], palette );
					dR = center.r - col8.r;
					dG = center.g - col8.g;
					dB = center.b - col8.b;
					distance = dR * dR + dG * dG + dB * dB;
					
					if ( ( error < bestError ) or ( ( error == bestError ) and ( distance < bestDistance ) ) ) {
						bestC = col5;
						bestT = t;
						bestDistance = distance;
						bestError = error;
					}
					
					if ( ( bestError == 0 ) and ( bestDistance == 0) )
						goto earlyExit;
				}
			}
		}
	}
	
earlyExit:
	
	*out_c = bestC;
	*out_t = bestT;
	
	convert555to888( &col8, bestC );
	computeRGBColorPaletteCommonID( palette, col8, bestT, ETC_MODIFIER_TABLE );
	return computeSubBlockError( out_modulation, &in_SUB_BLOCK_RGB[0], palette );
}
*/


static uint32_t bruteBlock( rgb5_t * out_c0, rgb5_t * out_c1, int * out_t0, int * out_t1, uint8_t out_modulation[4][4], const rgb8_t in_BLOCK_RGB[4][4] ) {
    rgb8_t palette[4];
	rgb8_t center[2];
	rgb8_t col8;
	rgb5_t col5, bestC0, bestC1;
	uint32_t blockError = 0xFFFFFFFF;
	uint32_t error[2] = { 0xFFFFFFFF, 0xFFFFFFFF };
	uint32_t bestError = 0xFFFFFFFF;
	uint32_t bestDistance = 0xFFFFFFFF;
	int distance[2], dR, dG, dB;
	int t0, t1, bestT0, bestT1;
	int r0, g0, b0, r1, g1, b1;
    uint32_t errors[8][32][32][32];
	
    for ( t1 = 0; t1 < ETC_TABLE_COUNT; t1++ ) {
		for ( b1 = 0; b1 < 32; b1++ ) {
			for ( g1 = 0; g1 < 32; g1++ ) {
				for ( r1 = 0; r1 < 32; r1++ ) {
					col5 = (rgb5_t){ b1, g1, r1 };
					convert555to888( &col8, col5 );
					computeRGBColorPaletteCommonID( palette, col8, t1, ETC_MODIFIER_TABLE );
					errors[t1][b1][g1][r1] = computeSubBlockError( NULL, &in_BLOCK_RGB[2], palette );
				}
			}
		}
	}
    
	bestError = 0xFFFFFFFF;
	computeSubBlockCenter( &center[0], &in_BLOCK_RGB[0] );
	computeSubBlockCenter( &center[1], &in_BLOCK_RGB[2] );
	
	for ( t0 = 0; t0 < ETC_TABLE_COUNT; t0++ ) {
		for ( b0 = 0; b0 < 32; b0++ ) {
			for ( g0 = 0; g0 < 32; g0++ ) {
				for ( r0 = 0; r0 < 32; r0++ ) {
					col5 = (rgb5_t){ b0, g0, r0 };
					convert555to888( &col8, col5 );
					computeRGBColorPaletteCommonID( palette, col8, t0, ETC_MODIFIER_TABLE );
					error[0] = computeSubBlockError( NULL, &in_BLOCK_RGB[0], palette );
					dR = center[0].r - col8.r;
					dG = center[0].g - col8.g;
					dB = center[0].b - col8.b;
					distance[0] = dR * dR + dG * dG + dB * dB;
					
					if ( error[0] > bestError )
						continue;
					
					for ( t1 = 0; t1 < ETC_TABLE_COUNT; t1++ ) {
						for ( b1 = -4; b1 < 4; b1++ ) {
                            if ( b0 + b1 < 0 or b0 + b1 >= 32 ) continue;
                            
							for ( g1 = -4; g1 < 4; g1++ ) {
                                if ( g0 + g1 < 0 or g0 + g1 >= 32 ) continue;
                                
								for ( r1 = -4; r1 < 4; r1++ ) {
									if ( r0 + r1 < 0 or r0 + r1 >= 32 ) continue;
									
									error[1] = error[0] + errors[t1][b0 + b1][g0 + g1][r0 + r1];
									dR = center[1].r - col8.r;
									dG = center[1].g - col8.g;
									dB = center[1].b - col8.b;
									distance[1] = distance[0] + dR * dR + dG * dG + dB * dB;
									
									if ( ( error[1] < bestError ) or ( ( error[1] == bestError ) and ( distance[1] < bestDistance ) ) ) {
										bestC0.r = r0;
										bestC0.g = g0;
										bestC0.b = b0;
										bestT0 = t0;
										bestC1.r = r0 + r1;
										bestC1.g = g0 + g1;
										bestC1.b = b0 + b1;
										bestT1 = t1;
										bestDistance = distance[1];
										bestError = error[1];
									}
									
									if ( ( bestError == 0 ) and ( bestDistance == 0 ) )
										goto earlyExit;
								}
							}
						}
					}
				}
			}
		}
	}
    
earlyExit:
	
	col5.r = bestC0.r;
	col5.g = bestC0.g;
	col5.b = bestC0.b;
	t0 = bestT0;
	convert555to888( &col8, col5 );
	computeRGBColorPaletteCommonID( palette, col8, t0, ETC_MODIFIER_TABLE );
	blockError  = computeSubBlockError( &out_modulation[0], &in_BLOCK_RGB[0], palette );
	
	col5.r = bestC1.r;
	col5.g = bestC1.g;
	col5.b = bestC1.b;
	t1 = bestT1;
	convert555to888( &col8, col5 );
	computeRGBColorPaletteCommonID( palette, col8, t1, ETC_MODIFIER_TABLE );
	blockError += computeSubBlockError( &out_modulation[2], &in_BLOCK_RGB[2], palette );
	
	*out_c0 = bestC0;
	*out_c1 = bestC1;
	*out_t0 = bestT0;
	*out_t1 = bestT1;
	return blockError;
}



#pragma mark - exposed interface



uint32_t compressD( ETCBlockColor_t * out_block, const rgb8_t in_BLOCK_RGB[4][4], const Strategy_t in_STRATEGY, const bool in_OPAQUE ) {
	rgb5_t c[2], bestC0;
	rgb3_t bestD1;
	int t[2], bestT[2], bestFlip;
	rgb8_t blockRGB[4][4];
	uint8_t modulation[4][4];
	uint8_t outModulation[4][4];
	uint32_t blockError = 0xFFFFFFFF;
	uint32_t bestBlockError = 0xFFFFFFFF;
    uint32_t subBlockError = 0xFFFFFFFF;
	
    if ( isUniformColorBlock( in_BLOCK_RGB ) ) {
        bestBlockError  = uniformColor( &bestC0, &bestT[0], &outModulation[0], &in_BLOCK_RGB[0], in_OPAQUE );
        bestBlockError += uniformColor( &bestC0, &bestT[1], &outModulation[2], &in_BLOCK_RGB[2], in_OPAQUE );
        bestD1 = (rgb3_t){ 0, 0, 0 };
        bestFlip = 0;
    } else {
        
        //  flip          no flip
        // +---------+   +-----+-----+
        // | a e i m |   | a e | i m |
        // | b f j n |   | b f | j n |
        // +---------+   | c g | k o |
        // | c g k o |   | d h | l p |
        // | d h l p |   +-----+-----+
        // +---------+
        
        for ( int flip = 0; flip < 2; flip++ ) {
            for ( int by = 0; by < 4; by++ ) {
                for ( int bx = 0; bx < 4; bx++ ) {
                    blockRGB[by][bx] = ( flip ) ? in_BLOCK_RGB[by][bx] : in_BLOCK_RGB[bx][by];
                }
            }
            
            blockError = 0;
            
            if ( in_STRATEGY == kBEST ) {
                blockError = bruteBlock( &c[0], &c[1], &t[0], &t[1], modulation, blockRGB );
            } else {
                for ( int sb = 0; sb < 2; sb++ ) {
                    if ( isUniformColorSubBlock( &blockRGB[sb * 2] ) ) {
                        blockError += uniformColor( &c[sb], &t[sb], &modulation[sb * 2], (const rgb8_t(*)[4])&blockRGB[sb * 2], in_OPAQUE );
                    } else {
                        subBlockError = 0xFFFFFFFF;
                        
                        switch ( in_STRATEGY ) {
							case kFAST:
                                subBlockError = quick( &c[sb], &t[sb], &modulation[sb * 2], (const rgb8_t(*)[4])&blockRGB[sb * 2] );
                                break;
								
							case kBEST:
								break;
                        }

                        blockError += subBlockError;
                    }
                }
            }
            
			int d[3];
            d[0] = c[1].r - c[0].r;
            d[1] = c[1].g - c[0].g;
            d[2] = c[1].b - c[0].b;
            
            if ( ( d[0] < -4 ) or ( d[0] > 3 ) )
                blockError = 0xFFFFFFFF;
            
            if ( ( d[1] < -4 ) or ( d[1] > 3 ) )
                blockError = 0xFFFFFFFF;
            
            if ( ( d[2] < -4 ) or ( d[2] > 3 ) )
                blockError = 0xFFFFFFFF;
            
//            if ( blockError == 0xFFFFFFFF ) {
//                puts( "fail" );
//            } else {
//                puts( "win" );
//            }
            
            if ( blockError < bestBlockError ) {
                bestBlockError = blockError;
                bestC0 = c[0];
                bestD1 = (rgb3_t){ d[2], d[1], d[0] };
                bestT[0] = t[0];
                bestT[1] = t[1];
                bestFlip = flip;
                
                for ( int by = 0; by < 4; by++ ) {
                    for ( int bx = 0; bx < 4; bx++ ) {
                        outModulation[by][bx] = ( flip ) ? modulation[by][bx] : modulation[bx][by];
                    }
                }
            }
        }
	}
	
	if ( bestBlockError < 0xFFFFFFFF )
		buildBlock( out_block, bestC0, bestD1, bestT[0], bestT[1], bestFlip, (const uint8_t(*)[4])outModulation );
	else
		out_block->b64 = 0x0ULL;
	
	return bestBlockError;
}



void computeUniformColorLUTD() {
	ETCUniformColorComposition_t tmp = { 0, 65535 };
	
	// set everything to undefined
	for ( int t = 0; t < ETC_TABLE_COUNT; t++ ) {
		for ( int p = 0; p < ETC_PALETTE_SIZE; p++ ) {
			for ( int c = 0; c < 256; c++ ) {
				ETC_UNIFORM_COLOR_LUT[t][p][c] = tmp;
				ETC_UNIFORM_COLOR_LUT_NON_OPAQUE[t][p][c] = tmp;
			}
		}
	}
	
	// compute all colors that can be constructed with 5bpp
	for ( int col5 = 0; col5 < 32; col5++ ) {
		int col8 = extend5to8bits( col5 );
		
		for ( int t = 0; t < ETC_TABLE_COUNT; t++ ) {
			for ( int p = 0; p < ETC_PALETTE_SIZE; p++ ) {
				int col = clampi( col8 + ETC_MODIFIER_TABLE[t][p], 0, 255 );
				ETC_UNIFORM_COLOR_LUT[t][p][col] = (ETCUniformColorComposition_t){ col5, 0 };
				
				if ( p != 2 ) {
					col = clampi( col8 + ETC_MODIFIER_TABLE_NON_OPAQUE[t][p], 0, 255 );
					ETC_UNIFORM_COLOR_LUT_NON_OPAQUE[t][p][col] = (ETCUniformColorComposition_t){ col5, 0 };
				}
			}
		}
	}
	
	// fill the gaps with the nearest color
	fillUniformColorPaletteLUTGaps( ETC_UNIFORM_COLOR_LUT );
	fillUniformColorPaletteLUTGaps( ETC_UNIFORM_COLOR_LUT_NON_OPAQUE );
}



void printInfoD( ETCBlockColor_t * in_BLOCK ) {
	ETCBlockD_t * block = (ETCBlockD_t *)in_BLOCK;
	
	printf( "D: (%3i %3i %3i) (%3i %3i %3i) %i %i : %i\n", block->r, block->g, block->b, block->dR, block->dG, block->dB, block->table0, block->table1, block->flip );
	printf( "D: (%3i %3i %3i) (%3i %3i %3i)\n", extend5to8bits( block->r ), extend5to8bits( block->g ), extend5to8bits( block->b ), extend5to8bits( block->r + block->dR ), extend5to8bits( block->g + block->dG ), extend5to8bits( block->b + block->dB ) );
}
