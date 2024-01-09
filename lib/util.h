/*
 * util.h
 *
 *
 *
 *  Created on: Oct 2, 2022
 *      Author: psilva
 *
 * Version 15
 *
 *
 * Utilidades varias
 *
 * Heartbeat
 * Temporizacion
 * Almacenamiento en flash
 * Versionado con fecha y hora para development
 * Utilidades básicas de manejo de string:
 * 							_strcpy
 * 							_uint32_to_str
 * 							_str_to_uint32
 * 							_strlen
 * 							_cut
 *
 * ------------------------------------------------------------------------*
 * HEARTBEAT
 * ------------------------------------------------------------------------*
 * typedef struct _hb define un patron de base y la funcion para modificarlo o resetearlo
 *
 * ------------------------------------------------------------------------*
 * TEMPORIZACION
 * ------------------------------------------------------------------------*
 * typedef struct _usrtick y _usrtick_proc
 *
 * Llamada a funciones por tiempo
 *
 * Funcionamiento con systick o un temporizador con un un periodo de 1us, depende de la implementacion
 * de las funciones static _usrtick_get_us_since() y _usrtick_get_us().
 *
 * En usrtick_init se establece el periodo base, cada funcion enlazada con usrtick_attach será
 * invocada cada n periodos base
 *
 * usrtick_get_process_utime retrna el tiempo en us consumido por una funcion en particlar en el
 * ultimo segundo. usrtick_get_global_utime retorna la suma de todas las funciones
 *
 * usrtick_get_uptime retorna un valor no exacto del tiempo de uptime, en segundos
 *
 * ------------------------------------------------------------------------*
 * ALMACENAMIENTO EN FLASH:
 * ------------------------------------------------------------------------*
 * Editar Linker Script, editar mapa de memoria y agregar seccion, ej.:
 *

 MEMORY
{
  RAM    (xrw)    : ORIGIN = 0x20000000,   LENGTH = 20K
  FLASH    (rx)    : ORIGIN = 0x8000000,   LENGTH = 127K
  FLASHDATA    (rx)    : ORIGIN = 0x801FC00,   LENGTH = 1K
}

  .configdata :
  {
    KEEP(*(.configdata))
    KEEP(*(.configdata*))
  } >FLASHDATA

 *
 *
 *
 *
 *------------------------------------------------------------------------*
 * VERSIONADO:
 * ------------------------------------------------------------------------*
 * El autoversionado utiliza las variables de precompilador __DATE__ y
 * __TIME__, por lo tanto este archivo se debe recompilar siempre.
 * Para forzar que el makefile haga esto ir a:
 * Project -> Properties -> C/C++ Build -> Settings -> Build Steps -> Pre-build Steps -> Command: touch "../Core/Src/util.c"
 *
 * Deberá existir entonces algo como

  #ifndef _FIRMWARE_VERSION_
  #define _FIRMWARE_VERSION_PREFIX_ "0.38.0_build_\0"
  #define _FIRMWARE_VERSION_SUFFIX_ "-alpha\0"
  char firmware_version[42];
  #else
  const char firmware_version[] = _FIRMWARE_VERSION_;
  #endif

 * y dentro de la funcion main()
 *

  #ifndef _FIRMWARE_VERSION_
  get_firmware_version(_FIRMWARE_VERSION_PREFIX_, _FIRMWARE_VERSION_SUFFIX_, firmware_version);
  #endif

 * ------------------------------------------------------------------------*
 */

#ifndef INC_UTIL_H_
#define INC_UTIL_H_
#include "stdint.h"


/* Utilizar rutinas de heartbeat */
#define _UTIL_USE_HEARTBEAT_UTILITIES

/* Utilizar llamadas cronometradas */
#define _UTIL_USE_USRTICK_UTILITIES

/* Si se utilizar llamadas cronometradas, realizar calculos de tiempos de uso */
#define _UTIL_USE_USRTICK_STATISTICS

/* Utilizar utilidades de almacenamineto en flash */
#define _UTIL_USE_STORAGE_CONFIG_ON_FLASH_UTILITIES

