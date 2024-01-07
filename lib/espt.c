/*
 * espt.c
 *
 * MAYOR 2
 *  Created on: Feb 23, 2023
 *      Author: psilva
 *
 *  Version: 2.0
 *
 *  TODO:
 *  - add proper TCP connection
 *  - add MUX connection
 *  - add passthrough capabilty
 *
 */

#include "espt.h"


/*
 * Constantes de AT - Privado - Cadenas terminadas en \0
 */
static const char *at_alive = "AT\r\n\0";
static const char *at_wifi_mode_soft_ap = "AT+CWMODE=1\r\n\0";
static const char *at_wifi_mode_station = "AT+CWMODE=2\r\n\0";
static const char *at_wifi_mode_soft_ap_and_station = "AT+CWMODE=3\r\n\0";
static const char *at_ap_connect = "AT+CWJAP=\"\0";
static const char *at_query_local_address = "AT+CIFSR\r\n\0";
static const char *at_list_available_ap = "AT+CWLAP\r\n\0";
static const char *at_socket_udp_text = "\"UDP\",\0";
static const char *at_socket_tcp_text = "\"TCP\",\0";
static const char *at_socket_open = "AT+CIPSTART=\0";
static const char *at_socket_send = "AT+CIPSEND=\0";
static const char *at_socket_close = "AT+CIPCLOSE\r\n\0";
static const char *at_ap_disconnect = "AT+CWQAP\r\n\0";
static const char *at_restore = "AT+RESTORE\r\n\0";
static const char *at_autoconn_disable = "AT+CWAUTOCONN=0\r\n\0";
static const char *at_autoconn_enable = "AT+CWAUTOCONN=1\r\n\0";

// Respuestas de AT
static const char *at_ok = "OK\r\n\0";
static const char *at_cipstart_already_connected = "ALREADY CONNECTED\r\n\r\nERROR\r\n\0"; // Respuesta a AT+CIPSTART si ya esta conectado
static const char *at_ipd = "+IPD,\0";
static const char *at_cifsr_apip = "+CIFSR:APIP,\"\0";
static const char *at_cifsr_staip = "+CIFSR:STAIP,\"\0";
static const char *at_send_ok = "SEND OK\r\n\0";
static const char *at_closed = "CLOSED\r\n\r\nOK\r\n\0";
static const char *at_ok_wifi_disconnect = "OK\r\nWIFI DISCONNECT\r\n\0";
static const char *at_wifi_disconnect = "WIFI DISCONNECT\r\n\0"; // Resupesta a AT+CWQAP cuando ya esta desconectado, sino responde "OK\r\nWIFI DISCONNECT\r\n\0"
static const char *at_wifi_connect_fail = "FAIL\r\n\0";





/*
 * Prototipos privados
 */
static uint8_t _ipd_manage(_espt *e);
static void _prompt_manage(_espt *e);
static void _auto_manage(_espt *e);
static uint8_t _rx_buffer_pop(_espt *e, const char *str1, const char *str2, const char *err);
static void _parse_address(_espt *e);
static void _set_status(_espt *e, uint8_t flag);
static void _reset_status(_espt *e, uint8_t flag);


/* UTILIDADES LOCALES */
static uint8_t _strcmp(const uint8_t *str1, uint32_t ir, const uint32_t iw, const uint32_t buflen, const char *str2);
static uint32_t _strlen(const char *str, uint32_t max);
static uint16_t _cut(const char *str, char separator, uint8_t field, char *res, uint16_t max_size);
static uint8_t _to_uint16(const char *str, uint16_t *number, uint8_t max_digits);
static uint8_t _to_str(uint16_t number, char *str);
static uint32_t _strcpy(const char *src, char *dst, uint32_t max);
static uint8_t _char_ipv4_to_uint8_array(const char *ipc, uint8_t *ipa);
static uint8_t _uint8_array_to_char_ipv4(const uint8_t *ipa, char *ipc);
static uint8_t _check_ipv4_is_valid(const uint8_t o1, const uint8_t o2, const uint8_t o3, const uint8_t o4);



/*
 * Inicializo valores precargados en el struct _espt
 * Ejecuto la funcion para habilitar el chip
 */
void espt_init(_espt *e){
    e->rx.ir = 0;
    e->rx.iw = 0;
    e->rx.imask = _ESPT_RX_BUFFER_SIZE-1;
    e->rx.toutcntr = 0;
    e->tx.iw = 0;
    e->tx.toutcntr = 0;
    e->wmode = _ESPT_WIFI_STATION_MODE;
    e->operation_mode = _ESPT_OPERATION_MODE_MANUAL;
    e->dst_port = 0;
    e->src_port = 0;
    e->transport = 0;
    e->status = 0;
    _set_status(e, _ESPT_STATUS_DOWN);
    e->process = _ESPT_STATUS_DOWN;
    e->busy = 0;
}




void espt_set_wifi_mode(_espt *e, uint8_t mode){
    e->wmode = mode;
}

void espt_set_operation_mode(_espt *e, uint8_t mode){
    e->operation_mode = mode;
}

void espt_set_SSID(_espt *e, const char *ssid){
    _strcpy(ssid,  e->SSID, 32);
}

void espt_set_WPA_PSK(_espt *e, const char *wpa_psk){
    _strcpy(wpa_psk, e->WPA_PSK, 63);
}

void espt_set_dst_ipaddr(_espt *e, const uint8_t *dst_ip){
    e->dst_ipaddr[0] = dst_ip[0];
    e->dst_ipaddr[1] = dst_ip[1];
    e->dst_ipaddr[2] = dst_ip[2];
    e->dst_ipaddr[3] = dst_ip[3];
} // :P

