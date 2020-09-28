#pragma once

#include <stdbool.h>
#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

struct swaybg_state
{
    struct wl_display *display;
    struct wl_compositor *compositor;
    struct wl_shm *shm;
    struct zwlr_layer_shell_v1 *layer_shell;
    struct zxdg_output_manager_v1 *xdg_output_manager;
    struct wl_list configs; // struct swaybg_output_config::link
    struct wl_list outputs; // struct swaybg_output::link
    int epfd;               // epoll file descriptor
    bool run_display;
};
