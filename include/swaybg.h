# ifndef __SWAYBG_INCLUDE
# define __SWAYBG_INCLUDE

# include <wayland-client.h>
# include "pool-buffer.h"
# include "background-image.h"
struct swaybg_output_config
{
	char *output;
  char *path;
  uint32_t seed;
  uint32_t seconds;
	cairo_surface_t *image;
	enum background_mode mode;
	uint32_t color;
	struct wl_list link;
};

struct swaybg_output
{
	uint32_t wl_name;
	struct wl_output *wl_output;
	struct zxdg_output_v1 *xdg_output;
	char *name;
	char *identifier;

	struct swaybg_state *state;
	struct swaybg_output_config *config;

	struct wl_surface *surface;
	struct zwlr_layer_surface_v1 *layer_surface;
	struct pool_buffer buffers[2];
	struct pool_buffer *current_buffer;

	uint32_t width, height;
	int32_t scale;

  int tfd;  // timer fd

	struct wl_list link;
};

# endif