void espt_set_transport(_espt *e, uint8_t transport){
    e->transport = transport;
}

void espt_set_dst_port(_espt *e, uint16_t port){
    e->dst_port = port;
}

void espt_set_src_port(_espt *e, uint16_t port){
    e->src_port = port;
}

void espt_connect(_espt *e){
    e->operation_mode = _ESPT_OPERATION_MODE_AUTO;
}

void espt_disconnect(_espt *e){
    e->operation_mode = _ESPT_OPERATION_MODE_CLOSED;
}

void espt_reconnect(_espt *e){
    e->operation_mode = _ESPT_OPERATION_MODE_RECONNECT;
}

void espt_wakeup(_espt *e){
    e->func_chipenable(1);
    e->process = _ESPT_STATUS_IDLE;
    e->status = 0;
    _set_status(e, _ESPT_STATUS_IDLE);
    e->busy = _ESPT_WAIT_ON_STARTUP;
}

void espt_sleep(_espt *e){
    e->func_chipenable(0);
    e->status = 0;
    _set_status(e, _ESPT_STATUS_DOWN);
}


/*
 * Attach de las funciones de entrada correspondientes
 */
void espt_attach_receive_data(_espt *e, espt_receive_def f){
    e->func_write_data_to_user = f;
}

void espt_attach_receive_control(_espt *e , espt_receive_def f){
    e->func_write_control_data_to_user = f;
}

void espt_attach_send_data(_espt *e, espt_receive_def f){
    e->func_write_data_to_esp = f;
}

void espt_attach_chip_enable(_espt *e, espt_chipenable_def f){
    e->func_chipenable = f;
}


/*
 * estado actual del espt
 */
uint8_t espt_status(_espt *e, uint8_t flag){
    return ((e->status & (1<<flag)) != 0);
}

uint8_t espt_is_busy(_espt *e){
    return ((e->status & (1<<_ESPT_STATUS_IDLE)) == 0);
}

void espt_error_reset(_espt *e){
    _reset_status(e, _ESPT_STATUS_ERROR);
}

void espt_send_data_finished(_espt *e){
    e->tx.iw = 0;   // El indice en 0 indica que se termino de enviar
}


/*
 * Se ingresa datos raw recibidos desde el espt, por ejemplo desde el puerto usart conectado al esp
 */
void espt_from_esp_received(_espt *e, uint8_t *buff, uint32_t Len){
    for(uint32_t i = 0; i<Len; i++){
        e->rx.buff[e->rx.iw++] = buff[i];
        e->rx.iw &= e->rx.imask;
    }
}

/*
 * Parsea los datos recibidos segun lo esperado
 * @param period es la cantidad de tiempo en ms entre llamados a work, utilizado para manejar los timeout
 *
 */
