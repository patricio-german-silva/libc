/*
 * seven_segments.h
 *
 *  Created on: May 29, 2024
 *      Author: psilva
 */
#ifndef INC_SEVEN_SEGMENTS_H_
#define INC_SEVEN_SEGMENTS_H_

#include "stdint.h"
#include "strutil.h"

// Cantidad de digitos del display
#define NUMBER_OF_DIGITS 3

// Tipo de display
//#define DISPLAY_COMMON_ANODE
#define DISPLAY_COMMON_CATODE

// Informacion a mostrar con titulo
#define NUMBER_OF_INFO_LABELS 7

// Tiempo que se muestra el label y la info
#define SHOW_INFO_LABEL_TIME 1000
#define SHOW_INFO_DATA_TIME 3000



// Caracteres especiales no mapeables
#define BLANK_ID 36 // Must match with 'digitCodeMap'
#define DASH_ID 37
#define PERIOD_ID 37
#define ASTERISK_ID 38
#define UNDERSCORE_ID 39

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
  0b00000000, // For future use
  0b00000000, // For future use
  0b00000000, // For future use
  0b00000000, // For future use
  0b00000000, // For future use
  0b01000000, // 45  '-'  DASH, ascci code matches array index
};


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


typedef void (*sseg_set_gpio_def)(uint32_t port, uint16_t pin);


typedef struct{
	uint32_t segments[8][2]; //, PORT, PIN, format  G F E D C B A DP
	uint32_t digits[NUMBER_OF_DIGITS][2];
	uint8_t info_labels[NUMBER_OF_INFO_LABELS][NUMBER_OF_DIGITS];
	uint8_t curr_data[NUMBER_OF_DIGITS];
	uint8_t curr_info[NUMBER_OF_DIGITS];
	uint8_t hold_info;
	uint8_t curr_digit;
	uint8_t curr_label;
	uint32_t label_time_remains;
	uint32_t info_time_remains;

	sseg_set_gpio_def sseg_sgpio_high;
	sseg_set_gpio_def sseg_sgpio_low;
}_sseg;

void sseg_init(_sseg *s);
void sseg_set_segments(_sseg *s, const uint32_t *port, const uint16_t *pin);
void sseg_set_digits(_sseg *s, const uint32_t *port, const uint16_t *pin);
void sseg_set_info_label(_sseg *s, char *d, uint8_t i);
void sseg_attach_gpio_high(_sseg *s, sseg_set_gpio_def f);
void sseg_attach_gpio_low(_sseg *s, sseg_set_gpio_def f);
void sseg_write_data(_sseg *s, int32_t n, int8_t dp);				// n: Number to show, dp: decimal places
void sseg_write_digit(_sseg *s, uint8_t digit, uint8_t value);		// digit, value, on G F E D C B A DP bits format
void sseg_write_info(_sseg *s, int32_t n, int8_t dp, uint8_t l);	// Number to show, decimal places, labbel to show
void sseg_abort_info(_sseg *s);										// Stop showing current info
void sseg_hold_info(_sseg *s, uint8_t h);							// The display holds the info values 1: true, 0: false
void sseg_work(_sseg *s);



#endif // INC_SEVEN_SEGMENTS_H_
