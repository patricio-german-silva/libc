/*
 * pcomm.c
 *
 *  Created on: Oct 3, 2022
 *      Author: psilva
 *
 *  Updated on: Jul 08, 2024:
 *  Version: 20
 *
 *  History:
 *  Jul 08, 2024:
 *      Arbitrary heaader, 0x36, 0xe2, 0xb3, 0x14, where 0x14 is version (20)
 *
 *  Feb 12, 2023:
 *      Added data type managment
 *		pcomm_rx_read_uint8_t
 *		pcomm_rx_read_int8_t
 *		pcomm_rx_read_uint16_t
 *		pcomm_rx_read_int16_t
 *		pcomm_rx_read_uint32_t
 *		pcomm_rx_read_int32_t
 *		pcomm_rx_read_char
 *		pcomm_rx_read_float
 *
 *		pcomm_tx_put_uint8_t
 *		pcomm_tx_put_int8_t
 *		pcomm_tx_put_uint16_t
 *		pcomm_tx_put_int16_t
 *		pcomm_tx_put_uint32_t
 *		pcomm_tx_put_int32_t
 *		pcomm_tx_put_char
 *		pcomm_tx_put_float
 *
 *  Nov 24, 2022:
 *      Added pcomm_tx_put_cmd_frame
 *
 *  Nov 8, 2022:
 *  	pcomm_rx_receive_array len parameter is no longer a pointer
 *
 *  Nov 3, 2022:
 *      Fix _checkframe, solo retorna 1 la primera vez
 *
 *  Nov 2, 2022:
 *      Fix _checkframe, case [0..5] volvia a estado 0 y luego hacia == para saber si se descarta o no ¬¬
 *      Added checkframe_active
 *
 */
#include "pcomm.h"

// Formato del header
static const uint8_t _header[] = {0x36, 0xe2, 0xb3, 0x14, 0x00, ':'};
static const uint8_t _header_size = 6;



/*
 * Porototipos privados
 */
static uint8_t _checkframe(_pcomm *p);


/*
 * Inicializa el protocolo
 */
void pcomm_init(_pcomm *p, uint8_t mode, uint8_t *rx_buff, uint16_t rx_size, uint8_t *tx_buff, uint16_t tx_size, uint32_t tb){
    p->rx.buff = rx_buff;
    p->rx.iw = 0;
    p->rx.ir = 0;
    p->rx.imask = rx_size - 1;
    p->rx.status = 0;
    p->rx.tout_base = tb;
    p->rx.tout_value = 0;
    p->rx.dataready = 0;
    p->rx.datastart = 0;
    p->rx.busy = 0;
    p->rx.chksum = 0;
    p->rx.checkframe_active = 0;
    p->rx.malformed_frame_errors= 0;
    p->rx.chksum_errors = 0;
    p->rx.timeout_errors = 0;
    p->tx.buff = tx_buff;
    p->tx.iw = 0;
    p->tx.ir = 0;
    p->tx.imask = tx_size - 1;
    p->tx.dataready = 0;
    p->tx.busy = 0;
    p->tx.chksum= 0;
    p->mode = mode;
}


/*   -----------  ENTRADA  ---------------*/

/*
 * Ingreso de datos al buffer de entrada
 */
void pcomm_rx_receive_array(_pcomm *p, uint8_t *b, const uint32_t len){
    p->rx.tout_value = 0;
    for(uint32_t i = 0; i < len; i++){
        p->rx.buff[p->rx.iw++] = b[i];
        p->rx.iw &= p->rx.imask;
    }
}

void pcomm_rx_receive_byte(_pcomm *p, uint8_t b){
    p->rx.tout_value = 0;
    p->rx.buff[p->rx.iw++] = b;
    p->rx.iw &= p->rx.imask;
}

/*
 * Retorna 1 si se recibió un frame de datos correcto, 0 en caso contrario
 * No procesa nuevos bytes hasta que no se ha leído el ultimo frame de datos
 * Tambien incrementa el timeout
 */
uint8_t pcomm_rx_data_ready(_pcomm *p){
    if(p->rx.dataready)
        return 1;
    else{
        p->rx.tout_value++;
        return _checkframe(p);
    }
}


/*
 * Se termino de leer datos de entrada
 */
void pcomm_rx_data_flush(_pcomm *p){
    p->rx.dataready = 0;
    p->rx.busy = 0;
}


/*   -----------  SALIDA  ---------------*/

/*
 * Ingreso de un header al buffer de salida
 * Tener en cuenta que p->tx.dataready solo se actualiza en pcomm_tx_put_chksum o pcomm_tx_put_data_frame
 */
void pcomm_tx_put_header(_pcomm *p, uint8_t size){
    p->tx.chksum = 0;
    for(int i = 0; i < _header_size-2; i++){
        p->tx.buff[p->tx.iw++] = _header[i];
        p->tx.iw &= p->tx.imask;
        p->tx.chksum ^= _header[i];
    }
    p->tx.buff[p->tx.iw++] = size;
    p->tx.iw &= p->tx.imask;
    p->tx.buff[p->tx.iw++] = _header[_header_size-1];
    p->tx.iw &= p->tx.imask;
    p->tx.chksum ^= size;
    p->tx.chksum ^= _header[_header_size-1];
}


