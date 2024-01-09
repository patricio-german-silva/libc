/*
 * util.c
 *
 *  Created on: Oct 2, 2022
 *      Author: psilva
 */
#include "util.h"



// Utilidad
static _work w;




//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _UTIL_USE_USRTICK_UTILITIES
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



// Si los ticks son cada mas de 1us, cantidad de ticks en 1us (como en el caso de systick)
#define _UTIL_USRTICK_TICKS_ON_US			72

static inline uint32_t _usrtick_get_us();
static inline uint32_t _usrtick_get_us_since(uint32_t since);


/* Inicializa temporizadores */
void usrtick_init(_usrtick *utk, uint32_t p){
	utk->ltime = 0;
	utk->ctime = 0;
	utk->ptime = p;
#ifdef _UTIL_USE_USRTICK_STATISTICS
	utk->stime = 0;
	utk->uptime = 0;
#endif
	utk->cbcnt = 0;
	utk->cbpcs = 0;
}


void usrtick_attach(_usrtick *utk, usrtick_attach_def f, uint32_t p){
	if(utk->cbcnt < _UTIL_USRTICK_CALLBACK_NUM){
		utk->proc[utk->cbcnt].period = p;
		utk->proc[utk->cbcnt].curr = 0;
		utk->proc[utk->cbcnt++].usrtick_callback = f;
	}
}


void usrtick_work(_usrtick *utk){
	if(utk->cbpcs != 0 || _usrtick_get_us_since(utk->ltime) >= utk->ptime){

		if(utk->cbpcs == 0){
#ifdef _UTIL_USE_USRTICK_STATISTICS
			utk->stime += _usrtick_get_us_since(utk->ltime);
#endif
			utk->ltime = _usrtick_get_us();
		}

		while(utk->cbpcs < utk->cbcnt){
			utk->proc[utk->cbpcs].curr++;
			if(utk->proc[utk->cbpcs].curr >= utk->proc[utk->cbpcs].period){
				utk->proc[utk->cbpcs].curr = 0;
#ifdef _UTIL_USE_USRTICK_STATISTICS
				uint32_t tmptime = _usrtick_get_us();

				utk->proc[utk->cbpcs].usrtick_callback();

				tmptime = _usrtick_get_us_since(tmptime);
				utk->proc[utk->cbpcs].utimectr += tmptime;
				if(tmptime >= utk->proc[utk->cbpcs].mtime)
					utk->proc[utk->cbpcs].mtimectr = tmptime;
#else
				utk->proc[utk->cbpcs].usrtick_callback();
#endif

				utk->cbpcs++;
				return;
			}
			utk->cbpcs++;
		}
		utk->cbpcs = 0;
#ifdef _UTIL_USE_USRTICK_STATISTICS
		if(utk->stime >= 1000000){
			for(uint8_t i = 0; i < utk->cbcnt ; i++){
				utk->proc[i].utime = utk->proc[i].utimectr;
				utk->proc[i].utimectr = 0;
				utk->proc[i].mtime = utk->proc[i].mtimectr;
				utk->proc[i].mtimectr = 0;
			}
			utk->uptime++;
			utk->stime -= 1000000;
		}
#endif
	}
}

#ifdef _UTIL_USE_USRTICK_STATISTICS
/*
 * Retorna el tiempo de ejecución del proceso proc enb el ultimo segundo
 */
uint32_t usrtick_get_process_utime(_usrtick *utk, uint8_t proc){
	return utk->proc[proc].utime;
}

/*
 * Retorna el mayor de los tiempos de ejecución del proceso proc en el ultimo segundo
 */
uint32_t usrtick_get_process_mtime(_usrtick *utk, uint8_t proc){
	return utk->proc[proc].mtime;
}

/*
 * Retorna el tiempo de ejecución global
 */
uint32_t usrtick_get_global_utime(_usrtick *utk){
	uint32_t utime = 0;
	for(uint8_t i = 0; i < utk->cbcnt; i++)
		utime += utk->proc[i].utime;
	return utime;
}

void usrtick_get_uptime(_usrtick *utk, uint32_t *s, uint32_t *us){
	*s = utk->uptime;
	*us = utk->stime/_UTIL_USRTICK_TICKS_ON_US;
}
#endif

//-------------------------------------------------------------------------------------------------------------------
//	PRIVADO

/*
 * Retorna la cantidad de us actual
 */
static inline uint32_t _usrtick_get_us(){
	return (uwTick*1000)+((SysTick->LOAD - SysTick->VAL)/_UTIL_USRTICK_TICKS_ON_US);
}

