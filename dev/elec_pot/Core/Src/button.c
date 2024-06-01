/*
 * button.c
 *
 *  Created on: May 30, 2024
 *      Author: psilva
 * TODO: switch mode
 */

#include "button.h"

void btn_init(_btn *b, uint8_t mode, uint32_t port, uint16_t pin){
  b->mode = mode;
  b->port = port;
  b->pin = pin;
  b->debounce = 0;
  b->high_times = 0;
  b->counter = 0;
  b->curr_state = 0;
  b->num_values = 0;
}


void btn_attach_read_gpio(_btn *b, btn_read_gpio_def f){
  b->btn_read_gpio = f;
}


void btn_set_mode(_btn *b, uint8_t mode){
  b->mode = mode;
}


void btn_switch_state_counter(_btn *b, uint8_t s){
  b->num_values = s;
}

uint8_t btn_read_status(_btn *b){
  uint8_t r = b->curr_state;
  if(b->mode <= _BUTTON_MODE_NORMAL_REPEAT_ACCELERATE_2)
    b->curr_state = 0;
  return r;
}

void btn_work(_btn *b){
  if(b->debounce == 0 && b->btn_read_gpio(b->port, b->pin) == 1){
    b->debounce++;
  }else if(b->debounce > 0 && b->debounce < _BUTTON_DEBOUNCE_TIME){
    b->debounce++;
  }else if(b->btn_read_gpio(b->port, b->pin) == 1){
    switch(b->mode){
      case _BUTTON_MODE_NORMAL:{
          if(b->high_times == 0)
            b->curr_state = 1;
          b->high_times++;
          break;
      }
      case _BUTTON_MODE_NORMAL_REPEAT:{
          if(b->high_times == _BUTTON_REPEAT_TIME)
            b->high_times = 0;
          if(b->high_times == 0)
            b->curr_state = 1;
          b->high_times++;
          break;
      }
      case _BUTTON_MODE_NORMAL_REPEAT_ACCELERATE_1:{
          if(b->counter <= _BUTTON_ACCELERATE_TO_TIME_1_ON && b->high_times == _BUTTON_REPEAT_TIME)
            b->high_times = 0;
          else if(b->counter > _BUTTON_ACCELERATE_TO_TIME_1_ON && b->high_times == _BUTTON_ACCELERATE_TIME_1)
            b->high_times = 0;
          if(b->high_times == 0){
            b->counter++;
            b->curr_state = 1;
          }
          b->high_times++;
          break;
      }
      case _BUTTON_MODE_NORMAL_REPEAT_ACCELERATE_2:{
          if(b->counter <= _BUTTON_ACCELERATE_TO_TIME_1_ON && b->high_times == _BUTTON_REPEAT_TIME)
            b->high_times = 0;
          else if(b->counter > _BUTTON_ACCELERATE_TO_TIME_1_ON && b->high_times == _BUTTON_ACCELERATE_TIME_1)
            b->high_times = 0;
          else if(b->counter > _BUTTON_ACCELERATE_TO_TIME_2_ON && b->high_times == _BUTTON_ACCELERATE_TIME_2)
            b->high_times = 0;
          if(b->high_times == 0){
            b->counter++;
            b->curr_state = 1;
          }
          b->high_times++;
          break;
      }/*
      case _BUTTON_MODE_SWITCH:
      case _BUTTON_MODE_MULTIVALUE_SWITCH:
      case _BUTTON_MODE_MULTIVALUE_SWITCH_REPEAT:
      case _BUTTON_MODE_MULTIVALUE_SWITCH_REPEAT_ACCELERATE_1:
      case _BUTTON_MODE_MULTIVALUE_SWITCH_REPEAT_ACCELERATE_1:*/
    }
  }else{
    b->debounce = 0;
    b->high_times = 0;
    b->counter = 0;
  }
}