/*
 * Ingreso de un byte al buffer de salida
 */
void pcomm_tx_put_data(_pcomm *p, uint8_t data){
    p->tx.buff[p->tx.iw++] = data;
    p->tx.iw &= p->tx.imask;
    p->tx.chksum ^= data;
}

/*
 * Ingreso in array de datos
 */
void pcomm_tx_put_data_array(_pcomm *p,  const uint8_t *data, const uint8_t size){
    for(uint8_t i = 0; i < size; i++){
        p->tx.buff[p->tx.iw++] = data[i];
        p->tx.iw &= p->tx.imask;
        p->tx.chksum ^= data[i];
    }
}


/*
 * Agrego checksum al buffer de salida, el frame está completo
 */
void pcomm_tx_put_chksum(_pcomm *p){
    p->tx.buff[p->tx.iw++] = p->tx.chksum;
    p->tx.iw &= p->tx.imask;
    if(p->tx.iw > p->tx.ir)
        p->tx.dataready = p->tx.iw - p->tx.ir;
    else
        p->tx.dataready = p->tx.imask - p->tx.ir + p->tx.iw + 1;
}


/*
 * Ingreso de un frame completo al buffer de salida, con *data datos de tamaño size
 */
void pcomm_tx_put_data_frame(_pcomm *p, const uint8_t *data, const uint8_t size){
    pcomm_tx_put_header(p, size);
    for(uint8_t i = 0; i < size; i++){
        p->tx.buff[p->tx.iw++] = data[i];
        p->tx.iw &= p->tx.imask;
        p->tx.chksum ^= data[i];
    }
    pcomm_tx_put_chksum(p);
}

/*
 * Ingreso de un frame de comando al buffer de salida, un comando de 1 byte al principio y *data datos de tamaño datasize
 */
void pcomm_tx_put_cmd_frame(_pcomm *p, const uint8_t cmd, const uint8_t *data, const uint8_t datasize){
    pcomm_tx_put_header(p, datasize+1);
    p->tx.buff[p->tx.iw++] = cmd;
    p->tx.iw &= p->tx.imask;
    p->tx.chksum ^= cmd;
    for(uint8_t i = 0; i < datasize; i++){
        p->tx.buff[p->tx.iw++] = data[i];
        p->tx.iw &= p->tx.imask;
        p->tx.chksum ^= data[i];
    }
    pcomm_tx_put_chksum(p);
}


/*
 * Si hay un frame listo a ser enviado en el buffer de salida retorna su tamaño, 0 en caso contrario
 */
uint16_t pcomm_tx_data_ready(_pcomm *p){
    return p->tx.dataready;
}


/*
 * Se termino de enviar datos en buffer de salida
 * Esta funcion solo tiene sentido para _PCOMM_TX_MODE_BATCH
 */
void pcomm_tx_data_flush(_pcomm *p){
    p->tx.dataready = 0;
    p->tx.busy = 0;
    if(p->mode == _PCOMM_TX_MODE_BATCH){
        p->tx.iw = 0;
        p->tx.ir = 0;
    }
}





/*  LECTURA DE TIPOS DE DATOS DESDE EL BUFFER - EMISOR Y RECEPTOR DEBEN MANEJAR LA MISMA ESTRUCTURA DE TIPOS Y ENDIANNESS  */
#ifdef _PCOMM_SUPPORT_DATATYPES

static union _work{
    uint8_t u8[4];
    int8_t i8[4];
    uint16_t u16[2];
    int16_t i16[2];
    uint32_t u32;
    int32_t i32;
    char c[4];
    float f;
} _w;

uint8_t pcomm_rx_read_uint8_t(_pcomm *p, uint16_t offset){
    return p->rx.buff[(p->rx.datastart + offset) & p->rx.imask];
}

int8_t pcomm_rx_read_int8_t(_pcomm *p, uint16_t offset){
    _w.u8[0] = p->rx.buff[(p->rx.datastart + offset) & p->rx.imask];
    return _w.i8[0];
}

uint16_t pcomm_rx_read_uint16_t(_pcomm *p, uint16_t offset){
    for(uint8_t i = 0; i < 2; i++)
        _w.u8[i] = p->rx.buff[(p->rx.datastart + offset + i) & p->rx.imask];
    return _w.u16[0];
}

int16_t pcomm_rx_read_int16_t(_pcomm *p, uint16_t offset){
    for(uint8_t i = 0; i < 2; i++)
        _w.u8[i] = p->rx.buff[(p->rx.datastart + offset + i) & p->rx.imask];
    return _w.i16[0];
}

uint32_t pcomm_rx_read_uint32_t(_pcomm *p, uint16_t offset){
    for(uint8_t i = 0; i < 4; i++)
        _w.u8[i] = p->rx.buff[(p->rx.datastart + offset + i) & p->rx.imask];
    return _w.u32;
}

