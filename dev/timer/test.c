#include <stdio.h>
#include <stdint.h>
#include "mytimer.h"


void on_timer1(){
	printf("Hello, im %d\n", 1);
}

void on_timer2(){
	printf("Hello, im %d\n", 2);
}

int main(){
	mytimer_init(100000);

	mytimer_attach(on_timer1, 10);
	mytimer_attach(on_timer2, 40);

	while (1);
	return 0;
}
