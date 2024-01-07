#include <stdint.h>
#include <stdio.h>
static const uint16_t CNY70_LOOKUP_TABLE[] = {
        2500, 2500, 1385, 1065,  855,  735,  665,  620,  575,  545,
         510,  485,  465,  445,  425,  415,  400,  385,  375,  365,
         355,  345,  335,  330,  325,  320,  315,  310,  305,  300,
         295,  285,  280,  275,  275,  270,  260,  250,  180,  150
};

uint16_t _get_cny70_distance(uint16_t x){
	if(x >= 3900) return CNY70_LOOKUP_TABLE[39];
	uint16_t x0 = x/100;
	uint16_t x1 = x0 + 1;
	uint16_t y0 = CNY70_LOOKUP_TABLE[x0];
	uint16_t y1 = CNY70_LOOKUP_TABLE[x1];
	uint32_t tmp = ((y0 - y1) * (x % 100));
	tmp /= 100;
	return y0 - tmp;
}

int main(){
	uint16_t x = 210;
    printf("%d: %d" , x, _get_cny70_distance(x));
    return 0;
}
