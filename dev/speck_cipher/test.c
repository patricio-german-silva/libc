#include "stdio.h"
#include "speck.h"


int main(){
   uint32_t K[] = {0, 0, 0, 0};
   uint32_t pt[] = {0, 1};
   uint32_t ct[2];
   uint32_t nt[2];
   _speck s;
   speck_init(&s, K);
   printf("ORIGINAL: %d %d\n" , pt[0], pt[1]);
   speck_encrypt(&s, pt, ct);
   printf("CIPHED: %d %d\n" , ct[0], ct[1]);
   speck_decrypt(&s, ct, nt);
   printf("DECIPHED: %d %d\n" , nt[0], nt[1]);
   return 0;
}