/* Utilizar utilidades de versionado */
#define _UTIL_USE_FIRMWARE_VERSION_UTILITIES

/* Utilizar utilidades string */
#define _UTIL_USE_STRING_UTILITIES


//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _UTIL_USE_STORAGE_CONFIG_ON_FLASH_UTILITIES

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_flash.h"
#include "stm32f1xx_hal_flash_ex.h"

#define _UTIL_CONFIG_DATA_SIZE		1024
#define _UTIL_CONFIG_DATA_TOKEN_0	0xf3
#define _UTIL_CONFIG_DATA_TOKEN_1	0x98
#define _UTIL_CONFIG_DATA_TOKEN_2	0xfd
#define _UTIL_CONFIG_DATA_TOKEN_3	0xc6

typedef struct{
	// IP config
	char SSID[33];
	uint8_t padd_0[3];
	char WPA_PSK[64];
	uint8_t dstipaddr[4];
	uint16_t dst_port;
	uint16_t src_port;
	uint8_t transport;	// 0: tcp. 1: udp
	uint8_t connect;
	uint8_t is_valid;
	uint8_t padd_1;

	// Control config
	uint16_t mi_kp;
	uint16_t mi_ki;
	uint16_t mi_kd;
	uint16_t md_kp;
	uint16_t md_ki;
	uint16_t md_kd;
	uint16_t pp;
	uint16_t btime;
	uint16_t fpwm;
	uint16_t spwm;
	uint16_t iet;
	uint16_t period;


	// Basic
	uint32_t version;
	uint8_t token[4];
	uint8_t padding[_UTIL_CONFIG_DATA_SIZE-4-33-3-64-4-2-2-1-1-1-1-2-2-2-2-2-2-2-2-2-2-2-2-4-4];
	uint32_t chksum;
} __attribute__ ((packed)) _config_data;

#endif
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


typedef union{
	uint8_t u8[4];
	int8_t i8[4];
	char c[4];
	uint16_t u16[2];
	int16_t i16[2];
	uint32_t u32;
	int32_t i32;
}_work;





//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _UTIL_USE_HEARTBEAT_UTILITIES

typedef void (*heartbeat_gpio_def)(uint8_t state);

typedef struct{
	uint16_t state;
	uint16_t stmsk;
	uint16_t dfmsk;
	heartbeat_gpio_def gpio_callback;
}_hb;


void heartbeat_init(_hb *hb);
void heartbeat(_hb *hb);
void heartbeat_attach(_hb *hb, heartbeat_gpio_def f);
void heartbeat_set_pattern(_hb *hb, uint16_t pattern);
void heartbeat_restore_pattern(_hb *hb);

#endif	// _UTIL_USE_HEARTBEAT_UTILITIES
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _UTIL_USE_USRTICK_UTILITIES

// Cantidad maxima de funciones callback
#define _UTIL_USRTICK_CALLBACK_NUM			5


/*
 * Typedef Funcion callback para tarea cronometrada
 */
typedef void (*usrtick_attach_def)();


/*
 *  usrtick_callback: funcion callback asociada al proceso
 *  period: frecuencia a la que se llama la funcion callback, period*ptime us
 *  curr: valor actual del contador para este proceso
 *  utime: used time: us consumidos por este proceso en 1 seg
 *  mtime: max time: maximo de us consumidos por este proceso en una ejecucion dentro del ultimo segundo
 *  utimectr y mtimectr: contadores de tiempo actual
 */
typedef struct{
	usrtick_attach_def usrtick_callback;
	uint32_t period;
	uint32_t curr;
#ifdef _UTIL_USE_USRTICK_STATISTICS
	uint32_t utime;
	uint32_t mtime;
	uint32_t utimectr;
	uint32_t mtimectr;
#endif
}_usrtick_proc;



