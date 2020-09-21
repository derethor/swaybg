# ifndef SWAYBG_OUTPUT
# define SWAYBG_OUTPUT

# include "swaybg.h"

extern void render_frame(struct swaybg_output *output);
extern void destroy_swaybg_output(struct swaybg_output *output);
extern void destroy_all_swaybg_output (struct swaybg_state *state);

# endif
