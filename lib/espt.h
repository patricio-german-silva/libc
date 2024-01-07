#ifndef ESPT_H
#define ESPT_H
/*
 * espt.h
 * MAYOR 2
 *  Created on: Feb 23, 2023
 *      Author: psilva
 *
 *  Version: 2.0
 *
 *  History: see espt.c
 *
 *
 * conexion 802.11 mediante modulo esp8266
 * Conexión wireless en modo 1,2 o 3
 *
 * Esta libreria no conecta con ningun medio de I/O, es un canal de paso donde
 * se parsea la informacion desde y hacia un modulo esp01 con el stack wireless 802.11
 *
 * El funcionamiento del parseo se realiza mediante llamadas regulares a espt_work, una llamada
 * demasiado espaciada puede dar problemas de buffer overflow, no deberia superar 1ms.
 *
 * Esta libreria recibe la información desde la red wireless
 * mediante llamadas al metodo espt_from_espt_received, que recibe los datos raw desde
 * un modulo espt y realiza el parseo necesario.
 *
 * El envio hacia la red wireless se hace mediante al puntero a funcion establecido
 * mediante la llamada a espt_attach_send_data, del lado del usuario esta funcion debera por ejemplo
 * poner los datos en el buffer de salida de una conexión USART, una vez que ha enviado los datos
 * se debe notificar a esta libreria mediante una llamada a espt_send_data_finished, de modo de poder
 * realizar nuevos envios, de no hacer esta llamada la libreria quedará stalled, ya que no estipula
 * un timeout para esto, la no llamada a esta funcion es un error de usuario y no un error que pueda
 * asociarse a un IO. Si existiera un error de IO el usuario debe detectarlo.
 *
 * El envio desde la aplicacion de usuario hacia el wireless mediante esta libreria se hace mediante la
 * llamada a espt_socket_send, una conexión UDP/TCP que previamente se debio establecer mediante la
 * llamadas a las funciones correspondiente.
 *
 * La recepcion de datos desde la red hacia la aplicacion de usuario se hace mediante la llamada al puntero
 * a funcion establecido mediante espt_attach_receive_data. Esta funcion enlazada debe tomar inmediatamente
 * los datos.
 *
 * Adicionalmente, la información de control proveniente desde el modulo esp, por ejemplo la lista de
 * access points disponibles, se recibe mediante la llamada a la funcion enlazada mediante
 * espt_attach_receive_control, los datos nuevamente deben ser leídos inmediatamente
 *
 * Modo de operacion:
 *      Automatico: tras la llamda a espt_connect la libreria mantiene la conexión tcp/udp
 *          establecida segun los paramatros de ssid, wep_psk, ip y puertos que deben establecerse antes
 *          y pasa al modo de operación _ESPT_OPERATION_MODE_AUTO
 *      Manual: la conexion se maneja realizando las llamadas a la cadena de funciones para pasar de un estado a otro
 *          - espt_wakeup
 *          - espt_set_wifi_mode
 *          - espt_ap_connect
 *          - espt_acquire_ip_data
 *          - espt_socket_open
 *      La llamada a las funciones
 *          - espt_socket_close
 *          - espt_ap_disconnect
 *          - espt_restore
 *          automaticamnete pasa al modo de operacion _ESPT_OPERATION_MODE_MANUAL
 *          Un estado de error durante el proceso de conexion pasa al modo definido por _ESPT_ON_ERROR_STATE_OPERATION_MODE
 *      Cerrado:
 *          Tras espt_disconnect se cierra los puertos de comunicación
 *          tcp/udp, desconecta del AP y llama a espt_sleep. Por ultimo se pasa a modo de operacion _ESPT_OPERATION_MODE_MANUAL
 *      Reconectar:
 *          Util para una reconexión a un AP/tcp/udp diferente. Cierra todas las conexiones y vuelve a conectar.
 *          Queda en modo de operación _ESPT_OPERATION_MODE_AUTO
 *
 *
 */

#include "stdint.h"
#include "stddef.h"


