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
	// Firmware version
	case _CMD_START_ADC_ON_CH1:{
		pcomm_tx_put_cmd_frame(p, _CMD_START_ADC_ON_CH1, (uint8_t *)(&(control->bf_adc_ch1[control->bf_adc_ch1_ir])), 1);
		control->bf_adc_ch1_iw = 0;
		break;
	}
	case _CMD_START_ADC_ON_CH2:{
		pcomm_tx_put_cmd_frame(p, _CMD_START_ADC_ON_CH2, (uint8_t *)(&(control->bf_adc_ch2[control->bf_adc_ch2_ir])), 1);
		control->bf_adc_ch2_iw = 0;
		break;
	}
	/*   ·····································································*/
	/*   ·······················   REPORTES DE INFO   ······················· */
	/*   ·····································································*/
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
