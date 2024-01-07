/*
 * pcomm.h
 *
 *  Created on: Oct 3, 2022
 *      Author: psilva
 *
 *  Updated on: Feb 12, 2023:
 *  Version: 19
 *
 *
 *  History: see pcomm.c
 */

#ifndef INC_PCOMM_H_
#define INC_PCOMM_H_

#include "stdint.h"

/* En el modo de envio sequencial el buffer tx es circular
 * En el modo de envio batch el metodo que hace en envio de datos lo hace de
 * a varios bytes, el buffer tx no es circular sino que siempre contiene un
 * frame que inicia en [0]
 */
#define _PCOMM_TX_MODE_SEQ 0
#define _PCOMM_TX_MODE_BATCH 1


/*
 * Si está definido, se incluyen las funciones de lectura y escritura de tipos de datos int, char y float
 * emisor y receptor deben manejar los mismos tipos standard de datos y endianness
 */
#define _PCOMM_SUPPORT_DATATYPES


/*
 * Estrucutras de datos
 */

/*
 * Si tout_value alcanza el valor de tout_base se produce un timeout en la lectura
 * Significa que el frame no se completo en ese tiempo.
 *
 * busy es una bandera exclusiva para uso externo, se pone a 0 tras realizar un flush
 * Se utiliza si se desea que cierta rutina particular sea la unica que envie/reciba o
 * como bandera para bloquear los buffer y evitar por ejemplo que una tarea en una interrupcion
 * agregue datos en un buffer a la vez que otra tarea hace lo mismo
 *
 *
 * imask define el tamaño del buffer, DEBE ser un valor potencia de 2 - 1 (255, 511, 1023, 2047)
 */
typedef struct{
    uint8_t *buff;
    uint16_t ir;
    uint16_t iw;
    uint16_t imask;
    uint32_t tout_base;
    uint32_t tout_value;
    uint8_t chksum;
    uint8_t status;
    uint8_t busy;
    uint8_t checkframe_active;  // Evito que vuelva a entrar si se es llamdo desde una interrupcion
    uint8_t dataready;	// Paquete de datos de entrada listo
    uint8_t datasize;	// Tamaño del paquete de datos de entrada
    uint8_t datasize_dec;	// Contador para procesamiento
    uint16_t datastart;	// Posicion inicial del paquete de datos de entrada
    // Contadores de errores
    uint32_t chksum_errors;
    uint32_t malformed_frame_errors;
    uint32_t timeout_errors;
}_rx_pcomm;

typedef struct{
    uint8_t *buff;
    uint16_t ir;
    uint16_t iw;
    uint16_t imask;
    uint8_t chksum;
    uint16_t dataready;	// Paquete de datos de salida listo.
    uint8_t busy;
}_tx_pcomm;

typedef struct{
    _tx_pcomm tx;
    _rx_pcomm rx;
    uint8_t mode;  // Modo de envio de tx
}_pcomm;



/*
 * Prototipos
 */

/*
 * Inicializa la estructura de datos del protocolo
 * mode es el modo de operacion (_PCOMM_TX_MODE_SEQ o _PCOMM_TX_MODE_BATCH)
 * tb es la base de timeout, si desde que se recibe el primer byte de un nuevo frame la cantidad
 * de llamadas a pcomm_rx_data_ready supera el valor de tb, el frame que se esta recibiendo se descarta.
 * Si las llamadas a pcomm_rx_data_ready son cada 1 ms, un valor adecuado para tb seria 100, es decir, si
 * un frame no se recibe completo dentro de una ventana de 100ms, se descarta
 */
void pcomm_init(_pcomm *p, uint8_t mode, uint8_t *rx_buff, uint16_t rx_size, uint8_t *tx_buff, uint16_t tx_size, uint32_t tb);
void pcomm_rx_receive_array(_pcomm *p, uint8_t *b, const uint32_t len);
void pcomm_rx_receive_byte(_pcomm *p, uint8_t b);
uint8_t pcomm_rx_data_ready(_pcomm *p);
void pcomm_rx_data_flush(_pcomm *p);
void pcomm_tx_put_header(_pcomm *p, uint8_t size);
void pcomm_tx_put_data(_pcomm *p, uint8_t data);
void pcomm_tx_put_data_array(_pcomm *p,  const uint8_t *data, const uint8_t size);
void pcomm_tx_put_chksum(_pcomm *p);
void pcomm_tx_put_data_frame(_pcomm *p, const uint8_t *data, const uint8_t size);
void pcomm_tx_put_cmd_frame(_pcomm *p, const uint8_t cmd, const uint8_t *data, const uint8_t datasize);
uint16_t pcomm_tx_data_ready(_pcomm *p);
void pcomm_tx_data_flush(_pcomm *p);



/*  LECTURA DE TIPOS DE DATOS DESDE EL BUFFER - EMISOR Y RECEPTOR DEBEN MANEJAR LA MISMA ESTRUCTURA DE TIPOS Y ENDIANNESS  */
#ifdef _PCOMM_SUPPORT_DATATYPES
// Lectura
uint8_t pcomm_rx_read_uint8_t(_pcomm *p, uint16_t offset);
int8_t pcomm_rx_read_int8_t(_pcomm *p, uint16_t offset);
uint16_t pcomm_rx_read_uint16_t(_pcomm *p, uint16_t offset);
int16_t pcomm_rx_read_int16_t(_pcomm *p, uint16_t offset);
uint32_t pcomm_rx_read_uint32_t(_pcomm *p, uint16_t offset);
int32_t pcomm_rx_read_int32_t(_pcomm *p, uint16_t offset);
char pcomm_rx_read_char(_pcomm *p, uint16_t offset);
float pcomm_rx_read_float(_pcomm *p, uint16_t offset);
// Escritura
void pcomm_tx_put_uint8_t(_pcomm *p, uint8_t data);
void pcomm_tx_put_int8_t(_pcomm *p, int8_t data);
void pcomm_tx_put_uint16_t(_pcomm *p, uint16_t data);
void pcomm_tx_put_int16_t(_pcomm *p, int16_t data);
void pcomm_tx_put_uint32_t(_pcomm *p, uint32_t data);
void pcomm_tx_put_int32_t(_pcomm *p, int32_t data);
void pcomm_tx_put_char(_pcomm *p, char data);
void pcomm_tx_put_float(_pcomm *p, float data);
#endif


#endif /* INC_PCOMM_H_ */
