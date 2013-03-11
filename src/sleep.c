/* (C)2004 Fabio Puglise Ornellas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "sleep.h"
#include "common.h"
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#define MAX_ERROR_MSG_LENGTH 256

typedef struct {
  struct timeval period;
  struct timeval last_call_time;
  char *errstr;
  char errbuf[MAX_ERROR_MSG_LENGTH];
} handler_t;

sleep_handler_t *sleep_create_handler(double freq, char **errstr) {
  handler_t *handler;

  handler=(handler_t *)x_malloc(sizeof(handler_t));

  handler->period.tv_sec=(long)(1/freq);
  handler->period.tv_usec=(long)((long)((1/freq)*1e6)-(long)(1/freq)*1e6);

  handler->errstr=NULL;

  errno=0;
  if(gettimeofday(&handler->last_call_time, NULL)) {
    x_asprintf(errstr, "Error calling gettimeofday(): %s\n", strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
    free(handler);
    return NULL;
  }

  return (sleep_handler_t *)handler;
}

// Substract the time x from y, and store the result in result.
// Return 1 if x<y, 0 if x>=y
int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y) {
  if(x->tv_sec<y->tv_sec||(x->tv_sec==y->tv_sec&&x->tv_usec<y->tv_usec)) {
    timeval_subtract(result, y, x);
    return 1;
  }
  if(x->tv_usec<y->tv_usec) {
    result->tv_sec=x->tv_sec-y->tv_sec-1;
    result->tv_usec=(long)(x->tv_usec-y->tv_usec+1e6);
  } else {
    result->tv_sec=x->tv_sec-y->tv_sec;
    result->tv_usec=(x->tv_usec-y->tv_usec);
  }
  return 0;
}

int sleep_period(sleep_handler_t *sleep_handler, double *freq) {
  handler_t *handler=(handler_t *)sleep_handler;
  struct timeval current_time;
  struct timeval diff;
  struct timeval  sleep_time_tv;
  struct timespec sleep_time_ts;

  // lets take current time
  if(gettimeofday(&current_time, NULL)) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Error calling gettimeofday(): %s\n", strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
    return 2;
  }
  // lets calculate diff
  if(timeval_subtract(&diff, &current_time, &handler->last_call_time)) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Your system clock is running  backwards!\n");
    handler->last_call_time.tv_sec=current_time.tv_sec;
    handler->last_call_time.tv_usec=current_time.tv_usec;
    return 2;
  }
  // and sleep time
  if(timeval_subtract(&sleep_time_tv, &handler->period, &diff)) {
    double period;
    // negative, we've got a delay; lets calculate it
    period=(double)diff.tv_sec;
    period+=((double)diff.tv_usec)/1e6;
    *freq=1/period;
    if(gettimeofday(&handler->last_call_time, NULL)) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Error calling gettimeofday(): %s\n", strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
      free(handler);
      return 2;
    }
    return 1;
  } else {
    // here, just sleep
    sleep_time_ts.tv_sec=sleep_time_tv.tv_sec;
    sleep_time_ts.tv_nsec=(long)(sleep_time_tv.tv_usec*1e3);
    errno=0;
    if(nanosleep(&sleep_time_ts, NULL)) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Error calling nanosleep(): %s\n", strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
    }
    if(gettimeofday(&handler->last_call_time, NULL)) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Error calling gettimeofday(): %s\n", strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
      free(handler);
      return 2;
    }
  }
  return 0;
}

int sleep_destroy_handler(sleep_handler_t *sleep_handler) {
  return 1;
}

char *sleep_strerror(sleep_handler_t *sleep_handler) {
  handler_t *handler=(handler_t *)sleep_handler;
  return handler->errstr;
}
