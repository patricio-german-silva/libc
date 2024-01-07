/*
 * esp8266_transport.h
 *
 *  Created on: Oct 4, 2022
 *      Author: psilva
 *
 * conexion 802.11 mediante modulo esp8266
 * Conexión wireless en modo 1,2 o 3
 * Hasta 4 conexiones tcp/udp, CIPMUX=1
 *
 */

#ifndef INC_ESP8266_TRANSPORT_H_
#define INC_ESP8266_TRANSPORT_H_

#include "stdint.h"
#include "stddef.h"


/*
 * Definicon de los valores de status
 * 			0: DOWN - No se utiliza el modulo
 * 			3: Establecido wifi mode
 * 			4: conectado a ap
 * 			5: obtenidos los datos de IP
 * 			6: conexión tcp/udp establecida
 * 			7: prompt > para enviar datos
 * 			8: datos enviados OK - volver a 6
 * 			9: tcp/udp cerrada
 * 			10: desconectado de AP - volver a 3
 * 			11: RESTORE Ok, volver a 0 para reiniciar
 */
#define _ESPT_STATUS_DOWN						0
#define _ESPT_STATUS_WIFI_MODE_ESTABLISHED 		3
#define _ESPT_STATUS_AP_CONNECTED				4
#define _ESPT_STATUS_IP_DATA_ACQUIRED			5
#define _ESPT_STATUS_TCP_UDP_ESTABLISHED		6
#define _ESPT_STATUS_SEND_DATA_PROMPT_RECEIVED	7
#define _ESPT_STATUS_SOCKET_SEND_OK				8
#define _ESPT_STATUS_TCP_UDP_CLOSED				9
#define _ESPT_STATUS_AP_DISCONNECTED			10
#define _ESPT_STATUS_RESTORE_OK					11

#define _ESPT_WIFI_STATION_MODE					1
#define _ESPT_WIFI_SOFT_AP_MODE					2
#define _ESPT_WIFI_SOFT_AP_AND_STATION_MODE		3

/*
 * Definicion de punteros a funcion
 * 1 - Funcion para ingresar datos recibidos mediante el comnado AT +IPD
 * 2 - Funcion para ingresar datos de control (Ej: resultado de un scan de redes disponiples)
 * 3 - Funcion de activación del pin chip_enable, status 1 -> enable chip, status 0 -> disable chip
 */
typedef void (*espt_receive_def)(uint8_t *data, uint8_t len);
typedef void (*espt_chipenable_def)(uint8_t status);


/*
 * Buffer de recepcion desde el modulo esp8266
 * RX es buffer circular
 */
typedef struct{
	uint8_t buff[256];
	uint8_t iw;
	uint8_t ir;
	uint32_t tout_base;
	uint32_t tout_value;
}_rx_espt;


/*
 * Buffer de envio al modulo esp8266
 * TX no es buffer circular
 * status:
 * 		0: Buffer libre para incorporar datos
 * 		1: Se solicitó enviar los datos en buffer
 * 		2: Los datos en buffer están siendo enviados, no se puede agragar datos
 */
typedef struct{
	uint8_t buff[256];
	uint8_t iw;
	uint8_t status;
	uint32_t tout_base;		// tiempo inactivo considerado timeout, depende de cada comando
	uint32_t tout_value;
	uint8_t *send_buffer;	// Buffer temporal durante un envio de datos
	uint8_t send_buffer_len;
}_tx_espt;


/*
 * Informacion referida a la conexión tcp/udp
 */
typedef struct{

}_espt_trconn;


