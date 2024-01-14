/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "ciphdev.h"
#include <stdio.h>
#include <time.h>

#define __USER_KEY "Heq3S52pJ"
#define __USER_KEY_LEN 9
#define __DEVICE_FILENAME "dev.img"
#define _DEFAULT_LOG_LEVEL 6
#define _BLOCK_MAX_SIZE 4294967295 // En sectores, tamaño maximo es 2T on a 64 bit system

// Descriptor File to read/write and other related data
FILE *fp = NULL;
char openmode[] = "rb+";


// Prototipos
static uint8_t local_initialize(uint8_t dev);
static uint8_t local_write(uint8_t dev, const uint8_t *buff, uint32_t sector, uint32_t count);
static uint8_t local_read(uint8_t dev, uint8_t *buff, uint32_t sector, uint32_t count);
static uint8_t local_ioctl(uint8_t dev, uint8_t cmd, uint32_t *buff);

// variables globales
_ciphdev cd;

static uint8_t local_initialize(uint8_t dev){
	if(fp == NULL)
		fp = fopen(__DEVICE_FILENAME , openmode);
	if(fp == NULL) return 1;
	return 0;
}

static uint8_t local_write(uint8_t dev, const uint8_t *buff, uint32_t sector, uint32_t count){
	if(fseek(fp, sector*512, SEEK_SET) == 0)
			if(fwrite(buff, 1, 512*count, fp) == 512*count)
					return 0;
	return 1;
}


static uint8_t local_read(uint8_t dev, uint8_t *buff, uint32_t sector, uint32_t count){
	if(fseek(fp, sector*512, SEEK_SET) == 0)
			if(fread(buff, 1, 512*count, fp) == 512*count)
					return 0;
	return 1;
}

static uint8_t local_ioctl(uint8_t dev, uint8_t cmd, uint32_t *buff){
	if(cmd == _CIPHDEV_CTRL_GET_SECTOR_COUNT)
		*buff = _BLOCK_MAX_SIZE;
	else if(cmd == _CIPHDEV_CTRL_GET_SECTOR_SIZE)
		*buff = _CIPHDEV_SECTOR_SIZE;
	return 0;
}

// Send output to stderr
static void local_debug(uint8_t level, const char *msg, const char *charg, const uint8_t *arg, uint8_t argl){
	const char levelstr[][6] = {"FATAL\0", "ERROR\0", "WARN\0", "INFO\0", "DEBUG\0", "TRACE\0"};
	if(level > _DEFAULT_LOG_LEVEL) return;
	if(level <= 6)
		fprintf(stderr, "[ %s ] : ", levelstr[level-1]);
	fprintf(stderr, "%s", msg);
  if(charg != NULL)	fprintf(stderr, ": %s", charg);
	if(arg != NULL && argl > 0){
		fprintf(stderr, ": ");
		for(uint32_t i = 0; i < argl; i++) fprintf(stderr, "%d " , arg[i]);
		fprintf(stderr, " [0x");
		for(uint32_t i = 0; i < argl; i++) fprintf(stderr, "%02x", arg[i]);
		fprintf(stderr, "]\n");
	}else{
		fprintf(stderr, "\n");
	}
}

// Preparo el struct cd
static void _attach_all(){
	ciphdev_attach_debug(&cd, &local_debug);
	ciphdev_attach_dev_read(&cd, &local_read);
	ciphdev_attach_dev_write(&cd, &local_write);
	ciphdev_attach_dev_ioctl(&cd, &local_ioctl);
	ciphdev_attach_dev_initialize(&cd, &local_initialize);
}
/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
  if(cd.status == _CIPHDEV_STATUS_INIT)
    return 0;
  else
  	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
  _attach_all();
  if(ciphdev_initialize(&cd, pdrv, __USER_KEY, __USER_KEY_LEN) == _CIPHDEV_STATUS_INIT)
    return 0;
  else
  	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
  if(ciphdev_read(&cd, buff, sector, count) == 0)
    return RES_OK;
  else
  	return RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
  if(ciphdev_write(&cd, buff, sector, count) == 0)
    return RES_OK;
  else
  	return RES_ERROR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
  if(ciphdev_ioctl(&cd, cmd, (uint32_t*)buff) == 0)
    return RES_OK;
  else
  	return RES_ERROR;
}


DWORD get_fattime (void)
{
    time_t t;
    struct tm *stm;


    t = time(0);
    stm = localtime(&t);

    return (DWORD)(stm->tm_year - 80) << 25 |
           (DWORD)(stm->tm_mon + 1) << 21 |
           (DWORD)stm->tm_mday << 16 |
           (DWORD)stm->tm_hour << 11 |
           (DWORD)stm->tm_min << 5 |
           (DWORD)stm->tm_sec >> 1;
}
