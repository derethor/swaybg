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
# include <glob.h>
# include <errno.h>
# include <stdio.h>
# include <unistd.h>
# include <memory.h>
# include <sys/epoll.h>
# include <sys/timerfd.h>

# include "swaybg.h"
# include "setup.h"
# include "color.h"
# include "output.h"
# include "registry.h"

#include "background-image.h"
#include "cairo.h"
#include "log.h"
#include "pool-buffer.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

# define LIST_GLOB_FLAGS GLOB_NOSORT

void list_files (const char * path , struct swaybg_state * state )
{

  glob_t  globlist;
  int     c;

  if (!path) return; // ERROR
  if (!state) return; // ERROR

	if (glob( path, LIST_GLOB_FLAGS , NULL, &globlist) == GLOB_NOSPACE || glob( path, GLOB_PERIOD, NULL, &globlist) == GLOB_NOMATCH)
		return ; // (FAILURE);

	if (glob( path, LIST_GLOB_FLAGS , NULL, &globlist) == GLOB_ABORTED)
		return ; // (ERROR);

  c = 0;

	while (globlist.gl_pathv[c]) c ++;


	uint32_t value = (( state->seed * 1103515245) + 12345) & 0x7fffffff;
	state->seed = value;
	const uint32_t e =	(uint32_t) ((  ((float ) (value) ) / 2147483646.0f ) * ( (float) c ));

	printf("%s\n", globlist.gl_pathv[e]);
}

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

///////////////////////////////////////////

static void timer_cb(int tfd , int revents , struct swaybg_state * state )
{
  uint64_t count = 0;
  ssize_t err = 0;

  err = read(tfd, &count, sizeof(uint64_t));
  if(err != sizeof(uint64_t)) return;

  //do timeout
  if (!state) return;
  struct swaybg_output *output = NULL;
  struct swaybg_output *tmp_output = NULL;
  wl_list_for_each_safe(output, tmp_output, &(state->outputs), link)
  {
    if (output && output->config && output->config->color)
    {
      output->config->color = (output->config->color) + 0x1100;
      swaybg_log ( LOG_ERROR , "color %x\n" , output->config->color );
      swaybg_log ( LOG_ERROR , "name %s\n" , output->name );
      render_frame(output);
    }
  }
}

static int timer_set ( int tfd , long ms )
{
  struct itimerspec its;

  its.it_interval.tv_sec =  ms / 1000;
  its.it_interval.tv_nsec = 0;

  its.it_value.tv_sec = ms / 1000;
  its.it_value.tv_nsec = 0;

  return timerfd_settime(tfd, /*flags=*/0, &its, NULL);
}

static int timer_init (int epfd , int * tfd )
{
  if (!tfd) return -1;

  *tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if(*tfd == -1) return -1;

  struct epoll_event ev;
  memset (&ev , 0x0, sizeof (struct epoll_event) );
  ev.events = EPOLLIN;
  ev.data.fd = *tfd;
  if ( epoll_ctl(epfd, EPOLL_CTL_ADD, *tfd, &ev) == -1 ) return -1;

  return 0;
}

static int displayfd_init (int epfd , struct wl_display * display )
{
  int wfd = wl_display_get_fd (display);
  if (wfd<=0) return -1;

  // ADD EPOLL
  struct epoll_event ev;
  memset (&ev , 0x0, sizeof (struct epoll_event) );
  ev.events = EPOLLIN;
  ev.data.fd = wfd;
  if ( epoll_ctl(epfd, EPOLL_CTL_ADD, wfd, &ev) == -1 ) return -1;
  return 0;
}

static int run_event_loop ( struct swaybg_state * state )
{
  int epfd = 0;
  epfd = epoll_create1(EPOLL_CLOEXEC);
  if(epfd == -1)
  {
    swaybg_log ( LOG_ERROR , "error create epoll");
    return -1;
  }

  int tfd = -1;

  if ( timer_init (epfd , &tfd ) != 0 )
  {
    swaybg_log ( LOG_ERROR , "error timer init");
    return -1;
  }

  if ( timer_set ( tfd , 1000 ) != 0 )
  {
    swaybg_log ( LOG_ERROR , "error timer set");
    return -1;
  }

  if ( displayfd_init (epfd , state->display ) != 0)
  {
    swaybg_log ( LOG_ERROR , "error display fd init");
    return -1;
  }

  int err;
  int idx;
  struct epoll_event events[16];

	state->run_display = true;
	while ( state->run_display)
  {
    // block other threads before polling
    while (wl_display_prepare_read(state->display) != 0)
      wl_display_dispatch_pending(state->display);
    wl_display_flush ( state->display );

    err = epoll_wait(epfd, events, sizeof(events)/sizeof(struct epoll_event) , -1 );

    if(err == -1)
    {
      if(errno == EINTR) { swaybg_log( LOG_DEBUG , "wait interrupted"); continue; }
      else return -1;
    }

    if ( err == 0)
    {
      // epoll timeout
    }
    else for(idx = 0; idx < err; ++idx)
    {
      if(events[idx].data.fd == tfd)
        timer_cb(tfd , events[idx].events , state );
    }

    // release other threads after pooling
    wl_display_read_events(state->display);
    wl_display_dispatch_pending(state->display);
  }

  return 0;
}

///////////////////////////////////////////

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

	struct swaybg_output *output;
	struct swaybg_output *tmp_output;
	wl_list_for_each_safe(output, tmp_output, &state.outputs, link) {
		destroy_swaybg_output(output);
	}

	struct swaybg_output_config *config = NULL, *tmp_config = NULL;
	wl_list_for_each_safe(config, tmp_config, &state.configs, link) {
		destroy_swaybg_output_config(config);
	}

	return 0;
}
