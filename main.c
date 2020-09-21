#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

# include <time.h>
# include <errno.h>
# include <stdio.h>
# include <unistd.h>
# include <memory.h>
# include <sys/epoll.h>

# include "swaybg.h"
# include "setup.h"
# include "color.h"
# include "output.h"
# include "registry.h"
# include "path.h"
# include "event.h"

#include "background-image.h"
#include "cairo.h"
#include "log.h"
#include "pool-buffer.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

static void parse_command_line(int argc, char **argv,
		struct swaybg_state *state) {
	static struct option long_options[] = {
		{"color", required_argument, NULL, 'c'},
		{"help", no_argument, NULL, 'h'},
		{"image", required_argument, NULL, 'i'},
		{"path", required_argument, NULL, 'p'},
		{"mode", required_argument, NULL, 'm'},
		{"output", required_argument, NULL, 'o'},
		{"version", no_argument, NULL, 'v'},
		{0, 0, 0, 0}
	};

	const char *usage =
		"Usage: swaybg <options...>\n"
		"\n"
		"  -c, --color            Set the background color.\n"
		"  -h, --help             Show help message and quit.\n"
		"  -i, --image            Set the image to display.\n"
		"  -p, --path             Set the path of images to display.\n"
		"  -m, --mode             Set the mode to use for the image.\n"
		"  -o, --output           Set the output to operate on or * for all.\n"
		"  -v, --version          Show the version number and quit.\n"
		"\n"
		"Background Modes:\n"
		"  stretch, fit, fill, center, tile, or solid_color\n";

	struct swaybg_output_config *config = calloc(sizeof(struct swaybg_output_config), 1);
	config->output = strdup("*");
	config->mode = BACKGROUND_MODE_INVALID;
	wl_list_init(&config->link); // init for safe removal

	int c;
	while (1) {

		int option_index = 0;
		c = getopt_long(argc, argv, "c:hi:p:m:o:v", long_options, &option_index);
		if (c == -1) {
			break;
		}
		switch (c) {
		case 'c':  // color
			if (!is_valid_color(optarg)) {
				swaybg_log(LOG_ERROR, "Invalid color: %s", optarg);
				continue;
			}
			config->color = parse_color(optarg);
			break;
		case 'i':  // image
			free(config->image);
			config->image = load_background_image(optarg);
			if (!config->image) {
				swaybg_log(LOG_ERROR, "Failed to load image: %s", optarg);
			}
			break;
		case 'p':  // path
			state->seed = time(NULL) % 0x7f ;
			list_files (optarg , state);
			break;
		case 'm':  // mode
			config->mode = parse_background_mode(optarg);
			if (config->mode == BACKGROUND_MODE_INVALID) {
				swaybg_log(LOG_ERROR, "Invalid mode: %s", optarg);
			}
			break;
		case 'o':  // output
			if (config && !store_swaybg_output_config(state, config)) {
				// Empty config or merged on top of an existing one
				destroy_swaybg_output_config(config);
			}
			config = calloc(sizeof(struct swaybg_output_config), 1);
			config->output = strdup(optarg);
			config->mode = BACKGROUND_MODE_INVALID;
			wl_list_init(&config->link);  // init for safe removal
			break;
		case 'v':  // version
			fprintf(stdout, "swaybg version " SWAYBG_VERSION "\n");
			exit(EXIT_SUCCESS);
			break;
		default:
			fprintf(c == 'h' ? stdout : stderr, "%s", usage);
			exit(c == 'h' ? EXIT_SUCCESS : EXIT_FAILURE);
		}
	}

	if (config && !store_swaybg_output_config(state, config))
  {
		// Empty config or merged on top of an existing one
		destroy_swaybg_output_config(config);
	}

	// Check for invalid options
	if (optind < argc) {
		config = NULL;
		struct swaybg_output_config *tmp = NULL;
		wl_list_for_each_safe(config, tmp, &state->configs, link) {
			destroy_swaybg_output_config(config);
		}
		// continue into empty list
	}
	if (wl_list_empty(&state->configs)) {
		fprintf(stderr, "%s", usage);
		exit(EXIT_FAILURE);
	}

	// Set default mode and remove empties
	config = NULL;
	struct swaybg_output_config *tmp = NULL;
	wl_list_for_each_safe(config, tmp, &state->configs, link) {
		if (!config->image && !config->color) {
			destroy_swaybg_output_config(config);
		} else if (config->mode == BACKGROUND_MODE_INVALID) {
			config->mode = config->image
				? BACKGROUND_MODE_STRETCH
				: BACKGROUND_MODE_SOLID_COLOR;
		}
	}
}

int main(int argc, char **argv) {
	swaybg_log_init(LOG_DEBUG);

	struct swaybg_state state = {0};
	wl_list_init(&state.configs);
	wl_list_init(&state.outputs);

	parse_command_line(argc, argv, &state);

	state.display = wl_display_connect(NULL);
	if (!state.display) {
		swaybg_log(LOG_ERROR, "Unable to connect to the compositor. "
				"If your compositor is running, check or set the "
				"WAYLAND_DISPLAY environment variable.");
		return 1;
	}

  struct wl_registry * registry = setup_registry ( &state) ;
  if (!registry) return 1;

#if 0
	state.run_display = true;
	while (wl_display_dispatch(state.display) != -1 && state.run_display) {
		// This space intentionally left blank
	}
#endif
  run_event_loop (&state);

  destroy_all_swaybg_output (&state);
  destroy_all_swaybg_output_config (&state);

	return 0;
}