void espt_work(_espt *e, uint16_t period){

    // Manejo de estados automaticos
    _auto_manage(e);

    if(espt_status(e, _ESPT_STATUS_DOWN)) return;   // Si estado es _ESPT_STATUS_DOWN no hay nada mas que procesar

    if(e->busy > period)
        e->busy -= period;
    else
        e->busy = 0;

    uint8_t cmp = 0;
    uint8_t break_while = 0;

    /* Estoy enviando datos, no proceso nada entrante (No deberia haber nada) */
    if(espt_status(e, _ESPT_STATUS_SEND_DATA_PROMPT_RECEIVED)){
        _prompt_manage(e);
        break_while = 1;
    }

    if(e->busy == 0 && e->process != _ESPT_STATUS_IDLE){    // Vencio tiempo de espera de alguna tarea
        if(e->tx.iw != 0){
            e->tx.iw = 0;
            e->tx.toutcntr++;
        }
        if(e->rx.ir == e->rx.iw){
            e->rx.toutcntr++;
        }
        _set_status(e, _ESPT_STATUS_IDLE);
        e->process = _ESPT_STATUS_IDLE;
    }

    /*
     * Proceso buffer de entrada
     */
    while(e->rx.ir != e->rx.iw && break_while == 0){
        switch(e->process){
        case _ESPT_STATUS_IDLE:{
            cmp = _rx_buffer_pop(e, at_ipd, "\0", "\0");
            if(cmp == 0){
                e->process = _ESPT_STATUS_RECEIVING_DATA;
                _reset_status(e, _ESPT_STATUS_IDLE);
                e->ipd_char_index = 0;
                e->busy = 250;
                e->parser = 0;
            }else if(cmp == 2)
                    break_while = 1;
            break;
        }
        case _ESPT_STATUS_RECEIVING_DATA:{
            break_while = _ipd_manage(e);
            break;
            }

        case _ESPT_STATUS_RESTORE_OK:
        case _ESPT_STATUS_WIFI_MODE_ESTABLISHED:
        case _ESPT_STATUS_SET_AUTOCONN_OK:{
            cmp = _rx_buffer_pop(e, at_ok, "\0", "\0");
            if(cmp == 0){
                // Caso especial
                if(e->process == _ESPT_STATUS_RESTORE_OK)
                    e->status = 0;
                _set_status(e, e->process);
                _set_status(e, _ESPT_STATUS_IDLE);
                e->process = _ESPT_STATUS_IDLE;
                e->busy = 100;
            }else if(cmp == 2)
                    break_while = 1;
            break;
        }
        case _ESPT_STATUS_AP_CONNECTED:{
            cmp = _rx_buffer_pop(e, at_ok, "\0", at_wifi_connect_fail);
            if(cmp == 0){
                _set_status(e, _ESPT_STATUS_AP_CONNECTED);
                _set_status(e, _ESPT_STATUS_IDLE);
                e->process = _ESPT_STATUS_IDLE;
                e->busy = 0;
            }else if(cmp == 3){ // Wrong password
                _set_status(e, _ESPT_STATUS_ERROR);
                _set_status(e, _ESPT_STATUS_IDLE);
                if(_ESPT_ON_ERROR_STATE_OPERATION_MODE)
                    e->operation_mode = _ESPT_ON_ERROR_STATE_OPERATION_MODE;
                e->process = _ESPT_STATUS_IDLE;
                e->busy = 100;
            }else if(cmp == 2)
                    break_while = 1;
            break;
        }
        case _ESPT_STATUS_TCP_UDP_ESTABLISHED:{
            cmp = _rx_buffer_pop(e, at_ok, at_cipstart_already_connected, "\0");
            if(cmp == 0){
                _set_status(e, _ESPT_STATUS_TCP_UDP_ESTABLISHED);
                _set_status(e, _ESPT_STATUS_IDLE);
                e->process = _ESPT_STATUS_IDLE;
                e->busy = 100;
            }else if(cmp == 2)
                    break_while = 1;
            break;
        }
        case _ESPT_STATUS_IP_DATA_ACQUIRED:{
            _parse_address(e);
            break_while = 1;
            break;
        }
        case _ESPT_STATUS_SEND_DATA_PROMPT_RECEIVED:{
            if(!espt_status(e, _ESPT_STATUS_SEND_DATA_PROMPT_RECEIVED)){
                if(e->rx.buff[e->rx.ir++] == '>'){		// Recibi el >
                    _set_status(e, _ESPT_STATUS_SEND_DATA_PROMPT_RECEIVED);
                }
                e->rx.ir &= e->rx.imask;
            }
            break_while = 1;
            break;
        }
        case _ESPT_STATUS_SEND_DATA_OK:{
        cmp =_rx_buffer_pop(e, at_send_ok, "\0", "\0");
        if(cmp == 0){
                _set_status(e, _ESPT_STATUS_SEND_DATA_OK);
                _reset_status(e, _ESPT_STATUS_SEND_DATA_PROMPT_RECEIVED);
                _set_status(e, _ESPT_STATUS_IDLE);
                e->process = _ESPT_STATUS_IDLE;
                e->busy = 0;
        }else if(cmp == 2)
                break_while = 1;
        break;
        }
        case _ESPT_STATUS_CONTROL_DATA_RECEIVED:{
            cmp = _strcmp(e->rx.buff, e->rx.ir, e->rx.iw, _ESPT_RX_BUFFER_SIZE, at_ok);
            if(cmp == 0){
                _set_status(e, _ESPT_STATUS_CONTROL_DATA_RECEIVED);
                _set_status(e, _ESPT_STATUS_IDLE);
                e->process = _ESPT_STATUS_IDLE;
                e->rx.ir += _strlen(at_ok, 255);
                e->rx.ir &= e->rx.imask;
                e->busy = 0;
            }else if(cmp == 1){
                e->func_write_control_data_to_user(&e->rx.buff[e->rx.ir++], 1);
                e->rx.ir &= e->rx.imask;
            }else{  // Faltan ingresar mas datos
                break_while = 1;
            }
            break;
        }
        case _ESPT_STATUS_TCP_UDP_CLOSED:{
            cmp = _rx_buffer_pop(e, at_closed, "\0", "\0");
            if(cmp == 0){
                _set_status(e, _ESPT_STATUS_TCP_UDP_CLOSED);
                _reset_status(e, _ESPT_STATUS_TCP_UDP_ESTABLISHED);
                _set_status(e, _ESPT_STATUS_IDLE);
                e->process = _ESPT_STATUS_IDLE;
                e->busy = 0;
            }else{
                if(cmp == 2){
                    break_while = 1;
                }
            }
            break;
        }
        case _ESPT_STATUS_AP_DISCONNECTED:{
            cmp = _rx_buffer_pop(e, at_ok_wifi_disconnect, at_wifi_disconnect, "\0");
            if(cmp == 0){
                _set_status(e, _ESPT_STATUS_AP_DISCONNECTED);
                _reset_status(e, _ESPT_STATUS_AP_CONNECTED);
                _set_status(e, _ESPT_STATUS_IDLE);
                e->process = _ESPT_STATUS_IDLE;
                e->busy = 0;
            }else{
                if(cmp == 2){
                    break_while = 1;
                }
            }
            break;
        }
        }
    }
    return;
}

/*
 * Establezco wifi mode, retorna 0 si el proceso se inicó, 1 si se está en espera
 */
uint8_t espt_send_wifi_mode(_espt *e){
    if(espt_status(e, _ESPT_STATUS_IDLE)){
        const char *i = NULL;
        switch (e->wmode) {
        case _ESPT_WIFI_SOFT_AP_MODE:
            i = at_wifi_mode_soft_ap;
            break;
        case _ESPT_WIFI_STATION_MODE:
            i = at_wifi_mode_station;
            break;
        default:
            i = at_wifi_mode_soft_ap_and_station;
            break;
        }
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        e->busy = 1000;
        e->process = _ESPT_STATUS_WIFI_MODE_ESTABLISHED;
        _reset_status(e, _ESPT_STATUS_IDLE);
        _reset_status(e, _ESPT_STATUS_WIFI_MODE_ESTABLISHED);
        e->func_write_data_to_esp(e->tx.buff, e->tx.iw);
        return 1;
    }
    return 0;
}




/*
 *  Peticion de alive
 *  Se envia solo si se esta en reposo
 */
