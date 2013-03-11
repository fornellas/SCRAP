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

#include "log.h"
#include "common.h"
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <error.h>

typedef struct {
  int level;
  FILE *stream;
  int motion;
} handler_t;

log_handler_t *log_create_handler(int verbose_level, int motion, const char *path, char **errstr) {
  handler_t *handler;
  char buffer[256];

  // allocate handler
  handler=(handler_t *)x_malloc(sizeof(handler_t));

  // set level
  handler->level=verbose_level;

  // motion
  handler->motion=motion;

  // open stream
  if(path==NULL)
    handler->stream=stdout;
  else {
    errno=0;
    handler->stream=fopen(path, "a");
    if(handler->stream==NULL) {
      x_asprintf(errstr, "Error opening log file '%s': %s", path, strerror_r(errno, buffer, 256));
      return NULL;
    }
  }

  return (log_handler_t *)handler;
}

void log_printf(log_handler_t *log_handler, int msg_level, const char *fmt, ...) {
  handler_t *handler=(handler_t *)log_handler;
  char strtime[20];
  char *str=NULL;
  time_t t;
  va_list ap;
  char type[3];

  if(msg_level>handler->level||(msg_level==LOG_MOTION&&(handler->level<LOG_ACT&&!handler->motion)))
    return;

  time(&t);
  strftime(strtime, 20, "%Y.%m.%d %H:%M:%S", localtime(&t));
  va_start(ap, fmt);
  if(-1==vasprintf(&str, fmt, ap)) {
    fprintf(stderr, "Unable to allocate memory or other error calling vasprintf\n");
    exit(1);
  }
  va_end(ap);
  switch(msg_level) {
    case LOG_ACT:
      strcpy(type, "[A]");
      break;
    case LOG_WARN:
      strcpy(type, "[W]");
      break;
    case LOG_MOTION:
      strcpy(type, "[M]");
      break;
    case LOG_ERR:
      strcpy(type, "[E]");
      break;
  }
  fprintf(handler->stream, "%s %s %s\n", strtime, type, str);

  errno=0;
  if(fflush(handler->stream))
    error(1, errno, "Error flushing log stream: ");

  free(str);
}

int log_destroy_handler(log_handler_t *log_handler, char **errstr) {
  handler_t *handler=(handler_t *)log_handler;
  char buffer[256];

  errno=0;
  if(EOF==fclose(handler->stream)) {
    x_asprintf(errstr, "Error closing log file: %s", strerror_r(errno, buffer, 256));
    return 1;
  }

  free(handler);

  return 0;
}
