#include "mytimer.h"

static sigset_t block ;

static _mytimer _mytimer_handler;

void timer_handler ();

void mytimer_init(uint32_t base_period){
    for(uint8_t i = 0; i<_MYTIMER_CALLBACK_NUM; i++){
	    _mytimer_handler.timer_callback[i] = NULL;
	    _mytimer_handler.period[i] = 0;
	    _mytimer_handler.curr[i] = 0;
    }
    sigemptyset(&block);
    sigaddset(&block,SIGALRM);
    struct sigaction act={0};
    struct timeval interval;
    struct itimerval period;
    act.sa_handler=timer_handler;
    assert(sigaction(SIGALRM,&act,NULL)==0);

    interval.tv_sec=0;
    interval.tv_usec=base_period;
    period.it_interval=interval;
    period.it_value=interval;
    setitimer(ITIMER_REAL,&period,NULL);
}

void timer_handler(int sig){
    for(uint8_t i = 0; i<_MYTIMER_CALLBACK_NUM; i++){
        if(_mytimer_handler.period[i] > 0){
            _mytimer_handler.curr[i]++;
            if(_mytimer_handler.curr[i] == _mytimer_handler.period[i]){
                _mytimer_handler.curr[i] = 0;
                _mytimer_handler.timer_callback[i]();
            }
        }
    }
}

int8_t mytimer_attach(_mytimer_callback_f t, uint32_t p){
    for(uint8_t i = 0; i<_MYTIMER_CALLBACK_NUM; i++){
	if(_mytimer_handler.period[i] == 0){
	    _mytimer_handler.timer_callback[i] = t;
	    _mytimer_handler.period[i] = p;
	    _mytimer_handler.curr[i] = 0;
	    return i;
	}
    }
    return -1;
}

int8_t mytimer_dettach(_mytimer_callback_f t){
    for(uint8_t i = 0; i<_MYTIMER_CALLBACK_NUM; i++){
	if(_mytimer_handler.timer_callback[i] == t){
	    _mytimer_handler.timer_callback[i] = NULL;
	    _mytimer_handler.period[i] = 0;
	    _mytimer_handler.curr[i] = 0;
	    return i;
	}
    }
    return -1;
}

/*
void on_timer1(){
	write(1, "Hi!, 1/n", 7);
}

void on_timer2(){
	write(1, "Hi!, 2/n", 7);
}



int main(){
	mytimer_init(99999);
	printf("Timer 1 on %d", mytimer_attach(on_timer1, 15));
	printf("Timer 2 on %d", mytimer_attach(on_timer2, 5));
	while(1);
	return 0;
}*/
