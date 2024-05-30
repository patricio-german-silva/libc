#include <stdint.h>
#include <stdio.h>

static const int32_t powers_of_10[] = {
  1, // 10^0
  10,
  100,
  1000,
  10000,
  100000,
  1000000,
  10000000,
  100000000,
  1000000000
}; // 10^9
void sseg_write_data(int32_t n){
	for(uint8_t j =3-1 ; j>=0; --j)
		printf("%d\n", (n/(powers_of_10[j]))%10);
}

void main(){
	sseg_write_data(589);
}