/*
 * Definicon de los valores de status
 * 			0: DOWN - No se utiliza el modulo
 *          1: en espera
 * 			3: Establecido wifi mode
 * 			4: conectado a ap
 * 			5: obtenidos los datos de IP
 * 			6: conexión tcp/udp establecida - se puede enviar y recibir datos
 * 			7: prompt > para enviar datos
 * 			8: datos enviados OK
 * 			9: Datos de control recibidos OK
 * 			10: tcp/udp cerrada
 * 			11: desconectado de AP - volver a 3
 * 			12: RESTORE Ok, estado queda todo lo demás en 0
 *          13: Seteado autoconn en la flash del esp
 *          14: Ultimo alive ok
 *          15: Estado de error
 *
 * EL flag _ESPT_STATUS_IDLE indica si se termino o no de procesar un comando
 * Por ejemplo, si se envia un apeticion de alive, este flag se pone en 0, cuando vuelva a
 * 1 el estado de ese alive estará en _ESPT_STATUS_ALIVE_OK, que será 1 si fue ok y 0 en caso contrario
 * El flag _ESPT_STATUS_IDLE solo estará en 0 por llamadas a funciones de esta libreria, o al incio,
 * cuando _ESPT_STATUS_DOWN será 1
 *
 * El flag _ESPT_STATUS_ERROR no vuelve a 0 por si sola, solo con llamadas a espt_error_reset
 *
 */
#define _ESPT_STATUS_DOWN						0
#define _ESPT_STATUS_IDLE						1
#define _ESPT_STATUS_RECEIVING_DATA             2
#define _ESPT_STATUS_WIFI_MODE_ESTABLISHED 		3
#define _ESPT_STATUS_AP_CONNECTED				4
#define _ESPT_STATUS_IP_DATA_ACQUIRED			5
#define _ESPT_STATUS_TCP_UDP_ESTABLISHED		6
#define _ESPT_STATUS_SEND_DATA_PROMPT_RECEIVED	7
#define _ESPT_STATUS_SEND_DATA_OK				8
#define _ESPT_STATUS_CONTROL_DATA_RECEIVED		9
#define _ESPT_STATUS_TCP_UDP_CLOSED				10
#define _ESPT_STATUS_AP_DISCONNECTED			11
#define _ESPT_STATUS_RESTORE_OK					12
#define _ESPT_STATUS_SET_AUTOCONN_OK			13
#define _ESPT_STATUS_ALIVE_OK       			14
#define _ESPT_STATUS_ERROR             			15




#define _ESPT_WIFI_STATION_MODE					1
#define _ESPT_WIFI_SOFT_AP_MODE					2
#define _ESPT_WIFI_SOFT_AP_AND_STATION_MODE		3

#define _ESPT_TRANSPORT_NONE            		0
#define _ESPT_TRANSPORT_TCP             		1
#define _ESPT_TRANSPORT_UDP                		2

#define _ESPT_OPERATION_MODE_AUTO               1
#define _ESPT_OPERATION_MODE_MANUAL             2
#define _ESPT_OPERATION_MODE_CLOSED             3
#define _ESPT_OPERATION_MODE_RECONNECT          4

/* Modo de operacion si un error ocurre durante la conexion. 0: continuar en modo actual */
#define _ESPT_ON_ERROR_STATE_OPERATION_MODE     0

/* Tiempo de espera inactivo tras activar la placa con wakeup */
#define _ESPT_WAIT_ON_STARTUP                   2000


// El tamaño del buffer RX '''DEBE''' ser una potencia de 2
#define _ESPT_RX_BUFFER_SIZE                    256
#define _ESPT_TX_BUFFER_SIZE                    256


/*
 * Definicion de punteros a funcion
 * 1 - Funcion para ingresar datos recibidos mediante el comnado AT +IPD
 * 2 - Funcion para ingresar datos de control (Ej: resultado de un scan de redes disponiples)
 * 3 - Funcion de activación del pin chip_enable, status 1 -> enable chip, status 0 -> disable chip
 */
typedef void (*espt_receive_def)(uint8_t *data, uint32_t len);
typedef void (*espt_chipenable_def)(uint8_t status);


/*
 * Buffer de recepcion desde el modulo esp8266
 * RX es buffer circular
 */
typedef struct{
    uint8_t buff[_ESPT_RX_BUFFER_SIZE];
    uint32_t imask;
    uint32_t iw;
    uint32_t ir;
    uint8_t toutcntr;		// Timeout counters
}_rx_espt;


/*
 * Buffer de envio al modulo esp8266
 * TX no es buffer circular
 * status:
 * 		0: Buffer libre para incorporar datos
 * 		1: Los datos en buffer están siendo enviados, no se puede agragar datos
 */
typedef struct{
    uint8_t buff[_ESPT_TX_BUFFER_SIZE];
    uint32_t iw;
    uint8_t *send_buffer;	// Buffer externo durante un envio de datos
    uint32_t send_buffer_len;
    uint8_t toutcntr;		// Timeout counters
}_tx_espt;


