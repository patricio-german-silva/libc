/**
 * ciphdev permite crear y mantener un bloque cifrado mediante el algoritmo
 * criptografico SPECK con clave de 128 bytes.
 *
 * @author Patricio Silva
 * @date Dec 21, 2023
 */
#include "ciphdev.h"

// Funciones de bajo nivel
static uint8_t ciphdev_header_verify(_ciphdev *cd);

/* Crea un bloque cifrado cd en el dispositvo dev de un tamaño bs sectores
 * utilizando la frase de cifrado de usuario user_key de longitud len que se
 * almacenará en el slot 0
 * @return 0 si el bloque se creó correctamente
 * @return 1 si el device no se pudo inicializar
 * @return 2 si el device es mas pequeño que bs
 * @return 3 si fallo la escritura en el device
 * @return 4 si fallo la llamada ioctl a SYNC
 */
uint8_t ciphdev_create (_ciphdev *cd, uint8_t dev, uint32_t bs, const char *user_key, uint8_t len){
	cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev CREATE device\0", 0, &dev, 1);

	// Inicializo el device
	if(cd->func_dev_initialize(cd->dev) != 0){
		cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev CREATE IOERROR device\0", 0, &dev, 1);
		return 1;
	}

	//check size
	uint32_t seccount, secsize;
	cd->func_dev_ioctl(cd->dev, _CIPHDEV_CTRL_GET_SECTOR_COUNT, &seccount);
	cd->func_dev_ioctl(cd->dev, _CIPHDEV_CTRL_GET_SECTOR_SIZE, &secsize);
	if((secsize/_CIPHDEV_SECTOR_SIZE)*seccount >= bs){
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: VOLUME SIZE (sectors)\0", 0, (uint8_t*)&bs, 4);
		// Create speck keys
		for(uint8_t i = 0; i < 4; i++){
			cd->func_random(&(cd->speck_key1[i]));
			cd->func_random(&(cd->speck_key2[i]));
		}
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: speck key 1 (decrypt)\0", 0, cd->u8speck_key1, 16);
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: speck key 2 (decrypt)\0", 0, cd->u8speck_key2, 16);
		// prepare md5 for key map
		_md5_context md5ctx;
		md5_init(&md5ctx);
		md5_update(&md5ctx, cd->u8speck_key1, 16);
		md5_update(&md5ctx, cd->u8speck_key2, 16);
		md5_finalize(&md5ctx);
		for(uint8_t i = 0; i < 16; i++)
			cd->u8key_map[i] = md5ctx.digest[i];
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: key_map\0", 0, cd->u8key_map, 16);

		// prepare md5 for user key hash
		md5_init(&md5ctx);
		md5_update(&md5ctx, (uint8_t*)user_key, len);
		md5_finalize(&md5ctx);  // hash on md5ctx.digest

		// encrypt keys
		_speck sp;
		speck_init(&sp, (uint32_t*)md5ctx.digest);
		speck_encrypt(&sp, cd->speck_key1, cd->buff_u32);
		speck_encrypt(&sp, &(cd->speck_key1[2]), &(cd->buff_u32[2]));
		speck_encrypt(&sp, cd->speck_key2, &(cd->buff_u32[4]));
		speck_encrypt(&sp, &(cd->speck_key2[2]), &(cd->buff_u32[6]));

		// randomize unused slots
		for(uint8_t i = 8; i < 80; i++)
			cd->func_random(&(cd->buff_u32[i]));

		// Agrego tamaño y version
		speck_init(&sp, cd->speck_key1);
		md5_init(&md5ctx);
		cd->buff_u32[126] = bs-1;	// 4 bytes for size
		cd->buff_u32[127] = _CIPHDEV_VERSION;	// 4 bytes for version
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: BLOCK SIZE (sectors)\0", 0, (uint8_t*)&cd->buff_u32[126], 4);
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: VERSION\0", 0, (uint8_t*)&cd->buff_u32[127], 4);
		speck_encrypt(&sp, &(cd->buff_u32[126]), &(cd->buff_u32[80]));
		md5_update(&md5ctx, &(cd->buff_u8[504]), 8);

    // Agrego currtime y user_data[0]
    cd->buff_u32[126] = cd->datetime;
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: DATETIME\0", 0, (uint8_t*)&cd->buff_u32[126], 4);
		cd->buff_u32[127] = cd->user_data[0];
		speck_encrypt(&sp, &(cd->buff_u32[126]), &(cd->buff_u32[82]));
		md5_update(&md5ctx, &(cd->buff_u8[504]), 8);

    // Agrego user_data[1,2]
		cd->buff_u32[126] = cd->user_data[1];
		cd->buff_u32[127] = cd->user_data[2];
		speck_encrypt(&sp, &(cd->buff_u32[126]), &(cd->buff_u32[84]));
		md5_update(&md5ctx, &(cd->buff_u8[504]), 8);
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: USER DATA\0", 0, &cd->u8user_data[0], 12);

    // randomizo 152 bytes y los cifro
		for(uint8_t i = 86; i < 124; i+=2){
			cd->func_random(&(cd->buff_u32[126]));
			cd->func_random(&(cd->buff_u32[127]));
			speck_encrypt(&sp, &(cd->buff_u32[126]), &(cd->buff_u32[i]));
			md5_update(&md5ctx, &(cd->buff_u8[504]), 8);
		}
		md5_finalize(&md5ctx);  // hash on md5ctx.digest

		// Los ultimos 4 bytes son el md5 de la randomización sin cifrar
		for(uint8_t i = 0; i < 16; i++)
			cd->buff_u8[i+496] = md5ctx.digest[i];
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: pad signature\0", 0, &(cd->buff_u8[496]), 16);

		// write to device inmediatly
		if(cd->func_dev_write(cd->dev, cd->buff_u8, 0, 1) != 0){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev CREATE WRITE_ERROR sector 0 device \0", 0, &dev, 1);
			return 3;
		}

		// Fill with random, if configured
		#ifdef _CIPHDEV_RANDOMIZE_UNUSED
		cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev CREATE randomize unused sectors on device\0", 0, &dev, 1);
		for(uint32_t i = 1; i < bs ; i++){
			for(uint32_t j = 0; j < _CIPHDEV_SECTOR_SIZE/4; j++)
				cd->func_random(&(cd->buff_u32[j]));
			if(cd->func_dev_write(cd->dev, cd->buff_u8, i, 1) != 0){
				cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev CREATE WRITE_ERROR sector\0", 0, (uint8_t*)&bs, 1);
				return 3;
			}
		}
		#endif

		if(cd->func_dev_ioctl(cd->dev, _CIPHDEV_CTRL_SYNC, cd->buff_u32) != 0){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev CREATE: SYNC CALL ERROR, device\0", 0, &dev, 1);
			return 4;
		}
		cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev CREATE SUCCESS on device\0", 0, &dev, 1);

		return 0;
	}
	cd->func_debug(_CIPHDEV_LOG_LEVEL_WARN, "Ciphdev CREATE No space left on device \0", 0, &dev, 1);
	return 2;
}

