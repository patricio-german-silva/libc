#include <stdint.h>
#include <stdio.h>
#include "../lib/strutil.h"



int main() {
	char c[9];
	uint32_t num = 12345678;
	_uint32_to_hex(&num, c);
	_touppercase(c, c, 100);
	_tolowercase(c, c, 100);
	printf("HEX %s; INT %u\n", c, num);
}