/*
 * Parsea los datos recibidos segun lo esperado
 *
 *  status:
 *      Cada bit es una bandera, su posicion se corresponde con
 *      los _ESPT_STATUS. Hay datos que puede estar activos a la vez
 *  process:
 *      Tarea actual en ciurso asociado a un estado, ej process = _ESPT_STATUS_AP_CONNECTED
 *      significa que se envio un pedido de conectar con el AP y se esta esperando
 *      el WIFI CONNECTED OK. process = 0 -> no se espera nada
 *  busy:
 *      Se actualiza decremental, cada estado setea un valor diferente para esta espera
 *      Vuelve a 0 cuando la tarea termina o si se vencio el tiempo de espera
 *
 */
typedef struct{
    uint8_t wmode;
    uint8_t operation_mode;
    char SSID[33];
    char WPA_PSK[64];
    uint8_t ap_ipaddr[4];
    uint8_t src_ipaddr[4];
    uint8_t transport; // 1: tcp, 2: udp, 0: none
    uint8_t dst_ipaddr[4];
    uint16_t dst_port;
    uint16_t src_port;

    // Control
    uint16_t status;
    uint8_t process;
    uint32_t busy;
    _tx_espt tx;
    _rx_espt rx;

    // Punteros a funciones para comunicación con el usuario de la libreria
    espt_receive_def func_write_data_to_user;
    espt_receive_def func_write_control_data_to_user;
    espt_receive_def func_write_data_to_esp;
    espt_chipenable_def func_chipenable;

    // Variables de control para _ipd_manage y _parse_address
    char ipd_bytes_rcvd[6];
    uint16_t ipd_bytes_cntr;
    uint8_t ipd_char_index;
    uint8_t parser;
}_espt;



/*
 * Inicializa un struct _espt
 */
void espt_init(_espt *e);

/*
 * Funciones de inicializacion de wifi mode, SSID y WPA_PSK, dst_ip, transport, dst_port, src_port
 */
void espt_set_wifi_mode(_espt *e, uint8_t mode);
void espt_set_SSID(_espt *e, const char *ssid);
void espt_set_WPA_PSK(_espt *e, const char *wpa_psk);
void espt_set_dst_ipaddr(_espt *e, const uint8_t *dst_ip);
void espt_set_transport(_espt *e, uint8_t transport);
void espt_set_dst_port(_espt *e, uint16_t port);
void espt_set_src_port(_espt *e, uint16_t port);

/*
 * Establece la conexión wifi y abre la comunicación tcp/udp
 */
void espt_connect(_espt *e);

/*
 * Cierra los puertos de comunicación tcp/udp, desconecta del AP y baja la placa
 */
void espt_disconnect(_espt *e);

/*
 * Cierra los puertos de comunicación tcp/udp, desconecta del AP y vuelve a conectar
 */
void espt_reconnect(_espt *e);


/*
 * Hace una llamda a la funcion espt_chipenable con el valor 1
 */
void espt_wakeup(_espt *e);

/*
 * Hace una llamda a la funcion espt_chipenable con el valor 0
 */
void espt_sleep(_espt *e);

/*
 * Retorna el valor del flag _espt.status correspondiente a flag
 */
uint8_t espt_status(_espt *e, uint8_t flag);

/*
 * Retorna 1 si el valor del flag _ESPT_STATUS_IDLE es 0
 */
uint8_t espt_is_busy(_espt *e);

/*
 * Vuelve a 0 el flag _ESPT_STATUS_ERROR
 */
void espt_error_reset(_espt *e);

/*
 * Setea wifi mode, retorna 1 si el seteo se pudo iniciar, 0 si no se incio el comando por
 * estar dentro de tiempo de espera, y por lo tanto se debe volver a llamar
 */
uint8_t espt_send_wifi_mode(_espt *e);


/*
 * Envia una peticion de alive al chip, retorna 1 si el comando se pudo iniciar, 0 si no se incio el comando
 */
uint8_t espt_alive(_espt *e);

/*
 * Envia una peticion de restore, retorna 1 si el comando se pudo iniciar, 0 si no se incio el comando por
 * estar dentro de tiempo de espera, y por lo tanto se debe volver a llamar
 */
uint8_t espt_restore(_espt *e);


/*
 * Modifica el comportamiento de autoconeccion
 * autoconn=1: Se reconecta automaticamente en el proximo startup con la configuracion guardada en flash
 * autoconn=0: No se reconecta automaticamente
 * retorna 1 si el comando se pudo iniciar, 0 si no se incio el comando
 */
uint8_t espt_set_autoconn(_espt *e, uint8_t autoconn);


