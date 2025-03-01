/*
 * cmdparser.h
 *
 *  Created on: Oct 7, 2022
 *      Author: psilva
 */

#ifndef INC_CMDPARSER_H_
#define INC_CMDPARSER_H_

#include "util.h"
#include "strutil.h"
#include "pcomm.h"
#include "local.h"




/*  DEFINICION DE COMANDOS  */
// ADC
#define _CMD_ADC_START_ON_CH1			0x01
#define _CMD_ADC_STOP_ON_CH1			0x02
#define _CMD_ADC_START_ON_CH2			0x03
#define _CMD_ADC_STOP_ON_CH2			0x04
#define _CMD_ADC_SET_TIMER				0x05
// PWM
#define _CMD_PWM_START_ON_CH1			0x10
#define _CMD_PWM_STOP_ON_CH1			0x11
// RANGE
#define _CMD_ADC_SET_LOW_RANGE_ON_CH1	0x20
#define _CMD_ADC_SET_HIGH_RANGE_ON_CH1	0x21
#define _CMD_ADC_SET_AUTO_RANGE_ON_CH1	0x22
#define _CMD_ADC_SET_LOW_RANGE_ON_CH2	0x23
#define _CMD_ADC_SET_HIGH_RANGE_ON_CH2	0x24
#define _CMD_ADC_SET_AUTO_RANGE_ON_CH2	0x25
// TRIGGER
#define _CMD_ADC_SET_TRIGGER_ON_CH1		0x30
#define _CMD_ADC_SET_TRIGGER_ON_CH2		0x31
// CONFIG
#define _CMD_SET_CONFIG					0x40
// Comandos que retornan información
#define _CMD_KEEP_ALIVE					0xA0
#define _CMD_FIRMWARE_VERSION			0xA1
#define _CMD_GET_CONFIG					0xA2
#define _CMD_DATA_FRAME					0xAA // Used as marker for IC->PC data only

/*
 * Recibe las estructuras de datos para posteriores llamadas a cmd_exec
 * Antes de llamar se debe verificar que ya hay un nuevo comando en _pcomm listo para ser procesado
 */
void cmd_init(_usrtick *ut, _hb *h, _pcomm *p, _control *c);

void cmd_exec(_pcomm *p);


#endif /* INC_CMDPARSER_H_ */
