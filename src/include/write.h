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

/** @defgroup write Write over frame
 *  @{
 */

/** @file write.h
 *  @brief Write text over frame functions
 *
 *  This file contain a function witch write given text over a frame.
 */

/** @file write.c
 *  @brief Write text over frame functions implementations
 *
 *  Implementation of the function described in write.h.
 */

#ifndef WRITE_H
#define WRITE_H
#include "common.h"

/** @brief Writing parameters
 */
typedef struct {
  /** @brief Font to use [1|2] */
  int font;
  /** @brief Font transparency % */
  int transparency;
} write_par_t;

/** @brief Set default write detection parameters
 */
void write_par_set_defaults(write_par_t *write_par);

/** @brief Motion handler
 */
typedef void write_handler_t;

/** @brief Create a new write handler
 *
 *  Create & initialize a new writing handler.
 *  @param write_par Writing parameters.
 *  @param frame_par Frame parameters.
 *  @return Adress of the new handler.
 */
write_handler_t *write_create_handler(write_par_t *write_par, frame_par_t *frame_par);

/** @brief Write text over frame
 *
 *  Write text over frame. Not all chars and frame size are allowed.
 *  @param write_handler Writing handler.
 *  @param frame Pointer to where to write text.
 *  @param text Text to write.
 *  @return Zero if OK, non zero if error. Errors include invalid chars in
 *  string and frame size to small to write all the text.
 */
int write_frame(write_handler_t *write_handler, pixel_t *frame, const char *text);

/** @brief Return string describing error
 *
 *  If a function of this module returns an error, then you can get a pointer
 *  to a string describing the error. Note that it must be called before
 *  calling any other function of this module, for it to return the correct
 *  error string.
 *  @param write_handler Writing handler.
 *  @return Pointer to a string describing error code.
 */
char *write_strerror(write_handler_t *write_handler);

/** @}
 */
#endif
