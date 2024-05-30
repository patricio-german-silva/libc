/*
 * seven_segments.c
 *
 *  Created on: May 29, 2024
 *      Author: psilva
 */
#include "seven_segments.h"

/*    LOCAL PROTOTYPES */
static void _sseg_write_number_to_array(uint8_t *data, int32_t n, int8_t dp);


void sseg_init(_sseg *s){
	s->hold_info = 0;
	s->curr_digit = 0;
	s->curr_label = 0;
	s->label_time_remains = 0;
	s->info_time_remains = 0;
}

void sseg_set_segments(_sseg *s, const uint32_t *port, const uint16_t *pin){
	for(uint8_t j = 0; j<8; ++j){
		s->segments[j][0] = port[j];
		s->segments[j][1] = pin[j];
	}
}

void sseg_set_digits(_sseg *s, const uint32_t *port, const uint16_t *pin){
	for(uint8_t j = 0; j<NUMBER_OF_DIGITS; ++j){
		s->digits[j][0] = port[j];
		s->digits[j][1] = pin[j];
	}
}


void sseg_set_info_label(_sseg *s, char *d, uint8_t i){
	for(uint8_t j = 0; j<NUMBER_OF_DIGITS; ++j){
		if(d[j]>= '0' && d[j]<= '9'){
			s->info_labels[i][j] = digits_map[(uint8_t)d[j]-'0'];
		}else if(d[j]>= 'a' && d[j]<= 'z'){
			s->info_labels[i][j] = digits_map[(uint8_t)d[j]-'a'+10];
		}else if(d[j]>= 'A' && d[j]<= 'Z'){
			s->info_labels[i][j] = digits_map[(uint8_t)d[j]-'A'+10];
		}else{
			switch(d[j]){
			case ' ':
				s->info_labels[i][j] = digits_map[BLANK_ID];
				break;
			case '-':
				s->info_labels[i][j] = digits_map[DASH_ID];
				break;
			case '.':
				s->info_labels[i][j] = digits_map[PERIOD_ID];
				break;
			case '*':
				s->info_labels[i][j] = digits_map[ASTERISK_ID];
				break;
			case '_':
				s->info_labels[i][j] = digits_map[UNDERSCORE_ID];
				break;
			default:
				s->info_labels[i][j] = digits_map[BLANK_ID];
			}
		}
	}
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
	_sseg_write_number_to_array(s->curr_data, n, dp);
}


void sseg_write_digit(_sseg *s, uint8_t digit, uint8_t value){
	s->curr_data[digit] = value;
}

void sseg_write_info(_sseg *s, int32_t n, int8_t dp, uint8_t l){
	_sseg_write_number_to_array(s->curr_info, n, dp);
	s->label_time_remains = SHOW_INFO_LABEL_TIME;
	s->info_time_remains = SHOW_INFO_DATA_TIME;
}

void sseg_abort_info(_sseg *s){
	s->label_time_remains = 0;
	s->info_time_remains = 0;
}

void sseg_hold_info(_sseg *s, uint8_t h){
	s->hold_info = h;
}



void sseg_work(_sseg *s){
	uint8_t *to_display;

	if(s->curr_digit > NUMBER_OF_DIGITS)
		s->curr_digit = 0;

	if(s->label_time_remains > 0){
		s->label_time_remains--;
		to_display = s->info_labels[s->curr_label];
	}else if(s->info_time_remains > 0){
		if(s->hold_info == 0) s->info_time_remains--;
		to_display = s->curr_info;
	}else{
		to_display = s->curr_data;
	}

	// Apago el display
	for(uint8_t j = 0; j<NUMBER_OF_DIGITS; ++j)
		s->sseg_sgpio_high(s->digits[j][0], (uint16_t)s->digits[j][1]);

	//Escribo el digito correspondiente
	for (uint8_t x = 0; x < 8; ++x){
		if((to_display[s->curr_digit])&(1<<x))
			s->sseg_sgpio_high(s->segments[6-x][0], (uint16_t)s->segments[6-x][1]);
		else
			s->sseg_sgpio_low(s->segments[6-x][0], (uint16_t)s->segments[6-x][1]);
	}
	s->sseg_sgpio_low(s->digits[s->curr_digit][0], (uint16_t)s->digits[s->curr_digit][1]);
	s->curr_digit++;
}



/******************************************************************************************/

static void _sseg_write_number_to_array(uint8_t *data, int32_t n, int8_t dp){
	char str[10];
	uint8_t numdig = _int32_to_str(n, str);

	// Number too big, show dashes
	if(numdig > NUMBER_OF_DIGITS){
		for(uint8_t j = 0; j < NUMBER_OF_DIGITS; ++j)
			data[j] = digits_map[DASH_ID];
		return;
	}

	uint8_t i = 0;
	uint8_t j = 0;
	while (i < NUMBER_OF_DIGITS){
		if(i < NUMBER_OF_DIGITS-numdig)
			data[i++] = digits_map[DASH_ID];
		else
			if(dp == i)
				data[i++] = (digits_map[(uint8_t)str[j++]])|(1<<8);
			else
				data[i++] = digits_map[(uint8_t)str[j++]];
	}
}
