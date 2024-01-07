/*
 * esp8266_transport.c
 *
 *  Created on: Oct 4, 2022
 *      Author: psilva
 */

#include "esp8266_transport.h"


/*
 * Constantes de AT - Privado - Cadenas terminadas en \0
 */
static const char *at_alive = "AT\r\n\0";

static const char *at_set_wifi_mode = "AT+CWMODE=\0";

static const char *at_ap_connect = "AT+CWJAP=\"\0";

static const char *at_query_local_address = "AT+CIFSR\r\n\0";

static const char *at_list_available_ap = "AT+CWLAP\r\n\0";

static const char *at_socket_udp_text = ",\"UDP\",\0";
static const char *at_socket_tcp_text = ",\"TCP\",\0";

static const char *at_socket_open = "AT+CIPSTART=\0";

static const char *at_socket_send = "AT+CIPSEND=\0";

static const char *at_socket_close = "AT+CIPCLOSE=\0";

static const char *at_ap_disconnect = "AT+CWQAP\r\n\0";

static const char *at_restore = "AT+RESTORE\r\n\0";

// Respuestas de AT
static const char *at_ok = "OK\r\n\0";

static const char *at_ipd = "+IPD,\0";

static const char *at_cifsr_apip = "+CIFSR:APIP,\"\0";
//static const char *at_cifsr_apmac = "+CIFSR:APMAC,\"\0";
static const char *at_cifsr_staip = "+CIFSR:STAIP,\"\0";
//static const char *at_cifsr_stamac = "+CIFSR:STAMAC,\"\0";

static const char *at_send_ok = "SEND OK\r\n\0";

static const char *at_closed = ",CLOSED\r\n\0";

static const char *at_ok_wifi_disconnect = "OK\r\nWIFI DISCONNECT\r\n\0";



/*
 * Prototipos privados
 */
static void _ipd_manage(_espt *e);
static void _query_local_address(_espt *e);
static void _parse_address(_espt *e);

/* UTILIDADES */
static uint8_t _strcmp(const uint8_t *str1, uint8_t ir, const uint8_t iw, const char *str2);
static uint8_t _strlen(const char *str);
static uint16_t _cut(const char *str, char separator, uint8_t field, char *res, uint16_t max_size);
static uint8_t _to_uint16(const char *str, uint16_t *number);
static uint8_t _to_str(uint16_t number, char *str);




/*
 * Inicializo valores precargados en el struct _espt
 * Ejecuto la funcion para habilitar el chip
 */
void espt_init(_espt *e, uint8_t wmode){
	e->rx.ir = 0;
	e->rx.iw = 0;
	e->rx.tout_base = 10;
	e->rx.tout_value = 0;
	e->tx.status = 0;
	e->tx.iw = 0;
	e->tx.tout_base = 10;
	e->tx.tout_value = 0;
	e->wmode = wmode;
	e->expected = 0;
	e->parser = 0;
	e->status = _ESPT_STATUS_DOWN;
	e->transport_status = 0;
	e->alive = 0;
	e->tout = 0;
	e->wait = 0;
}


void espt_wakeup(_espt *e){
	e->func_chipenable(1);
}


void espt_sleep(_espt *e){
	e->func_chipenable(0);
}


/*
 * Attach de las funciones de entrada correspondientes
 */
void espt_attach_receive_data(_espt *e, espt_receive_def f){
	e->func_write_data = f;
}

void espt_attach_receive_control(_espt *e , espt_receive_def f){
	e->func_write_control = f;
}

