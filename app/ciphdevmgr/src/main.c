#include "ciphdev.h"
#include "strutil.h"
#include "stdint.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define _DEFAULT_LOG_LEVEL 6
#define _BLOCK_MAX_SIZE 4294967295 // En sectores, tamaño maximo es 2T on a 64 bit system

// Descriptor File to read/write and other related data
FILE *fp = NULL;
char openmode[] = "rb+";

// El indice debe coincidir con la clave del array
#define _CMD_CREATE_INDEX 0
#define _CMD_DUMP_INDEX 1
#define _CMD_LOAD_INDEX 2
#define _CMD_SETKEY_INDEX 3
#define _CMD_DELKEY_INDEX 4
#define _CMD_SETDATA_INDEX  5
#define _CMD_SETDATETIME_INDEX  6
#define _CMD_SETSIZE_INDEX  7
#define _CMD_INFO_INDEX 8

#define _OP_SIZE_INDEX 0
#define _OP_KEY_INDEX 1
#define _OP_SLOT_INDEX 2
#define _OP_FILENAME_INDEX 3
#define _OP_DATETIME_INDEX 4
#define _OP_NEWKEY_INDEX 5
#define _OP_USERDATA0_INDEX 6
#define _OP_USERDATA1_INDEX 7
#define _OP_USERDATA2_INDEX 8
#define _OP_LOGLEVEL_INDEX 9

// Command and ops list
const char cmds[][12] = {"CREATE\0", "DUMP\0", "LOAD\0", "SETKEY\0", "DELKEY\0", "SETDATA\0", "SETDATETIME\0", "SETSIZE\0", "INFO\0"};
const char ops[][12] = {"--SIZE\0", "--KEY\0", "--SLOT\0", "--FILENAME\0", "--DATETIME\0", "--NEWKEY\0", "--USERDATA0\0", "--USERDATA1\0", "--USERDATA2\0", "--LOGLEVEL\0"};

// Usage comment
const char usage[] = "USAGE: %s COMMAND OPTIONS\n";

// Curr cmds/ops values
// Flags indica si cada parametro fue o no seteado, dependiendo del parametreo puede requerirlo
typedef struct{
  uint32_t flags;
  uint8_t cmd;
  char filename[256];
  uint32_t size;
  uint8_t slot;
  char key[256];
  char newkey[256];
  uint32_t datetime;
  uint32_t user_data[3];
  uint8_t loglevel;
}_currcmdsops;

// variables globales
_currcmdsops curcmdsops;
_ciphdev cd;


/*
 * Funciones locales para Attach
 */
static void local_rand(uint32_t *r){
	*r = rand();
}

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
	if(argl > 0){
		fprintf(stderr, ": ");
		for(uint32_t i = 0; i < argl; i++) fprintf(stderr, "%d " , arg[i]);
		fprintf(stderr, " [0x");
		for(uint32_t i = 0; i < argl; i++) fprintf(stderr, "%02x", arg[i]);
		fprintf(stderr, "]\n");
	}else{
		fprintf(stderr, "\n");
	}
}


