/*
 * seven_segments.h
 *
 * Manages a multi-digit seven segments display
 *
 *  Created on: May 29, 2024
 *      Author: psilva
 */
#ifndef INC_SEVEN_SEGMENTS_H_
#define INC_SEVEN_SEGMENTS_H_

#include "stdint.h"
#include "strutil.h"

// Number of digits on the display
#define _SSEG_NUMBER_OF_DIGITS 3

// type of display
//#define _SSEG_DISPLAY_COMMON_ANODE
#define _SSEG_DISPLAY_COMMON_CATODE

//  Maximum number of info labels
#define _SSEG_NUMBER_OF_INFO_LABELS 16

/* Amount of time to be shown the label and info values
 * based on the number of calls to the work function */
#define _SSEG_SHOW_INFO_LABEL_TIME 1000
#define _SSEG_SHOW_INFO_DATA_TIME 3000



// Non-mapeable digits
#define _SSEG_BLANK_ID 36 // Must match with 'digitCodeMap'
#define _SSEG_DASH_ID 40
#define _SSEG_PERIOD_ID 37
#define _SSEG_ASTERISK_ID 38
#define _SSEG_UNDERSCORE_ID 39

static const uint8_t digits_map[] = {
  // GFEDCBA  Segments      7-segment map:
  0b00111111, // 0   "0"          AAA
  0b00000110, // 1   "1"         F   B
  0b01011011, // 2   "2"         F   B
  0b01001111, // 3   "3"          GGG
  0b01100110, // 4   "4"         E   C
  0b01101101, // 5   "5"         E   C
  0b01111101, // 6   "6"          DDD  DP
  0b00000111, // 7   "7"
  0b01111111, // 8   "8"
  0b01101111, // 9   "9"
  0b01110111, // 65  'A'
  0b01111100, // 66  'b'
  0b00111001, // 67  'C'
  0b01011110, // 68  'd'
  0b01111001, // 69  'E'
  0b01110001, // 70  'F'
  0b00111101, // 71  'G'
  0b01110110, // 72  'H'
  0b00110000, // 73  'I'
  0b00001110, // 74  'J'
  0b01110110, // 75  'K'  Same as 'H'
  0b00111000, // 76  'L'
  0b00000000, // 77  'M'  NO DISPLAY
  0b01010100, // 78  'n'
  0b00111111, // 79  'O'
  0b01110011, // 80  'P'
  0b01100111, // 81  'q'
  0b01010000, // 82  'r'
  0b01101101, // 83  'S'
  0b01111000, // 84  't'
  0b00111110, // 85  'U'
  0b00111110, // 86  'V'  Same as 'U'
  0b00000000, // 87  'W'  NO DISPLAY
  0b01110110, // 88  'X'  Same as 'H'
  0b01101110, // 89  'y'
  0b01011011, // 90  'Z'  Same as '2'
  0b00000000, // 32  ' '  BLANK
  0b10000000, // 46  '.'  PERIOD
  0b01100011, // 42 '*'  DEGREE ..
  0b00001000, // 95 '_'  UNDERSCORE
  0b01000000, // 45  '-'  DASH
};


typedef void (*sseg_set_gpio_def)(uint32_t port, uint16_t pin);


typedef struct{
	uint32_t segments[8][2]; //, PORT, PIN, format  G F E D C B A DP
	uint32_t digits[_SSEG_NUMBER_OF_DIGITS][2];
	uint8_t info_labels[_SSEG_NUMBER_OF_INFO_LABELS][_SSEG_NUMBER_OF_DIGITS];
	uint8_t curr_data[_SSEG_NUMBER_OF_DIGITS];
	uint8_t curr_info[_SSEG_NUMBER_OF_DIGITS];
	uint8_t hold_info;
	uint8_t curr_digit;
	uint8_t curr_label;
	uint32_t label_time_remain;
	uint32_t info_time_remain;

	sseg_set_gpio_def sseg_sgpio_high;
	sseg_set_gpio_def sseg_sgpio_low;
}_sseg;

/* Initialize object*/
void sseg_init(_sseg *s);

/* Sets port and pin for each led segment (8 total, seven segments plus the dot)*/
void sseg_set_segments(_sseg *s, const uint32_t *port, const uint16_t *pin);

/* Sets port and pin for each digit, _SSEG_NUMBER_OF_DIGITS total */
void sseg_set_digits(_sseg *s, const uint32_t *port, const uint16_t *pin);

/* Populate the info labels, max is _SSEG_NUMBER_OF_INFO_LABELS */
void sseg_set_info_label(_sseg *s, const char *d, uint8_t i);

/* Attaches a function that puts a specific port and pin high */
void sseg_attach_gpio_high(_sseg *s, sseg_set_gpio_def f);

/* Attaches a function that puts a specific port and pin low */
void sseg_attach_gpio_low(_sseg *s, sseg_set_gpio_def f);

/* Numeric data to be displayed
 * n: Number to show
 * dp: decimal places. dp=0 -> no decimal places but dot on the last digit, dp=-1 -> no dot at all
 */
void sseg_write_numeric_data(_sseg *s, int32_t n, int8_t dp);

/* Char data to be displayed */
void sseg_write_char_data(_sseg *s, const char *d);

/* Low level, writes specific segments on specific data digit
 * digit, value, on G F E D C B A DP bits format
 */
void sseg_write_data_digit(_sseg *s, uint8_t digit, uint8_t value);

/* shift digits to the left */
void sseg_shift_data_left(_sseg *s);	

/* shift digits to the rigth */
void sseg_shift_data_rigth(_sseg *s);

/* Writes numeric info
 * Number to show, decimal places, labbel index 
 */
void sseg_write_numeric_info(_sseg *s, int32_t n, int8_t dp, uint8_t l);

/* Writes numeric info
 * Number to show, decimal places, labbel index 
 */
void sseg_write_char_info(_sseg *s, const char *d, uint8_t l);

/* If info is displayed, updates current info, only if label matches.
 * Number to show, decimal places, labeli
 * ut:1: resets time to _SSEG_SHOW_INFO_DATA_TIME
 * olt = 1: override label time, showing info immediately
 */
void sseg_update_info(_sseg *s, int32_t n, int8_t dp, uint8_t l, uint8_t ut, uint8_t olt);


/* Low level, writes specific segments on specific info digit
 * digit, value, on G F E D C B A DP bits format
 */
void sseg_write_info_digit(_sseg *s, uint8_t digit, uint8_t value);

/* shift info digits to the left */
void sseg_shift_info_left(_sseg *s);

/* shift info digits to the rigth */
void sseg_shift_info_rigth(_sseg *s);

/* Stop showing current info */
void sseg_abort_info(_sseg *s);

/* The display holds the info values
 * 1: true, 0: false */
void sseg_hold_info(_sseg *s, uint8_t h);

/* Retruns 1 if the display holds the info values */
uint8_t sseg_get_hold_info(_sseg *s);

/* The display holds the info values 1: true, 0: false */
void sseg_toggle_hold_info(_sseg *s);	

/* 0: show data, 1: show info label, 2: show info data */
uint8_t sseg_display_status(_sseg *s);

/* Call periodicaly, all the time-based actions are based on the period between calls to this function */
void sseg_work(_sseg *s);



#endif // INC_SEVEN_SEGMENTS_H_
