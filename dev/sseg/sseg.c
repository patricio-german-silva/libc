#include <stdio.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include "mytimer.h"
#include "seven_segments.h"


uint8_t flag_timer1 = 1;
static const char seghon[] =  "   ########   ";
static const char seghoff[] = "              ";
static const char segvon[] =  "   #   ";
static const char segvoff[] = "       ";
static const char doton[] =   " # ";
static const char dotoff[] =  "   ";

uint8_t pins[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void changemode(int);
int  kbhit(char *ch);

void print_screen();
void set_pin_on(uint32_t port, uint16_t pin){
	pins[pin] = 1;
}

void set_pin_off(uint32_t port, uint16_t pin){
	pins[pin] = 0;
}

void on_timer1(){
    flag_timer1 = 1;
}

int main(void){
	char ch = 0;
    mytimer_init(999);
    mytimer_attach(on_timer1, 1);

    changemode(1);
    while(ch != 'q'){
        kbhit(&ch);
        if(flag_timer1 == 1){
            flag_timer1 = 0;
            print_screen();
        }
    }
    changemode(0);
    return 0;
}



void print_screen(){
	printf("\e[1;1H\e[2J");
   for(uint8_t i = 0; i < 13; i++){
		// A
		
   }
}





void changemode(int dir){
    tcflush(0, TCIFLUSH);
    static struct termios oldt, newt;
    if (dir == 1){
        tcgetattr( STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~( ICANON | ECHO );
        tcsetattr( STDIN_FILENO, TCSANOW, &newt);
    }else{
        tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
    }
}

int kbhit (char *ch){
    struct timeval tv;
    fd_set rdfs;

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&rdfs);
    FD_SET (STDIN_FILENO, &rdfs);

    select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
    if(FD_ISSET(STDIN_FILENO, &rdfs)){
        *ch = getchar();
        if(*ch == 27){
            *ch = getchar();
            *ch = getchar();
        }
        return 1;
    }else{
        return 0;
    }
}
