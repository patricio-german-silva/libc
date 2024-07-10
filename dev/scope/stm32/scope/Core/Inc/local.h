/*
 * local.h
 *
 *  Created on: Jul 9, 2024
 *      Author: psilva
 */

#ifndef INC_LOCAL_H_
#define INC_LOCAL_H_

#include "stdint.h"

/* Variables de estado y control
* ADC
*   ADC_Trigger
*   	0x 0000 0000: No trigger
*   	0x 0000 0001: Trigger rising, single
*   	0x 0000 0011: Trigger falling, single
*   	0x 0000 0101: Trigger rising, auto (normal)
*   	0x 0000 0111: Trigger falling, auto (normal)
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

	// ADC
	uint32_t adc_sps;
	uint8_t adc_trigger;
	uint8_t adc_ch1_active;
	uint16_t adc_ch1_trigger;
	uint8_t adc_ch2_active;
	uint16_t adc_ch2_trigger;

	// PWM
	uint8_t pwm_active;
	uint32_t pwm_period_high_ns;
	uint32_t pwm_period_low_ns;
}_control;


void local_init_control(_control *c, uint16_t *ch1_bf, uint16_t ch1_bf_size, uint16_t *ch2_bf, uint16_t ch2_bf_size);



#endif /* INC_LOCAL_H_ */
