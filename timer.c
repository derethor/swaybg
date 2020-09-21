# include <assert.h>
# include <sys/epoll.h>
# include <sys/timerfd.h>
# include "swaybg.h"
# include "log.h"
# include "output.h"
# include "setup.h"
# include "path.h"

void timer_cb(int tfd , int revents , struct swaybg_state * state )
{
  uint64_t count = 0;
  ssize_t err = 0;

  assert (state!=NULL);

  err = read(tfd, &count, sizeof(uint64_t));
  if(err != sizeof(uint64_t))
  {
    swaybg_log ( LOG_ERROR , "error reading timer events");
    return;
  }

  //do timeout

  struct swaybg_output *output = NULL;
  struct swaybg_output *tmp_output = NULL;

  wl_list_for_each_safe(output, tmp_output, &(state->outputs), link)
  {
    if (!output) continue;
    if (setup_next_image ( state, output->config) ) render_frame(output);
  }
}

int timer_set ( int tfd , long ms )
{
  struct itimerspec its;

  its.it_interval.tv_sec =  ms / 1000;
  its.it_interval.tv_nsec = 0;

  its.it_value.tv_sec = ms / 1000;
  its.it_value.tv_nsec = 0;

  return timerfd_settime(tfd, /*flags=*/0, &its, NULL);
}

int timer_init (int epfd , int * tfd )
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
