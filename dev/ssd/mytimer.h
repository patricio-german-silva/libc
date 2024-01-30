#ifndef INC_MYTIMER_H_DEF__
#define INC_MYTIMER_H_DEF__

#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

/* Number of available timers */
#define _MYTIMER_CALLBACK_NUM 10

typedef void (*_mytimer_callback_f)();

typedef struct{
	_mytimer_callback_f timer_callback[_MYTIMER_CALLBACK_NUM];
	uint32_t period[_MYTIMER_CALLBACK_NUM];
	uint32_t curr[_MYTIMER_CALLBACK_NUM];
}_mytimer;

void mytimer_init(uint32_t base_period);
int8_t mytimer_attach(_mytimer_callback_f t, uint32_t p);
int8_t mytimer_dettach(_mytimer_callback_f t);

#endif	/*  INC_MYTIMER_H_DEF__   */
