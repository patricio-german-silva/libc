/*
 * button.h
 *
 *  Created on: May 30, 2024
 *      Author: psilva
 * Time is in btn_work call period
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
#define _BUTTON_MODE_MULTIVALUE_SWITCH                      5
#define _BUTTON_MODE_MULTIVALUE_SWITCH_REPEAT               6
#define _BUTTON_MODE_MULTIVALUE_SWITCH_REPEAT_ACCELERATE_1  7
#define _BUTTON_MODE_MULTIVALUE_SWITCH_REPEAT_ACCELERATE_1  8

#define _BUTTON_REPEAT_TIME                               500
#define _BUTTON_ACCELERATE_TIME_1                         150
#define _BUTTON_ACCELERATE_TIME_2                          50
#define _BUTTON_ACCELERATE_TO_TIME_1_ON                     5
#define _BUTTON_ACCELERATE_TO_TIME_2_ON                    15

typedef uint8_t (*btn_read_gpio_def)(uint32_t port, uint16_t pin);


typedef struct{
  uint32_t port;
  uint16_t pin;
  /*
   * Modo de operacion del boton
   * 0: normal
   * 1: normal, repeat
   * 2: normal, repeat and accel 1
   * 3: normal, repeat and accel 2
   * 4: switch mode  
   * 5: multivalue switch mode, 255 values max
   * 6: multivalue switch mode, repeat
   * 7: multivalue switch mode, repeat and accel 1
   * 8: multivalue switch mode, repeat and accel 2
   */
  uint8_t mode;
  uint8_t debounce;
  uint32_t high_times;
  uint32_t counter;
  uint8_t curr_state;
  uint8_t num_values;

	btn_read_gpio_def btn_read_gpio;
}_btn;

void btn_init(_btn *b, uint8_t mode, uint32_t port, uint16_t pin);
void btn_attach_read_gpio(_btn *b, btn_read_gpio_def f);
void btn_set_mode(_btn *b, uint8_t mode);
void btn_switch_state_counter(_btn *b, uint8_t s);
uint8_t btn_read_status(_btn *b);
void btn_work(_btn *b);

#endif /* INC_BUTTON_H_ */
