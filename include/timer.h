# pragma once
# include "swaybg.h"

extern void timer_cb(int tfd , int revents , struct swaybg_state * state );
extern int timer_set ( int tfd , long ms );
extern int timer_init (int epfd , int * tfd );

