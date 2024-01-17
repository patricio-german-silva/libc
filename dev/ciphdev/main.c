#include "ciphdev.h"
#include "stdint.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define _LOG_LEVEL 6

// File to write
FILE *fp = NULL;

void local_rand(uint32_t *r){
	*r = rand();
}

uint32_t local_time(){
	return time(NULL);
}

uint8_t local_initialize(uint8_t dev){
	if(fp == NULL)
		fp = fopen("/home/psilva/dev.img" ,"rb+");
  if(fp == NULL) return 1;
	return 0;
}

uint8_t local_write(uint8_t dev, const uint8_t *buff, uint32_t sector, uint32_t count){
	if(fseek(fp, sector*512, SEEK_SET) == 0)
			if(fwrite(buff, 1, 512*count, fp) == 512*count)
					return 0;
	return 1;
}


uint8_t local_read(uint8_t dev, uint8_t *buff, uint32_t sector, uint32_t count){
	if(fseek(fp, sector*512, SEEK_SET) == 0)
			if(fread(buff, 1, 512*count, fp) == 512*count)
					return 0;
	return 1;
}

uint8_t local_ioctl(uint8_t dev, uint8_t cmd, uint32_t *buff){
	if(cmd == _CIPHDEV_CTRL_GET_SECTOR_COUNT)
		*buff = 1024*1024*10*2;
	else if(cmd == _CIPHDEV_CTRL_GET_SECTOR_SIZE)
		*buff = 512;
	return 0;
}

// Send output to stderr
static void local_debug(uint8_t level, const char *msg, const char *charg, const uint8_t *arg, uint8_t argl){
        const char levelstr[][6] = {"FATAL\0", "ERROR\0", "WARN\0", "INFO\0", "DEBUG\0", "TRACE\0"};
        if(level > _LOG_LEVEL) return;
        if(level <= 6)
                fprintf(stderr, "[ %s ] : ", levelstr[level-1]);
        fprintf(stderr, "%s", msg);
  if(charg != NULL)     fprintf(stderr, ": %s", charg);
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

int main(){
	_ciphdev cd;
	srand(time(NULL));
	ciphdev_attach_random(&cd, &local_rand);
	ciphdev_attach_debug(&cd, &local_debug);
	ciphdev_attach_dev_read(&cd, &local_read);
	ciphdev_attach_dev_write(&cd, &local_write);
	ciphdev_attach_dev_ioctl(&cd, &local_ioctl);
	ciphdev_attach_dev_initialize(&cd, &local_initialize);
  cd.user_data[0] = 29;
  cd.user_data[1] = 171;
  cd.user_data[2] = 702;
  cd.datetime = time(NULL);
	printf("Create ecod: %d\n", ciphdev_create(&cd, 0, 1024*100*2, "laguagua", 8, 0));
	printf("Initialize ecod: %d\n", ciphdev_initialize(&cd, 0, "laguagua", 8, 0));
	printf("add key ecod: %d\n", ciphdev_addkey(&cd, "watafucke", 9, 3));
	//printf("delete key ecod: %d\n", ciphdev_deletekey(&cd, 0));


  cd.datetime = 1;
  cd.version = 31;
  cd.user_data[0] = 32;
  cd.user_data[1] = 296;
  cd.user_data[2] = 228;
	printf("rewrite header ecod: %d\n", ciphdev_rewrite_header(&cd));

	printf("Initialize ecod: %d\n", ciphdev_initialize(&cd, 0, "watafucke", 9, 3));

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

	printf("Initialize ecod: %d\n", ciphdev_initialize(&cd, 0, "laguagua", 8, 0));

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
