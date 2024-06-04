/*
 * button.c
 *
 *  Created on: May 30, 2024
 *      Author: psilva
 * Version: 1.0
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
  b->state_changed = 0;
  b->min_value = 0;
  b->max_value = 1;
}

void btn_reset(_btn *b){
  b->debounce = 0;
  b->high_times = 0;
  b->counter = 0;
  b->curr_state = 0;
  b->state_changed = 0;
}

void btn_attach_read_gpio(_btn *b, btn_read_gpio_def f){
  b->btn_read_gpio = f;
}


void btn_set_mode(_btn *b, uint8_t mode){
  b->mode = mode;
}


void btn_min_switch_mode_value(_btn *b, uint8_t s){
  b->min_value = s;
}


void btn_max_switch_mode_value(_btn *b, uint8_t s){
  b->max_value = s;
}

uint8_t btn_read_state(_btn *b){
  uint8_t r = b->curr_state;
  if(b->mode <= _BUTTON_MODE_NORMAL_REPEAT_ACCELERATE_2)
    b->curr_state = 0;
  b->state_changed = 0;
  return r;
}

uint8_t btn_get_state(_btn *b){
  return b->curr_state;
}

uint8_t btn_state_changed(_btn *b){
  return b->state_changed;
}

void btn_write_status(_btn *b, uint8_t status){
  b->curr_state = status;
  b->state_changed = 1;
}

void btn_work(_btn *b){
  if(b->debounce == 0 && b->btn_read_gpio(b->port, b->pin) == 1){
    b->debounce++;
  }else if(b->debounce > 0 && b->debounce < _BUTTON_DEBOUNCE_TIME){
    b->debounce++;
  }else if(b->btn_read_gpio(b->port, b->pin) == 1){
    switch(b->mode){
      case _BUTTON_MODE_NORMAL:{
          if(b->high_times == 0){
            b->curr_state = 1;
            b->state_changed = 1;
          }
          b->high_times++;
          break;
      }
      case _BUTTON_MODE_NORMAL_REPEAT:{
          if(b->high_times == _BUTTON_REPEAT_TIME)
            b->high_times = 0;
          if(b->high_times == 0){
            b->curr_state = 1;
            b->state_changed = 1;
          }
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
            b->state_changed = 1;
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
            b->state_changed = 1;
          }
          b->high_times++;
          break;
      }
      case _BUTTON_MODE_SWITCH:{
          if(b->high_times == 0){
            b->curr_state++;
            b->state_changed = 1;
          }
          if(b->curr_state > b->max_value)
            b->curr_state = b->min_value;
          b->high_times++;
          break;
          }
      case _BUTTON_MODE_SWITCH_REPEAT:{
          if(b->high_times == _BUTTON_REPEAT_TIME)
            b->high_times = 0;
          if(b->high_times == 0){
            b->curr_state++;
            b->state_changed = 1;
          }
          if(b->curr_state > b->max_value)
            b->curr_state = b->min_value;
          b->high_times++;
          break;
      }
      case _BUTTON_MODE_SWITCH_REPEAT_ACCELERATE_1:{
          if(b->counter <= _BUTTON_ACCELERATE_TO_TIME_1_ON && b->high_times == _BUTTON_REPEAT_TIME)
            b->high_times = 0;
          else if(b->counter > _BUTTON_ACCELERATE_TO_TIME_1_ON && b->high_times == _BUTTON_ACCELERATE_TIME_1)
            b->high_times = 0;
          if(b->high_times == 0){
            b->counter++;
            b->curr_state++;
            b->state_changed = 1;
          }
          if(b->curr_state > b->max_value)
            b->curr_state = b->min_value;
          b->high_times++;
          break;
      }
      case _BUTTON_MODE_SWITCH_REPEAT_ACCELERATE_2:{
          if(b->counter <= _BUTTON_ACCELERATE_TO_TIME_1_ON && b->high_times == _BUTTON_REPEAT_TIME)
            b->high_times = 0;
          else if(b->counter > _BUTTON_ACCELERATE_TO_TIME_1_ON && b->high_times == _BUTTON_ACCELERATE_TIME_1)
            b->high_times = 0;
          else if(b->counter > _BUTTON_ACCELERATE_TO_TIME_2_ON && b->high_times == _BUTTON_ACCELERATE_TIME_2)
            b->high_times = 0;
          if(b->high_times == 0){
            b->counter++;
            b->curr_state++;
            b->state_changed = 1;
          }
          if(b->curr_state > b->max_value)
            b->curr_state = b->min_value;
          b->high_times++;
          break;
      }
    }
  }else{
    b->debounce = 0;
    b->high_times = 0;
    b->counter = 0;
  }
}