/* Agrega/sobreescribe una clave al bloque cifrado cd previamente inizializado
 * utilizando la frase de cifrado de usuario user_key de longitud len
 * @return 0 si la clave se creó correctamente
 * @return 1 si el device no está inizializado
 * @return 2 si no fue posible leer el header
 * @return 3 si el header no verifica un bloque correcto
 * @return 4 si el indice indicado esta fuera de rango
 * @return 5 si fallo la escritura en el device
 * @return 6 si fallo la llamada ioctl a SYNC */
uint8_t ciphdev_addkey (_ciphdev *cd, const char *user_key, uint8_t len, uint8_t index){
	if(cd->status == _CIPHDEV_STATUS_INIT){
		cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev ADD_KEY on index\0", 0, &index , 1);
		if(index > 9){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev ADD_KEY: Slot index out of bonds. index\0", 0, &index , 1);
			return 4;
		}

		if(ciphdev_header_verify(cd) != 0) return 3;

		// Read sector 0
		if(cd->func_dev_read(cd->dev, cd->buff_u8, 0, 1) != 0) return 2;

		// prepare md5 for user key hash
		_md5_context md5ctx;
		md5_init(&md5ctx);
		md5_update(&md5ctx, (uint8_t*)user_key, len);
		md5_finalize(&md5ctx);  // hash on md5ctx.digest
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev ADD_KEY: user key hash\0", 0, md5ctx.digest, 16);

		// prepare cipher
		_speck sp;
		speck_init(&sp, (uint32_t*)md5ctx.digest);
		speck_encrypt(&sp, cd->speck_key1, &(cd->buff_u32[index*8]));
		speck_encrypt(&sp, &(cd->speck_key1[2]), &(cd->buff_u32[(index*8)+2]));
		speck_encrypt(&sp, cd->speck_key2, &(cd->buff_u32[(index*8)+4]));
		speck_encrypt(&sp, &(cd->speck_key2[2]), &(cd->buff_u32[(index*8)+6]));

		// write to device inmediatly
		if(cd->func_dev_write(cd->dev, cd->buff_u8, 0, 1) != 0){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev ADD_KEY WRITE_ERROR sector 0\0", 0, 0, 0);
			cd->status = _CIPHDEV_STATUS_WRITE_ERROR;
			return 5;
		}
		if(cd->func_dev_ioctl(cd->dev, _CIPHDEV_CTRL_SYNC, cd->buff_u32) != 0){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev ADD_KEY: SYNC CALL ERROR\0", 0, 0, 0);
			cd->status = _CIPHDEV_STATUS_IOERROR;
			return 6;
		}
		cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev ADD_KEY SUCCESS on index\0", 0, &index , 1);
		return 0;
	}
	cd->func_debug(_CIPHDEV_LOG_LEVEL_WARN, "Ciphdev ADD_KEY FAILURE on index\0", 0, &index , 1);
	return 1;
}