uint8_t espt_alive(_espt *e){
    if(espt_status(e, _ESPT_STATUS_IDLE)){
        const char *i = NULL;
        i = at_alive;
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        e->busy = 100;
        e->process = _ESPT_STATUS_ALIVE_OK;
        _reset_status(e, _ESPT_STATUS_IDLE);
        _reset_status(e, _ESPT_STATUS_ALIVE_OK);
        e->func_write_data_to_esp(e->tx.buff, e->tx.iw);
        return 1;
    }
    return 0;
}


uint8_t espt_restore(_espt *e){
    if(espt_status(e, _ESPT_STATUS_IDLE)){
        e->operation_mode = _ESPT_OPERATION_MODE_MANUAL;
        const char *i = NULL;
        i = at_restore;
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        e->busy = 20000;
        e->process = _ESPT_STATUS_RESTORE_OK;
        _reset_status(e, _ESPT_STATUS_IDLE);
        _reset_status(e, _ESPT_STATUS_RESTORE_OK);
        e->func_write_data_to_esp(e->tx.buff, e->tx.iw);
        return 1;
    }
    return 0;
}

uint8_t espt_set_autoconn(_espt *e, uint8_t autoconn){
    if(espt_status(e, _ESPT_STATUS_IDLE)){
        const char *i = NULL;
        if(autoconn == 0)
            i = at_autoconn_disable;
        else
            i = at_autoconn_enable;

        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        e->busy = 5000;
        e->process = _ESPT_STATUS_SET_AUTOCONN_OK;
        _reset_status(e, _ESPT_STATUS_IDLE);
        _reset_status(e, _ESPT_STATUS_SET_AUTOCONN_OK);
        e->func_write_data_to_esp(e->tx.buff, e->tx.iw);
        return 1;
    }
    return 0;
}


/*
 * Solicita lista de redes disponibles
 */
uint8_t espt_list_available_ap(_espt *e){
    if(espt_status(e, _ESPT_STATUS_IDLE)){
        const char *i = NULL;
        i = at_list_available_ap;
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        e->busy = 20000;
        e->process = _ESPT_STATUS_CONTROL_DATA_RECEIVED;
        _reset_status(e, _ESPT_STATUS_IDLE);
        _reset_status(e, _ESPT_STATUS_CONTROL_DATA_RECEIVED);
        e->func_write_data_to_esp(e->tx.buff, e->tx.iw);
        return 1;
    }
    return 0;
}

uint8_t espt_ap_connect(_espt *e){
    if(espt_status(e, _ESPT_STATUS_IDLE) && espt_status(e, _ESPT_STATUS_WIFI_MODE_ESTABLISHED) && !espt_status(e, _ESPT_STATUS_AP_CONNECTED)){
        const char *i = NULL;
        i = at_ap_connect;
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        i = e->SSID;
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        e->tx.buff[e->tx.iw++] = '"';
        e->tx.buff[e->tx.iw++] = ',';
        e->tx.buff[e->tx.iw++] = '"';
        i = e->WPA_PSK;
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        e->tx.buff[e->tx.iw++] = '"';
        e->tx.buff[e->tx.iw++] = '\r';
        e->tx.buff[e->tx.iw++] = '\n';
        e->busy = 20000;
        e->process = _ESPT_STATUS_AP_CONNECTED;
        _reset_status(e, _ESPT_STATUS_IDLE);
        _reset_status(e, _ESPT_STATUS_AP_CONNECTED);
        _reset_status(e, _ESPT_STATUS_AP_DISCONNECTED);
        _reset_status(e, _ESPT_STATUS_IP_DATA_ACQUIRED);
        e->func_write_data_to_esp(e->tx.buff, e->tx.iw);
        return 1;
    }
    return 0;
}

uint8_t espt_ap_disconnect(_espt *e){
    if(espt_status(e, _ESPT_STATUS_IDLE) && espt_status(e, _ESPT_STATUS_AP_CONNECTED)){
        if(e->operation_mode == _ESPT_OPERATION_MODE_AUTO) e->operation_mode = _ESPT_OPERATION_MODE_MANUAL;
        const char *i = NULL;
        i = at_ap_disconnect;
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        e->busy = 10000;
        e->process = _ESPT_STATUS_AP_DISCONNECTED;
        _reset_status(e, _ESPT_STATUS_IDLE);
        _reset_status(e, _ESPT_STATUS_AP_DISCONNECTED);
        _reset_status(e, _ESPT_STATUS_AP_CONNECTED);
        _reset_status(e, _ESPT_STATUS_IP_DATA_ACQUIRED);
        e->func_write_data_to_esp(e->tx.buff, e->tx.iw);
        return 1;
    }
    return 0;
}


/*
 * Devuelve el puntero, please dont modify it
 */
uint8_t* espt_get_apaddr(_espt *e){
    return e->ap_ipaddr;
}

uint8_t espt_get_char_apaddr(_espt *e, char *ipc){
    return _uint8_array_to_char_ipv4(e->ap_ipaddr, ipc);
}

uint8_t* espt_get_srcaddr(_espt *e){
    return e->src_ipaddr;
}

uint8_t espt_get_char_srcaddr(_espt *e, char *ipc){
    return _uint8_array_to_char_ipv4(e->src_ipaddr, ipc);
}

/*
 * Inicia una conexion tcp/udp con la información suministrada
 * Retorna:
 * 			0: si la peticion se pudo iniciar
 * 			1: si la peticion no se pudo iniciar
 *          2: invalid src_port
 *          3: Invalid dst_port
 *          4: invalid transport
 *          5: invalid dst_ipaddr
 *
 */
