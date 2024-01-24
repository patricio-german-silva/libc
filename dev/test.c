#include <stdint.h>
#include <stdio.h>


uint8_t _hex_to_uint32(const char *str, uint32_t *number){
	uint8_t hl[8];
	for (uint8_t i = 0; i < 8; i++) {
		if(str[i] >= '0' && str[i]<='9')
			hl[i] = str[i] - '0';
		else if(str[i] >= 'a' && str[i]<='f')
			hl[i] = str[i] - 'a' + 10;
		else if(str[i] >= 'A' && str[i]<='F')
			hl[i] = str[i] - 'A' + 10;
		else
			return 0; // Conversion failed
	}
	*number = (hl[0]<<28) | (hl[1]<<24) | (hl[2]<<20) | (hl[3]<<16) | (hl[4]<<12) | (hl[5]<<8) | (hl[6]<<4) | hl[7];
	return 1;
}

void _uint32_to_hex(const uint32_t *number, char *str){
	uint8_t value;
	for (uint8_t i = 0; i < 8; ++i) {
		value = ((*number<<(i*4))>>28);
		if(value < 10)
			str[i] = value + '0';
		else
			str[i] = value + 'a' - 10;
	}
}

int main() {
	char c[9];
	uint32_t num = 12345678;
	_uint32_to_hex(&num, c);
	printf("HEX %s; INT %u\n", c, num);
}