void espt_attach_chip_enable(_espt *e, espt_chipenable_def f){
	e->func_chipenable = f;
}

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
 * 			11: solicitado AT+RESTORE, esperando OK\r\n y paso a expected 0 (descartar el dump de configuración tras espera 100)
 * 			100:  Querys sin cambio de estado, se envian raw al buffer auxuliar de entrada y se vuelve a expected = 1;
 *
 * status:
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
 *
 * 	alive:	Comando recurrente con lo otros, tiene su propia bandera de estado.
 * 			0: no alive rq
 * 			1: alive rq
 * 			2: alive ok
 * 			3: alive timeout
 *
 * 	Un caso particular es cuando _strcmp retorna 1, significando que la cadena se comparó y es diferente
 * 	de la esperada, esto significa un error en si mismo (no llego lo que se esperaba) sin embargo se mueve
 * 	el indice de lectura un byte hacia adelante y se vuelve a comparar , esto se hace en un nuevo ingreso
 * 	Por lo que esta limpieza demorar el parseo hasta alcanzar el timeout
 *
 */
void espt_work(_espt *e){
	uint8_t cmp = 0;
	if(e->rx.ir != e->rx.iw){
		switch(e->expected){
		case 0:{
			if(e->alive == 1){
				if(_strcmp(e->rx.buff, e->rx.ir, e->rx.iw, at_ok) == 0){
					e->alive = 2;
					e->rx.ir = e->rx.iw;
				}
			}else{
				e->rx.ir = e->rx.iw;  // En este punto solo tiene sentido recibir respuesta de un alive
			}
			break;
		}
		case 1:{
			if(e->alive == 1 && _strcmp(e->rx.buff, e->rx.ir, e->rx.iw, at_ok) == 0){
				e->alive = 2;
				e->rx.ir += _strlen(at_ok);
			}else{
				cmp =_strcmp(e->rx.buff, e->rx.ir, e->rx.iw, at_ipd);
				if(cmp == 0){
					e->rx.ir += _strlen(at_ipd);
					e->expected = 2;
					e->parser = 0;
					e->rx.tout_base = 10;
					e->rx.tout_value = 0;
				}else{
					if(cmp == 1)
						e->rx.ir++;	// Descarto el byte actual
				}
			}
			break;
		}
		case 2:{
			_ipd_manage(e);
			break;
			}
		case 3:
		case 4:
		case 6:{
			cmp = _strcmp(e->rx.buff, e->rx.ir, e->rx.iw, at_ok);
			if(cmp == 0){
				e->status = e->expected;
				e->expected = 1;
				e->rx.ir += _strlen(at_ok);
			}else{
				if(cmp == 1)
					e->rx.ir++;
			}
			break;
		}
		case 5:{
			_parse_address(e);
			break;
		}
		case 7:{
			if(e->parser == 0 && e->rx.buff[e->rx.ir++] == '>'){		//Todavia no recibi el >
				e->parser = 1;
			}else{
				e->status = _ESPT_STATUS_SEND_DATA_PROMPT_RECEIVED;
				if(e->tx.send_buffer_len == 0){
					e->expected = 1;
					e->status = _ESPT_STATUS_SOCKET_SEND_OK;
				}
				if(e->tx.status == 0){
					while(e->tx.send_buffer_len > 0 && e->tx.iw < 255){
						e->tx.buff[e->tx.iw++] = *e->tx.send_buffer;
						e->tx.send_buffer++;
						e->tx.send_buffer_len--;
					}
					e->tx.status = 1;
				}
			}
			break;
		}
		case 8:{
			cmp = _strcmp(e->rx.buff, e->rx.ir, e->rx.iw, at_send_ok);
			if(cmp == 0){
				e->status = e->expected;
				e->expected = 1;
				e->rx.ir += _strlen(at_ok);
			}else{
				if(cmp == 1)
					e->rx.ir++;
			}
			break;
		}
		case 9:{
			cmp = _strcmp(e->rx.buff, e->rx.ir, e->rx.iw, at_closed);
			if(cmp == 0){
				e->status = e->expected;
				e->expected = 1;
				e->rx.ir += _strlen(at_ok);
			}else{
				if(cmp == 1)
					e->rx.ir++;
			}
			break;
		}
		case 10:{
			cmp = _strcmp(e->rx.buff, e->rx.ir, e->rx.iw, at_ok_wifi_disconnect);
			if(cmp == 0){
				e->status = e->expected;
				e->expected = 1;
				e->rx.ir += _strlen(at_ok);
			}else{
				if(cmp == 1)
					e->rx.ir++;
			}
			break;
		}
		case 11:{
			cmp = _strcmp(e->rx.buff, e->rx.ir, e->rx.iw, at_ok);
			if(cmp == 0){
				e->status = _ESPT_STATUS_RESTORE_OK;
				e->expected = 0;
				e->rx.ir += _strlen(at_ok);
				e->wait = 10;
			}else{
				if(cmp == 1)
					e->rx.ir++;
			}
			break;
		}
		case 100:{
			/* Comandos con respuesta de control
			 */
			cmp = _strcmp(e->rx.buff, e->rx.ir, e->rx.iw, at_ok);
			if(cmp == 0){
				e->expected = 1;
				e->rx.ir += _strlen(at_ok);
			}else if(cmp == 1){
				e->func_write_control(&e->rx.buff[e->rx.ir++], 1);
			}
			break;
		}
		}
	}
	/*
	 *  Verifico si se alcanzo timeout en una espera activa (e->expected > 1) o alive
	 */
	if(e->rx.tout_base <= e->rx.tout_value){
		if(e->expected > 1 ){
			e->expected = 1;
			e->tout++;
		}
		if(e->alive == 1){
			e->alive = 3;
			e->tout++;
		}
	}
	/*
	 * Escalamiento de estados automaticos, si se esta IDLE
	 * Si estoy en estado 4 debo obtener datos de IP y pasar a 5
	 */
	if(e->expected == 1 && e->wait == 0){
		switch(e->status){
		case _ESPT_STATUS_AP_CONNECTED:{
			_query_local_address(e);
			break;
		}
		}
	}
}

