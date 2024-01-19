#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>




static uint8_t _to_str(uint16_t number, char *str){
	char result[] = "00000\0";
	char *ptr = &result[4];
	uint8_t i = 0;
	do {
		*ptr += number % 10;
		number /= 10;
		ptr--;
	} while (number > 0);

	do{
		ptr++;	// its ok
		*str = *ptr;
		str++;
		i++;
	} while(*ptr);
	return i-1;	// No cuento el \0
}


int main(){
	uint8_t dst_ipaddr[4] = {192, 168, 100, 34};
	char ch_dst_ipaddr[16];
	uint8_t x = 0;
	for(uint8_t i = 0; i < 4; i++){
		x += _to_str(dst_ipaddr[i], &ch_dst_ipaddr[x]);
		ch_dst_ipaddr[x++] = '.';
	}
	ch_dst_ipaddr[x-1] = '\0';


	printf("%s", ch_dst_ipaddr);

	return 0;
}
