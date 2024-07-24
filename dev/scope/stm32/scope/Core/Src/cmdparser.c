/*
 * cmdparser.c
 *
 *  Created on: Oct 7, 2022
 *      Author: psilva
 */
#include "cmdparser.h"

#ifndef _FIRMWARE_VERSION_
extern char firmware_version[42];
#else
extern const char *firmware_version;
#endif

static const char *cmd_error = "ERR";


static _pcomm *cdc;
static _usrtick *usrtick;
static _hb *hb;
static _control *control;


/*
 *inicializa las estructuras de datos
 */
void cmd_init(_usrtick *ut, _hb *h, _pcomm *p, _control *c){
	usrtick = ut;
	hb = h;
	cdc = p;
	control = c;
}


/*
 * Antes de llamar se debe verificar que ya hay un nuevo comando en _pcomm listo para ser procesado
 *
 *
 * 	Respuesta Err: Comando desconocodo
 *
 */
void cmd_exec(_pcomm *p){
	switch(p->rx.buff[p->rx.datastart]){
	/*   ·······················   ADC   ······················· */
	case _CMD_ADC_START_ON_CH1:{
		control->control_status |= 0b1000000000000000;
		pcomm_tx_put_data_frame(p, &p->rx.buff[p->rx.datastart], 1);
		break;
	}
	case _CMD_ADC_START_ON_CH2:{
		control->control_status |= 0b0100000000000000;
		pcomm_tx_put_data_frame(p, &p->rx.buff[p->rx.datastart], 1);
		break;
	}
	case _CMD_ADC_STOP_ON_CH1:{
		control->control_status &= 0b0111111111111111;
		pcomm_tx_put_data_frame(p, &p->rx.buff[p->rx.datastart], 1);
		break;
	}
	case _CMD_ADC_STOP_ON_CH2:{
		control->control_status &= 0b1011111111111111;
		pcomm_tx_put_data_frame(p, &p->rx.buff[p->rx.datastart], 1);
		break;
	}
	case _CMD_ADC_SET_TIMER:{
		if(p->rx.datasize == 5){
			control->adc_prescaler = pcomm_rx_read_uint16_t(p, 1);
			control->adc_autoreload = pcomm_rx_read_uint16_t(p, 3);
			pcomm_tx_put_data_frame(p, &p->rx.buff[p->rx.datastart], 1);
		}else{
			pcomm_tx_put_data_frame(p, (uint8_t *)cmd_error, 3);
		}
		break;
	}

	/*   ·······················   PWM   ······················· */
	/* Activar PWM:
	 * [0]	: este codigo, _CMD_START_PWM_ON_CH1
	 * [1,2]: prescaler
	 * [3,4]: compare
	 * [5,6]: autoreload
	 */
	case _CMD_PWM_START_ON_CH1:{
		if(p->rx.datasize == 7){
			control->control_status |= 0b0010000000000000;
			control->pwm_prescaler = pcomm_rx_read_uint16_t(p, 1);
			control->pwm_compare = pcomm_rx_read_uint16_t(p, 3);
			control->pwm_autoreload = pcomm_rx_read_uint16_t(p, 5);
			pcomm_tx_put_data_frame(p, &p->rx.buff[p->rx.datastart], 1);
		}else{
			pcomm_tx_put_data_frame(p, (uint8_t *)cmd_error, 3);
		}
		break;
	}
	case _CMD_PWM_STOP_ON_CH1:{
		control->control_status &= 0b1101111111111111;
		pcomm_tx_put_data_frame(p, &p->rx.buff[p->rx.datastart], 1);
		break;
	}
	/*   ·······················   REPORTES e INFO   ······················· */
	// Keep alive
	case _CMD_KEEP_ALIVE:{
		pcomm_tx_put_data_frame(p, &p->rx.buff[p->rx.datastart], 1);
		break;
	}
	// Firmware version
	case _CMD_FIRMWARE_VERSION:{
		uint8_t firmware_version_size = 0;
		while(firmware_version[firmware_version_size++]);
		pcomm_tx_put_cmd_frame(p, _CMD_FIRMWARE_VERSION, (uint8_t *)firmware_version, firmware_version_size-1);
		break;
	}

	// Comando desconocido
	default:{
		pcomm_tx_put_data_frame(p, (uint8_t *)cmd_error, 3);
		break;
	}
	}
	pcomm_rx_data_flush(p);
}