/*
 * Envia una peticion listar redes disponibles
 * La respuesta a este comando se envoa mediamte la funcion de asociada con espt_attach_receive_control
 * retorna 1 si el comando se pudo iniciar, 0 si no se incio el comando por
 */
uint8_t espt_list_available_ap(_espt *e);

/*
 * Envia una peticion conectar con el ap de ssid y passwd enviado
 * retorna 1 si el comando se pudo iniciar, 0 si no se incio el comando
 */
uint8_t espt_ap_connect(_espt *e);


/*
 * Envia una peticion de desconexion, retorna 1 si el comando se pudo iniciar, 0 si no se incio el comando
 */
uint8_t espt_ap_disconnect(_espt *e);


/*
 * Obtiene la informacion IP local
 */
void espt_acquire_ip_data(_espt *e);


/*
 * retorna un array de uint8_t de 4 posicionees con al IP del ap asociado
 */
uint8_t* espt_get_apaddr(_espt *e);


/*
 * Retorna la IP del ap asociado como el char array *ip, terminado en '\0'
 * Retorna la cantidad de caracteres en el char array
 * Para que el metodo sea seguro *ipc deberia tener al menos 16 bytes
 */
uint8_t espt_get_char_apaddr(_espt *e, char *ipc);


/*
 * retorna un array de uint8_t de 4 posiciones con la IP asociada al esp
 */
uint8_t* espt_get_srcaddr(_espt *e);


/*
 * Setea la IP asociada al esp como el char array *ip terminado en '\0'
 * Retorna la cantidad de caracteres en el char array
 * Para que el metodo sea seguro *ipc deberia tener al menos 16 bytes
 */
uint8_t espt_get_char_srcaddr(_espt *e, char *ipc);


/*
 * Envia una peticion para abrir un socket tcp/udp, retorna 1 si el comando se pudo iniciar, 0 si no se incio el comando
 */
uint8_t espt_socket_open(_espt *e);


/*
 * Envia una peticion de envio de datos por el socket previamente abierto
 * Los datos en el puntero buff debe permanecer hasta que la bandera _ESPT_STATUS_ESP_IS_BUSY sea 0 y _ESPT_STATUS_SOCKET_SEND_OK
 * retorna 1 si el comando se pudo iniciar, 0 si no se incio el comando
 */
uint8_t espt_socket_send(_espt *e, uint8_t *buff, uint32_t Len);

/*
 * Envia una peticion de cierre del socket previamente abierto
 * retorna 1 si el comando se pudo iniciar, 0 si no se incio el comando por
 * estar dentro de tiempo de espera, y por lo tanto se debe volver a llamar
 */
uint8_t espt_socket_close(_espt *e);


/*
 * El procesamiento se realiza mediante las llamadas recurrentes a este procedimiento
 * @param period es la cantidad de tiempo en ms entre llamados a work, utilizado para manejar los timeout
 */
void espt_work(_espt *e, uint16_t period);


/*
 * Le notifica a este modulo que un envio iniciado medainte la funcion enlazada mediante espt_attach_send_data
 * ya fue finalizada y se esta listo para enviar mas datos
 */
void espt_send_data_finished(_espt *e);


/*
 * Se ingresa datos raw recibidos desde el espt, por ejemplo desde el puerto usart conectado al esp
 */
void espt_from_esp_received(_espt *e, uint8_t *buff, uint32_t Len);


/*
 * Enlaza la funcion callback para recepcion de datos desde el esp
 * La funcion enazada será llamada por esta libreria cada vez que se reciban datos
 * desde el chip, ya parseados.
 */
void espt_attach_receive_data(_espt *e , espt_receive_def f);


/*
 * Enlaza la funcion callback para recepcion de datos de control desde el esp
 * La funcion enazada será llamada por esta libreria cada vez que se reciban datos
 * de control desde el chip, por ejemplo una peticion de listar ap disponible
 */
void espt_attach_receive_control(_espt *e, espt_receive_def f);


/*
 * Enlaza la funcion callback para envio de datos hacia el esp
 * La funcion enazada será llamada por esta libreria cada vez que se deba enviar
 * datos hacia el chip.
 * Por ejemplo, la funcion enlazada recibe los datos y los escribe en el puerto UART conectado al chip esp
 */
void espt_attach_send_data(_espt *e, espt_receive_def f);


/*
 * Enlaza la funcion callback para habilitar o deshablitar el chip
 */
void espt_attach_chip_enable(_espt *e, espt_chipenable_def f);


#endif // ESPT_H