/* borra la clave con indice indice index del bloque cifrado cd
 * Retorna 0 la clave se borro correctamente
 * @return 1 si el bloque no fue inicializado
 * @return 2 si el indice esta fuer ade rango
 * @return 3 si fallo la escritura en el device
 * @return 4 si fallo la llamada ioctl a SYNC */
uint8_t ciphdev_deletekey (_ciphdev *cd, uint8_t index){
	if(cd->status == _CIPHDEV_STATUS_INIT){
		cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev DELETE_KEY on index\0", 0, &index , 1);
		if(index > 9){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev DELETE_KEY: Slot index out of bonds. index\0", 0, &index , 1);
			return 4;
		}

		if(ciphdev_header_verify(cd) != 0) return 3;

		// Read sector 0
		if(cd->func_dev_read(cd->dev, cd->buff_u8, 0, 1) != 0) return 2;

		for(uint8_t i = 0; i < 8; i++)
			cd->func_random(&(cd->buff_u32[(index*8)+i]));

		// write to device inmediatly
		if(cd->func_dev_write(cd->dev, cd->buff_u8, 0, 1) != 0){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev DELETE_KEY WRITE_ERROR sector 0\0", 0, 0, 0);
			cd->status = _CIPHDEV_STATUS_WRITE_ERROR;
			return 5;
		}
		if(cd->func_dev_ioctl(cd->dev, _CIPHDEV_CTRL_SYNC, cd->buff_u32) != 0){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev DELETE_KEY: SYNC CALL ERROR\0", 0, 0, 0);
			cd->status = _CIPHDEV_STATUS_IOERROR;
			return 6;
		}
		cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev DELETE_KEY SUCCESS on index\0", 0, &index , 1);
		return 0;
	}
	cd->func_debug(_CIPHDEV_LOG_LEVEL_WARN, "Ciphdev DELETE_KEY FAILURE on index\0", 0, &index , 1);
	return 1;
}

