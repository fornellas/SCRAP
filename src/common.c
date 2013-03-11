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

#include "common.h"
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void *x_calloc(size_t nmemb, size_t size) {
  void *ptr;
  errno=0;
  if((ptr=calloc(nmemb, size))==NULL)
    error(1, errno, "Unable to allocate memory");
  return ptr;
}

void *x_malloc(size_t size) {
  void *ptr;
  errno=0;
  if((ptr=malloc(size))==NULL)
    error(1, errno, "Unable to allocate memory");
  return ptr;
}

void *x_realloc(void *ptr, size_t size) {
  errno=0;
  if((ptr=realloc(ptr, size))==NULL)
    error(1, errno, "Unable to allocate memory");
  return ptr;
}

int x_asprintf(char **strp, const char *fmt, ...) {
  va_list ap;
  int r;

  va_start(ap, fmt);
  errno=0;
  r=vasprintf(strp, fmt, ap);
  if(r==-1)
    error(1, errno, "Unable to allocate memory or other error calling vasprintf");
  va_end(ap);
  return r;
}

char *x_strdup(const char *s) {
  char *str;

  errno=0;
  if(NULL==(str=strdup(s)))
    error(1, errno, "Unable to allocate memory");
  return str;
}
