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

/** @defgroup Common Common
 *  @brief Common data and functions
 *
 *  Common data types, macros and definitions, used by other modules.
 *  @{
 */

/** @file common.h
 *  @brief Common definitions/functions to whole program
 *
 *  Defines and macros and functions used everywhere.
 */

/** @file common.c
 *  @brief Common functions implementations
 *
 *  Implementatios of the functions described in common.h
 */

#ifndef COMMON_H
#define COMMON_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_GNU
#define __USE_GNU
#endif

#include "config.h"
#include "authors.h"
#include <stdlib.h>

/** @brief Frame information
 */
typedef struct {
  int width;
  int height;
  int size;
} frame_par_t;

/** @brief Frame data
 */
typedef unsigned char pixel_t;

/** @brief Maximum variable name length
 */
#ifndef MAX_NAME_LENGTH
#define MAX_NAME_LENGTH 128
#endif

/** @brief Maximum variable value length
 */
#ifndef MAX_VALUE_LENGTH
#define MAX_VALUE_LENGTH 256
#endif

/** @brief Palette depth
 *
 *  In the whole software, we only work with a YUYV palette.
 */
#define PALETTE_DEPTH 2

/** @brief Smart calloc()
 *
 *  Similar to calloc, but call error() if error.
 */
void *x_calloc(size_t nmemb, size_t size);

/** @brief Smart malloc()
 *
 *  Similar to malloc, but call error() if error.
 */
void *x_malloc(size_t size);

/** @brief Smart realloc()
 *
 *  Similar to realloc, but call error() if error.
 */
void *x_realloc(void *ptr, size_t size);

/** @brief Smart asprintf()
 *
 *  Similar to asprintf, but call error() asprintf return -1, and call
 *  free(*strp) before all.
 */
int x_asprintf(char **strp, const char *fmt, ...);

/** @brief Smart strdup()
 *
 *  Smart strdup
 */
char *x_strdup(const char *s);



/** @}
 */
#endif
