#include <assert.h>
#include "state.h"
#include "log.h"
#include "output.h"
#include "path.h"
#include "output_config.h"

bool setup_next_image(struct swaybg_output_config *config)
{
  assert(config != NULL);
  assert(config->path != NULL);

  char *imagename = next_image(config);
  if (imagename)
  {
    cairo_surface_t * image = load_background_image(imagename);
    if (!image) return false;

    release_background_image(config->image);
    config->image = image;

    swaybg_log(LOG_INFO, "display %s", imagename);
    free(imagename);

    return true;
  }

  return false;
}

bool store_swaybg_output_config(struct swaybg_state *state, struct swaybg_output_config *config)
{
  struct swaybg_output_config *oc = NULL;

  wl_list_for_each(oc, &state->configs, link)
  {
    if (strcmp(config->output, oc->output) == 0)
    {
      // Merge on top
      if (config->image)
      {
        free(oc->image);
        oc->image = config->image;
        config->image = NULL;
      }
      if (config->color)
      {
        oc->color = config->color;
      }
      if (config->mode != BACKGROUND_MODE_INVALID)
      {
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
  if (!config)
    return;

  wl_list_remove(&config->link);
  free(config->path);
  free(config->output);
  free(config);
}

void destroy_all_swaybg_output_config(struct swaybg_state *state)
{
  struct swaybg_output_config *config = NULL, *tmp_config = NULL;
  wl_list_for_each_safe(config, tmp_config, &(state->configs), link)
  {
    destroy_swaybg_output_config(config);
  }
}

void find_config(struct swaybg_output *output, const char *name)
{
  struct swaybg_output_config *config = NULL;
  wl_list_for_each(config, &output->state->configs, link)
  {
    if (strcmp(config->output, name) == 0)
    {
      output->config = config;
      return;
    }
    else if (!output->config && strcmp(config->output, "*") == 0)
    {
      output->config = config;
    }
  }
}
