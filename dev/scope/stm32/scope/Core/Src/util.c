/*
 * util.c
 *
 *  Created on: Oct 2, 2022
 *      Author: psilva
 */
#include "util.h"


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
static _work w;

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
