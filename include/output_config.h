#pragma once

#include <stdint.h>
#include "background-image.h"

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
