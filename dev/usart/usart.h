#ifndef __USART_H_DEF__
#define __USART_H_DEF__


#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>


typedef void (*usart_error_mesage_callback_f)(char *msg, uint8_t ecode);

void attach_error_message(usart_error_mesage_callback_f f);

int set_interface_attribs (int fd, int speed, int parity);

void set_blocking (int fd, int should_block);


#endif	/*  __USART_H_DEF__  */