/*
 * ltime: last time: valor de tiempo anterior
 * ctime: current time: us entre tiempo actual y ltime
 * ptime: period time: periodo base, en us. El periodo de cada callback esta dado en esta unidad, debe ser el mayor posible para reducir sobrecarga
 * stime: 1 second time: contador de us para alcanzar 1 segundo y calcular tiempos;
 * uptime: tiempo desde el arranque, en segundos
 * cbcnt: callback counter: cantidad de funciones callback actualmente en uso
 * cbpcs: callback processed: funcion callback hasta la que se ha procesado (no se procesan dos seguidas sin forzar un rellamado desde el main while)
 */
typedef struct{
	uint32_t ltime;
	uint32_t ctime;
	uint32_t ptime;
#ifdef _UTIL_USE_USRTICK_STATISTICS
	uint32_t stime;
	uint32_t uptime;
#endif
	uint8_t cbcnt;
	uint8_t cbpcs;
	_usrtick_proc proc[_UTIL_USRTICK_CALLBACK_NUM];
}_usrtick;


void usrtick_init(_usrtick *utk, uint32_t p);
void usrtick_attach(_usrtick *utk, usrtick_attach_def f, uint32_t p);
void usrtick_work(_usrtick *utk);
#ifdef _UTIL_USE_USRTICK_STATISTICS
uint32_t usrtick_get_process_utime(_usrtick *utk, uint8_t proc);
uint32_t usrtick_get_process_mtime(_usrtick *utk, uint8_t proc);
uint32_t usrtick_get_global_utime(_usrtick *utk);
void usrtick_get_uptime(_usrtick *utk, uint32_t *s, uint32_t *us);
#endif
#endif	// _UTIL_USE_USRTICK_UTILITIES
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _UTIL_USE_STORAGE_CONFIG_ON_FLASH_UTILITIES
/*
 * @brief Carga en la estructura de datos *c la configuración almacenada en flash
 * y verifica su integridad
 * @param *c la estructura de datos de tipo _config_data donde se almacenará la configuración
 * almacenada en flash
 * @retval 1 si la configuracion en flash es válida, almacena en c->is_valid ese resultado
 * @retval 0 si la estructura en flash con cuenta con el token y/o checksum correcto
 *
 */
uint8_t load_config_data(_config_data *c);

/*
 * @brief Almacena en flash la estructura de datos *c con la configuración.
 * @param *c la estructura de datos de tipo _config_data donde se almacena la configuración
 * a ser almacenada en flash
 * @retval 0: se pudo almacenar correctamente
 * @retval 1: el token de la estructura de datos *c es incorrecto
 * @retval 2: fallo en borrado
 * @retval 3: fallo de escritura
 * @retval 4: error de verificación tras el guardado
 *
 */
uint8_t store_config_data(_config_data *c);

/*
 * @brief Calcula el checksum de la estructura de datos *c
 * @param *c la estructura de datos de tipo _config_data
 */
uint32_t checksum_config_data(_config_data *c);

/*
 * @brief valida la estructura de datos *c y actualiza el valor de c->is_valid
 * @param *c la estructura de datos de tipo _config_data a validar
 * @retval el valor actual de c->is_valid
 */
uint8_t validate_config_data(_config_data *c);

#endif
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------





//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _UTIL_USE_FIRMWARE_VERSION_UTILITIES
/*
 * time and date adds 19 chars, plus prefix, suffix and the trailing \0
 * @retval total string length
 */
uint8_t get_firmware_version(const char *prefix, const char *suffix, char *str);
#endif
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------




//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _UTIL_USE_STRING_UTILITIES

/*
 * Convierte el entero en un *str
 * @param *number es el numero a convertir
 * @param *str es el resultado, terminado en '\0'
 * @result retorna la cantidad de digitos convertidos, sin el contar el '\0'
 */
uint8_t _uint32_to_str(uint32_t number, char *str);

/*
 * Convierte el entero con signo en un *str
 * @param *number es el numero a convertir
 * @param *str es el resultado, terminado en '\0'
 * @result retorna la cantidad de digitos convertidos, contando el - si corresponde y sin el contar el '\0'
 */
