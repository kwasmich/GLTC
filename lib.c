//
//  lib.c
//  GLTC
//
//  Created by Michael Kwasnicki on 29.02.12.
//  Copyright (c) 2014 Michael Kwasnicki. All rights reserved.
//  This content is released under the MIT License.
//

#include "lib.h"


#include <stdlib.h>
#include <string.h>
//#include <glob.h>
//#include <wordexp.h> //wordexp (globbing of ~)

#warning this is a potential memoryleak
char * expandTilde( const char in_PATH[] ) {
	char * expanded = calloc( 1024, sizeof( char ) );
	
	if ( in_PATH[0] == '~' ) {
		strncat( expanded, getenv( "HOME" ), 1024 );
		strncat( expanded, &in_PATH[1], 1024 );
	} else {
		strncat( expanded, &in_PATH[0], 1024 );
	}
	
	return expanded;
}
