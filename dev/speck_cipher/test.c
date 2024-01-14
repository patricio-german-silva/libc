#include "stdio.h"
//#include "speck.h"
#include "speck_nsa.h"


typedef union{
  uint8_t u8[16];
  uint32_t u32[4];
} _k;

typedef union{
  uint8_t u8[8];
  uint32_t u32[2];
} _t;

int main(){
   _k K = {0x00 , 0x01 , 0x02 , 0x03 , 0x08 , 0x09 , 0x0a , 0x0b , 0x10 , 0x11 , 0x12 , 0x13 , 0x18 , 0x19 , 0x1a , 0x1b};
   _t pt = {0x2d , 0x43 , 0x75 , 0x74 , 0x74 , 0x65 , 0x72 , 0x3b};
   _t ct;
   _t nt;
   //_speck s;
    uint32_t rk[27];


   Speck64128KeySchedule(K.u32, rk);
   for(uint8_t i = 0; i< 27; i++) printf("rk[%d] = %08x\n", i, rk[i]);
   for(uint8_t i = 0; i< 16; i++) printf("K[%d] = %02x\n", i, K.u8[i]);
   printf("ORIGINAL: %02x %02x %02x %02x %02x %02x %02x %02x\n" , pt.u8[0], pt.u8[1], pt.u8[2], pt.u8[3], pt.u8[4], pt.u8[5], pt.u8[6], pt.u8[7]);
   Speck64128Encrypt(pt.u32, ct.u32, rk);
   printf("CIPHED: %02x %02x %02x %02x %02x %02x %02x %02x\n" , ct.u8[0], ct.u8[1], ct.u8[2], ct.u8[3], ct.u8[4], ct.u8[5], ct.u8[6], ct.u8[7]);
   Speck64128Decrypt(nt.u32, ct.u32, rk);
   printf("DECIPHED: %02x %02x %02x %02x %02x %02x %02x %02x\n" , nt.u8[0], nt.u8[1], nt.u8[2], nt.u8[3], nt.u8[4], nt.u8[5], nt.u8[6], nt.u8[7]);
   return 0;




/*

   speck_init(&s, K.u32);
   for(uint8_t i = 0; i< 27; i++) printf("rk[%d] = %08x\n", i, s.kb[i]);
   for(uint8_t i = 0; i< 16; i++) printf("K[%d] = %02x\n", i, K.u8[i]);

   printf("ORIGINAL: %02x %02x %02x %02x %02x %02x %02x %02x\n" , pt.u8[0], pt.u8[1], pt.u8[2], pt.u8[3], pt.u8[4], pt.u8[5], pt.u8[6], pt.u8[7]);
   speck_encrypt(&s, pt.u32, ct.u32);
   printf("CIPHED: %02x %02x %02x %02x %02x %02x %02x %02x\n" , ct.u8[0], ct.u8[1], ct.u8[2], ct.u8[3], ct.u8[4], ct.u8[5], ct.u8[6], ct.u8[7]);
   speck_decrypt(&s, ct.u32, nt.u32);
   printf("DECIPHED: %02x %02x %02x %02x %02x %02x %02x %02x\n" , nt.u8[0], nt.u8[1], nt.u8[2], nt.u8[3], nt.u8[4], nt.u8[5], nt.u8[6], nt.u8[7]);
   return 0;
   */
}
