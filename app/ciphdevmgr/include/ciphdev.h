/**
 * ciphdev permite crear y mantener un bloque cifrado mediante el algoritmo
 * criptografico SPECK con clave de 128 bytes.
 *
 * La clave de cifrado SPECK es creada durante la inicialización y se almacena
 * cifrada utilizando las claves de usuario. Por defecto se cuenta con 10 slots
 * para almacenar las claves cifradas.
 *
 * El bloque posee un cifrado dual por lo que se posee dos claves de cifrado
 * SPECK. La clave de cifrado a utilizar para cada sector depende del md5
 * de la concatenación de las claves.
 *
 * EL header de control ocupa el sector 0 de 512 bytes, cada slot con las Claves
 * cifradas se ubican consucutivamente, ocupando 32 bytes cada par de claves
 * osea [0..319].
 * El resto del sector se cifra con la key 1, siendo:
 * La ubicación [320..323] es el tamaño del bloque en sectores
 * La ubicación [324..327] es la version de ciphdev (_CIPHDEV_VERSION)
 * La ubicación [328..331] es la fecha de creacion del bloque
 * La ubicación [332..343] son 12 bytes arbitrarios definibles por el usuario
 * La ubicación [344..492] se randomiza y se cifra con la key 1.
 * Los ultimos 16 bytes corresponden al hash md5 de esos 176 bytes cifrados
 * Es necesario que esto se cumpla para poder inicializar un bloque.
 *
 *
 * SPECK is a lightweight block cipher publicly released by the National
 * Security Agency (NSA) in June 2013.
 * Speck has been optimized for performance in software implementations
 *
 * This implementation is the 64 bit block, 128 bit key version.
 *
 * Doc: https://eprint.iacr.org/2013/404.pdf
 * https://nsacyber.github.io/simon-speck/implementations/ImplementationGuide1.1.pdf
 *
 * @author Patricio Silva
 * @date Dec 21, 2023
 * @version 0.4
 */
#ifndef CIPHDEV_H
#define CIPHDEV_H

#include "md5.h"
#include "speck.h"
#include "stdint.h"

#define _CIPHDEV_VERSION 4
#define _CIPHDEV_SECTOR_SIZE 512
#define _CIPHDEV_RANDOMIZE_UNUSED

#define _CIPHDEV_STATUS_NOINIT 0
#define _CIPHDEV_STATUS_INIT 1
#define _CIPHDEV_STATUS_KEYERROR 2
#define _CIPHDEV_STATUS_IOERROR 3
#define _CIPHDEV_STATUS_READ_ERROR 4
#define _CIPHDEV_STATUS_WRITE_ERROR 5
#define _CIPHDEV_STATUS_HEADER_ERROR 6

// Must be implemented on dev interface
#define _CIPHDEV_CTRL_SYNC 0
#define _CIPHDEV_CTRL_GET_SECTOR_COUNT 1
#define _CIPHDEV_CTRL_GET_SECTOR_SIZE 2


// well known log levels
#define _CIPHDEV_LOG_LEVEL_FATAL 1
#define _CIPHDEV_LOG_LEVEL_ERROR 2
#define _CIPHDEV_LOG_LEVEL_WARN 3
#define _CIPHDEV_LOG_LEVEL_INFO 4
#define _CIPHDEV_LOG_LEVEL_DEBUG 5
#define _CIPHDEV_LOG_LEVEL_TRACE 6

/* Callback functions */
typedef uint8_t (*ciphdev_dev_initialize_def)(uint8_t dev);
typedef uint8_t (*ciphdev_dev_status_def)(uint8_t dev);
typedef uint8_t (*ciphdev_dev_read_def)(uint8_t dev, uint8_t *buff, uint32_t sector, uint32_t count);
typedef uint8_t (*ciphdev_dev_write_def)(uint8_t dev, const uint8_t *buff, uint32_t sector, uint32_t count);
typedef uint8_t (*ciphdev_dev_ioctl_def)(uint8_t dev, uint8_t cmd, uint32_t *buff);
typedef void (*ciphdev_random_def)(uint32_t *buff);
typedef void (*ciphdev_debug_def)(uint8_t level, const char *msg, const char *charg, const uint8_t *arg, uint8_t argl);

