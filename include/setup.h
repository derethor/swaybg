# ifndef __SWAYBG_SETUP
# define __SWAYBG_SETUP

# include "swaybg.h"
extern bool setup_next_image ( struct swaybg_state *state, struct swaybg_output_config *config);
extern bool store_swaybg_output_config(struct swaybg_state *state, struct swaybg_output_config *config);
extern void destroy_swaybg_output_config(struct swaybg_output_config *config);
extern void destroy_all_swaybg_output_config (struct swaybg_state *state);
extern void find_config(struct swaybg_output *output, const char *name);

# endif