static uint8_t _parse_params(int argc, char *argv[]){
  // Initialize default values
  curcmdsops.flags = 0;
  curcmdsops.slot = 0;
  curcmdsops.loglevel = _DEFAULT_LOG_LEVEL;
  curcmdsops.datetime = time(NULL);
  curcmdsops.user_data[0] = 0;
  curcmdsops.user_data[1] = 0;
  curcmdsops.user_data[2] = 0;
  // Parse params
  char c[256];
  if(_touppercase(argv[1], c, 11) != 0) return 2;

  for(uint8_t i = 0; i < sizeof(cmds)/sizeof(cmds[0]); i++)   // Find command
    if(_strcmp(cmds[i], c, 12) == 0){
      curcmdsops.cmd = i;
      curcmdsops.flags = 0;
      local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "COMMAND NAME (ID)\0", c, &(curcmdsops.cmd), 1);
    }

  // Load params
  for(uint8_t i = 2; i < argc; i++){
    if(_cut(argv[i], '=', 1, c, 12) == 0) return 3;
    if(_touppercase(c, c, 12) != 0) return 4;
    for(uint8_t j = 0; j < sizeof(ops)/sizeof(ops[0]); j++)   // Find ops
      if(_strcmp(ops[j], c, 12) == 0){
        curcmdsops.flags |= 1<<j;
        local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "OPTION NAME (ID)\0", c, &j, 1);
        if(_cut(argv[i], '=', 2, c, 255) == 0) return 5;     // extract opt value
        switch(j){
            case _OP_SIZE_INDEX:
                     if(_str_to_uint32(c, &(curcmdsops.size), 10) == 0) return 6;
                     local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "OPTION VALUE\0", c, (uint8_t*)&(curcmdsops.size), 4);
                     break;
            case _OP_KEY_INDEX:
                     if(_strcpy(c, curcmdsops.key, 255) == 0) return 7;
                     local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "OPTION VALUE\0", curcmdsops.key, 0, 0);
                     break;
            case _OP_SLOT_INDEX:
                     if(_str_to_uint32(c, (uint32_t*)&(curcmdsops.slot), 2) > 1) return 8;
                     local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "OPTION VALUE\0", 0, &(curcmdsops.slot), 1);
                     break;
            case _OP_FILENAME_INDEX:
                     if(_strcpy(c, curcmdsops.filename, 255) == 0) return 7;
                     local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "OPTION VALUE\0", curcmdsops.filename, 0, 0);
                     break;
            case _OP_DATETIME_INDEX:
                     if(_str_to_uint32(c, &(curcmdsops.datetime), 10) == 0) return 9;
                     local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "OPTION VALUE\0", 0, (uint8_t*)&(curcmdsops.datetime), 4);
                     break;
            case _OP_NEWKEY_INDEX:
                     if(_strcpy(c, curcmdsops.newkey, 255) == 0) return 10;
                     local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "OPTION VALUE\0", curcmdsops.newkey, 0, 0);
                     break;
            case _OP_USERDATA0_INDEX:
            case _OP_USERDATA1_INDEX:
            case _OP_USERDATA2_INDEX:
                     if(_str_to_uint32(c, &(curcmdsops.user_data[j - _OP_USERDATA0_INDEX]), 10) == 0) return 11;
                     local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "OPTION VALUE\0", 0, (uint8_t*)&(curcmdsops.user_data[j - _OP_USERDATA0_INDEX]), 4);
                     break;
            case _OP_LOGLEVEL_INDEX:
                     if(_str_to_uint32(c, (uint32_t*)&(curcmdsops.loglevel), 2) > 1) return 12;
                     curcmdsops.flags ^= (1<<_OP_LOGLEVEL_INDEX); // A esta opcion la quito del flags para que no me interfiera con los requeridos
                     local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "OPTION VALUE\0", 0, &(curcmdsops.loglevel), 1);
                     break;
          }
      }
  }
  return 0;
}


// Preparo el struct cd
static void _attach_all(){
  struct timespec ts;
  timespec_get(&ts, time(NULL));
	srand(ts.tv_nsec);
	ciphdev_attach_random(&cd, &local_rand);
	ciphdev_attach_debug(&cd, &local_debug);
	ciphdev_attach_dev_read(&cd, &local_read);
	ciphdev_attach_dev_write(&cd, &local_write);
	ciphdev_attach_dev_ioctl(&cd, &local_ioctl);
	ciphdev_attach_dev_initialize(&cd, &local_initialize);
}


// Decrypt and dump the block content
static uint8_t _dump_content_from_block(){
  local_debug(_CIPHDEV_LOG_LEVEL_INFO, "NOT IMPLEMENTED\0", 0, 0, 0);
  return 0;
}


// encrypt and load stdin to the the BLOCK
static uint8_t _load_data_to_block(){
 local_debug(_CIPHDEV_LOG_LEVEL_INFO, "NOT IMPLEMENTED\0", 0, 0, 0);
  return 0;
}


// encrypt and load stdin to the the BLOCK
static uint8_t _show_block_info(){
 local_debug(_CIPHDEV_LOG_LEVEL_INFO, "NOT IMPLEMENTED\0", 0, 0, 0);
  return 0;
}



