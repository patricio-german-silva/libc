/*
 * MD5 Message-Digest Algorithm
 */

#ifndef MD5_H
#define MD5_H

#include "stdint.h"


typedef struct{
	uint32_t size[2];
	uint32_t buffer[4];
	uint8_t input[64];
	uint8_t digest[16];
}_md5_context;

void md5_init(_md5_context *ctx);
void md5_update(_md5_context *ctx, const uint8_t *input, uint32_t input_len);
void md5_finalize(_md5_context *ctx);

#endif // MD5_H