/* Inicializa (abre) el bloque cifrado
 * Retorna el estado actual
#define _CIPHDEV_STATUS_NOINIT 0
#define _CIPHDEV_STATUS_INIT 1
#define _CIPHDEV_STATUS_KEYERROR 2
#define _CIPHDEV_STATUS_IOERROR 3
#define _CIPHDEV_STATUS_READ_ERROR 4
#define _CIPHDEV_STATUS_WRITE_ERROR 5
#define _CIPHDEV_STATUS_HEADER_ERROR 6
*/
uint8_t ciphdev_initialize (_ciphdev *cd, uint8_t dev, const char *user_key, uint8_t len){
		cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev INITIALIZE device\0", 0, &dev, 1);
		// prepare md5 for user key hash
		_md5_context md5ctx;
		_speck sp;

		// Inicializo el device
		if(cd->func_dev_initialize(cd->dev) != 0){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev INITIALIZE IOERROR device\0", 0, &dev, 1);
			cd->status = _CIPHDEV_STATUS_IOERROR;
			return cd->status;
		}

		// Read sector 0
		if(cd->func_dev_read(cd->dev, cd->buff_u8, 0, 1) != 0){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev INITIALIZE READ_ERROR sector 0 device\0", 0, &dev, 1);
			cd->status = _CIPHDEV_STATUS_READ_ERROR;
			return cd->status;
		}

		cd->status = _CIPHDEV_STATUS_NOINIT;

		// search slot
		for(uint8_t index = 0; index < 10; index++){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: test slot\0", 0, &index, 1);
			md5_init(&md5ctx);
			md5_update(&md5ctx, (uint8_t*)user_key, len);
			md5_finalize(&md5ctx);  // hash on md5ctx.digest
			cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: user key hash\0", 0, md5ctx.digest, 16);

			// prepare cipher
			speck_init(&sp, (uint32_t*)md5ctx.digest);
			speck_decrypt(&sp, &(cd->buff_u32[index*8]),  cd->speck_key1);
			speck_decrypt(&sp, &(cd->buff_u32[(index*8)+2]),  &(cd->speck_key1[2]));
			speck_decrypt(&sp, &(cd->buff_u32[(index*8)+4]),  cd->speck_key2);
			speck_decrypt(&sp, &(cd->buff_u32[(index*8)+6]),  &(cd->speck_key2[2]));
			cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: speck key 1 (decrypt)\0", 0, cd->u8speck_key1, 16);
			cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: speck key 2 (decrypt)\0", 0, cd->u8speck_key2, 16);

			if(ciphdev_header_verify(cd) == 0){
				cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev INITIALIZE: SUCCESS on slot\0", 0, &index, 1);
				// Initialize struct
				cd->status = _CIPHDEV_STATUS_INIT;
				cd->dev = dev;

        // Get block data
				speck_init(&sp, cd->speck_key1);

        speck_decrypt(&sp, &(cd->buff_u32[80]), &(cd->buff_u32[126]));
				cd->block_size = cd->buff_u32[126];
				cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: BLOCK SIZE (sectors)\0", 0, (uint8_t*)&cd->block_size, 4);
				cd->version = cd->buff_u32[127];
				cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: VERSION\0", 0, (uint8_t*)&cd->version, 4);

        speck_decrypt(&sp, &(cd->buff_u32[82]), &(cd->buff_u32[126]));
				cd->datetime = cd->buff_u32[126];
				cd->user_data[0] = cd->buff_u32[127];
				cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: DATETIME\0", 0, (uint8_t*)&cd->datetime, 4);

				speck_decrypt(&sp, &(cd->buff_u32[84]), &(cd->buff_u32[126]));
				cd->user_data[1] = cd->buff_u32[126];
				cd->user_data[2] = cd->buff_u32[127];
		    cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: USER DATA\0", 0, &cd->u8user_data[0], 12);

				// Load key_map
				md5_init(&md5ctx);
				md5_update(&md5ctx, cd->u8speck_key1, 16);
				md5_update(&md5ctx, cd->u8speck_key2, 16);
				md5_finalize(&md5ctx);
				for(uint8_t i = 0; i < 16; i++)
					cd->u8key_map[i] = md5ctx.digest[i];
				cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: key_map\0", 0, cd->u8key_map, 16);
				return cd->status;  // all OK
			}else{
				cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: FAILURE on slot\0", 0, &index, 1);
			}
		}
		cd->status = _CIPHDEV_STATUS_KEYERROR;
		return cd->status;
}