/*
 * Establezco wifi mode, retorna 0 si el proceso se inicó, 1 si se está en espera
 */
uint8_t espt_set_wifi_mode(_espt *e){
	if(e->wait == 0 && e->tx.status == 0){
		const char *i = NULL;
		i = at_set_wifi_mode;
		while(*i){
			e->tx.buff[e->tx.iw++] = *i;
			i++;
		}
		_to_str((uint16_t)e->wmode,(char *)&e->tx.buff[e->tx.iw++]);
		e->tx.buff[e->tx.iw++] = '\r';
		e->tx.buff[e->tx.iw++] = '\n';
		e->rx.tout_base = 10;
		e->rx.tout_value = 0;
		e->expected = 3;
		e->parser = 0;
		e->wait = 10;
		e->tx.status = 1;  // Habilita el envio de datos por usart
		return 0;
	}else
		return 1;
}


/*
 * Se debe llamar cada 10ms y actualiza los contadores de timeout y espera
 * Hacer mas espaciada la llamada  a este metodo hace que se espere mas tiempo respuestas
 */
void espt_update_tout(_espt *e){
	e->rx.tout_value++;
	e->tx.tout_value++;
	if(e->wait)
		e->wait--;
}



/*
 * estado actual del espt
 */
uint8_t espt_status(_espt *e){
	return e->status;
}


/*
 *  Peticion de alive
 *  Se envia solo si se esta en reposo
 */
uint8_t espt_alive(_espt *e){
	if(e->expected <= 1 && e->status != _ESPT_STATUS_SEND_DATA_PROMPT_RECEIVED && e->alive != 1 && e->wait == 0 && e->tx.status == 0){
		const char *i = NULL;
		i = at_alive;
		while(*i){
			e->tx.buff[e->tx.iw++] = *i;
			i++;
		}
		e->rx.tout_base = 10;
		e->rx.tout_value = 0;
		e->alive = 1;
		e->wait = 10;
		e->tx.status = 1;  // Habilita el envio de datos por usart
		return 0;
	}
	return 1;
}

