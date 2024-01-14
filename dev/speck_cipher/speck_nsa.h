#include <stdint.h>

void Speck64128KeySchedule(uint32_t K[], uint32_t rk[]);
void Speck64128Encrypt(uint32_t Pt[], uint32_t Ct[], uint32_t rk[]);
void Speck64128Decrypt(uint32_t Pt[], uint32_t Ct[], uint32_t rk[]);