/* Retorna el estado actual */
uint8_t ciphdev_status (_ciphdev *cd){
	return cd->status;
}

/* Lee sectores
 * @return 0 si la lectura fue correcta
 * @return 1 si el bloque no está inicializado
 * @return 2 si se intenta leer fuera del bloque
 * @return 3 si hubo error de lectura en el device*/
uint8_t ciphdev_read (_ciphdev *cd, uint8_t* buff, uint32_t sector, uint32_t count){
	if(cd->status == _CIPHDEV_STATUS_INIT){
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev READ: sector\0", 0, (uint8_t*)&sector, 4);
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev READ: count\0", 0, (uint8_t*)&count, 4);
		if(sector+count >= cd->block_size){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev READ: Out of bounds\0", 0, 0, 0);
			return 2;
		}

		_speck sp;
		uint32_t cursect;
		for(uint32_t i = 0; i < count; i++){
			// prepare cipher
			if(cd->u8key_map[(sector+i)%16] & (1<<((sector+i)%8)))
				speck_init(&sp, cd->speck_key1);
			else
				speck_init(&sp, cd->speck_key2);

			cursect = sector+i+1;
			if(cd->func_dev_read(cd->dev, cd->buff_u8, cursect, 1) != 0){
				cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev READ: READ_ERROR, VOLUME sector\0", 0, (uint8_t*)&cursect, 4);
				cd->status = _CIPHDEV_STATUS_READ_ERROR;
				return 3;
			}
			for(uint8_t j = 0; j < 128; j+=2)
				speck_decrypt(&sp, &(cd->buff_u32[j]), (uint32_t*)&(buff[(i*_CIPHDEV_SECTOR_SIZE)+(j*4)]));
		}
		return 0;
	}
	cd->func_debug(_CIPHDEV_LOG_LEVEL_WARN, "Ciphdev READ: BLOCK not initialized\0", 0, 0, 0);
	return 1;
}

/* Escribe sectores
 * @return 0 si la escritura fue correcta
 * @return 1 si el bloque no está inicializado
 * @return 2 si se intenta escribir fuera del bloque
 * @return 3 si hubo error de escritura en el device*/
uint8_t ciphdev_write (_ciphdev *cd, const uint8_t *buff, uint32_t sector, uint32_t count){
	if(cd->status == _CIPHDEV_STATUS_INIT){

		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev WRITE: sector\0", 0, (uint8_t*)&sector, 4);
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev WRITE: count\0", 0, (uint8_t*)&count, 4);

		if(sector+count >= cd->block_size){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev WRITE: Out of bounds\0", 0, 0, 0);
			return 2;
		}

		// prepare cipher
		_speck sp;
		uint32_t cursect;
		for(uint32_t i = 0; i < count; i++){
			if(cd->u8key_map[(sector+i)%16] & (1<<((sector+i)%8)))
				speck_init(&sp, cd->speck_key1);
			else
				speck_init(&sp, cd->speck_key2);

			for(uint8_t j = 0; j < 128; j+=2)
				speck_encrypt(&sp, (uint32_t*)&(buff[(i*_CIPHDEV_SECTOR_SIZE)+(j*4)]), &(cd->buff_u32[j]));

			cursect = sector+i+1;
			if(cd->func_dev_write(cd->dev, cd->buff_u8, cursect, 1) != 0){
				cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev WRITE: WRITE_ERROR, sector\0", 0, (uint8_t*)&cursect, 4);
				cd->status = _CIPHDEV_STATUS_WRITE_ERROR;
				return 3;
			}
		}
		return 0;
	}
	cd->func_debug(_CIPHDEV_LOG_LEVEL_WARN, "Ciphdev WRITE: BLOCK not initialized\0", 0, 0, 0);
	return 1;
}


