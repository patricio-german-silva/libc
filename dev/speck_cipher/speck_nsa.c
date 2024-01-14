#include "speck_nsa.h"


void Speck64128KeySchedule(uint32_t K[],uint32_t rk[])
{
  uint32_t D=K[3],C=K[2],B=K[1],A=K[0];
  for(uint8_t i = 0 ; i < 27 ;){
   rk[i]=A; (B=(((B)>>(8)) | ((B)<<(32-(8)))), B+=A, B^=i++, A=(((A)<<(3)) | (A>>(32-(3)))), A^=B);
   rk[i]=A; (C=(((C)>>(8)) | ((C)<<(32-(8)))), C+=A, C^=i++, A=(((A)<<(3)) | (A>>(32-(3)))), A^=C);
   rk[i]=A; (D=(((D)>>(8)) | ((D)<<(32-(8)))), D+=A, D^=i++, A=(((A)<<(3)) | (A>>(32-(3)))), A^=D);
  }
}



void Speck64128Encrypt(uint32_t Pt[],uint32_t Ct[],uint32_t rk[])
{
  uint32_t i;
  Ct[0]=Pt[0]; Ct[1]=Pt[1];
  for(i=0;i<27;)
    (Ct[1]=(((Ct[1])>>(8)) | ((Ct[1])<<(32-(8)))), Ct[1]+=Ct[0], Ct[1]^=rk[i++], Ct[0]=(((Ct[0])<<(3)) | (Ct[0]>>(32-(3)))), Ct[0]^=Ct[1]);
}


void Speck64128Decrypt(uint32_t Pt[],uint32_t Ct[],uint32_t rk[])
{
  int i;
  Pt[0]=Ct[0]; Pt[1]=Ct[1];
  for(i=26;i>=0;)
    (Pt[0]^=Pt[1], Pt[0]=(((Pt[0])>>(3)) | ((Pt[0])<<(32-(3)))), Pt[1]^=rk[i--], Pt[1]-=Pt[0], Pt[1]=(((Pt[1])<<(8)) | (Pt[1]>>(32-(8)))));
}