/*
 * Retorna la cantidad de us desde since
 */
static inline uint32_t _usrtick_get_us_since(uint32_t since){
	uint32_t curr = (uwTick*1000)+((SysTick->LOAD - SysTick->VAL)/_UTIL_USRTICK_TICKS_ON_US);
	if(since > curr)
		return 0xFFFFFFFF - since + curr;
	return curr - since;
}


#endif	// _UTIL_USE_USRTICK_UTILITIES
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------





//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _UTIL_USE_HEARTBEAT_UTILITIES
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* Inicializa hearbeat */
void heartbeat_init(_hb *hb){
	hb->state = 0;
	hb->dfmsk = 0b1010000000000000;
	hb->stmsk = 0b1010000000000000;
}

void heartbeat(_hb *hb){
	hb->gpio_callback((hb->stmsk & (1 << hb->state)) == 0);
	hb->state = (hb->state+1) & 15;
}

void heartbeat_attach(_hb *hb, heartbeat_gpio_def f){
	hb->gpio_callback = f;
}

void heartbeat_set_pattern(_hb *hb, uint16_t pattern){
	hb->stmsk = pattern;
}

void heartbeat_restore_pattern(_hb *hb){
	hb->stmsk = hb->dfmsk;
}

#endif	// _UTIL_USE_HEARTBEAT_UTILITIES
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _UTIL_USE_STORAGE_CONFIG_ON_FLASH_UTILITIES
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* Configuracion en memoria no volatil */
__attribute__((__section__(".configdata"), used)) _config_data config_data_flash;


/* Cargo en la estructura en ram  *c la configuracion en flash
 * El valor de config_data_flash.is_valid almacenado en flash es ignorado, la
 * condicion de validez se recalcula en cada carga y se establece en c->is_valid
 * de acuerdo al resultado de verificacion
 * Si resulta invalido, los valores correctos de token se almacenan en c->token[4] de
 * modo que se pueda cargar datos validos y recalcular chksum para almacenar en flash
 */
uint8_t load_config_data(_config_data *c){
	uint32_t chk = 0;
	for(w.u16[0] = 0; w.u16[0] < (_UTIL_CONFIG_DATA_SIZE/4)-1; w.u16[0]++){	// last u32 is checksum
		((uint32_t *)c)[w.u16[0]] = ((uint32_t *)&config_data_flash)[w.u16[0]];
		chk ^= ((uint32_t *)c)[w.u16[0]];
	}
	((uint32_t *)c)[w.u16[0]] = ((uint32_t *)&config_data_flash)[w.u16[0]];	// No se computa checksum sobre el checksum (ultimos 4bytes)

	c->is_valid = (chk == c->chksum && chk == config_data_flash.chksum && c->token[0] == 0xf3 &&
			c->token[1] == 0x98 && c->token[2] == 0xfd &&
			c->token[3] == 0xc6);
	return c->is_valid;
}



/*
 * Almacena la configuracion en FLASH
 * @retval 0: se pudo almacenar correctamente, 1: estructura en memoria invalida, 2: fallo en borrado, 3: fallo de escritura, 4: error de verificación
 */
uint8_t store_config_data(_config_data *c){
	uint32_t u32 = (uint32_t)&config_data_flash;
	uint8_t u8;
	FLASH_EraseInitTypeDef EraseInitStruct;

	// Computo checksum en memoria
	c->chksum = checksum_config_data(c);

	// Verifico la validez de la estructura de datos en memoria antes de almacenar
	if(!validate_config_data(c))
		return 1;

	HAL_FLASH_Unlock();
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.PageAddress = u32;
	EraseInitStruct.NbPages = 1;
	u8 = HAL_FLASHEx_Erase(&EraseInitStruct, &w.u32);
	if(u8 == HAL_OK){
		for(uint16_t u16 = 0; u16 < 1024 ; u16 +=4){
			w.u8[0] = ((uint8_t *)c)[u16];
			w.u8[1] = ((uint8_t *)c)[u16+1];
			w.u8[2] = ((uint8_t *)c)[u16+2];
			w.u8[3] = ((uint8_t *)c)[u16+3];
			u8 = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, u32, w.u32);
			if(u8 != HAL_OK){
				HAL_FLASH_Lock();
				return 3;
			}
			u32 += 4;
		}
	}else{
		HAL_FLASH_Lock();
		return 2;
	}
	HAL_FLASH_Lock();
	// Verifico
	u32 = 0;
	for(w.u16[0] = 0; w.u16[0] < (_UTIL_CONFIG_DATA_SIZE/4)-1; w.u16[0]++){	// last u32 is checksum
		u32 ^= ((uint32_t *)&config_data_flash)[w.u16[0]];
	}
	if(u32 == c->chksum && u32 == config_data_flash.chksum 	&&
			config_data_flash.token[0] == _UTIL_CONFIG_DATA_TOKEN_0 &&
			config_data_flash.token[1] == _UTIL_CONFIG_DATA_TOKEN_1 &&
			config_data_flash.token[2] == _UTIL_CONFIG_DATA_TOKEN_2 &&
			config_data_flash.token[3] == _UTIL_CONFIG_DATA_TOKEN_3)
		return 0;
	return 4;
}