/* Reescribe el header con la informacion en el struc _ciphdev cd
 * para version, datetime y user_data
 * @return 0 si la escritura fue correcta
 * @return 1 si el bloque no está inicializado
 * @return 2 si hubo error de escritura en el device*/
uint8_t ciphdev_rewrite_header (_ciphdev *cd){
	if(cd->status == _CIPHDEV_STATUS_INIT){

		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev REWRITE HEADER\0", 0, 0, 0);

		if(cd->func_dev_read(cd->dev, cd->buff_u8, 0, 1) != 0){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev REWRITE HEADER: READ_ERROR, VOLUME sector 0\0", 0, 0, 0);
			cd->status = _CIPHDEV_STATUS_READ_ERROR;
			return 3;
		}

    // prepare cipher
		_speck sp;
		_md5_context md5ctx;
		md5_init(&md5ctx);

		// Agrego tamaño y version
		speck_init(&sp, cd->speck_key1);
		md5_init(&md5ctx);
		cd->buff_u32[126] = cd->block_size;	// 4 bytes for size
		cd->buff_u32[127] = cd->version;	// 4 bytes for version
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev REWRITE HEADER: BLOCK SIZE (sectors)\0", 0, (uint8_t*)&cd->buff_u32[126], 4);
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev REWRITE HEADER: VERSION\0", 0, (uint8_t*)&cd->buff_u32[127], 4);
		speck_encrypt(&sp, &(cd->buff_u32[126]), &(cd->buff_u32[80]));
		md5_update(&md5ctx, &(cd->buff_u8[504]), 8);

    // Agrego datetime y user_data[0]
    cd->buff_u32[126] = cd->datetime;
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev REWRITE HEADER: DATETIME\0", 0, (uint8_t*)&cd->buff_u32[126], 4);
		cd->buff_u32[127] = cd->user_data[0];
		speck_encrypt(&sp, &(cd->buff_u32[126]), &(cd->buff_u32[82]));
		md5_update(&md5ctx, &(cd->buff_u8[504]), 8);

    // Agrego user_data[1,2]
		cd->buff_u32[126] = cd->user_data[1];
		cd->buff_u32[127] = cd->user_data[2];
		speck_encrypt(&sp, &(cd->buff_u32[126]), &(cd->buff_u32[84]));
		md5_update(&md5ctx, &(cd->buff_u8[504]), 8);
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev REWRITE HEADER: USER DATA\0", 0, &cd->u8user_data[0], 12);

    // randomizo 152 bytes y los cifro
		for(uint8_t i = 86; i < 124; i+=2){
			cd->func_random(&(cd->buff_u32[126]));
			cd->func_random(&(cd->buff_u32[127]));
			speck_encrypt(&sp, &(cd->buff_u32[126]), &(cd->buff_u32[i]));
			md5_update(&md5ctx, &(cd->buff_u8[504]), 8);
		}
		md5_finalize(&md5ctx);  // hash on md5ctx.digest

		// Los ultimos 4 bytes son el md5 de la randomización sin cifrar
		for(uint8_t i = 0; i < 16; i++)
			cd->buff_u8[i+496] = md5ctx.digest[i];
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev REWRITE HEADER: new pad signature\0", 0, &(cd->buff_u8[496]), 16);

		// write to device inmediatly
		if(cd->func_dev_write(cd->dev, cd->buff_u8, 0, 1) != 0){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev REWRITE HEADER WRITE_ERROR sector 0 device \0", 0, &(cd->dev), 1);
			return 3;
		}

		if(cd->func_dev_ioctl(cd->dev, _CIPHDEV_CTRL_SYNC, cd->buff_u32) != 0){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev REWRITE HEADER: SYNC CALL ERROR, device\0", 0, &(cd->dev), 1);
			return 4;
		}
		cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev REWRITE HEADER SUCCESS on device\0", 0, &(cd->dev), 1);

		return 0;

	}
	cd->func_debug(_CIPHDEV_LOG_LEVEL_WARN, "Ciphdev REWRITE HEADER: BLOCK not initialized\0", 0, 0, 0);
	return 1;
}



