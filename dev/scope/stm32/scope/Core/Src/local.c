/*
 * local.c
 *
 *  Created on: Jul 9, 2024
 *      Author: psilva
 */

#include "local.h"

void local_init_control(_control *c, uint16_t *ch1_bf, uint16_t ch1_bf_size, uint16_t *ch2_bf, uint16_t ch2_bf_size){
	c->bf_adc_ch1 = ch1_bf;
	c->bf_adc_ch1_size = ch1_bf_size;
	c->bf_adc_ch2 = ch2_bf;
	c->bf_adc_ch2_size = ch2_bf_size;
	c->bf_adc_ch1_iw = 0;
	c->bf_adc_ch1_ir = 0;
	c->bf_adc_ch2_iw = 0;
	c->bf_adc_ch2_ir = 0;
	c->adc_sps = 0;
	c->adc_trigger = 0;
	c->adc_ch1_active = 0;
	c->adc_ch1_trigger = 0;
	c->adc_ch2_active = 0;
	c->adc_ch2_trigger = 0;
	c->pwm_active = 0;
	c->pwm_period_high_ns = 0;
	c->pwm_period_low_ns = 0;
}