uint8_t espt_socket_open(_espt *e){
    char tmp_c[6];
    const char *i = NULL;
    char ch_dst_ipaddr[16];
    uint8_t tmpi = 0;

    if(e->src_port == 0) return 2;
    if(e->dst_port == 0) return 3;
    if(e->transport != _ESPT_TRANSPORT_TCP && e->transport != _ESPT_TRANSPORT_UDP) return 4;
    if(!_check_ipv4_is_valid(e->dst_ipaddr[0], e->dst_ipaddr[1], e->dst_ipaddr[2], e->dst_ipaddr[3])) return 5;

    if(espt_status(e, _ESPT_STATUS_IDLE) && espt_status(e, _ESPT_STATUS_AP_CONNECTED) && !espt_status(e, _ESPT_STATUS_TCP_UDP_ESTABLISHED)){
        for(uint8_t i = 0; i < 4; i++){
            tmpi += _to_str(e->dst_ipaddr[i], &ch_dst_ipaddr[tmpi]);
            ch_dst_ipaddr[tmpi++] = '.';
        }
        ch_dst_ipaddr[tmpi-1] = '\0';


        // Cominezo a armar la cadena de conexion
        i = at_socket_open;
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);

        if(e->transport == _ESPT_TRANSPORT_TCP)
            i = at_socket_tcp_text;
        else if(e->transport == _ESPT_TRANSPORT_UDP)
            i = at_socket_udp_text;
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        e->tx.buff[e->tx.iw++] = '"';

        i = ch_dst_ipaddr;
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        e->tx.buff[e->tx.iw++] = '"';
        e->tx.buff[e->tx.iw++] = ',';

        _to_str(e->dst_port, tmp_c);
        i = tmp_c;
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        e->tx.buff[e->tx.iw++] = ',';

        _to_str(e->src_port, tmp_c);
        i = tmp_c;
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        e->tx.buff[e->tx.iw++] = ',';
        e->tx.buff[e->tx.iw++] = '0';
        e->tx.buff[e->tx.iw++] = '\r';
        e->tx.buff[e->tx.iw++] = '\n';

        e->busy = 5000;
        e->process = _ESPT_STATUS_TCP_UDP_ESTABLISHED;
        _reset_status(e, _ESPT_STATUS_IDLE);
        _reset_status(e, _ESPT_STATUS_TCP_UDP_ESTABLISHED);
        _reset_status(e, _ESPT_STATUS_TCP_UDP_CLOSED);
        e->func_write_data_to_esp(e->tx.buff, e->tx.iw);
        return 1;
    }
    return 0;
}


/*
 * Envia por esp los datos del buffer buff
 */
uint8_t espt_socket_send(_espt *e, uint8_t *buff, uint32_t Len){
    const char *i = NULL;
    char tmp_c[6];
    if(espt_status(e, _ESPT_STATUS_IDLE) && espt_status(e, _ESPT_STATUS_TCP_UDP_ESTABLISHED)){
        i = at_socket_send;
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        _to_str(Len, tmp_c);
        i = tmp_c;
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        e->tx.buff[e->tx.iw++] = '\r';
        e->tx.buff[e->tx.iw++] = '\n';
        e->busy = 5000;
        e->process = _ESPT_STATUS_SEND_DATA_PROMPT_RECEIVED;
        _reset_status(e, _ESPT_STATUS_IDLE);
        _reset_status(e, _ESPT_STATUS_SEND_DATA_PROMPT_RECEIVED);
        _reset_status(e, _ESPT_STATUS_SEND_DATA_OK);
        e->parser = 0;
        e->tx.send_buffer = buff;
        e->tx.send_buffer_len = Len;
        e->func_write_data_to_esp(e->tx.buff, e->tx.iw);
        return 1;
    }
    return 0;
}



uint8_t espt_socket_close(_espt *e){
    if(espt_status(e, _ESPT_STATUS_IDLE) && espt_status(e, _ESPT_STATUS_TCP_UDP_ESTABLISHED)){
        if(e->operation_mode == _ESPT_OPERATION_MODE_AUTO) e->operation_mode = _ESPT_OPERATION_MODE_MANUAL;
        const char *i = NULL;

        i = at_socket_close;
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        e->busy = 5000;
        e->process = _ESPT_STATUS_TCP_UDP_CLOSED;
        _reset_status(e, _ESPT_STATUS_IDLE);
        _reset_status(e, _ESPT_STATUS_TCP_UDP_CLOSED);
        _reset_status(e, _ESPT_STATUS_TCP_UDP_ESTABLISHED);
        e->func_write_data_to_esp(e->tx.buff, e->tx.iw);
        return 1;
    }
    return 0;
}


/*
 * Envia una peticion AT+CIFSR
 */
void espt_acquire_ip_data(_espt *e){
    if(espt_status(e, _ESPT_STATUS_IDLE) && espt_status(e, _ESPT_STATUS_AP_CONNECTED)){
        const char *i = NULL;
        i = at_query_local_address;
        while(*i)
            e->tx.buff[e->tx.iw++] = *(i++);
        e->parser = 0;
        e->busy = 10000;
        e->process = _ESPT_STATUS_IP_DATA_ACQUIRED;
        _reset_status(e, _ESPT_STATUS_IDLE);
        _reset_status(e, _ESPT_STATUS_IP_DATA_ACQUIRED);
        e->func_write_data_to_esp(e->tx.buff, e->tx.iw);
    }
}

/*
 * ---------------------   PRIVADO   ---------------------
 */



/*
 * Parsea una respuesta AT+CIFSR para cargar la ip de origen
 */