/*
 * Parsea los datos recibidos segun lo esperado
 *
 *
 * expected:0: no se espera nada. Se descarta lo que hay en el buffer
 * 			1: IDLE.
 * 			2: Recibiendo datos "+IPD,"
 * 			3: solicitado set wifi mode, esperando OK\r\n
 * 			4: conectando a ap, esperando OK\r\n
 * 			5: Solicitado AT+CIFSR, esperando datos para parsear
 * 			6: tcp/udp open, esperando OK\r\n
 * 			7: Enviar datos, esperando prompt >
 * 			8: Datos enviados, esperando SEND OK\r\n
 * 			9: tcp/udp close, esperando n,CLOSED\r\n
 * 			10: solicitado desconectar de ap, esperando OK\r\nWIFI DISCONNECT
 * 			11: solicitado AT+RESTORE, esperando OK\r\n y paso a expected 0 (descartar el dump de configuración tras espera 500ms)
 *
 * status:
 * 			0: DOWN - No se utiliza el modulo
 * 			1: IDLE - No se hace nada
 * 			3: Establecido wifi mode
 * 			4: conectado a ap
 * 			5: obtenidos los datos de IP
 * 			6: conexión tcp/udp establecida
 * 			7: prompt > para enviar datos
 * 			8: datos enviados OK - volver a 5
 * 			9: tcp/udp cerrada
 * 			10: desconectado de AP - volver a 1
 * 			11: RESTORE Ok, volver a 0 para reiniciar
 *
 * 	alive:	Comando recurrente con lo otros, tiene su propia bandera de estado.
 * 			0: no alive rq
 * 			1: alive rq
 * 			2: alive ok
 * 			3: alive timeout
 *
 */
typedef struct{
	uint8_t wmode;
	uint8_t ap_ipaddr[4];
	uint8_t src_ipaddr[4];
	  // 1: tcp, 2: udp, 0: none
	uint8_t transport;
	// 0: CLOSED, 1: SYNC_SEND, 2: ESTABLISHED, 3: CLOSE_WAIT
	uint8_t transport_status;
	uint16_t dst_port;
	uint8_t dst_ipaddr[4];
	uint16_t src_port;
	uint8_t expected;
	uint8_t status;
	uint8_t parser;
	uint8_t alive;
	uint8_t wait;		// Esperar a que sea 0 antes de enviar, se actualiza decremental, cada estado setea un valor diferente para esta espera
	uint8_t tout;		// Timeout counter
	_tx_espt tx;
	_rx_espt rx;

	// Punteros a funciones para comunicación con el usuario de la libreria
	espt_receive_def func_write_data;
	espt_receive_def func_write_control;
	espt_chipenable_def func_chipenable;

	// Misc
	char ipd_tmpchar[10];
	uint8_t ipd_tmpint;
}_espt;


/*
 * Prototipos
 */
void espt_init(_espt *e, uint8_t wmode);
void espt_wakeup(_espt *e);
void espt_sleep(_espt *e);
uint8_t espt_set_wifi_mode(_espt *e);
uint8_t espt_status(_espt *e);
uint8_t espt_alive(_espt *e);
uint8_t espt_restore(_espt *e);
uint8_t espt_list_available_ap(_espt *e);
uint8_t espt_ap_connect(_espt *e, const char *ssid, const char *passwd);
uint8_t espt_ap_disconnect(_espt *e);
uint8_t* espt_get_apaddr(_espt *e);
uint8_t* espt_get_srcaddr(_espt *e);
uint8_t espt_socket_open(_espt *e, uint8_t transport, const char *dst_ipaddr, uint16_t dst_port, uint16_t src_port);
uint8_t espt_socket_send(_espt *e, uint8_t *buff, uint8_t Len);
uint8_t espt_socket_close(_espt *e);
void espt_work(_espt *e);  // El procesamiento se realiza mediante las recurrentes llamadas a este método
void espt_update_tout(_espt *e);
void espt_attach_receive_data(_espt *e , espt_receive_def f);
void espt_attach_receive_control(_espt *e, espt_receive_def f);
void espt_attach_chip_enable(_espt *e, espt_chipenable_def f);


#endif /* INC_ESP8266_TRANSPORT_H_ */