/* Calcula el checksum de la estructura de datos de configuracion en ram y retorna su valor */
uint32_t checksum_config_data(_config_data *c){
	uint32_t chk = 0;
	for(w.u16[0] = 0; w.u16[0] < (_UTIL_CONFIG_DATA_SIZE/4)-1; w.u16[0]++){	// last u32 is checksum
		chk ^= ((uint32_t *)c)[w.u16[0]];
	}
	return chk;
}

/* Valida la estructura de datos de configuracion en ram y retorna el resultado */
uint8_t validate_config_data(_config_data *c){
	c->is_valid = (checksum_config_data(c) == c->chksum &&
			c->token[0] == _UTIL_CONFIG_DATA_TOKEN_0 &&
			c->token[1] == _UTIL_CONFIG_DATA_TOKEN_1 &&
			c->token[2] == _UTIL_CONFIG_DATA_TOKEN_2 &&
			c->token[3] == _UTIL_CONFIG_DATA_TOKEN_3);
	return c->is_valid;
}

#endif	// _UTIL_USE_STORAGE_CONFIG_ON_FLASH_UTILITIES
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------




//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _UTIL_USE_FIRMWARE_VERSION_UTILITIES
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/*
 * time and date adds 19 chars, plus prefix, suffix and the trailing \0
 * @retval total string length
 */
uint8_t get_firmware_version(const char *prefix, const char *suffix, char *str){
	uint8_t i = 0;
	char curtime[] = __TIME__;
	char curdate[] = __DATE__;
	while(*prefix)
		str[i++] = *(prefix++);
	str[i++] = curdate[ 7];
	str[i++] = curdate[ 8];
	str[i++] = curdate[ 9];
	str[i++] = curdate[10];
	str[i++] = '.';
	str[i++] = '0';
	if(curdate[0] == 'J' && curdate[1] == 'a' && curdate[2] == 'n')
		str[i++] = '1';
	else if(curdate[0] == 'F')
		str[i++] = '2';
	else if(curdate[0] == 'M' && curdate[1] == 'a' && curdate[2] == 'r')
		str[i++] = '3';
	else if(curdate[0] == 'A' && curdate[1] == 'p')
		str[i++] = '4';
	else if(curdate[0] == 'M' && curdate[1] == 'a' && curdate[2] == 'y')
		str[i++] = '5';
	else if(curdate[0] == 'J' && curdate[1] == 'u' && curdate[2] == 'n')
		str[i++] = '6';
	else if(curdate[0] == 'J' && curdate[1] == 'u' && curdate[2] == 'l')
		str[i++] = '7';
	else if(curdate[0] == 'A' && curdate[1] == 'u')
		str[i++] = '8';
	else if(curdate[0] == 'S')
		str[i++] = '9';
	else{
		i--;
		str[i++] = '1';
		if(curdate[0] == 'O')
			str[i++] = '0';
		else if(curdate[0] == 'N')
			str[i++] = '1';
		else if(curdate[0] == 'D')
			str[i++] = '2';
	}
	str[i++] = '.';
	if(curdate[4] != ' ')
		str[i++] = curdate[4];
	else
		str[i++] = '0';
	str[i++] = curdate[5];
	str[i++] = '_';
	for(uint8_t j = 0; j < 8; j++)
		str[i++] = curtime[j];

	while(*suffix)
		str[i++] = *(suffix++);


	str[i++] = 0;
	return i;
}
#endif	// _UTIL_USE_FIRMWARE_VERSION_UTILITIES
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------





//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _UTIL_USE_STRING_UTILITIES
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

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
 * copia el string src terminado en \0 en dst terminado en \0
 * @param max: maxima cantidad de caracteres que se van a escribir en dst (incluido el '\0')
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

#endif	// _UTIL_USE_STRING_UTILITIES
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