/* ioctl, retorna valor segun el comando cmd */
uint8_t ciphdev_ioctl (_ciphdev *cd, uint8_t cmd, uint32_t *buff){
  switch(cmd){
    case _CIPHDEV_CTRL_GET_SECTOR_COUNT:
      *buff = cd->block_size;
      return 0;
    case _CIPHDEV_CTRL_GET_SECTOR_SIZE:
      *buff = _CIPHDEV_SECTOR_SIZE;
      return 0;
    case _CIPHDEV_CTRL_SYNC:
      return cd->func_dev_ioctl(cd->dev, _CIPHDEV_CTRL_SYNC, buff);
    default:
	    return cd->func_dev_ioctl(cd->dev, cmd, buff);
  }
}

/* Attach de las funciones Callback */
void ciphdev_attach_dev_initialize(_ciphdev *cd, ciphdev_dev_initialize_def f){
	cd->func_dev_initialize = f;
}

void ciphdev_attach_dev_status(_ciphdev *cd, ciphdev_dev_status_def f){
	cd->func_dev_status = f;
}

void ciphdev_attach_dev_read(_ciphdev *cd, ciphdev_dev_read_def f){
	cd->func_dev_read = f;
}

void ciphdev_attach_dev_write(_ciphdev *cd, ciphdev_dev_write_def f){
	cd->func_dev_write = f;
}

void ciphdev_attach_dev_ioctl(_ciphdev *cd, ciphdev_dev_ioctl_def f){
	cd->func_dev_ioctl = f;
}

void ciphdev_attach_random(_ciphdev *cd, ciphdev_random_def f){
	cd->func_random = f;
}

void ciphdev_attach_debug(_ciphdev *cd, ciphdev_debug_def f){
	cd->func_debug = f;
}


// Funciones estaticas de bajo nivel, escritura de sectores individuales
static uint8_t ciphdev_header_verify(_ciphdev *cd){
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev HEADER VERIFY\0", 0, (uint8_t*)0, 0);
		_md5_context md5ctx;
		_speck sp;
		uint8_t tmp[8];

		// Read sector 0
		if(cd->func_dev_read(cd->dev, cd->buff_u8, 0, 1) != 0){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev HEADER VERIFY READ_ERROR sector 0\0", 0, (uint8_t*)0, 0);
			cd->status = _CIPHDEV_STATUS_READ_ERROR;
			return cd->status;
		}

		speck_init(&sp, cd->speck_key1);
		md5_init(&md5ctx);
		for(uint8_t i = 80; i < 124; i+=2){
			speck_decrypt(&sp, &(cd->buff_u32[i]), (uint32_t*)tmp);
			md5_update(&md5ctx, tmp, 8);
		}
		md5_finalize(&md5ctx);  // hash on md5ctx.digest
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev HEADER VERIFY: pad signature\0", 0, md5ctx.digest, 16);

		// Los ultimos 4 bytes son el md5 de la randomización sin cifrar
		for(uint8_t i = 0; i < 16; i++)
			if(cd->buff_u8[i+496] != md5ctx.digest[i]){
				cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev HEADER VERIFY FAILURE\0", 0, (uint8_t*)0, 0);
				return 1;
			}
	cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev HEADER VERIFY SUCCESS\0", 0, (uint8_t*)0, 0);
	return 0;
}
