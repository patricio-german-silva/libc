/**
 * SPECK is a lightweight block cipher publicly released by the National
 * Security Agency (NSA) in June 2013.
 * Speck has been optimized for performance in software implementations
 *
 * This implementation is the 64 bit block, 128 bit key version.
 *
 * Doc: https://eprint.iacr.org/2013/404.pdf
 * https://nsacyber.github.io/simon-speck/implementations/ImplementationGuide1.1.pdf
 *
 * @author Patricio Silva
 * @date May 21, 2023
 * @version 0.1
 */

#ifndef SPECK_H
#define SPECK_H

#include "stdint.h"


typedef struct{
    uint32_t rk[27];
} _speck;


/*
 * Initializes a spec context
 * given a 128-bit key, extend it for 27 rounds
 */
void speck_init(_speck *s, const uint32_t K[4]);


/*
 * encrypt a 64 bit block with an expanded key
 */
void speck_encrypt(_speck *s, uint32_t const pt[2], uint32_t ct[2]);


/*
 * decrypt a 64 bit block with an expanded key
 */
void speck_decrypt(_speck *s, uint32_t const ct[2], uint32_t pt[2]);


#endif // SPECK_H
