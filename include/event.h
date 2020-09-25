# pragma once
# include "swaybg.h"

extern int setup_event_loop (struct swaybg_state * state);
extern int setup_output_event (struct swaybg_output *output);
extern int run_event_loop ( struct swaybg_state * state );
