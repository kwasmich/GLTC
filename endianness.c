//
//  endianness.c
//  GLTC
//
//  Created by Michael Kwasnicki on 12.07.15.
//
//

#include "endianness.h"



uint64_t endianness_switch_64(uint64_t const in_DATA) {
    uint64_t out_data;
    char *p_in = (char *) &in_DATA;
    char *p_out = (char *) &out_data;
    p_out[0] = p_in[7];
    p_out[1] = p_in[6];
    p_out[2] = p_in[5];
    p_out[3] = p_in[4];
    p_out[4] = p_in[3];
    p_out[5] = p_in[2];
    p_out[6] = p_in[1];
    p_out[7] = p_in[0];
    return out_data;
}



uint32_t endianness_switch_32(uint32_t const in_DATA) {
    uint32_t out_data;
    char *p_in = (char *) &in_DATA;
    char *p_out = (char *) &out_data;
    p_out[0] = p_in[3];
    p_out[1] = p_in[2];
    p_out[2] = p_in[1];
    p_out[3] = p_in[0];
    return out_data;
}



uint16_t endianness_switch_16(uint16_t const in_DATA) {
    uint16_t out_data;
    char *p_in = (char *) &in_DATA;
    char *p_out = (char *) &out_data;
    p_out[0] = p_in[1];
    p_out[1] = p_in[0];
    return out_data;
}
