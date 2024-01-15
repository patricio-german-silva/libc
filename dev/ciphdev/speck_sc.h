/**
 * Speck Stream Cipher implementation
 *
 * This module incorporates a stream cipher based on the speck block cipher and
 * Cipher block chaining (CBC) mode of operation. The incialization vector must
 * be provided in the initialization proccess.
 *
 * At the risk of being pedantic, it is clear that you cannot mix encryption
 * and decryption in the same initialization.
 *
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
 * @date Jan 15, 2024
 * @version 0.1
 */

#ifndef SPECK_SC_H
#define SPECK_SC_H

#include "stdint.h"


// Stream data struct
typedef struct{
    uint32_t rk[27];
    uint32_t iv[2];
} _speck_sc;


/*
 * Initializes a speck context for stream encription
 * given a 128-bit key, extend it for 27 rounds
 */
void speck_sc_init(_speck_sc *s, const uint32_t K[4], const uint32_t I[2]);


/*
 * encrypt a 64 bit block of a stream with an expanded key
 */
void speck_sc_encrypt(_speck_sc *s, uint32_t const pt[2], uint32_t ct[2]);


/*
 * decrypt a 64 bit block of a stream with an expanded key
 */
void speck_sc_decrypt(_speck_sc *s, uint32_t const ct[2], uint32_t pt[2]);


#endif // SPECK_SC_H
