#include "ciphdev.h"
#include "stdint.h"
#include <stdio.h>
#include <unistd.h>

// Descriptor File to read/write and other related data
FILE *fp = NULL;
char openmode[] = "rb+";


// Prototipos
static void local_rand(uint32_t *r);
static uint8_t local_initialize(uint8_t dev);
static uint8_t local_write(uint8_t dev, const uint8_t *buff, uint32_t sector, uint32_t count);
static uint8_t local_read(uint8_t dev, uint8_t *buff, uint32_t sector, uint32_t count);
static uint8_t local_ioctl(uint8_t dev, uint8_t cmd, uint32_t *buff);
static void local_debug(uint8_t level, const char *msg, const char *charg, const uint8_t *arg, uint8_t argl);

// variables globales
_ciphdev cd;

static uint8_t local_initialize(uint8_t dev){
	if(fp == NULL)
		fp = fopen(curcmdsops.filename , openmode);
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
	if(level > curcmdsops.loglevel) return;
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
	ciphdev_attach_random(&cd, &local_rand);
	ciphdev_attach_debug(&cd, &local_debug);
	ciphdev_attach_dev_read(&cd, &local_read);
	ciphdev_attach_dev_write(&cd, &local_write);
	ciphdev_attach_dev_ioctl(&cd, &local_ioctl);
	ciphdev_attach_dev_initialize(&cd, &local_initialize);
}



// Main program
int main(int argc, char *argv[]){
  // At leas one param, the command
  if(argc < 2){
    printf(usage, argv[0]);
    return 1;
  }
  // Linqueo todas las Funciones
  _attach_all();
  return 0;
}
