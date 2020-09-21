# include <sys/epoll.h>
# include <sys/timerfd.h>
# include "swaybg.h"
# include "output.h"

void timer_cb(int tfd , int revents , struct swaybg_state * state )
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
