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

/** @defgroup sleep Sleep function
 *  @{
 */

/** @file sleep.h
 *  @brief Sleep function definition
 *
 *  This file contain a function to sleep 1/FPS (used by capture thread).
 */

/** @file sleep.c
 *  @brief Sleep function implementaion
 *
 *  Implementation of the functions described at sleep.h.
 */

#ifndef SLEEP_H
#define SLEEP_H

/** @brief Sleep handler
 */
typedef void sleep_handler_t;

/** @brief Create a new sleep handler
 *
 *  Create & initialize a new sleep handler.
 *  If NULL is returned, then a string describing the error message will be
 *  allocated at errstr using malloc(). User must free() errstr after using it.
 *  @param freq Frequency of sleep cycle.
 *  @param errstr String describing the error.
 *  @return Adress of the new handler, or NULL if error.
 */
sleep_handler_t *sleep_create_handler(double freq, char **errstr);

/** @brief Delay for 1/freq
 *
 *  Hold thread execution for 1/freq, counting time starting from last call of
 *  sleep_create_handler() or self.
 *  @param freq Filled with achieved frequency, if returns 1.
 *  @return Return 0 if success;
 *  1 if we had a delay, storing achieved frequency at freq;
 *  2 if error.
 */
int sleep_period(sleep_handler_t *sleep_handler, double *freq);

/** @brief Destroy sleep handler
 *
 *  Destroy sleep handler.
 *  @param sleep_handler Sleep handler.
 *  @return Zero if OK, or error if not.
 */
int sleep_destroy_handler(sleep_handler_t *sleep_handler);

/** @brief Return string describing error
 *
 *  If a function of this module returns an error, then you can get a pointer
 *  to a string describing the error. Note that it must be called before
 *  calling any other function of this module, for it to return the correct
 *  error string.
 *  @param sleep_handler Sleep handler.
 *  @return Pointer to a string describing error code.
 */
char *sleep_strerror(sleep_handler_t *sleep_handler);

/** @}
 */
#endif
