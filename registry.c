#include <assert.h>
#include <stdint.h>
#include <wayland-client.h>

#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

#include "state.h"
#include "swaybg.h"
#include "log.h"
#include "setup.h"
#include "output.h"
#include "event.h"

static void output_geometry(void *data, struct wl_output *output, int32_t x,
                            int32_t y, int32_t width_mm, int32_t height_mm, int32_t subpixel,
                            const char *make, const char *model, int32_t transform)
{
  // Who cares
}

static void output_mode(void *data, struct wl_output *output, uint32_t flags,
                        int32_t width, int32_t height, int32_t refresh)
{
  // Who cares
}

static void output_done(void *data, struct wl_output *output)
{
  // Who cares
}

static void output_scale(void *data, struct wl_output *wl_output,
                         int32_t scale)
{
  struct swaybg_output *output = data;
  output->scale = scale;
  if (output->state->run_display && output->width > 0 && output->height > 0)
  {
    render_frame(output);
  }
}

static const struct wl_output_listener output_listener = {
    .geometry = output_geometry,
    .mode = output_mode,
    .done = output_done,
    .scale = output_scale,
};

static void xdg_output_handle_logical_position(void *data,
                                               struct zxdg_output_v1 *xdg_output, int32_t x, int32_t y)
{
  // Who cares
}

static void xdg_output_handle_logical_size(void *data,
                                           struct zxdg_output_v1 *xdg_output, int32_t width, int32_t height)
{
  // Who cares
}

static void layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *surface, uint32_t serial, uint32_t width, uint32_t height)
{
  struct swaybg_output *output = data;
  output->width = width;
  output->height = height;
  zwlr_layer_surface_v1_ack_configure(surface, serial);
  render_frame(output);
}

static void layer_surface_closed(void *data, struct zwlr_layer_surface_v1 *surface)
{
  struct swaybg_output *output = data;
  swaybg_log(LOG_DEBUG, "Destroying output %s (%s)", output->name, output->identifier);
  destroy_swaybg_output(output);
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_configure,
    .closed = layer_surface_closed,
};

static void create_layer_surface(struct swaybg_output *output)
{
  assert(output);

  output->surface = wl_compositor_create_surface(output->state->compositor);
  assert(output->surface);

  // Empty input region
  struct wl_region *input_region = wl_compositor_create_region(output->state->compositor);
  assert(input_region);
  wl_surface_set_input_region(output->surface, input_region);
  wl_region_destroy(input_region);

  output->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
      output->state->layer_shell, output->surface, output->wl_output,
      ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND, "wallpaper");
  assert(output->layer_surface);

  zwlr_layer_surface_v1_set_size(output->layer_surface, 0, 0);
  zwlr_layer_surface_v1_set_anchor(output->layer_surface,
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                       ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
                                       ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                       ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
  zwlr_layer_surface_v1_set_exclusive_zone(output->layer_surface, -1);
  zwlr_layer_surface_v1_add_listener(output->layer_surface,
                                     &layer_surface_listener, output);
  wl_surface_commit(output->surface);
}

static void xdg_output_handle_name(void *data, struct zxdg_output_v1 *xdg_output, const char *name)
{
  struct swaybg_output *output = data;
  output->name = strdup(name);

  // If description was sent first, the config may already be populated. If
  // there is an identifier config set, keep it.
  if (!output->config || strcmp(output->config->output, "*") == 0)
    find_config(output, name);
}

static void xdg_output_handle_description(void *data, struct zxdg_output_v1 *xdg_output, const char *description)
{
  struct swaybg_output *output = data;

  // wlroots currently sets the description to `make model serial (name)`
  // If this changes in the future, this will need to be modified.
  char *paren = strrchr(description, '(');
  if (paren)
  {
    size_t length = paren - description;
    output->identifier = malloc(length);
    if (!output->identifier)
    {
      swaybg_log(LOG_ERROR, "Failed to allocate output identifier");
      return;
    }
    strncpy(output->identifier, description, length);
    output->identifier[length - 1] = '\0';

    find_config(output, output->identifier);
  }
}

static void xdg_output_handle_done(void *data, struct zxdg_output_v1 *xdg_output)
{
  struct swaybg_output *output = data;

  if (!output->config)
  {
    swaybg_log(LOG_DEBUG, "Could not find config for output %s (%s)", output->name, output->identifier);
    destroy_swaybg_output(output);
  }
  else if (!output->layer_surface)
  {
    swaybg_log(LOG_DEBUG, "Found config %s for output %s (%s)", output->config->output, output->name, output->identifier);
    create_layer_surface(output);
    setup_output_event(output);
  }
}

static const struct zxdg_output_v1_listener xdg_output_listener = {
    .logical_position = xdg_output_handle_logical_position,
    .logical_size = xdg_output_handle_logical_size,
    .name = xdg_output_handle_name,
    .description = xdg_output_handle_description,
    .done = xdg_output_handle_done,
};

static void handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
  struct swaybg_state *state = data;
  if (strcmp(interface, wl_compositor_interface.name) == 0)
  {
    state->compositor =
        wl_registry_bind(registry, name, &wl_compositor_interface, 4);
  }
  else if (strcmp(interface, wl_shm_interface.name) == 0)
  {
    state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
  }
  else if (strcmp(interface, wl_output_interface.name) == 0)
  {
    struct swaybg_output *output = calloc(1, sizeof(struct swaybg_output));
    output->state = state;
    output->wl_name = name;
    output->wl_output = wl_registry_bind(registry, name, &wl_output_interface, 3);
    wl_output_add_listener(output->wl_output, &output_listener, output);
    wl_list_insert(&state->outputs, &output->link);

    if (state->run_display)
    {
      output->xdg_output = zxdg_output_manager_v1_get_xdg_output(state->xdg_output_manager, output->wl_output);
      zxdg_output_v1_add_listener(output->xdg_output, &xdg_output_listener, output);
    }
  }
  else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0)
  {
    state->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
  }
  else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0)
  {
    state->xdg_output_manager = wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, 2);
  }
}

static void handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
  struct swaybg_state *state = data;
  struct swaybg_output *output, *tmp;
  wl_list_for_each_safe(output, tmp, &state->outputs, link)
  {
    if (output->wl_name == name)
    {
      swaybg_log(LOG_DEBUG, "Destroying output %s (%s)",
                 output->name, output->identifier);
      destroy_swaybg_output(output);
      break;
    }
  }
}

static const struct wl_registry_listener registry_listener = {
    .global = handle_global,
    .global_remove = handle_global_remove,
};

struct wl_registry *setup_registry(struct swaybg_state *state)
{
  assert(state != NULL);

  struct wl_registry *registry = wl_display_get_registry(state->display);
  assert(registry != NULL);

  wl_registry_add_listener(registry, &registry_listener, state);

  wl_display_roundtrip(state->display);

  if (state->compositor == NULL || state->shm == NULL || state->layer_shell == NULL || state->xdg_output_manager == NULL)
  {
    swaybg_log(LOG_ERROR, "Missing a required Wayland interface");
    return NULL;
  }

  struct swaybg_output *output;
  wl_list_for_each(output, &(state->outputs), link)
  {
    output->xdg_output = zxdg_output_manager_v1_get_xdg_output(state->xdg_output_manager, output->wl_output);
    zxdg_output_v1_add_listener(output->xdg_output, &xdg_output_listener, output);
  }

  return registry;
}
