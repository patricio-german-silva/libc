/*
 * local.h
 *
 *  Created on: Jul 9, 2024
 *      Author: psilva
 */

#ifndef INC_LOCAL_H_
#define INC_LOCAL_H_

#include "stdint.h"

/* Variable de estado y control adc_mode = uint8_t[4]
 *  control_status[0]: On/OFF
 *   	0b 0xxx xxxx: CH1 OFF
 *   	0b 1xxx xxxx: CH1 ON
 *   	0b x0xx xxxx: CH2 OFF
 *   	0b x1xx xxxx: CH2 ON
 *   	0b xx0x xxxx: PWM OFF
 *   	0b xx1x xxxx: PWM ON
 *
 *   control_status[1]: Trigger
 *   	0b xxxx 0000: No trigger
 *   	0b xxxx 1xxx: Trigger on CH1
 *   	0b xxxx x1xx: Trigger on CH2
 *   	0b xxxx xx0x: Trigger rising on CH1
 *   	0b xxxx xx1x: Trigger falling on CH1
 *   	0b xxxx xxx0: Trigger rising on CH2
 *   	0b xxxx xxx1: Trigger falling on CH2
 *
 *   control_status[1]: range
 *      0b xx00 xxxx: CH1 range low
 *   	0b xx01 xxxx: CH1 range high
 *   	0b xx10 xxxx: CH1 range auto
 *      0b 00xx xxxx: CH2 range low
 *   	0b 01xx xxxx: CH2 range high
 *   	0b 10xx xxxx: CH2 range auto
 */
typedef struct{
	// Buffers
	uint16_t *bf_adc_ch1;
	uint16_t *bf_adc_ch2;

	uint16_t bf_adc_ch1_size;
	uint16_t bf_adc_ch2_size;

	// Indices del buffer ADC
	uint16_t bf_adc_ch1_iw;
	uint16_t bf_adc_ch1_ir;
	uint16_t bf_adc_ch2_iw;
	uint16_t bf_adc_ch2_ir;

	// Status
	uint32_t control_status;

	// ADC
	uint16_t adc_prescaler;
	uint16_t adc_autoreload;
	uint16_t adc_ch1_trigger;
	uint16_t adc_ch2_trigger;

	// PWM
	uint16_t pwm_prescaler;
	uint16_t pwm_compare;
	uint16_t pwm_autoreload;
}_control;


void local_init_control(_control *c, uint16_t *ch1_bf, uint16_t ch1_bf_size, uint16_t *ch2_bf, uint16_t ch2_bf_size);



#endif /* INC_LOCAL_H_ */
