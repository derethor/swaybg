#include <assert.h>
#include <sys/epoll.h>
#include "state.h"
#include "output_config.h"
#include "log.h"
#include "timer.h"

static int displayfd_init(int epfd, struct wl_display *display)
{
  int wfd = wl_display_get_fd(display);
  if (wfd <= 0)
    return -1;

  // ADD EPOLL
  struct epoll_event ev;
  memset(&ev, 0x0, sizeof(struct epoll_event));
  ev.events = EPOLLIN;
  ev.data.fd = wfd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, wfd, &ev) == -1)
    return -1;
  return 0;
}

#if 0
	state.run_display = true;
	while (wl_display_dispatch(state.display) != -1 && state.run_display) {
		// This space intentionally left blank
	}
#endif

int setup_event_loop(struct swaybg_state *state)
{
  assert(state != NULL);

  // todo: delete previus fd
  state->epfd = epoll_create1(EPOLL_CLOEXEC);
  if (state->epfd == -1)
  {
    swaybg_log(LOG_ERROR, "error creating epoll");
    return -1;
  }

  // connect wayland fd
  if (displayfd_init(state->epfd, state->display) != 0)
  {
    swaybg_log(LOG_ERROR, "error display fd init");
    return -1;
  }

  return 0;
}

int setup_output_event(struct swaybg_output *output)
{
  assert(output);
  assert(output->state != NULL);
  assert(output->config != NULL);

  if (output->config->seconds <= 0)
    return 0;

  output->tfd = -1;

  if (timer_init(output->state->epfd, &(output->tfd)) != 0)
  {
    swaybg_log(LOG_ERROR, "error timer init");
    return -1;
  }

  if (timer_set(output->tfd, output->config->seconds * 1000) != 0)
  {
    swaybg_log(LOG_ERROR, "error timer set");
    return -1;
  }

  return 1;
}

static int check_timer_event(const struct epoll_event *event, struct swaybg_state *state)
{
  struct swaybg_output *output;
  struct swaybg_output *tmp_output;

  assert(event != NULL);
  assert(state != NULL);

  wl_list_for_each_safe(output, tmp_output, &(state->outputs), link)
  {
    if (event->data.fd == output->tfd)
    {
      timer_cb(event->events, output, state);
      return 1;
    }
  }

  return 0;
}

int run_event_loop(struct swaybg_state *state)
{
  int err;
  int idx;
  struct epoll_event events[32];

  assert(state != NULL);

  state->run_display = true;

  while (state->run_display)
  {
    // block other threads before polling
    while (wl_display_prepare_read(state->display) != 0)
      wl_display_dispatch_pending(state->display);
    wl_display_flush(state->display);

    err = epoll_wait(state->epfd, events, sizeof(events) / sizeof(struct epoll_event), -1);

    if (err == -1)
    {
      wl_display_cancel_read (state->display);
      if (errno == EINTR)
      {
        swaybg_log(LOG_DEBUG, "wait interrupted");
        continue;
      }
      else
        return -1;
    }

    // release other threads after pooling
    assert ( wl_display_read_events(state->display) == 0 );
    wl_display_dispatch_pending(state->display);
    wl_display_flush(state->display);

    if (err == 0)
    {
      // epoll timeout
    }
    else
      for (idx = 0; idx < err; ++idx)
      {
        if (check_timer_event(&events[idx], state) != 0)
          continue;
      }

  }

  return 0;
}