uint8_t espt_restore(_espt *e){
	const char *i = NULL;

	// Respeto espera
	if(e->wait > 0 || e->tx.status != 0) return 1;

	i = at_restore;
	while(*i){
		e->tx.buff[e->tx.iw++] = *i;
		i++;
	}
	e->rx.tout_base = 10;
	e->rx.tout_value = 0;
	e->expected = 11;
	e->parser = 0;
	e->wait = 10;
	e->tx.status = 1;  // Habilita el envio de datos por usart
	return 0;
}


/*
 * Solicita lista de redes disponibles
 */
uint8_t espt_list_available_ap(_espt *e){
	if(e->wait == 0 && e->tx.status == 0){
		const char *i = NULL;
		i = at_list_available_ap;
		while(*i){
			e->tx.buff[e->tx.iw++] = *i;
			i++;
		}
		e->rx.tout_base = 100;
		e->rx.tout_value = 0;
		e->expected = 100;
		e->parser = 0;
		e->wait = 50;
		e->tx.status = 1;  // Habilita el envio de datos por usart
		return 0;
	}
	return 1;
}

uint8_t espt_ap_connect(_espt *e, const char *ssid, const char *passwd){
	if(e->wait == 0 && e->tx.status == 0){
		const char *i = NULL;
		i = at_ap_connect;
		while(*i){
			e->tx.buff[e->tx.iw++] = *i;
			i++;
		}
		while(*ssid){
			e->tx.buff[e->tx.iw++] = *ssid;
			ssid++;
		}
		e->tx.buff[e->tx.iw++] = '"';
		e->tx.buff[e->tx.iw++] = ',';
		e->tx.buff[e->tx.iw++] = '"';
		while(*passwd){
			e->tx.buff[e->tx.iw++] = *passwd;
			passwd++;
		}
		e->tx.buff[e->tx.iw++] = '"';
		e->tx.buff[e->tx.iw++] = '\r';
		e->tx.buff[e->tx.iw++] = '\n';
		e->rx.tout_base = 10;
		e->rx.tout_value = 0;
		e->expected = 4;
		e->wait = 10;
		e->tx.status = 1;  // Habilita el envio de datos por usart
		return 0;
	}
	return 1;
}

uint8_t espt_ap_disconnect(_espt *e){
	const char *i = NULL;

	// Respeto espera
	if(e->wait > 0 || e->tx.status != 0) return 1;

	i = at_ap_disconnect;
	while(*i){
		e->tx.buff[e->tx.iw++] = *i;
		i++;
	}
	e->rx.tout_base = 10;
	e->rx.tout_value = 0;
	e->expected = 10;
	e->parser = 0;
	e->wait = 10;
	e->tx.status = 1;  // Habilita el envio de datos por usart
	return 0;
}


/*
 * Devuelve el puntero, please dont modify it
 */
uint8_t* espt_get_apaddr(_espt *e){
	return e->ap_ipaddr;
}

uint8_t* espt_get_srcaddr(_espt *e){
	return e->src_ipaddr;
}

/*
 * Inicia una conexion tcp/udp con la información suministrada
 * Retorna:
 * 			1: si la peticion no se pudo iniciar
 * 			2: malformed IP
 * 			3: malformed port/transport
 *
 */