// Main program
int main(int argc, char *argv[]){
  // At leas one param, the command
  if(argc < 2){
    printf(usage, argv[0]);
    return 1;
  }

  // Obtengo parametros de la linea de commandos
  local_debug(_CIPHDEV_LOG_LEVEL_INFO, "LOADING PARAMETERS\0", 0, 0, 0);
  uint8_t ecod = _parse_params(argc, argv);
  if(ecod != 0){
    local_debug(_CIPHDEV_LOG_LEVEL_WARN, "Wrong parameters, error code\0", 0, &ecod, 1);
    printf("Wrong parameters. ecod: %d\n\n%s", ecod, usage);
    return ecod;
  }

  // Linqueo todas las Funciones
  _attach_all();

  uint32_t required_opts = 0;  // Opciones minimas requeridas segun el comando
  uint8_t command_ecod = 1;   // Default 1, parametros faltantes
  local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "CURRENT OPTS FOR COMMAND\0", cmds[curcmdsops.cmd], (uint8_t*)&(curcmdsops.flags), 2);
  switch(curcmdsops.cmd){
    // Create new volume
    case _CMD_CREATE_INDEX:
      required_opts = (1<<_OP_SIZE_INDEX) | (1<<_OP_KEY_INDEX) | (1<<_OP_FILENAME_INDEX);
      local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "REQUIERED OPTS (AT LEAST)\0", 0, (uint8_t*)&required_opts, 2);

      if((required_opts & curcmdsops.flags) == required_opts){ // Puede haber otras opciones ademas de las requeridas
        cd.user_data[0] = curcmdsops.user_data[0];
        cd.user_data[1] = curcmdsops.user_data[1];
        cd.user_data[2] = curcmdsops.user_data[2];
        command_ecod = ciphdev_create(&cd, 0, curcmdsops.size, curcmdsops.key, _strlen(curcmdsops.key, 255), curcmdsops.slot);
      }else
        local_debug(_CIPHDEV_LOG_LEVEL_WARN, "REQUIERED OPTS MISSING\0", 0, 0, 0);
      break;

    //Dump block content
    case _CMD_DUMP_INDEX:
      required_opts = (1<<_OP_KEY_INDEX) | (1<<_OP_FILENAME_INDEX);
      local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "REQUIERED OPTS (STRICT)\0", 0, (uint8_t*)&required_opts, 2);

      if(required_opts == curcmdsops.flags) // Las opciones seteadas deben ser estrictamente las requeridas
        command_ecod = _dump_content_from_block();
      else
        local_debug(_CIPHDEV_LOG_LEVEL_WARN, "REQUIERED OPTS MISSING\0", 0, 0, 0);
      break;

    //Load raw data to block
    case _CMD_LOAD_INDEX:
      required_opts = (1<<_OP_KEY_INDEX) | (1<<_OP_FILENAME_INDEX);
      local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "REQUIERED OPTS (STRICT)\0", 0, (uint8_t*)&required_opts, 2);

      if(required_opts == curcmdsops.flags) // Las opciones seteadas deben ser estrictamente las requeridas
        command_ecod = _load_data_to_block();
      else
        local_debug(_CIPHDEV_LOG_LEVEL_WARN, "REQUIERED OPTS MISSING\0", 0, 0, 0);
      break;

    // Set/add key
    case _CMD_SETKEY_INDEX:
      required_opts = (1<<_OP_SLOT_INDEX) | (1<<_OP_KEY_INDEX) | (1<<_OP_FILENAME_INDEX) | (1<<_OP_NEWKEY_INDEX);
      local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "REQUIERED OPTS (STRICT)\0", 0, (uint8_t*)&required_opts, 2);

      if(required_opts == curcmdsops.flags) // Las opciones seteadas deben ser estrictamente las requeridas
        command_ecod = ciphdev_addkey(&cd, curcmdsops.key, _strlen(curcmdsops.key, 255), curcmdsops.slot);
      else
        local_debug(_CIPHDEV_LOG_LEVEL_WARN, "REQUIERED OPTS MISSING\0", 0, 0, 0);
      break;

    // Delete key
    case _CMD_DELKEY_INDEX:
      required_opts = (1<<_OP_SLOT_INDEX) | (1<<_OP_KEY_INDEX) | (1<<_OP_FILENAME_INDEX);
      local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "REQUIERED OPTS (STRICT)\0", 0, (uint8_t*)&required_opts, 2);

      if(required_opts == curcmdsops.flags) // Las opciones seteadas deben ser estrictamente las requeridas
        command_ecod = ciphdev_deletekey(&cd, curcmdsops.slot);
      else
        local_debug(_CIPHDEV_LOG_LEVEL_WARN, "REQUIERED OPTS MISSING\0", 0, 0, 0);
      break;

    // Set user data
    case _CMD_SETDATA_INDEX:
      required_opts = (1<<_OP_KEY_INDEX) | (1<<_OP_FILENAME_INDEX);
      local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "REQUIERED OPTS (AT LEAST)\0", 0, (uint8_t*)&required_opts, 2);

      if((required_opts & curcmdsops.flags) == required_opts){  // Puede haber otras opciones ademas de las requeridas
        if((((1<<_OP_USERDATA0_INDEX) | (1<<_OP_USERDATA1_INDEX) | (1<<_OP_USERDATA2_INDEX)) & curcmdsops.flags) != 0){
          if((curcmdsops.flags & (1<<_OP_USERDATA0_INDEX)) != 0) cd.user_data[0] = curcmdsops.user_data[0]; else cd.user_data[0] = 0;
          if((curcmdsops.flags & (1<<_OP_USERDATA1_INDEX)) != 0) cd.user_data[1] = curcmdsops.user_data[1]; else cd.user_data[1] = 0;
          if((curcmdsops.flags & (1<<_OP_USERDATA2_INDEX)) != 0) cd.user_data[2] = curcmdsops.user_data[2]; else cd.user_data[2] = 0;
          command_ecod = ciphdev_rewrite_header(&cd);
        }else{// No data is set, do nothing
          local_debug(_CIPHDEV_LOG_LEVEL_WARN, "NO DATAUSER SET, QUIT\0", 0, 0, 0);
        }
      }else{
        local_debug(_CIPHDEV_LOG_LEVEL_WARN, "REQUIERED OPTS MISSING\0", 0, 0, 0);
      }
      break;

    // Set creation datetime
    case _CMD_SETDATETIME_INDEX:
      required_opts = (1<<_OP_DATETIME_INDEX) | (1<<_OP_KEY_INDEX) | (1<<_OP_FILENAME_INDEX);
      local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "REQUIERED OPTS (STRICT)\0", 0, (uint8_t*)&required_opts, 2);

      if(required_opts == curcmdsops.flags) // Las opciones seteadas deben ser estrictamente las requeridas
        command_ecod = ciphdev_rewrite_header(&cd);
      else
        local_debug(_CIPHDEV_LOG_LEVEL_WARN, "REQUIERED OPTS MISSING\0", 0, 0, 0);
      break;

    // Set block size (WARNING)
    case _CMD_SETSIZE_INDEX:
      required_opts = (1<<_OP_SIZE_INDEX) | (1<<_OP_KEY_INDEX) | (1<<_OP_FILENAME_INDEX);
      local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "REQUIERED OPTS (STRICT)\0", 0, (uint8_t*)&required_opts, 2);

      if(required_opts == curcmdsops.flags) // Las opciones seteadas deben ser estrictamente las requeridas
        command_ecod = ciphdev_rewrite_header(&cd);
      else
        local_debug(_CIPHDEV_LOG_LEVEL_WARN, "REQUIERED OPTS MISSING\0", 0, 0, 0);
      break;

    // Show block Info
    case _CMD_INFO_INDEX:
      required_opts = (1<<_OP_KEY_INDEX) | (1<<_OP_FILENAME_INDEX);
      local_debug(_CIPHDEV_LOG_LEVEL_DEBUG, "REQUIERED OPTS (STRICT)\0", 0, (uint8_t*)&required_opts, 2);

      if(required_opts == curcmdsops.flags){ // Las opciones seteadas deben ser estrictamente las requeridas
        command_ecod = ciphdev_initialize(&cd, 0, curcmdsops.key, _strlen(curcmdsops.key, 255));
        if(command_ecod == 0)
          _show_block_info();
      }else{
        local_debug(_CIPHDEV_LOG_LEVEL_WARN, "REQUIERED OPTS MISSING\0", 0, 0, 0);
      }
      break;
  }
  local_debug(_CIPHDEV_LOG_LEVEL_INFO, "COMMAND ECOD\0", 0, &command_ecod, 1);
}




int nomain(){
  cd.user_data[0] = 29;
  cd.user_data[1] = 171;
  cd.user_data[2] = 702;
	printf("Create ecod: %d\n", ciphdev_create(&cd, 0, 1024*100*2, "laguagua", 8, 0));
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