static void _parse_address(_espt *e){
    switch(e->parser){
    case 0:{
        uint8_t cmp = _rx_buffer_pop(e, at_cifsr_apip, "\0", "\0");
        if(cmp == 0)
            e->parser = 1;
        break;
    }
    case 1:{
        // Busco la comilla al final
        uint32_t last = e->rx.ir;
        char char_ip[16];
        uint8_t i = 0;
        while(last != e->rx.iw){
            if(e->rx.buff[last] == '"'){
                while(e->rx.ir != last && i < 16){
                    char_ip[i++] = e->rx.buff[e->rx.ir++];
                    e->rx.ir &= e->rx.imask;
                }
                char_ip[i] = '\0';
                if(_char_ipv4_to_uint8_array(char_ip, e->ap_ipaddr) == 0){ // Fallo la conversion
                    e->process = _ESPT_STATUS_IDLE;
                    e->busy = 0;
                    _set_status(e, _ESPT_STATUS_IDLE);
                    _set_status(e, _ESPT_STATUS_ERROR);
                    if(_ESPT_ON_ERROR_STATE_OPERATION_MODE)
                        e->operation_mode = _ESPT_ON_ERROR_STATE_OPERATION_MODE;
                    return;
                }
                e->parser = 2;
                break;
            }
            last++;
            last &= e->rx.imask;
        }
        break;
    }
    case 2:{
        uint8_t cmp = _rx_buffer_pop(e, at_cifsr_staip, "\0", "\0");
        if(cmp == 0)
            e->parser = 3;
        break;
    }
    case 3:{
        // Busco la comilla al final
        uint32_t last = e->rx.ir;
        char char_ip[16];
        uint8_t i = 0;
        while(last != e->rx.iw){
            if(e->rx.buff[last] == '"'){
                while(e->rx.ir != last && i < 16){
                    char_ip[i++] = e->rx.buff[e->rx.ir++];
                    e->rx.ir &= e->rx.imask;
                }
                char_ip[i] = '\0';
                if(_char_ipv4_to_uint8_array(char_ip, e->src_ipaddr) == 0){ // Fallo la conversion
                    _set_status(e, _ESPT_STATUS_ERROR);
                    if(_ESPT_ON_ERROR_STATE_OPERATION_MODE)
                        e->operation_mode = _ESPT_ON_ERROR_STATE_OPERATION_MODE;
                }else
                    _set_status(e, _ESPT_STATUS_IP_DATA_ACQUIRED);
                _set_status(e, _ESPT_STATUS_IDLE);
                e->process = _ESPT_STATUS_IDLE;
                e->busy = 100;
                break;
            }
            last++;
            last &= e->rx.imask;

        }
        break;
    }
    }
}


/*
 * Recepcion de datos IPD
 * el primer numero despues de "+IPD," corresponde a la cantidad de bytes, ej: +IPD,7:0123456
 * La cabecera +IPD, ya fue eliminada del buffer
 * e->parser:
 * 				0: leyendo campo size hasta encontrar ":"
 * 				1: leyendo datos, cantidad en ipd_bytes_cntr
 */
static uint8_t _ipd_manage(_espt *e){
    switch(e->parser){
    case 0:{
        if(e->rx.buff[e->rx.ir] != ':'){
            e->ipd_bytes_rcvd[e->ipd_char_index++] = e->rx.buff[e->rx.ir++];
            e->rx.ir &= e->rx.imask;
            if(e->ipd_char_index > 5){  // Ya procese demasiados caracteres y no encontre el :
                e->process = _ESPT_STATUS_IDLE;
                e->busy = 0;
                _set_status(e, _ESPT_STATUS_IDLE);
                _reset_status(e, _ESPT_STATUS_RECEIVING_DATA);
            }
        }else{
            e->ipd_bytes_rcvd[e->ipd_char_index] = '\0';
            if(_to_uint16(e->ipd_bytes_rcvd, &e->ipd_bytes_cntr, 4) == 0){		// No se recuperó un numero
                e->process = _ESPT_STATUS_IDLE;
                e->busy = 0;
                _set_status(e, _ESPT_STATUS_IDLE);
                _reset_status(e, _ESPT_STATUS_RECEIVING_DATA);
                return 1;   // brake_while en espt_wprk
            }else{
                e->parser++;
                e->rx.ir++;
                e->rx.ir &= e->rx.imask;
            }
        }
        break;
    }
    case 1:{
        // Es un buffer circular, envio de 1 byte para que e->rx.ir no se salte a e->rx.iw
        e->func_write_data_to_user(&e->rx.buff[e->rx.ir++], 1);
        e->rx.ir &= e->rx.imask;
        e->ipd_bytes_cntr--;
        if(e->ipd_bytes_cntr == 0){	// Termine de recibir datos
            e->process = _ESPT_STATUS_IDLE;
            e->busy = 0;
            _set_status(e, _ESPT_STATUS_IDLE);
            _reset_status(e, _ESPT_STATUS_RECEIVING_DATA);
        }
        break;
    }
    }
    return 0;   // Dont brake_while, continue processing
}


/*
 * Recibido prompt de envio de datos
 */
static void _prompt_manage(_espt *e){
    if(e->tx.send_buffer_len == 0){
        _reset_status(e, _ESPT_STATUS_SEND_DATA_PROMPT_RECEIVED);
        e->process = _ESPT_STATUS_SEND_DATA_OK;
        e->busy = 250;
        return;
    }
    if(e->tx.iw == 0){
        while(e->tx.send_buffer_len > 0 && e->tx.iw < _ESPT_TX_BUFFER_SIZE){
            e->tx.buff[e->tx.iw++] = *e->tx.send_buffer;
            e->tx.send_buffer++;
            e->tx.send_buffer_len--;
        }
        e->busy = 500;
        e->func_write_data_to_esp(e->tx.buff, e->tx.iw);
    }
}


/*
 * Escalamiento automatico
 */