uint8_t espt_socket_open(_espt *e, uint8_t transport, const char *dst_ipaddr, uint16_t dst_port, uint16_t src_port){
	char tmp_c[6];
	uint16_t tmp_n;
	const char *i = NULL;

	// Respeto espera
	if(e->wait > 0 || e->tx.status != 0) return 1;

	for(uint8_t j = 1; j < 5; j++){
		_cut(dst_ipaddr,  '.',  j, tmp_c, 3);
		if(_to_uint16(tmp_c, &tmp_n) > 0 && tmp_n < 255){
			e->dst_ipaddr[j-1] = tmp_n;
		}else{
			return 2;
		}
	}
	if(dst_port > 0 && src_port > 0 && transport > 0 && transport < 3){
		e->dst_port = dst_port;
		e->src_port = src_port;
	}else{
		return 3;
	}
	e->transport_status = 1;

	// Cominezo a armar la cadena de conexion
	i = at_socket_open;
	while(*i){
		e->tx.buff[e->tx.iw++] = *i;
		i++;
	}
	e->tx.buff[e->tx.iw++] = '1';

	if(transport == 1)
		i = at_socket_tcp_text;
	else if(transport == 2)
		i = at_socket_udp_text;

	while(*i){
		e->tx.buff[e->tx.iw++] = *i;
		i++;
	}
	e->tx.buff[e->tx.iw++] = '"';
	i = dst_ipaddr;
	while(*i){
		e->tx.buff[e->tx.iw++] = *i;
		i++;
	}
	e->tx.buff[e->tx.iw++] = '"';
	e->tx.buff[e->tx.iw++] = ',';
	_to_str(dst_port, tmp_c);
	i = tmp_c;
	while(*i){
		e->tx.buff[e->tx.iw++] = *i;
		i++;
	}
	e->tx.buff[e->tx.iw++] = ',';
	_to_str(src_port, tmp_c);
	i = tmp_c;
	while(*i){
		e->tx.buff[e->tx.iw++] = *i;
		i++;
	}
	e->tx.buff[e->tx.iw++] = ',';
	e->tx.buff[e->tx.iw++] = '0';
	e->tx.buff[e->tx.iw++] = '\r';
	e->tx.buff[e->tx.iw++] = '\n';
	e->rx.tout_base = 10;
	e->rx.tout_value = 0;
	e->expected = 6;
	e->parser = 0;
	e->wait = 10;
	e->tx.status = 1;  // Habilita el envio de datos por usart
	return 0;
}


/*
 * Envia por esp los datos del buffer buff
 * EL envio no es inmediato, los datos deben mantenerse en buff hasta que espt->status sea 8, y se debe volver a 6
 */
uint8_t espt_socket_send(_espt *e, uint8_t *buff, uint8_t Len){
	const char *i = NULL;
	char tmp_c[6];

	// Respeto espera
	if(e->wait > 0 || e->tx.status != 0) return 1;

	i = at_socket_send;
	while(*i){
		e->tx.buff[e->tx.iw++] = *i;
		i++;
	}
	e->tx.buff[e->tx.iw++] = '1';
	e->tx.buff[e->tx.iw++] = ',';
	_to_str(Len, tmp_c);
	i = tmp_c;
	while(*i){
		e->tx.buff[e->tx.iw++] = *i;
		i++;
	}
	e->tx.buff[e->tx.iw++] = '\r';
	e->tx.buff[e->tx.iw++] = '\n';
	e->rx.tout_base = 10;
	e->rx.tout_value = 0;
	e->expected = 7;
	e->parser = 0;
	e->tx.send_buffer = buff;
	e->tx.send_buffer_len = Len;
	e->tx.status = 1;  // Habilita el envio de datos por usart
	return 0;
}

uint8_t espt_socket_close(_espt *e){
	const char *i = NULL;

	// Respeto espera
	if(e->wait > 0 || e->tx.status != 0) return 1;

	i = at_socket_close;
	while(*i){
		e->tx.buff[e->tx.iw++] = *i;
		i++;
	}
	e->tx.buff[e->tx.iw++] = '1';
	e->tx.buff[e->tx.iw++] = '\r';
	e->tx.buff[e->tx.iw++] = '\n';
	e->rx.tout_base = 10;
	e->rx.tout_value = 0;
	e->expected = 9;
	e->parser = 0;
	e->wait = 10;
	e->tx.status = 1;  // Habilita el envio de datos por usart
	return 0;
}




/*
 * ---------------------   PRIVADO   ---------------------
 */

/*
 * Envia una peticion AT+CIFSR
 */
static void _query_local_address(_espt *e){
	if(e->wait == 0 && e->tx.status == 0){
		const char *i = NULL;
		i = at_query_local_address;
		while(*i){
			e->tx.buff[e->tx.iw++] = *i;
			i++;
		}
		e->rx.tout_base = 10;
		e->rx.tout_value = 0;
		e->expected = 5;
		e->parser = 0;
		e->wait = 10;
		e->tx.status = 1;  // Habilita el envio de datos por usart
	}
}

