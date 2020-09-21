# include "swaybg.h"

bool store_swaybg_output_config(struct swaybg_state *state, struct swaybg_output_config *config)
{
	struct swaybg_output_config *oc = NULL;

	wl_list_for_each(oc, &state->configs, link)
  {
		if (strcmp(config->output, oc->output) == 0)
    {
			// Merge on top
			if (config->image) {
				free(oc->image);
				oc->image = config->image;
				config->image = NULL;
			}
			if (config->color) {
				oc->color = config->color;
			}
			if (config->mode != BACKGROUND_MODE_INVALID) {
				oc->mode = config->mode;
			}
			return false;
		}
	}
	// New config, just add it
	wl_list_insert(&state->configs, &config->link);
	return true;
}

void destroy_swaybg_output_config(struct swaybg_output_config *config)
{
	if (!config) return;

	wl_list_remove(&config->link);
	free(config->output);
	free(config);
}

void find_config(struct swaybg_output *output, const char *name)
{
	struct swaybg_output_config *config = NULL;
	wl_list_for_each(config, &output->state->configs, link) {
		if (strcmp(config->output, name) == 0) {
			output->config = config;
			return;
		} else if (!output->config && strcmp(config->output, "*") == 0) {
			output->config = config;
		}
	}
}


