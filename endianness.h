//
//  endianness.h
//  GLTC
//
//  Created by Michael Kwasnicki on 12.07.15.
//
//

#ifndef __GLTC__endianness__
#define __GLTC__endianness__

#include <stdint.h>


uint64_t endianness_switch_64( uint64_t const in_DATA );
uint32_t endianness_switch_32( uint32_t const in_DATA );
uint16_t endianness_switch_16( uint16_t const in_DATA );


/**
 * Convert Host to Big/Little Endian and vice versa
 */

#ifdef __LITTLE_ENDIAN__
#warning Compiling on Little Endian Architecture
#define htole64( X )
#define htole32( X )
#define htole16( X )
#define letoh64( X )
#define letoh32( X )
#define letoh16( X )
#define htobe64( X ) X = endianness_switch_64( X )
#define htobe32( X ) X = endianness_switch_32( X )
#define htobe16( X ) X = endianness_switch_16( X )
#define betoh64( X ) X = endianness_switch_64( X )
#define betoh32( X ) X = endianness_switch_32( X )
#define betoh16( X ) X = endianness_switch_16( X )
#endif


#ifdef __BIG_ENDIAN__
#warning Compiling on Big Endian Architecture
#define htole64( X ) X = endianness_switch_64( X )
#define htole32( X ) X = endianness_switch_32( X )
#define htole16( X ) X = endianness_switch_16( X )
#define letoh64( X ) X = endianness_switch_64( X )
#define letoh32( X ) X = endianness_switch_32( X )
#define letoh16( X ) X = endianness_switch_16( X )
#define htobe64( X )
#define htobe32( X )
#define htobe16( X )
#define betoh64( X )
#define betoh32( X )
#define betoh16( X )
#endif


#endif /* defined(__GLTC__endianness__) */