uint8_t _int32_to_str(int32_t number, char *str);

/*
 * Convierte *str en su valor numerico
 * @param *str cadena a convertir, la cadena es finalizada en cualquier caracter que no sea numerico, idealmente '\0'
 * @param *number es el resultado
 * @param max_digits la maxima cantidad de digitos
 * @result retorna la cantidad de caracteres convertidos, 0 significa que falló la conversion
 */
uint8_t _str_to_uint32(const char *str, uint32_t *number, uint8_t max_digits);

/*
 * Convierte *str en su valor numerico
 * @param *str cadena a convertir, la cadena puede contener un signo - al inicio y es finalizada en cualquier caracter que no sea numerico, idealmente '\0'
 * @param *number es el resultado
 * @param max_digits la maxima cantidad de digitos
 * @result retorna la cantidad de caracteres convertidos sin contar el '-' (si lo hubiera), 0 significa que falló la conversion
 */
uint8_t _str_to_int32(const char *str, int32_t *number, uint8_t max_digits);

/*
 * copia el string src terminado en \0 en dst terminado en \0
 * @param max: maxima cantidad de caracteres que se van a escribir en dst (incluido el '\0')
 * OJO al orden de los parametros: es src -> dst, alrevez del strcpy de C que es dst <- src
 * Para mi es mas natural asi, es como cp src dst
 * Además, max no le da el comportamiento de strncpy, no se hace padding
 */
uint32_t _strcpy(const char *src, char *dst, uint32_t max);

/*
 * Retorna el largo de una cadena de caracteres terminada en \0
 * @param *str cadena a determinar el largo
 * @param max, maximo de caracteres a contar
 * @result la cantidad de caracteres en la cadena, sin contar el \0
 */
uint32_t _strlen(const char *str, uint32_t max);

/*
 * Similar a GNU cut, pero para una sola linea, terminada '\0', CR o LF (0x0D0A)
 * @param *str cadena a dividir
 * @param separator, caracter a tomar como divisor
 * @param campo seleccionado (1...)
 * @param *res resultado, con terminador '\0'
 * @param max_size tamaño maximo del resultado, sin contar el '\0', notece que *res debe
 * poder almacenar al menos max_size+1 caracteres
 * @result retorna la cantidad de caracteres en *str2, sin contar '\0'
 */
uint16_t _cut(const char *str, char separator, uint8_t field, char *res, uint16_t max_size);


/*
 * Compara dos cadenas de caracteres finalizadas en \0
 * @param *str1 es la cadena 1, terminada en \0
 * @param *str2 es la cadena 2, terminada en \0
 * @param max es la longitud maxima que se va a comparar
 * @result retorna 0 si las cadenas son iguales, 1 si son difeentes, 2 si se alcanzó max
 */
uint8_t _strcmp(const char *str1, const char *str2, uint32_t max){

/*
 * Convierte a mayusculas una cadena de caracteres terminada en \0
 * @param strin es la cadena a convertir
 * @param strout es la cadena convertida, puede apuntar a la misma cadena que strin
 * @param max es la cantidad de caracteres maximas a convertir, debe ser mayor a 0
 * Notece que strout debe poder almacenar max+1 caracteres para poder finalizar
 * la cadena de salida con \0
 * @result 0 si la cadena se convirtió sin problemas, 1 si se alcanzo el valor max
 */
uint8_t _touppercase(const char *strin, char *strout, uint32_t max);

/*
 * Convierte a minusculas una cadena de caracteres terminada en \0
 * @param strin es la cadena a convertir
 * @param strout es la cadena convertida, puede apuntar a la misma cadena que strin
 * @param max es la cantidad de caracteres maximas a convertir, debe ser mayor a 0
 * Notece que strout debe poder almacenar max+1 caracteres para poder finalizar
 * la cadena de salida con \0
 * @result 0 si la cadena se convirtió sin problemas, 1 si se alcanzo el valor max
 */
uint8_t _tolowercase(const char *strin, char *strout, uint32_t max);


#endif /* INC_UTIL_H_ */
