/**
 * ciphdev permite crear y mantener un bloque cifrado mediante el algoritmo
 * criptografico de bloque SPECK 64/128 y modo de operación CBC.
 *
 * @author Patricio Silva
 * @date Dec 21, 2023
 */
#include "ciphdev.h"

// Funciones de bajo nivel
static uint8_t ciphdev_header_verify(_ciphdev *cd);



/* Crea un bloque cifrado cd en el dispositvo dev de un tamaño bs sectores
 * utilizando la frase de cifrado de usuario user_key de longitud len que se
 * almacenará en el slot slot
 * @return 0 si el bloque se creó correctamente
 * @return 1 si el indice de slot no es valido
 * @return 2 si el device no se pudo inicializar
 * @return 3 si el device es mas pequeño que bs
 * @return 4 si fallo la escritura en el device
 * @return 5 si fallo la llamada ioctl a SYNC
 */
uint8_t ciphdev_create (_ciphdev *cd, uint8_t dev, uint32_t bs, const char *user_key, uint8_t len, uint8_t slot){
	cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev CREATE device\0", 0, &dev, 1);
	cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev CREATE slot\0", 0, &slot, 1);

	// El slot indicado no es válido
	if(slot > 9){
		cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev ADD_KEY: Slot index out of bonds. slot\0", 0, &slot , 1);
		return 1;
	}

	// Inicializo el device
	if(cd->func_dev_initialize(cd->dev) != 0){
		cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev CREATE IOERROR device\0", 0, &dev, 1);
		return 2;
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

		// Create initialization vector
		cd->func_random(&(cd->init_vector[0]));
		cd->func_random(&(cd->init_vector[1]));
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: Initialization Vector\0", 0, cd->u8init_vector, 8);

		// prepare md5 for key map
		_md5_context md5ctx;
		md5_init(&md5ctx);
		md5_update(&md5ctx, cd->u8speck_key1, 16);
		md5_update(&md5ctx, cd->u8speck_key2, 16);
		md5_finalize(&md5ctx);
		for(uint8_t i = 0; i < 16; i++)
			cd->u8key_map[i] = md5ctx.digest[i];
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: key_map\0", 0, cd->u8key_map, 16);

		// randomize slots
		for(uint8_t i = 0; i < 80; i++)
			cd->func_random(&(cd->buff_u32[i]));

		// prepare md5 for user key hash
		md5_init(&md5ctx);
		md5_update(&md5ctx, (uint8_t*)user_key, len);
		md5_finalize(&md5ctx);  // hash on md5ctx.digest
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: user key hash\0", 0, md5ctx.digest, 16);

		// encrypt and store keys
		_speck_sc sp;
		uint32_t local_u32[2];  // Utility array
		local_u32[0] = slot;
		local_u32[1] = 0;
		speck_sc_init(&sp, (uint32_t*)md5ctx.digest, local_u32);
		speck_sc_encrypt(&sp, cd->speck_key1, &(cd->buff_u32[(slot*8)]));
		speck_sc_encrypt(&sp, &(cd->speck_key1[2]), &(cd->buff_u32[(slot*8)+2]));
		speck_sc_encrypt(&sp, cd->speck_key2, &(cd->buff_u32[(slot*8)+4]));
		speck_sc_encrypt(&sp, &(cd->speck_key2[2]), &(cd->buff_u32[(slot*8)+6]));

		// Almaceno vector de inicializacion
		cd->buff_u32[80] = cd->init_vector[0];
		cd->buff_u32[81] = cd->init_vector[1];

		// Inicializo cifrado en stream con vector de inicialización
		// Agrego tamaño y version
		speck_sc_init(&sp, cd->speck_key1, cd->init_vector);
		md5_init(&md5ctx);
		local_u32[0] = bs-1;	// 4 bytes for size
		local_u32[1] = _CIPHDEV_VERSION;	// 4 bytes for version
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: BLOCK SIZE (sectors)\0", 0, (uint8_t*)&local_u32[0], 4);
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: VERSION\0", 0, (uint8_t*)&local_u32[1], 4);
		speck_sc_encrypt(&sp, local_u32, &(cd->buff_u32[82]));
		md5_update(&md5ctx, (uint8_t*)&(local_u32[0]), 8);

		// Agrego currtime y user_data[0]
		local_u32[0] = cd->datetime;
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: DATETIME\0", 0, (uint8_t*)&local_u32[0], 4);
		local_u32[1] = cd->user_data[0];
		speck_sc_encrypt(&sp, local_u32, &(cd->buff_u32[84]));
		md5_update(&md5ctx, (uint8_t*)&(local_u32[0]), 8);

		// Agrego user_data[1,2]
		local_u32[0] = cd->user_data[1];
		local_u32[1] = cd->user_data[2];
		speck_sc_encrypt(&sp, local_u32, &(cd->buff_u32[86]));
		md5_update(&md5ctx, (uint8_t*)&(local_u32[0]), 8);
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev CREATE: USER DATA\0", 0, cd->u8user_data, 12);

		// randomizo 144 bytes y los cifro
		for(uint8_t i = 88; i < 124; i+=2){
			cd->func_random(&(local_u32[0]));
			cd->func_random(&(local_u32[1]));
			speck_sc_encrypt(&sp, local_u32, &(cd->buff_u32[i]));
			md5_update(&md5ctx, (uint8_t*)&(local_u32[0]), 8);
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
				return 4;
			}
		}
		#endif

		if(cd->func_dev_ioctl(cd->dev, _CIPHDEV_CTRL_SYNC, cd->buff_u32) != 0){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev CREATE: SYNC CALL ERROR, device\0", 0, &dev, 1);
			return 5;
		}
		cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev CREATE SUCCESS on device\0", 0, &dev, 1);

		return 0;
	}
	cd->func_debug(_CIPHDEV_LOG_LEVEL_WARN, "Ciphdev CREATE No space left on device \0", 0, &dev, 1);
	return 3;
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
uint8_t ciphdev_addkey (_ciphdev *cd, const char *user_key, uint8_t len, uint8_t slot){
	if(cd->status == _CIPHDEV_STATUS_INIT){
		cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev ADD_KEY on slot\0", 0, &slot , 1);
		if(slot > 9){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev ADD_KEY: Slot index out of bonds. slot\0", 0, &slot , 1);
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
		_speck_sc sp;
		uint32_t local_u32[2];  // Utility array
		local_u32[0] = slot;
		local_u32[1] = 0;
		speck_sc_init(&sp, (uint32_t*)md5ctx.digest, local_u32);
		speck_sc_encrypt(&sp, cd->speck_key1, &(cd->buff_u32[slot*8]));
		speck_sc_encrypt(&sp, &(cd->speck_key1[2]), &(cd->buff_u32[(slot*8)+2]));
		speck_sc_encrypt(&sp, cd->speck_key2, &(cd->buff_u32[(slot*8)+4]));
		speck_sc_encrypt(&sp, &(cd->speck_key2[2]), &(cd->buff_u32[(slot*8)+6]));

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
		cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev ADD_KEY SUCCESS on slot\0", 0, &slot , 1);
		return 0;
	}
	cd->func_debug(_CIPHDEV_LOG_LEVEL_WARN, "Ciphdev ADD_KEY FAILURE on slot\0", 0, &slot , 1);
	return 1;
}

/* borra la clave en el slot slot del bloque cifrado cd
 * Retorna 0 la clave se borro correctamente
 * @return 1 si el bloque no fue inicializado
 * @return 2 si el indice esta fuer ade rango
 * @return 3 si fallo la escritura en el device
 * @return 4 si fallo la llamada ioctl a SYNC */
uint8_t ciphdev_deletekey (_ciphdev *cd, uint8_t slot){
	if(cd->status == _CIPHDEV_STATUS_INIT){
		cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev DELETE_KEY on slot\0", 0, &slot , 1);
		if(slot > 9){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev DELETE_KEY: Slot index out of bonds. slot\0", 0, &slot , 1);
			return 4;
		}

		if(ciphdev_header_verify(cd) != 0) return 3;

		// Read sector 0
		if(cd->func_dev_read(cd->dev, cd->buff_u8, 0, 1) != 0) return 2;

		for(uint8_t i = 0; i < 8; i++)
			cd->func_random(&(cd->buff_u32[(slot*8)+i]));

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
		cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev DELETE_KEY SUCCESS on slot\0", 0, &slot , 1);
		return 0;
	}
	cd->func_debug(_CIPHDEV_LOG_LEVEL_WARN, "Ciphdev DELETE_KEY FAILURE on slot\0", 0, &slot , 1);
	return 1;
}

/* Inicializa (abre) el bloque cifrado con la clave key en el slot slot
 * Retorna el estado actual
#define _CIPHDEV_STATUS_NOINIT 0
#define _CIPHDEV_STATUS_INIT 1
#define _CIPHDEV_STATUS_KEYERROR 2
#define _CIPHDEV_STATUS_IOERROR 3
#define _CIPHDEV_STATUS_READ_ERROR 4
#define _CIPHDEV_STATUS_WRITE_ERROR 5
#define _CIPHDEV_STATUS_HEADER_ERROR 6
*/
uint8_t ciphdev_initialize (_ciphdev *cd, uint8_t dev, const char *user_key, uint8_t len, uint8_t slot){
		cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev INITIALIZE device\0", 0, &dev, 1);
		// prepare md5 for user key hash
		_md5_context md5ctx;
		_speck_sc sp;

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
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: attempt key on slot\0", 0, &slot, 1);
		md5_init(&md5ctx);
		md5_update(&md5ctx, (uint8_t*)user_key, len);
		md5_finalize(&md5ctx);  // hash on md5ctx.digest
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: user key hash\0", 0, md5ctx.digest, 16);

		// prepare cipher
		uint32_t local_u32[2];  // Utility array
		local_u32[0] = slot;
		local_u32[1] = 0;
		speck_sc_init(&sp, (uint32_t*)md5ctx.digest, local_u32);
		speck_sc_decrypt(&sp, &(cd->buff_u32[slot*8]), cd->speck_key1);
		speck_sc_decrypt(&sp, &(cd->buff_u32[(slot*8)+2]), &(cd->speck_key1[2]));
		speck_sc_decrypt(&sp, &(cd->buff_u32[(slot*8)+4]), cd->speck_key2);
		speck_sc_decrypt(&sp, &(cd->buff_u32[(slot*8)+6]), &(cd->speck_key2[2]));
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: speck key 1 (decrypt)\0", 0, cd->u8speck_key1, 16);
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: speck key 2 (decrypt)\0", 0, cd->u8speck_key2, 16);

		// Initialization vector is plain text
		cd->init_vector[0] = cd->buff_u32[80];
		cd->init_vector[1] = cd->buff_u32[81];
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: Initialization vector\0", 0, cd->u8init_vector, 8);

		if(ciphdev_header_verify(cd) == 0){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_INFO, "Ciphdev INITIALIZE: SUCCESS on slot\0", 0, &slot, 1);
			// Initialize struct
			cd->status = _CIPHDEV_STATUS_INIT;
			cd->dev = dev;

			// Get block data
			speck_sc_init(&sp, cd->speck_key1, cd->init_vector);

			// Get block size and ciphdev version
			speck_sc_decrypt(&sp, &(cd->buff_u32[82]), local_u32);
			cd->block_size = local_u32[0];
			cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: BLOCK SIZE (sectors)\0", 0, (uint8_t*)&cd->block_size, 4);
			cd->version = local_u32[1];
			cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: VERSION\0", 0, (uint8_t*)&cd->version, 4);

			// Get datetime and user_data[0]
			speck_sc_decrypt(&sp, &(cd->buff_u32[84]), local_u32);
			cd->datetime = local_u32[0];
			cd->user_data[0] = local_u32[1];
			cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: DATETIME\0", 0, (uint8_t*)&cd->datetime, 4);

			// Get user_data[0,1]
			speck_sc_decrypt(&sp, &(cd->buff_u32[86]), local_u32);
			cd->user_data[1] = local_u32[0];
			cd->user_data[2] = local_u32[1];
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
			cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev INITIALIZE: FAILURE on slot\0", 0, &slot, 1);
			cd->status = _CIPHDEV_STATUS_KEYERROR;
		}
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
		if(sector+count-1 >= cd->block_size){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev READ: Out of bounds\0", 0, 0, 0);
			return 2;
		}

		_speck_sc sp;
		uint32_t cursect;
		uint32_t local_u32[2];
		local_u32[0] = cd->init_vector[0];
		for(uint32_t i = 0; i < count; i++){
			local_u32[1] = (cd->init_vector[1] ^ (sector+i));
			// prepare cipher
			if(cd->u8key_map[(sector+i)%16] & (1<<((sector+i)%8)))
				speck_sc_init(&sp, cd->speck_key1, local_u32);
			else
				speck_sc_init(&sp, cd->speck_key2, local_u32);

			cursect = sector+i+1;
			if(cd->func_dev_read(cd->dev, cd->buff_u8, cursect, 1) != 0){
				cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev READ: READ_ERROR, VOLUME sector\0", 0, (uint8_t*)&cursect, 4);
				cd->status = _CIPHDEV_STATUS_READ_ERROR;
				return 3;
			}
			for(uint8_t j = 0; j < 128; j+=2)
				speck_sc_decrypt(&sp, &(cd->buff_u32[j]), (uint32_t*)&(buff[(i*_CIPHDEV_SECTOR_SIZE)+(j*4)]));
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

		if(sector+count-1 >= cd->block_size){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev WRITE: Out of bounds\0", 0, 0, 0);
			return 2;
		}

		// prepare cipher
		_speck_sc sp;
		uint32_t cursect;
		uint32_t local_u32[2];
		local_u32[0] = cd->init_vector[0];
		for(uint32_t i = 0; i < count; i++){
			local_u32[1] = (cd->init_vector[1] ^ (sector+i));
			if(cd->u8key_map[(sector+i)%16] & (1<<((sector+i)%8)))
				speck_sc_init(&sp, cd->speck_key1, local_u32);
			else
				speck_sc_init(&sp, cd->speck_key2, local_u32);

			for(uint8_t j = 0; j < 128; j+=2)
				speck_sc_encrypt(&sp, (uint32_t*)&(buff[(i*_CIPHDEV_SECTOR_SIZE)+(j*4)]), &(cd->buff_u32[j]));

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
		_speck_sc sp;
		_md5_context md5ctx;


		// Inicializo cifrado en stream con vector de inicialización
		// Agrego tamaño y version
		speck_sc_init(&sp, cd->speck_key1, cd->init_vector);
		md5_init(&md5ctx);
		uint32_t local_u32[2];
		local_u32[0] = cd->block_size;	// 4 bytes for size
		local_u32[1] = cd->version;	// 4 bytes for version
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev REWRITE HEADER: BLOCK SIZE (sectors)\0", 0, (uint8_t*)&local_u32[0], 4);
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev REWRITE HEADER: VERSION\0", 0, (uint8_t*)&local_u32[1], 4);
		speck_sc_encrypt(&sp, local_u32, &(cd->buff_u32[82]));
		md5_update(&md5ctx, (uint8_t*)&(local_u32[0]), 8);

		// Agrego currtime y user_data[0]
		local_u32[0] = cd->datetime;
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev REWRITE HEADER: DATETIME\0", 0, (uint8_t*)&local_u32[0], 4);
		local_u32[1] = cd->user_data[0];
		speck_sc_encrypt(&sp, local_u32, &(cd->buff_u32[84]));
		md5_update(&md5ctx, (uint8_t*)&(local_u32[0]), 8);

		// Agrego user_data[1,2]
		local_u32[0] = cd->user_data[1];
		local_u32[1] = cd->user_data[2];
		speck_sc_encrypt(&sp, local_u32, &(cd->buff_u32[86]));
		md5_update(&md5ctx, (uint8_t*)&(local_u32[0]), 8);
		cd->func_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "Ciphdev REWRITE HEADER: USER DATA\0", 0, cd->u8user_data, 12);

		// randomizo 144 bytes y los cifro
		for(uint8_t i = 88; i < 124; i+=2){
			cd->func_random(&(local_u32[0]));
			cd->func_random(&(local_u32[1]));
			speck_sc_encrypt(&sp, local_u32, &(cd->buff_u32[i]));
			md5_update(&md5ctx, (uint8_t*)&(local_u32[0]), 8);
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

/* Setea el buffer de trabajo, minimo 512 bytes */
void ciphdev_attach_buffer(_ciphdev *cd, uint8_t *b){
	cd->buff_u8 = b;
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
		_speck_sc sp;
		uint8_t tmp[8];

		// Read sector 0
		if(cd->func_dev_read(cd->dev, cd->buff_u8, 0, 1) != 0){
			cd->func_debug(_CIPHDEV_LOG_LEVEL_ERROR, "Ciphdev HEADER VERIFY READ_ERROR sector 0\0", 0, (uint8_t*)0, 0);
			cd->status = _CIPHDEV_STATUS_READ_ERROR;
			return cd->status;
		}

		speck_sc_init(&sp, cd->speck_key1, cd->init_vector);
		md5_init(&md5ctx);
		for(uint8_t i = 82; i < 124; i+=2){
			speck_sc_decrypt(&sp, &(cd->buff_u32[i]), (uint32_t*)tmp);
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
