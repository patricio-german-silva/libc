#include "speck.h"


/*
 * Initializes a spec context
 * given a 128-bit key, extend it for 27 rounds
 */
void speck_init(_speck *s, uint32_t K[4]){
   uint32_t i, a, b;

   a = K[0];

   for (i = 0; i<27; i++) {
      s->kb[i] = a;
      b = K[(i % 3)+1];
      b = ((b >> 8) | (b << (32 - 8)));
      b += a;
      b ^= i;
      a = ((a << 3) | (a >> (32 - 3)));
      a ^= b;
   }
}




/*
 * encrypt a 64 bit block with an expanded key
 */
void speck_encrypt(_speck *s, uint32_t const pt[2], uint32_t ct[2]){
   uint32_t y = pt[0], x = pt[1], i;

   for (i = 0; i < 27; i++) {
      x = ((x >> 8) | (x << (32 - 8)));
      x += y;
      x ^= s->kb[i];
      y = ((y << 3) | (y >> (32 - 3)));
      y ^= x;
   }

   ct[0] = y;
   ct[1] = x;
}




/*
 * decrypt a 64 bit block with an expanded key
 */
void speck_decrypt(_speck *s, uint32_t const ct[2], uint32_t pt[2]){
   uint32_t y = ct[0], x = ct[1];
   int i;

   for (i = 26; i >= 0; i--) {
      y ^= x;
      y = ((y >> 3) | (y << (32 - 3)));
      x ^= s->kb[i];
      x -= y;
      x = ((x << 8) | (x >> (32 - 8)));
   }

   pt[0] = y;
   pt[1] = x;
}
