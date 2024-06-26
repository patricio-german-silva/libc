/*
 * button.h
 *
 *  Created on: May 30, 2024
 *      Author: psilva
 * 
 * Manages and debounce a button, controls behaviour
 *
 * All the Time-based parameters are in btn_work call period
 *
 */

#include "stdint.h"


#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#define _BUTTON_DEBOUNCE_TIME                              50

#define _BUTTON_MODE_NORMAL                                 0
#define _BUTTON_MODE_NORMAL_REPEAT                          1
#define _BUTTON_MODE_NORMAL_REPEAT_ACCELERATE_1             2
#define _BUTTON_MODE_NORMAL_REPEAT_ACCELERATE_2             3
#define _BUTTON_MODE_SWITCH                                 4
#define _BUTTON_MODE_SWITCH_REPEAT                          5
#define _BUTTON_MODE_SWITCH_REPEAT_ACCELERATE_1             6
#define _BUTTON_MODE_SWITCH_REPEAT_ACCELERATE_2             7

#define _BUTTON_REPEAT_TIME                               500
#define _BUTTON_ACCELERATE_TIME_1                         150
#define _BUTTON_ACCELERATE_TIME_2                          50
#define _BUTTON_ACCELERATE_TO_TIME_1_ON                     3
#define _BUTTON_ACCELERATE_TO_TIME_2_ON                    10

typedef uint8_t (*btn_read_gpio_def)(uint32_t port, uint16_t pin);


typedef struct{
  uint32_t port;
  uint16_t pin;
  /*
   * Operation mode
   * 0: normal
   * 1: normal, repeat
   * 2: normal, repeat and accel 1
   * 3: normal, repeat and accel 2
   * 4: multivalue switch mode, 255 values max ( default: multivalue switch with min_value = 0 and max_value = 1 -> normal on/off switch)
   * 5: multivalue switch mode, repeat
   * 6: multivalue switch mode, repeat and accel 1
   * 7: multivalue switch mode, repeat and accel 2
   */
  uint8_t mode;
  uint8_t debounce;
  uint32_t high_times;
  uint32_t counter;
  uint8_t curr_state;
  uint8_t state_changed;
  uint8_t min_value;
  uint8_t max_value;

	btn_read_gpio_def btn_read_gpio;
}_btn;

/* Initialize button, specified mode, port and pin*/
void btn_init(_btn *b, uint8_t mode, uint32_t port, uint16_t pin);

/* Resets button status */
void btn_reset(_btn *b);

/* Attachs the read port and pin function */
void btn_attach_read_gpio(_btn *b, btn_read_gpio_def f);

/* Sets the button mode of operation */
void btn_set_mode(_btn *b, uint8_t mode);

/* When multivalue shitch mode is specified, defines min value */
void btn_min_switch_mode_value(_btn *b, uint8_t s);

/* When multivalue shitch mode is specified, defines max value */
void btn_max_switch_mode_value(_btn *b, uint8_t s);

/* Read button state
 * If mode of operation is switch, returns the state value
 * If mode of operation is one of the normals, acknowledge the state (resets the state to 0) */
uint8_t btn_read_state(_btn *b);

/* Return state but dont acknowledge */
uint8_t btn_get_state(_btn *b);

/* Returns 1 if state changed since de last btn_read_state */
uint8_t btn_state_changed(_btn *b);

/* Force given status on the button and set it as if it was pressed */
void btn_write_status(_btn *b, uint8_t status);

/* Call it periodicaly, time-based parameters are based on the periods bettween calls to this function*/
void btn_work(_btn *b);

#endif /* INC_BUTTON_H_ */