/* Estructura de datos que mantiene un bloque cifrado */
typedef struct{
	// Identificador del dispositivo
	uint8_t dev;

  // Estado actual
	uint8_t status;

	// Claves speck
	union{
		uint32_t speck_key1[4];
		uint8_t u8speck_key1[16];
	};
	union{
		uint32_t speck_key2[4];
		uint8_t u8speck_key2[16];
	};

	// Mapa de cifrado
	union{
		uint32_t key_map[4];
		uint8_t u8key_map[16];
	};

	// Tamaño del bloque, en sectores de 512 bytes
	uint32_t block_size;

  // ciphdev version
	uint32_t version;

  // create time
	uint32_t datetime;

  // User data
  union{
    uint8_t u8user_data[12];
    uint32_t user_data[3];
  };

	// Buffer
	union{
		uint32_t buff_u32[128];
		uint8_t buff_u8[512];
	};

  // Funciones Callback
	ciphdev_dev_initialize_def func_dev_initialize;
	ciphdev_dev_status_def func_dev_status;
	ciphdev_dev_read_def func_dev_read;
	ciphdev_dev_write_def func_dev_write;
	ciphdev_dev_ioctl_def func_dev_ioctl;
	ciphdev_random_def func_random;
	ciphdev_debug_def func_debug;
} _ciphdev;

/* Crea un bloque cifrado cd en el dispositvo dev de un tamaño bs sectores
 * utilizando la frase de cifrado de usuario user_key de longitud len que se
 * almacenará en el slot index
 * @return 0 si el bloque se creó correctamente
 * @return 1 si el indice de slot no es valido
 * @return 2 si el device no se pudo inicializar
 * @return 3 si el device es mas pequeño que bs
 * @return 4 si fallo la escritura en el device
 * @return 5 si fallo la llamada ioctl a SYNC
 */
uint8_t ciphdev_create (_ciphdev *cd, uint8_t dev, uint32_t bs, const char *user_key, uint8_t len, uint8_t index);

/* Agrega/sobreescribe una clave al bloque cifrado cd previamente inizializado
 * utilizando la frase de cifrado de usuario user_key de longitud len
 * @return 0 si la clave se creó correctamente
 * @return 1 si el device no está inizializado
 * @return 2 si no fue posible leer el header
 * @return 3 si el header no verifica un bloque correcto
 * @return 4 si el indice indicado esta fuera de rango
 * @return 5 si fallo la escritura en el device
 * @return 6 si fallo la llamada ioctl a SYNC */
uint8_t ciphdev_addkey (_ciphdev *cd, const char *user_key, uint8_t len, uint8_t index);

/* borra la clave con indice indice index del bloque cifrado cd
 * Retorna 0 la clave se borro correctamente
 * @return 1 si el bloque no fue inicializado
 * @return 2 si el indice esta fuer ade rango
 * @return 3 si fallo la escritura en el device
 * @return 4 si fallo la llamada ioctl a SYNC */
uint8_t ciphdev_deletekey (_ciphdev *cd, uint8_t index);

/* Inicializa (abre) el bloque cifrado
 * Retorna el estado actual */
uint8_t ciphdev_initialize (_ciphdev *cd, uint8_t dev, const char *user_key, uint8_t len);

/* Retorna el estado actual */
uint8_t ciphdev_status (_ciphdev *cd);

/* Lee sectores
 * @return 0 si la lectura fue correcta
 * @return 1 si el bloque no está inicializado
 * @return 2 si se intenta leer fuera del bloque
 * @return 3 si hubo error de lectura en el device*/
uint8_t ciphdev_read (_ciphdev *cd, uint8_t* buff, uint32_t sector, uint32_t count);

/* Escribe sectores
 * @return 0 si la escritura fue correcta
 * @return 1 si el bloque no está inicializado
 * @return 2 si se intenta escribir fuera del bloque
 * @return 3 si hubo error de escritura en el device*/
uint8_t ciphdev_write (_ciphdev *cd, const uint8_t *buff, uint32_t sector, uint32_t count);

/* Reescribe el header con la informacion en el struc _ciphdev cd
 * para block_size, version, datetime y user_data
 * An smaller block_size value may corrupt the data.
 * block signature will be changed only if something changes. This procedure
 * does not require random or time function to be linked.
 * @return 0 si la escritura fue correcta
 * @return 1 si el bloque no está inicializado
 * @return 2 si hubo error de escritura en el device*/
uint8_t ciphdev_rewrite_header (_ciphdev *cd);

/* ioctl, retorna valor segun el comando cmd */
uint8_t ciphdev_ioctl (_ciphdev *cd, uint8_t cmd, uint32_t *buff);

/* Attach de las funciones Callback */
void ciphdev_attach_dev_initialize(_ciphdev *cd, ciphdev_dev_initialize_def f);
void ciphdev_attach_dev_status(_ciphdev *cd, ciphdev_dev_status_def f);
void ciphdev_attach_dev_read(_ciphdev *cd, ciphdev_dev_read_def f);
void ciphdev_attach_dev_write(_ciphdev *cd, ciphdev_dev_write_def f);
void ciphdev_attach_dev_ioctl(_ciphdev *cd, ciphdev_dev_ioctl_def f);
void ciphdev_attach_random(_ciphdev *cd, ciphdev_random_def f);
void ciphdev_attach_debug(_ciphdev *cd, ciphdev_debug_def f);

#endif /*  CIPHDEV_H  */