static void _auto_manage(_espt *e){
    if(e->busy > 0) return;     // Espero
    if(e->operation_mode == _ESPT_OPERATION_MODE_AUTO){

        if(!espt_status(e, _ESPT_STATUS_TCP_UDP_ESTABLISHED) && espt_status(e, _ESPT_STATUS_IP_DATA_ACQUIRED))
            espt_socket_open(e);
        else if(!espt_status(e, _ESPT_STATUS_IP_DATA_ACQUIRED) && espt_status(e, _ESPT_STATUS_AP_CONNECTED))
            espt_acquire_ip_data(e);
        else if(!espt_status(e, _ESPT_STATUS_AP_CONNECTED) && espt_status(e, _ESPT_STATUS_WIFI_MODE_ESTABLISHED))
            espt_ap_connect(e);
        else if(!espt_status(e, _ESPT_STATUS_WIFI_MODE_ESTABLISHED) && espt_status(e, _ESPT_STATUS_IDLE))
            espt_send_wifi_mode(e);
        else if(espt_status(e, _ESPT_STATUS_DOWN))
            espt_wakeup(e);

    }else

    /*
     * Desconexion automatica
     */
    if(e->operation_mode == _ESPT_OPERATION_MODE_CLOSED && espt_status(e, _ESPT_STATUS_IDLE)){
        if(!espt_status(e, _ESPT_STATUS_TCP_UDP_CLOSED))
            espt_socket_close(e);
        else if(!espt_status(e, _ESPT_STATUS_AP_DISCONNECTED))
            espt_ap_disconnect(e);
        else if(!espt_status(e, _ESPT_STATUS_DOWN))
            espt_sleep(e);
        else
            e->operation_mode = _ESPT_OPERATION_MODE_MANUAL;
    }else

    /*
     * Reconexión automatica
     */
    if(e->operation_mode == _ESPT_OPERATION_MODE_RECONNECT && espt_status(e, _ESPT_STATUS_IDLE)){
        if(!espt_status(e, _ESPT_STATUS_TCP_UDP_CLOSED) && espt_status(e, _ESPT_STATUS_TCP_UDP_ESTABLISHED))
            espt_socket_close(e);
        else if(!espt_status(e, _ESPT_STATUS_AP_DISCONNECTED) && espt_status(e, _ESPT_STATUS_AP_CONNECTED))
            espt_ap_disconnect(e);
        else
            e->operation_mode = _ESPT_OPERATION_MODE_AUTO;
    }
}


/*
 * Busca las cadenas str1 y str2  enviadas en el tope del buffer rx.
 * Se buscan en el orden que fueron enviadas
 * @retval 0 si la cadena está en el tope, ademas la quita del buffer
 * @retval 1 el contenido del buffer no es ninguna de las cadenas buscadas, ademas desplaza el indice un byte adelante
 * @retval 2 si no hay suficiente contenido en el buffer aún como para saber si es o no alguna de las dos cadenas buscadas
 * @retval 3 si la cadena buscada es err, la quita del buffer
 */
static uint8_t _rx_buffer_pop(_espt *e, const char *str1, const char *str2, const char *err){
    uint8_t cmp1 = _strcmp(e->rx.buff, e->rx.ir, e->rx.iw, _ESPT_RX_BUFFER_SIZE, str1);
    uint8_t cmp2 = _strcmp(e->rx.buff, e->rx.ir, e->rx.iw, _ESPT_RX_BUFFER_SIZE, str2);
    uint8_t cmperr = _strcmp(e->rx.buff, e->rx.ir, e->rx.iw, _ESPT_RX_BUFFER_SIZE, err);
    if(cmp1 == 0 && *str1){		// La cadena vacia nunca matchea
        e->rx.ir += _strlen(str1, 255);
        e->rx.ir &= e->rx.imask;
        return 0;
    }else if(cmp2 == 0 && *str2){
        e->rx.ir += _strlen(str2, 255);
        e->rx.ir &= e->rx.imask;
        return 0;
    }else if(cmperr == 0 && *err){
        e->rx.ir += _strlen(err, 255);
        e->rx.ir &= e->rx.imask;
        return 3;
    }else if((cmp1 == 1 || !*str1) && (cmp2 == 1 || !*str2) && (cmperr == 1 || !*err)){
        e->rx.ir++;
        e->rx.ir &= e->rx.imask;
        return 1;
    }
    return 2;
}


/*
 *  Setea el flag de la variable status
 */
static void _set_status(_espt *e, uint8_t flag){
    e->status |= (1<<flag);
}

/*
 *  Resetea a 0 el flag de la variable status
 */
static void _reset_status(_espt *e, uint8_t flag){
    e->status &= ~(1<<flag);
}

/* -------   UTILIDADES -----------  */


/*
 * Compara dos cadenas de caracteres, str2 terminada en \0
 * @param *str1 buffer con la cadena 1 a comparar
 * @param ir indice donde comienza la cadena str1 dentro del buffer circular str1
 * @param iw indice donde termina la cadena str1 dentro del buffer circular str1
 * @param buflen el tamaño del buffer circular
 * @param *str2 cadena 2
 * @result retorna 0 si son iguales, 1 si se compararon y son diferentes, 2 si str1 es mas corta que str2
 */
static uint8_t _strcmp(const uint8_t *str1, uint32_t ir, const uint32_t iw, const uint32_t buflen, const char *str2){
    while(*str2){
        if(ir == iw)
            return 2;
        if(*str2 != str1[ir])
            return 1;
        ir++;
        if(ir == buflen) ir = 0;
        str2++;
    }
    return 0;
}

/*
 * Retorna el largo de una cadena de caracteres terminada en \0
 * @param *str cadena a determinar el largo
 * @param max, maximo de caracteres a contar
 * @result la cantidad de caracteres en la cadena, sin contar el \0
 */
