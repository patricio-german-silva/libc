/*
 * strutil.h
 *
 *
 *
 *  Created on: Jan 9, 2024
 *      Author: psilva
 *
 * Version 1
 *
 *
 * Utilidades básicas de manejo de string:
 * 							_strcpy
 * 							_uint32_to_str
 * 							_str_to_uint32
 * 							_strlen
 * 							_cut
 * 							_strcmp
 * 							_touppercase
 */
#ifndef INC_STRUTIL_H_
#define INC_STRUTIL_H_
#include "stdint.h"

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
 * @result retorna la cantidad de caracteres copiados, sin contar el \0
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
uint8_t _strcmp(const char *str1, const char *str2, uint32_t max);

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


#endif /* INC_STRUTIL_H_ */