int32_t pcomm_rx_read_int32_t(_pcomm *p, uint16_t offset){
    for(uint8_t i = 0; i < 4; i++)
        _w.u8[i] = p->rx.buff[(p->rx.datastart + offset + i) & p->rx.imask];
    return _w.i32;
}

char pcomm_rx_read_char(_pcomm *p, uint16_t offset){
    _w.u8[0] = p->rx.buff[(p->rx.datastart + offset) & p->rx.imask];
    return _w.c[0];
}

float pcomm_rx_read_float(_pcomm *p, uint16_t offset){
    for(uint8_t i = 0; i < 4; i++)
        _w.u8[i] = p->rx.buff[(p->rx.datastart + offset + i) & p->rx.imask];
    return _w.f;
}

void pcomm_tx_put_uint8_t(_pcomm *p, uint8_t data){
    pcomm_tx_put_data(p, data);
}

void pcomm_tx_put_int8_t(_pcomm *p, int8_t data){
    _w.i8[0] = data;
    pcomm_tx_put_data(p, _w.u8[0]);
}

void pcomm_tx_put_uint16_t(_pcomm *p, uint16_t data){
    _w.u16[0] = data;
    pcomm_tx_put_data_array(p, _w.u8, 2);
}

void pcomm_tx_put_int16_t(_pcomm *p, int16_t data){
    _w.i16[0] = data;
    pcomm_tx_put_data_array(p, _w.u8, 2);
}

void pcomm_tx_put_uint32_t(_pcomm *p, uint32_t data){
    _w.u32 = data;
    pcomm_tx_put_data_array(p, _w.u8, 4);
}

void pcomm_tx_put_int32_t(_pcomm *p, int32_t data){
    _w.i32 = data;
    pcomm_tx_put_data_array(p, _w.u8, 4);
}

void pcomm_tx_put_char(_pcomm *p, char data){
    _w.c[0] = data;
    pcomm_tx_put_data(p, _w.u8[0]);
}

void pcomm_tx_put_float(_pcomm *p, float data){
    _w.f = data;
    pcomm_tx_put_data_array(p, _w.u8, 4);
}

#endif





/*   -------------------------      PRIVADO    -----------------------    */

/*
 * Maquina de estado para rx
 * Retorna 1 si hay un nuevo frame listo, 0 en caso contrario
 * No se empieza a procesar nuevos frames si no se ha leido el ultimo frame
 * p->rx.status:
 * 		[0..5] : corresponde a cada campo del header, 4 es el tamaño del frame
 * 		6 : en bucle de lectura de datos
 * 		7 : esperando checksum, si checksum coincide hay un frame completo y listo
 */
static uint8_t _checkframe(_pcomm *p){
    if(p->rx.checkframe_active == 1) return 0;

    if(p->rx.ir == p->rx.iw){
        if(p->rx.tout_value > p->rx.tout_base){
            p->rx.tout_value = 0;
            p->rx.status = 0;
            p->rx.chksum = 0;
            p->rx.timeout_errors++;
        }
        return 0;
    }
    p->rx.tout_value = 0;
    p->rx.checkframe_active = 1;
    while(p->rx.ir != p->rx.iw && p->rx.dataready == 0){
        switch(p->rx.status){
            case 0:
            case 1:
            case 2:
            case 3:
            case 5:{
                if(_header[p->rx.status] == p->rx.buff[p->rx.ir]){
                    p->rx.chksum ^= p->rx.buff[p->rx.ir];
                    p->rx.status++;
                    p->rx.ir++;
                    p->rx.ir &= p->rx.imask;
                }else{
                    if(p->rx.status == 0){ // Reproceso el byte actual a menos que sea estado 0, donde si no es una U se descarta
                        p->rx.ir++;
                        p->rx.ir &= p->rx.imask;
                    }else{
                        p->rx.malformed_frame_errors++;
                    }
                    p->rx.status = 0;
                    p->rx.chksum = 0;
                }
                break;
            }
            case 4:{
                p->rx.datasize = p->rx.buff[p->rx.ir];
                p->rx.chksum ^= p->rx.buff[p->rx.ir];
                p->rx.datasize_dec = p->rx.datasize;
                p->rx.datastart = p->rx.ir+2;
                p->rx.ir++;
                p->rx.ir &= p->rx.imask;
                p->rx.status++;
                break;
            }
            case 6:{
                p->rx.datasize_dec--;
                p->rx.chksum ^= p->rx.buff[p->rx.ir];
                p->rx.ir++;
                p->rx.ir &= p->rx.imask;
                if(p->rx.datasize_dec == 0){
                    p->rx.status++;
                }
                break;
            }
            case 7:{
                if(p->rx.chksum == p->rx.buff[p->rx.ir]){ // Se recibió un frame correcto
                    p->rx.ir++;
                    p->rx.ir &= p->rx.imask;
                    p->rx.dataready = 1;
                }else{
                    p->rx.chksum_errors++;
                }
                // Si no está correcto se reprocesa el byte actual
                p->rx.status = 0;
                p->rx.chksum = 0;
                break;
            }
        }
    }
    p->rx.checkframe_active = 0;
    return p->rx.dataready;
}
