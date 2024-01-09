#include "ciphdev.h"
#include "strutil.h"
#include "stdint.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define _LOG_LEVEL 6
#define _BLOCK_MAX_SIZE 4294967295 // En sectores, tamaño maximo es 2T on a 64 bit system

// Descriptor File to read/write and other related data
FILE *fp = NULL;
char openmode[] = "rb+";

// Command and ops list
const char cmds[][12] = {"CREATE\0", "DUMP\0", "LOAD\0", "SETKEY\0", "DELKEY\0", "SETDATA\0", "SETDATETIME\0", "SETSIZE\0", "INFO\0"};
const char ops[][12] = {"--SIZE\0", "--KEY\0", "--SLOT\0", "--FILENAME\0", "--DATETIME\0", "--USERDATA0\0", "--USERDATA1\0", "--USERDATA2\0"};

// Usage comment
const char usage[] = "USAGE: %s COMMAND OPTIONS\n";

// Curr cmds/ops values
// Flags indica si cada parametro fue o no seteado, dependiendo del parametreo puede requerirlo
typedef struct{
  uint8_t flags;
  uint8_t cmd;
  char filename[256];
  uint32_t size;
  uint8_t slot;
  char key[256];
  uint32_t datetime;
  uint32_t user_data[3];
}_currcmdsops;

_currcmdsops curcmdsops;


/*
 * Funciones locales para Attach
 */
static void local_rand(uint32_t *r){
	*r = rand();
}

static uint32_t local_time(){
	return time(NULL);
}

static uint8_t local_initialize(uint8_t dev){
	if(fp == NULL)
		fp = fopen(curcmdsops.filename , openmode);
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

static void local_debug(uint8_t level, const char *msg, const uint8_t *arg, uint8_t argl){
	const char levelstr[][6] = {"FATAL\0", "ERROR\0", "WARN\0", "INFO\0", "DEBUG\0", "TRACE\0"};
	if(level > _LOG_LEVEL) return;
	if(level <= 6)
		printf("[ %s ] : ", levelstr[level-1]);
	printf("%s", msg);
	if(argl > 0){
		printf(": ");
		for(uint32_t i = 0; i < argl; i++) printf("%d " , arg[i]);
		printf(" [0x");
		for(uint32_t i = 0; i < argl; i++) printf("%02x", arg[i]);
		printf("]\n");
	}else{
		printf("\n");
	}
}


// Main program
int main(int argc, char *argv[]){
  // At leas one param, the command
  if(argc < 2){
    printf(usage, argv[0]);
    return 1;
  }


  // Parse params
  curcmdsops.flags = 0;
  char c[256];
  if(_touppercase(argv[1], c, 11) != 0) return 2;

  for(uint8_t i = 0; i < sizeof(cmds)/sizeof(cmds[0]); i++)   // Find command
    if(_strcmp(cmds[i], c, 12) == 0){
      curcmdsops.cmd = i;
      curcmdsops.flags = 1;
      local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "COMMAND ID:\0", &(curcmdsops.cmd), 1);
    }

  // Load params
  for(uint8_t i = 2; i < argc; i++){
    if(_cut(argv[i], '=', 1, c, 11) == 0) return 3;
    if(_touppercase(c, c, 11) != 0) return 4;
    for(uint8_t j = 0; j < sizeof(ops)/sizeof(ops[0]); j++)   // Find ops
      if(_strcmp(ops[j], c, 12) == 0){
        curcmdsops.flags |= 1<<j;
        local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "OPTION ID:\0", &j, 1);
          if(_cut(argv[i], '=', 2, c, 255) == 0) return 5;     // extract opt value
          switch(j){
            case 0:
                     if(_str_to_uint32(c, &(curcmdsops.size), 10) == 0) return 6;
                     local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "OPTION VALUE:\0", (uint8_t*)&(curcmdsops.size), 4);
                     break;
            case 1:
                     if(_strcpy(c, curcmdsops.key, 255) == 0) return 7;
                     local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "OPTION VALUE:\0", 0, 0);
                     local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, curcmdsops.key, 0, 0);
                     break;
            case 2:
                     if(_str_to_uint32(c, (uint32_t*)&(curcmdsops.slot), 2) > 1) return 8;
                     local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "OPTION VALUE:\0", &(curcmdsops.slot), 1);
                     break;
            case 3:
                     if(_strcpy(c, curcmdsops.filename, 255) == 0) return 7;
                     local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "OPTION VALUE:\0", 0, 0);
                     local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, curcmdsops.filename, 0, 0);
                     break;
            case 5:
                     if(_str_to_uint32(c, &(curcmdsops.size), 10) == 0) return 9;
                     local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "OPTION VALUE:\0", 0, 0);
                     break;
            case 6:
            case 7:
            case 8:
                     if(_str_to_uint32(c, &(curcmdsops.user_data[j-6]), 10) == 0) return 10;
                     local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "OPTION VALUE:\0", (uint8_t*)&(curcmdsops.user_data[j-6]), 4);
                     break;
          }

      }
  }


}


int nomain(){
	_ciphdev cd;
	srand(time(NULL));
	ciphdev_attach_random(&cd, &local_rand);
	ciphdev_attach_time(&cd, &local_time);
	ciphdev_attach_debug(&cd, &local_debug);
	ciphdev_attach_dev_read(&cd, &local_read);
	ciphdev_attach_dev_write(&cd, &local_write);
	ciphdev_attach_dev_ioctl(&cd, &local_ioctl);
	ciphdev_attach_dev_initialize(&cd, &local_initialize);
  cd.user_data[0] = 29;
  cd.user_data[1] = 171;
  cd.user_data[2] = 702;
	printf("Create ecod: %d\n", ciphdev_create(&cd, 0, 1024*100*2, "laguagua", 8));
	printf("Initialize ecod: %d\n", ciphdev_initialize(&cd, 0, "laguagua", 8));
	printf("add key ecod: %d\n", ciphdev_addkey(&cd, "watafucke", 9, 3));
	//printf("delete key ecod: %d\n", ciphdev_deletekey(&cd, 0));


  cd.datetime = 1;
  cd.version = 31;
  cd.user_data[0] = 32;
  cd.user_data[1] = 296;
  cd.user_data[2] = 228;
	printf("rewrite header ecod: %d\n", ciphdev_rewrite_header(&cd));

	printf("Initialize ecod: %d\n", ciphdev_initialize(&cd, 0, "watafucke", 9));

	FILE *fpread = fopen("/home/psilva/read.txt" ,"r");
	FILE *fpwrite = fopen("/home/psilva/write.txt" ,"w+");
	uint8_t b[1024];
	uint32_t scount = 0;
	uint32_t bcount = 0;
	char c;
	while(1) {
		c = fgetc(fpread);
    if( feof(fpread) ) {
			 if(bcount > 0){
				 ciphdev_write(&cd, b, scount, 2);
				 scount += 2;
			 }
       break ;
    }
		b[bcount++] = (uint8_t)c;
		if(bcount == 1024){
			ciphdev_write(&cd, b, scount, 2);
			scount += 2;
			bcount = 0;
		}
	}

	printf("Initialize ecod: %d\n", ciphdev_initialize(&cd, 0, "laguagua", 8));

	for(uint32_t i = 0; i<scount; i++){
		ciphdev_read(&cd, b, i, 1);
		if(i == scount-1)
			fwrite(b, 1, bcount, fpwrite);
		else
			fwrite(b, 1, 512, fpwrite);
	}

	fclose(fp);
	fclose(fpread);
	fclose(fpwrite);
	return 0;
}