/*
 * Parsea una respuesta AT+CIFSR para cargar la ip de origen
 */
static void _parse_address(_espt *e){
	switch(e->parser){
	case 0:{
		uint8_t cmp = _strcmp(e->rx.buff, e->rx.ir, e->rx.iw, at_cifsr_apip);
		if(cmp == 0){
			e->rx.ir += _strlen(at_cifsr_apip);
			e->parser = 1;
		}else{
			if(cmp == 2)
				e->rx.ir++;
		}
	}
	case 1:{
		// Busco la comilla al final
		uint8_t last = e->rx.ir;
		char tmp_c[16];
		char tmp_c2[3];
		uint8_t i = 0;
		while(last != e->rx.iw)
			if(e->rx.buff[last++] == '"'){
				e->expected = 1;
				while(e->rx.ir != last && i < 16)	// don't fuck my memory, esp bug
					tmp_c[i++] = e->rx.buff[e->rx.ir++];
				for(i = 1; i <= 4; i++){
					if(_cut(tmp_c, '.', i, tmp_c2, 3) == 0)
						return;  // Fallo la conversion - expected queda en 1 y status en 4
					if(_to_uint16(tmp_c2, (uint16_t *)&e->ap_ipaddr[i]) == 0)
						return;  // Fallo la conversion
				}
				e->expected = 5;
				e->parser = 3;
			}
		break;
	}
	case 2:{
		uint8_t cmp = _strcmp(e->rx.buff, e->rx.ir, e->rx.iw, at_cifsr_staip);
		if(cmp == 0){
			e->rx.ir += _strlen(at_cifsr_staip);
			e->parser = 3;
		}else{
			if(cmp == 2)
				e->rx.ir++;
		}
	}
	case 3:{
		uint8_t last = e->rx.ir;
		char tmp_c[16];
		char tmp_c2[3];
		uint8_t i = 0;
		while(last != e->rx.iw)
			if(e->rx.buff[last++] == '"'){
				e->expected = 1;
				while(e->rx.ir != last && i < 16)	// don't fuck my memory, esp bug
					tmp_c[i++] = e->rx.buff[e->rx.ir++];
				for(i = 1; i <= 4; i++){
					if(_cut(tmp_c, '.', i, tmp_c2, 3) == 0)
						return; // Fallo la conversion, status queda en 4
					if(_to_uint16(tmp_c2, (uint16_t *)&e->src_ipaddr[i]) == 0)
						return;  // Fallo la conversion
				}
				e->status = _ESPT_STATUS_IP_DATA_ACQUIRED;
			}
		break;
	}
	}
}


/*
 * Recepcion de datos IPD
 * el primer numero despues de "+IPD," corresponde al link id, el segundo  despues de la "," es la cantidad de bytes, ej: +IPD,1,7:0123456
 * La cabecera +IPD, ya fue eliminada del buffer
 * e->parser:	0: esperando link id
 * 				1: esperando coma tras link id
 * 				2: leyendo campo size hasta encontrar ":"
 * 				3: leyendo datos, cantidad en ipd_tmpint
 */