static uint32_t _strlen(const char *str, uint32_t max){
    uint32_t cnt = 0;
    while(*str && cnt < max){
        str++;
        cnt++;
    }
    return cnt;
}

/*
 * Similar a GNU cut, pero para una sola linea, terminada '\0' o en CRLF (0x0D0A)
 * @param *str cadena a dividir
 * @param separator, caracter a tomar como divisor
 * @param campo seleccionado (1...)
 * @param *res resultado, con terminador '\0'
 * @param max_size tamaño maximo del resultado, sin contar el '\0', notece que *res debe
 * poder almacenar al menos max_size+1 caracteres
 * @result retorna la cantidad de caracteres en *str2, sin contar '\0'
 */
static uint16_t _cut(const char *str, char separator, uint8_t field, char *res, uint16_t max_size){
    uint16_t size = 0;
    while(*str != '\r' && *str != '\n' && *str != '\0' && size != max_size){
        if(field == 1){
            if(*str == separator){
                res[size] = 0;
                return size;
            }
            else
                res[size++] = *str;

        }else{
            if(*str == separator)
                field--;
        }
        str++;
    }
    res[size] = 0;
    return size;
}


/*
 * Convierte *str en su valor numerico
 * @param *str cadena a convertir, la cadena es finalizada en cualquier caracter que no sea numerico, idealmente '\0'
 * @param *number es el resultado
 * @param max_digits la maxima cantidad de digitos
 * @result retorna la cantidad de caracteres convertidos, 0 significa que falló la conversion
 */
static uint8_t _to_uint16(const char *str, uint16_t *number, uint8_t max_digits){
    uint8_t size = 0;
    uint8_t count = 0;
    const uint16_t pow[] = {1, 10, 100, 1000, 10000};
    *number = 0;
    while(str[size] >= '0' && str[size] <= '9' && size < max_digits){
            size++;
    }
    if (size == 0) return 0;
    while(size--)
        *number += (str[count++] - '0') * pow[size];	// its ok
    return count;
}


/*
 * Convierte un numero en char[] ascii
 * @param number es el numero a convertir
 * @param *str es el resultado, finalizada en \0
 * @result cantidad de digitos en el numero [1..5] sin contar el \0
 */
static uint8_t _to_str(uint16_t number, char *str){
    char result[] = "00000\0";
    char *ptr = &result[4];
    uint8_t i = 0;
    do {
        *ptr += number % 10;
        number /= 10;
        ptr--;
    } while (number > 0);

    do{
        ptr++;	// its ok
        *str = *ptr;
        str++;
        i++;
    } while(*ptr);
    return i-1;	// No cuento el \0
}


/*
 * copia el string src terminado en \0 en dst terminado en \0
 * @param max: maxima cantidad de caracteres que se van a escribir en dst (incluido el '\0')
 * OJO al orden de los parametros: es src -> dst, alrevez del strcpy de C que es dst <- src
 * Para mi es mas natural asi, es como cp src dst
 * Además, max no le da el comportamiento de strncpy, no se hace padding
 */
static uint32_t _strcpy(const char *src, char *dst, uint32_t max){
    uint32_t cnt = 0;
    max--;  //descuento el \0 que va si o si
    while(src[cnt] && cnt < max){
        dst[cnt] = src[cnt];
        cnt++;
    }
    dst[cnt++] = '\0';
    return cnt;
}


/*
 * Convierte una direccion IPv4 pasada como cadena de caracteres terminada en \0 en un array de 4 elementos
 * retorna 1 si se pudo convertir, 0 en caso contrario
 */
static uint8_t _char_ipv4_to_uint8_array(const char *ipc, uint8_t *ipa){
    char tmp_c[4];
    uint16_t tmp_n = 0;
    for(uint8_t j = 0; j < 4; j++){
        _cut(ipc,  '.',  j+1, tmp_c, 3);
        if(_to_uint16(tmp_c, &tmp_n, 3) > 0 && tmp_n < 256){
            ipa[j] = tmp_n;
        }else{
            return 0;
        }
    }
    return 1;
}

/*
 * Convierte una direccion IPv4 pasada como array de uint8_t de 4 elementos en una cadena de caracteres terminada en \0
 * Para que el metodo sea seguro *ipc deberia tener al menos 16 bytes
 * retorna 1 si se pudo convertir, 0 en caso contrario
 */
static uint8_t _uint8_array_to_char_ipv4(const uint8_t *ipa, char *ipc){
    char tmp_c[4];
    uint8_t io = 0;
    uint8_t ia = 0;
    for(uint8_t j = 0; j < 4; j++){
        _to_str(ipa[j], tmp_c);
        while(tmp_c[io] && io < 4){
            ipc[ia++] = tmp_c[io++];
        }
        ipc[ia++] = '.';
    io = 0;
    }
    ia--;
    ipc[ia] = '\0';
    return ia;
}

/*
 * Retorna 1 si la direccion IP es valida, 0 en caso contrario
 */
static uint8_t _check_ipv4_is_valid(const uint8_t o1, const uint8_t o2, const uint8_t o3, const uint8_t o4){
    //Broadcast
    if(o1 == 255 && o2 == 255 && o3 == 255 && o4 == 255) return 0;
    //Link Local
    if(o1 == 169 && o2 == 254) return 0;
    //Local Wildcard
    if(o1 == 0) return 0;
    //Loopback
    if(o1 == 127) return 0;
    //Multicast
    if(o1 >=224 && o1 <= 239) return 0;
    //Shared
    if(o1 == 100 && o2 >= 64 && o2 <= 127) return 0;
    return 1;
}
