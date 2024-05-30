/*
 * seven_segments.c
 *
 *  Created on: May 29, 2024
 *      Author: psilva
 */
#include "seven_segments.h"

void sseg_set_segments(_sseg *s, uint32_t *port, uint32_t *pin){
	for(uint8_t j = 0; j<8; ++j){
		s->segments[j][0] = port[j];
		s->segments[j][1] = pin[j];
	}
}
void sseg_set_digits(_sseg *s, uint32_t *port, uint32_t *pin){
	for(uint8_t j = 0; j<NUMBER_OF_DIGITS; ++j){
		s->digits[j][0] = port[j];
		s->digits[j][1] = pin[j];
	}
}


void sseg_set_info_label(_sseg *s, char *d, uint8_t i){
	for(uint8_t j = 0; j<NUMBER_OF_DIGITS; ++j)
		s->info_labels[i][j] = d[j];
}

// Change funtions to change behabior, dirty but if works it works
void sseg_attach_gpio_high(_sseg *s, sseg_set_gpio_def f){
#ifdef DISPLAY_COMMON_ANODE
	s->sseg_sgpio_high = f;
#endif
#ifdef DISPLAY_COMMON_CATODE
	s->sseg_sgpio_low = f;
#endif
}

void sseg_attach_gpio_low(_sseg *s, sseg_set_gpio_def f){
#ifdef DISPLAY_COMMON_ANODE
	s->sseg_sgpio_low = f;
#endif
#ifdef DISPLAY_COMMON_CATODE
	s->sseg_sgpio_high = f;
#endif
}

void sseg_write_data(_sseg *s, int32_t n, int8_t dp){
	char str[10];
	uint8_t numdig = _int32_to_str(n, str);

	for(uint8_t j = 0; j < NUMBER_OF_DIGITS; ++j){
		if(numdig < NUMBER_OF_DIGITS)s->curr_data[j] = digits_map[str[j]];

	s->dp = dp;

}


void sseg_write_digit(_sseg *s, uint8_t digit, uint8_t value){
	s->curr_data[digit] = value;
}
void sseg_write_info(_sseg *s, int32_t n, uint8_t l){}

void sseg_hold_info(_sseg *s, uint8_t h){
	s->hold_info = h;
}



void sseg_work(_sseg *s){
	if(s->curr_digit > NUMBER_OF_DIGITS)
		s->curr_digit = 0;

	// Apago el display
	for(uint8_t j = 0; j<NUMBER_OF_DIGITS; ++j){
		s->sseg_sgpio_high(s->digits[j][0], s->digits[j][1]);

	//Escribo el digito correspondiente
	for (uint8_t x = 0; x < 7; ++x){
		if((digits_map[s->curr_data[s->curr_digit]]&(1<<x)))
			s->sseg_sgpio_high(s->segments[6-x][0], s->segments[6-x][1]);
		else
			s->sseg_sgpio_low(s->segments[6-x][0], s->segments[6-x][1]);
	}
	// Dot, if corresponds
	if(s->dp == s->curr_digit)
		s->sseg_sgpio_high(s->segments[7][0], s->segments[7][1]);
	else
		s->sseg_sgpio_low(s->segments[7][0], s->segments[7][1]);
	}
	s->sseg_sgpio_low(s->digits[s->curr_digit][0], s->digits[s->curr_digit][1]);
	s->curr_digit++;
}
