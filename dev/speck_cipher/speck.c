#include "speck.h"


/*
 * Initializes a spec context
 * given a 128-bit key, extend it for 27 rounds
 */
void speck_init(_speck *s, const uint32_t K[4]){
	uint32_t D=K[3];
	uint32_t C=K[2];
	uint32_t B=K[1];
	uint32_t A=K[0];
	for(uint8_t i = 0 ; i < 27 ;){
		s->rk[i]=A;
		B=(B>>8 | B<<24);
		B+=A;
		B^=i++;
		A=(A<<3 | A>>29);
		A^=B;

		s->rk[i]=A;
		C=(C>>8 | C<<24);
		C+=A;
		C^=i++;
		A=(A<<3 | A>>29);
		A^=C;

		s->rk[i]=A;
		D=(D>>8 | D<<24);
		D+=A;
		D^=i++;
		A=(A<<3 | A>>29);
		A^=D;
	}
}



/*
 * encrypt a 64 bit block with an expanded key
 */
void speck_encrypt(_speck *s, uint32_t const pt[2], uint32_t ct[2]){
	ct[0]=pt[0];
	ct[1]=pt[1];
	for(uint8_t i = 0; i < 27;){
		ct[1]=(ct[1]>>8 | ct[1]<<24);
		ct[1]+=ct[0];
		ct[1]^=s->rk[i++];
		ct[0]=(ct[0]<<3 | ct[0]>>29);
		ct[0]^=ct[1];
	}
}



/*
 * decrypt a 64 bit block with an expanded key
 */
void speck_decrypt(_speck *s, uint32_t const ct[2], uint32_t pt[2]){
	pt[0]=ct[0];
	pt[1]=ct[1];
	for(int8_t i = 26 ; i >= 0;){
		pt[0]^=pt[1];
		pt[0]=(pt[0]>>3 | pt[0]<<29);
		pt[1]^=s->rk[i--];
		pt[1]-=pt[0];
		pt[1]=(pt[1]<<8) | (pt[1]>>24);
	}
}
