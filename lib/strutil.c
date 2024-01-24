/*
 * strutil.c
 *
 *  Created on: Jan 9, 2024
 *      Author: psilva
 */
#include "strutil.h"

/*
 * Convierte el entero en un *str
 * @param *number es el numero a convertir
 * @param *str es el resultado, terminado en '\0'
 * @result retorna la cantidad de digitos convertidos, sin el contar el '\0'
 */
uint8_t _uint32_to_str(uint32_t number, char *str){
    char result[] = "0000000000\0";
    char *ptr = &result[9];
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
 * Convierte el entero con signo en un *str
 * @param *number es el numero a convertir
 * @param *str es el resultado, terminado en '\0'
 * @result retorna la cantidad de digitos convertidos, contando el - si corresponde y sin el contar el '\0'
 */
uint8_t _int32_to_str(int32_t number, char *str){
	if(number < 0){
		str[0] = '-';
		return 1 + _uint32_to_str(-1*number, str+1);
	}else
		return _uint32_to_str(number, str);
}

/*
 * Convierte *str en su valor numerico
 * @param *str cadena a convertir, la cadena es finalizada en cualquier caracter que no sea numerico, idealmente '\0'
 * @param *number es el resultado
 * @param max_digits la maxima cantidad de digitos
 * @result retorna la cantidad de caracteres convertidos, 0 significa que falló la conversion
 */
uint8_t _str_to_uint32(const char *str, uint32_t *number, uint8_t max_digits){
    uint8_t size = 0;
    uint8_t count = 0;
    const uint32_t pow[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
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
 * Convierte *str en su valor numerico
 * @param *str cadena a convertir, la cadena puede contener un signo - al inicio y es finalizada en cualquier caracter que no sea numerico, idealmente '\0'
 * @param *number es el resultado
 * @param max_digits la maxima cantidad de digitos
 * @result retorna la cantidad de caracteres convertidos sin contar el '-' (si lo hubiera), 0 significa que falló la conversion
 */
uint8_t _str_to_int32(const char *str, int32_t *number, uint8_t max_digits){
	uint32_t n;
	uint8_t count;
	if(*str == '-'){
		n = (uint32_t)(*number * -1);
		count = _str_to_uint32(str+1, &n, max_digits);
		*number = (int32_t)(-1*n);
		return count;
	}else{
		n = (uint32_t)*number;
		count = _str_to_uint32(str, &n, max_digits);
		*number = (int32_t)n;
		return count;
	}
}


/*
 * Convierte *str de dos caracteres representando un hexadecimal en su valor numerico
 * @param *str cadena a convertir
 * @param *number es el resultado
 * @result retorna 1 si se convirtió correctamente, 0 en caso contrario
 */
uint8_t _hex_to_uint8(const char *str, uint8_t *number){
	uint8_t hl[2];
	for (uint8_t i = 0; i < 2; i++) {
		if(str[i] >= '0' && str[i]<='9')
			hl[i] = str[i] - '0';
		else if(str[i] >= 'a' && str[i]<='f')
			hl[i] = str[i] - 'a' + 10;
		else if(str[i] >= 'A' && str[i]<='F')
			hl[i] = str[i] - 'A' + 10;
		else
			return 0; // Conversion failed
	}
	*number = (hl[0]<<4) | hl[1];
	return 1;
}


/*
 * Convierte *str de ocho caracteres representando un hexadecimal en su valor numerico
 * @param *str cadena a convertir
 * @param *number es el resultado
 * @result retorna 1 si se convirtió correctamente, 0 en caso contrario
 */
uint8_t _hex_to_uint32(const char *str, uint32_t *number){
	uint8_t hl[8];
	for (uint8_t i = 0; i < 8; i++) {
		if(str[i] >= '0' && str[i]<='9')
			hl[i] = str[i] - '0';
		else if(str[i] >= 'a' && str[i]<='f')
			hl[i] = str[i] - 'a' + 10;
		else if(str[i] >= 'A' && str[i]<='F')
			hl[i] = str[i] - 'A' + 10;
		else
			return 0; // Conversion failed
	}
	*number = (hl[0]<<28) | (hl[1]<<24) | (hl[2]<<20) | (hl[3]<<16) | (hl[4]<<12) | (hl[5]<<8) | (hl[6]<<4) | hl[7];
	return 1;
}


/*
 * Convierte number en su valor hexadecimal de dos caracteres
 * @param *number es el numero a convertir
 * @param *str cadena que representa el hexadecimal
 */
void _uint8_to_hex(const uint8_t *number, char *str){
	if((*number>>4) < 10)
		str[0] = (*number>>4)+'0';
	else
		str[0] = (*number>>4)+'a'-10;
	if((*number&15) < 10)
		str[1] = (*number&15)+'0';
	else
		str[1] = (*number&15)+'a'-10;
}


/*
 * Convierte number en su valor hexadecimal de 8 caracteres
 * @param *number es el numero a convertir
 * @param *str cadena que representa el hexadecimal
 */
void _uint32_to_hex(const uint32_t *number, char *str){
	uint8_t value;
	for (uint8_t i = 0; i < 8; ++i) {
		value = ((*number<<(i*4))>>28);
		if(value < 10)
			str[i] = value + '0';
		else
			str[i] = value + 'a' - 10;
	}
}


/*
 * copia el string src terminado en \0 en dst terminado en \0
 * @param max: maxima cantidad de caracteres que se van a escribir en dst (incluido el '\0')
 * @result retorna la cantidad de caracteres copiados, sin contar el \0
 * OJO al orden de los parametros: es src -> dst, alrevez del strcpy de C que es dst <- src
 * Para mi es mas natural asi, es como cp src dst
 * Además, max no le da el comportamiento de strncpy, no se hace padding
 */
uint32_t _strcpy(const char *src, char *dst, uint32_t max){
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
 * Retorna el largo de una cadena de caracteres terminada en \0
 * @param *str cadena a determinar el largo
 * @param max, maximo de caracteres a contar
 * @result la cantidad de caracteres en la cadena, sin contar el \0
 */
uint32_t _strlen(const char *str, uint32_t max){
    uint32_t cnt = 0;
    while(*str && cnt < max){
        str++;
        cnt++;
    }
    return cnt;
}

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
uint16_t _cut(const char *str, char separator, uint8_t field, char *res, uint16_t max_size){
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
 * Compara dos cadenas de caracteres finalizadas en \0
 * @param *str1 es la cadena 1, terminada en \0
 * @param *str2 es la cadena 2, terminada en \0
 * @param max es la longitud maxima que se va a comparar
 * @result retorna 0 si las cadenas son iguales, 1 si son difeentes, 2 si se alcanzó max
 */
uint8_t _strcmp(const char *str1, const char *str2, uint32_t max){
    while (*str1 == *str2++){
        if (*str1++ == 0) return 0;
        if (max-- == 0) return 2;
    }
    return 1;
}

/*
 * Convierte a mayusculas una cadena de caracteres terminada en \0
 * @param strin es la cadena a convertir
 * @param strout es la cadena convertida
 * @param max es la cantidad de caracteres maximas a convertir
 * Notece que strout debe poder almacenar max+1 caracteres para poder finalizar
 * la cadena de salida con \0
 * @result 0 si la cadena se convirtió sin problemas, 1 si se alcanzo el valor max
 */
uint8_t _touppercase(const char *strin, char *strout, uint32_t max){
  const uint8_t d = ('A'-'a');
  while(*strin != 0){
    if(*strin >= 'a' && *strin <= 'z')
      *strout++ = *strin + d;
    else
      *strout++ = *strin;
    if (--max == 0){
      *strout = 0;
      return 1;
    }
    strin++;
  }
  return 0;
}

/*
 * Convierte a minusculas una cadena de caracteres terminada en \0
 * @param strin es la cadena a convertir
 * @param strout es la cadena convertida
 * @param max es la cantidad de caracteres maximas a convertir
 * Notece que strout debe poder almacenar max+1 caracteres para poder finalizar
 * la cadena de salida con \0
 * @result 0 si la cadena se convirtió sin problemas, 1 si se alcanzo el valor max
 */
uint8_t _tolowercase(const char *strin, char *strout, uint32_t max){
  const uint8_t d = ('A'-'a');
  while(*strin != 0){
    if(*strin >= 'A' && *strin <= 'Z')
      *strout++ = *strin - d;
    else
      *strout++ = *strin;
    if (--max == 0){
      *strout = 0;
      return 1;
    }
    strin++;
  }
  return 0;
}
