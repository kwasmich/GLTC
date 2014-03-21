//
//  lib.h
//  GLTC
//
//  Created by Michael Kwasnicki on 29.02.12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef GLTC_lib_h
#define GLTC_lib_h

#define REINTERPRET(x) *(x *)&
#define free_s(x) free(x);x = NULL

static inline int clampi( const int in_VALUE, const int in_MIN, const int in_MAX ) {
	int result = ( in_VALUE < in_MIN ) ? in_MIN : in_VALUE;
    return ( result > in_MAX ) ? in_MAX : result;
}


char * expandTilde( const char in_PATH[] );

#endif
