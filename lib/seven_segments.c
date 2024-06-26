/*
 * seven_segments.c
 *
 *  Created on: May 29, 2024
 *      Author: psilva
 */
#include "seven_segments.h"

/*    LOCAL PROTOTYPES */
static void _sseg_write_number_to_array(uint8_t *data, int32_t n, int8_t dp);
static void _sseg_write_chars_to_array(uint8_t *data, const char *value);


void sseg_init(_sseg *s){
	s->hold_info = 0;
	s->curr_digit = 0;
	s->curr_label = 0;
	s->label_time_remain = 0;
	s->info_time_remain = 0;
}

void sseg_set_segments(_sseg *s, const uint32_t *port, const uint16_t *pin){
	for(uint8_t j = 0; j<8; ++j){
		s->segments[j][0] = port[j];
		s->segments[j][1] = pin[j];
	}
}

void sseg_set_digits(_sseg *s, const uint32_t *port, const uint16_t *pin){
	for(uint8_t j = 0; j<_SSEG_NUMBER_OF_DIGITS; ++j){
		s->digits[j][0] = port[j];
		s->digits[j][1] = pin[j];
	}
}


void sseg_set_info_label(_sseg *s, const char *d, uint8_t i){
  _sseg_write_chars_to_array(s->info_labels[i], d);
}

// Change functions to change behavior, I'm not proud of it, ok
void sseg_attach_gpio_high(_sseg *s, sseg_set_gpio_def f){
#ifdef _SSEG_DISPLAY_COMMON_ANODE
	s->sseg_sgpio_high = f;
#endif
#ifdef _SSEG_DISPLAY_COMMON_CATODE
	s->sseg_sgpio_low = f;
#endif
}

void sseg_attach_gpio_low(_sseg *s, sseg_set_gpio_def f){
#ifdef _SSEG_DISPLAY_COMMON_ANODE
	s->sseg_sgpio_low = f;
#endif
#ifdef _SSEG_DISPLAY_COMMON_CATODE
	s->sseg_sgpio_high = f;
#endif
}

void sseg_write_numeric_data(_sseg *s, int32_t n, int8_t dp){
	_sseg_write_number_to_array(s->curr_data, n, dp);
}

void sseg_write_char_data(_sseg *s, const char *d){
  _sseg_write_chars_to_array(s->curr_data, d);
}

void sseg_write_data_digit(_sseg *s, uint8_t digit, uint8_t value){
    s->curr_data[digit] = value;
}

void sseg_shift_data_left(_sseg *s){
  for (uint8_t x = 0; x < _SSEG_NUMBER_OF_DIGITS-1; ++x){
    s->curr_data[x] = s->curr_data[x+1];
    s->curr_data[x+1] = digits_map[_SSEG_BLANK_ID];
  }
}

void sseg_shift_data_rigth(_sseg *s){
  for (uint8_t x = _SSEG_NUMBER_OF_DIGITS-1; x > 0; --x){
    s->curr_data[x] = s->curr_data[x-1];
    s->curr_data[x-1] = digits_map[_SSEG_BLANK_ID];
  }
}

void sseg_write_numeric_info(_sseg *s, int32_t n, int8_t dp, uint8_t l){
	_sseg_write_number_to_array(s->curr_info, n, dp);
  s->label_time_remain = _SSEG_SHOW_INFO_LABEL_TIME;
	s->info_time_remain = _SSEG_SHOW_INFO_DATA_TIME;
  s->curr_label = l;
}

void sseg_write_char_info(_sseg *s, const char *d, uint8_t l){
  _sseg_write_chars_to_array(s->curr_info, d);
  s->label_time_remain = _SSEG_SHOW_INFO_LABEL_TIME;
	s->info_time_remain = _SSEG_SHOW_INFO_DATA_TIME;
  s->curr_label = l;
}

void sseg_write_info_digit(_sseg *s, uint8_t digit, uint8_t value){
	  s->curr_info[digit] = value;
}

void sseg_shift_info_left(_sseg *s){
  for (uint8_t x = 0; x < _SSEG_NUMBER_OF_DIGITS-1; ++x){
    s->curr_info[x] = s->curr_info[x+1];
    s->curr_info[x+1] = digits_map[_SSEG_BLANK_ID];
  }
}

void sseg_shift_info_rigth(_sseg *s){
  for (uint8_t x = _SSEG_NUMBER_OF_DIGITS-1; x > 0; --x){
    s->curr_info[x] = s->curr_info[x-1];
    s->curr_info[x-1] = digits_map[_SSEG_BLANK_ID];
  }
}

