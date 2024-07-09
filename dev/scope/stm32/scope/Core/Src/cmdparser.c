/*
 * cmdparser.c
 *
 *  Created on: Oct 7, 2022
 *      Author: psilva
 */
#include "cmdparser.h"

extern const char firmware_version[];
static const char *cmd_error = "ERR";


static _pcomm *cdc;
static _usrtick *usrtick;
static _hb *hb;


/*
 *inicializa las estructuras de datos
 */
void cmd_init(_usrtick *ut, _hb *h, _pcomm *p){
	usrtick = ut;
	hb = h;
	cdc = p;
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
