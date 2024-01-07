#include <stdio.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include "mytimer.h"

#define _WINDOW_HEIGHT      25
#define _WINDOW_WIDTH       80
#define _BAR_HEIGHT         5
#define _PADDING            ((_WINDOW_WIDTH - 3) / 2)


char padding[_PADDING];
uint8_t flag_timer1 = 1;
uint64_t start_time100ms = 0;

void changemode(int);
int  kbhit(char *ch);
void print_screen(uint8_t rbar, uint8_t lbar, uint8_t lscore, uint8_t rscore, uint8_t x, uint8_t y);

void on_timer1(){
    flag_timer1 = 1;
    start_time100ms++;
}

int main(void){
    char ch;
    uint8_t lbar = 0;
    uint8_t rbar = 0;
    uint8_t rscore = 0;
    uint8_t lscore = 0;


    for(int i  = 0; i<_PADDING; i++)
        padding[i] = ' ';

    mytimer_init(999);
    mytimer_attach(on_timer1, 100);

    changemode(1);
    while(ch != 'q'){
        if(kbhit(&ch)){
            switch(ch){
                case 65:{
                            if(lbar > 0) lbar--;
                            break;
                        }
                case 66:{
                            if(lbar < _WINDOW_HEIGHT-_BAR_HEIGHT-1) lbar++;
                            break;
                        }
                case 97:{
                            if(rbar > 0) rbar--;
                            break;
                        }
                case 122:{
                            if(rbar < _WINDOW_HEIGHT-_BAR_HEIGHT-1) rbar++;
                            break;
                        }
            }
        }
        if(flag_timer1 == 1){
            flag_timer1 = 0;
            print_screen(rbar, lbar, lscore, rscore, 0, 0);
        }
    }
    changemode(0);
    return 0;
}



void print_screen(uint8_t rbar, uint8_t lbar, uint8_t lscore, uint8_t rscore, uint8_t x, uint8_t y){
    char lch = ' ';
    char rch = ' ';
    printf("\e[1;1H\e[2J");
    for(uint8_t i = 0; i < _WINDOW_HEIGHT; i++){
        if(i > lbar && i < lbar+_BAR_HEIGHT+1)
            lch = '|';
        else
            lch = ' ';

        if(i > rbar && i < rbar+_BAR_HEIGHT+1)
            rch = '|';
        else
            rch = ' ';

        printf("\033[%d;0H %c%s|%s%c", i, lch, padding, padding, rch);
    }
    printf("\033[%d;%dH %d", _WINDOW_HEIGHT, _WINDOW_WIDTH-12, start_time100ms);
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