void sseg_update_info(_sseg *s, int32_t n, int8_t dp, uint8_t l, uint8_t ut, uint8_t olt){
  if(s->curr_label != l)
    return;
  _sseg_write_number_to_array(s->curr_info, n, dp);
  if(ut == 1)
    s->info_time_remain = _SSEG_SHOW_INFO_DATA_TIME;
  if(olt == 1)
    s->label_time_remain = 0;
}

void sseg_abort_info(_sseg *s){
	s->label_time_remain = 0;
	s->info_time_remain = 0;
}

void sseg_hold_info(_sseg *s, uint8_t h){
	s->hold_info = h;
}

uint8_t sseg_get_hold_info(_sseg *s){
  return s->hold_info;
}

void sseg_toggle_hold_info(_sseg *s){
	if(s->hold_info == 0)
    s->hold_info = 1;
  else
    s->hold_info = 0;
}

uint8_t sseg_display_status(_sseg *s){
  if(s->label_time_remain > 0)
    return 1;
  else if(s->info_time_remain > 0)
    return 2;
  else
    return 0;
}

void sseg_work(_sseg *s){
	uint8_t *to_display;

	// Apago el display
	s->sseg_sgpio_high(s->digits[s->curr_digit][0], (uint16_t)s->digits[s->curr_digit][1]);
  
  s->curr_digit++;
	if(s->curr_digit == _SSEG_NUMBER_OF_DIGITS)
		s->curr_digit = 0;

	if(s->label_time_remain > 0){
		s->label_time_remain--;
		to_display = s->info_labels[s->curr_label];
	}else if(s->info_time_remain > 0){
		if(s->hold_info == 0)
      s->info_time_remain--;
    else
      s->info_time_remain = 1;  // back from a hold imediatly
		to_display = s->curr_info;
	}else{
		to_display = s->curr_data;
	}



	//Escribo el digito correspondiente
	for (uint8_t x = 0; x < 8; ++x){
		if((to_display[s->curr_digit])&(1<<x))
			s->sseg_sgpio_high(s->segments[x][0], (uint16_t)s->segments[x][1]);
		else
			s->sseg_sgpio_low(s->segments[x][0], (uint16_t)s->segments[x][1]);
	}
	s->sseg_sgpio_low(s->digits[s->curr_digit][0], (uint16_t)s->digits[s->curr_digit][1]);
	
}



/******************************************************************************************/
/*                          LOCAL                                                         */
/******************************************************************************************/

static void _sseg_write_number_to_array(uint8_t *data, int32_t n, int8_t dp){
	char str[10];
	uint8_t numdig = _int32_to_str(n, str);

	// Number too big, show dashes
	if(numdig > _SSEG_NUMBER_OF_DIGITS){
		for(uint8_t j = 0; j < _SSEG_NUMBER_OF_DIGITS; ++j)
			data[j] = digits_map[_SSEG_DASH_ID];
		return;
	}

	uint8_t i = 0;
	uint8_t j = 0;
	while (i < _SSEG_NUMBER_OF_DIGITS){
		if(i < _SSEG_NUMBER_OF_DIGITS-numdig)
			data[i++] = digits_map[_SSEG_BLANK_ID];
		else if(str[j] == '-'){
      data[i++] = digits_map[_SSEG_DASH_ID];
      j++;
    }else if(dp == _SSEG_NUMBER_OF_DIGITS-j-1)
			data[i++] = (digits_map[(uint8_t)str[j++]-'0'])|(1<<7);
		else
			data[i++] = digits_map[(uint8_t)str[j++]-'0'];
	}
}

static void _sseg_write_chars_to_array(uint8_t *data, const char *value){
	for(uint8_t j = 0; j<_SSEG_NUMBER_OF_DIGITS; ++j){
		if(value[j]>= '0' && value[j]<= '9'){
			data[j] = digits_map[(uint8_t)value[j]-'0'];
		}else if(value[j]>= 'a' && value[j]<= 'z'){
			data[j] = digits_map[(uint8_t)value[j]-'a'+10];
		}else if(value[j]>= 'A' && value[j]<= 'Z'){
			data[j] = digits_map[(uint8_t)value[j]-'A'+10];
		}else{
			switch(value[j]){
			case ' ':
				data[j] = digits_map[_SSEG_BLANK_ID];
				break;
			case '-':
				data[j] = digits_map[_SSEG_DASH_ID];
				break;
			case '.':
				data[j] = digits_map[_SSEG_PERIOD_ID];
				break;
			case '*':
				data[j] = digits_map[_SSEG_ASTERISK_ID];
				break;
			case '_':
				data[j] = digits_map[_SSEG_UNDERSCORE_ID];
				break;
			default:
				data[j] = digits_map[_SSEG_BLANK_ID];
			}
		}
	}
}