static void _ipd_manage(_espt *e){
	switch(e->parser){
	case 0:{
		e->ipd_tmpchar[0] = e->rx.buff[e->rx.ir];
		e->ipd_tmpchar[1] = '\0';
		if(e->rx.buff[e->rx.ir] == '1'){
			// No es el link id esperado, descarto el proceso actual
			e->expected = 1;
		}else{
			if(e->transport_status == 1){
				e->parser++;
				e->rx.ir++;
			}else{
				// e->espt_trconn_w no es una conexion activa
				e->expected = 1;
			}
		}
		break;
	}
	case 1:{
		if(e->rx.buff[e->rx.ir] != ','){
			// No es una coma, descarto el proiceso actual
			e->expected = 1;
		}else{
			e->parser++;
			e->rx.ir++;
			e->ipd_tmpint = 0;
		}
		break;
	}
	case 2:{
		if(e->rx.buff[e->rx.ir] != ':'){
			e->ipd_tmpchar[e->ipd_tmpint++] = e->rx.buff[e->rx.ir++];
		}else{
			e->ipd_tmpchar[e->ipd_tmpint] = '\0';
			if(_to_uint16(e->ipd_tmpchar, (uint16_t *)&e->ipd_tmpint) == 0){		// No se recuperó un numero
				e->expected = 1;
			}else{
				e->parser++;
				e->rx.ir++;
			}
		}
		break;
	}
	case 3:{
		if(256 - e->rx.ir > e->ipd_tmpint){
			e->func_write_data(&e->rx.buff[e->rx.ir], e->ipd_tmpint);
			e->rx.ir += e->ipd_tmpint;
			e->ipd_tmpint = 0;
		}else{
			e->func_write_data(&e->rx.buff[e->rx.ir++], 1);
			e->ipd_tmpint--;
		}
		if(e->ipd_tmpint == 0){	// Termine de recibir datos
			e->expected = 1;
		}
		break;
	}
	}
}


/* -------   UTILIDADES -----------  */


/*
 * Compara dos cadenas de caracteres, str2 terminada en \0
 * @param *str1 buffer con la cadena 1 a comparar
 * @param ir indice donde comienza la cadena str1 dentro del buffer circular str1
 * @param iw indice donde termina la cadena str1 dentro del buffer circular str1
 * @param *str2 cadena 2
 * @result retorna 0 si son iguales, 1 si se compararon y son diferentes, 2 si str1 es mas corta que str2
 */
static uint8_t _strcmp(const uint8_t *str1, uint8_t ir, const uint8_t iw, const char *str2){
	while(*str2){
		if(ir == iw)
			return 2;
		if(*str2 != str1[ir])
			return 1;
		ir++;
		str2++;
	}
	return 0;
}

/*
 * Retorna el largo de una cadena de caracteres terminada en \0
 * @param *str cadena a determinar el largo
 * @result la cantidad de caracteres en la cadena, sin contar el \0
 */

static uint8_t _strlen(const char *str){
	uint8_t cnt = 0;
	while(*str){
		str++;
		cnt++;
	}
	return cnt;
}

/*
 * Similar a GNU cut, pero para una sola linea, terminada en CRLF (0x0D0A)
 * @param *str cadena a dividir
 * @param separator, caracter a tomar como divisor
 * @param campo seleccionado (1...)
 * @param *res resultado, sin terminador \0
 * @param max_size tamaño maximo del resultado
 * @result retorna la cantidad de caracteres en *str2
 */
static uint16_t _cut(const char *str, char separator, uint8_t field, char *res, uint16_t max_size){
	uint16_t size = 0;
	while(*str != '\r' && *str != '\n' && *str != '\0' && size != max_size){
		if(field == 1){
			if(*str == separator)
				return size;
			else
				res[size++] = *str;

		}else{
			if(*str == separator)
				field--;
		}
		str++;
	}
	return size;
}


/*
 * Convierte *str en su valor numerico
 * @param *str cadena a convertir, finalizada en \0
 * @param *number es el resultado
 * @result retorna la cantidad de caracteres convertidos, 0 significa que falló la conversion
 */
static uint8_t _to_uint16(const char *str, uint16_t *number){
	uint8_t size = 0;
	uint8_t count = 0;
	const uint16_t pow[] = {1, 10, 100, 1000, 10000};
	*number = 0;
	while(str[size] != '\0'){
		if(str[size] < 48 || str[size] > 57)		// No es un numero
			return 0;
		else
			size++;
	}
	while(size--)
		*number += (str[count++]-48) * pow[size];	// its ok
	return count;
}


/*
 * Convierte un numero en char[] ascii
 * @param number es el numero a convertir
 * @param *str es el resultado, finalizada en \0
 * @result cantidad de digitos en el numero [1..5]
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


