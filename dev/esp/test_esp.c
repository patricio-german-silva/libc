#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include "usart.h"
#include "mytimer.h"
#include "esp8266_transport.h"


void error_message(char *message, uint8_t ecode){
	printf(message, ecode);
	putchar('\n');
}


int main(){
	attach_error_message(error_message);

	char *portname = "/dev/ttyUSB0";

	int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0){
	        printf("error %d opening %s: %s\n", errno, portname, strerror (errno));
	}

	set_interface_attribs (fd, B115200, 0);
	set_blocking(fd, 0);

	write(fd, "AT\r\n", 4);
	char buff;
	
	while(1){
		if (ready_for_reading == -1) {
			printf("Unable to read your input\n");
	        	return -1;
		}
		if (ready_for_reading) {
			read_bytes = read(0, &buff, 1);
			printf("read returns %d\n", read_bytes);
			if(buff != 'Q')
				ready_for_reading = 0;
		}
	}
	return 0;
}
