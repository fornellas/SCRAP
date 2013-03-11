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

/** @defgroup enc_video Video encoding
 *  @{
 */

/** @file enc_video.h
 *  @brief Video encoding functions
 *
 *  These functions will generate an AVI file, with a XviD
 *  compressed video.
 */

/** @file enc_video.c
 *  @brief Video encoding functions implementaion
 *
 *  Implementation of the functions described at enc_video.h.
 */

#ifndef ENC_VIDEO_H
#define ENC_VIDEO_H
#include "common.h"

/** @brief Initialize video encoding functions
 *
 *  Call this function before calling any other function of this module.
 *  If error is reported, then a string describing the error message will be
 *  allocated at errstr using malloc(). User must free() errstr after using it.
 *  @return Zero if OK, non zero if error.
 */
int evid_init(char **errmsg);

/** @brief Video encoding handler
 */
typedef void evid_handler_t;

/** @brief XviD parameters
 *
 *  Parameters for XviD encoder.
 */
typedef struct {
  /** @brief Agorithm quality [0-6] */
  int quality;
  /** @brief Constant quantizer [1-31] */
  int quant;
  /** @brief Greyscale encoding */
  int gray;
} evid_xvidpar_t;

/** @brief Set default XviD parameters
 */
void evid_xvidpar_set_defaults(evid_xvidpar_t *evid_xvidpar);

/** @brief Create a new video encoding handler
 *
 *  Create & initialize a new video encoding handler. This will create an AVI
 *  file named filename, and initialize the XviD encoder.
 *  If NULL is returned, then a string describing the error message will be
 *  allocated at errstr using malloc(). User must free() errstr after using it.
 *  @param frame_par Frame parameters.
 *  @param evid_xvidpar XviD encoding parameters.
 *  @param framerate Video framerate.
 *  @param filename Filename to record AVI to.
 *  @param errstr String describing the error.
 *  @return Adress of the new handler, or NULL if error.
 */
evid_handler_t *evid_create_handler(frame_par_t *frame_par, evid_xvidpar_t *evid_xvidpar, double framerate, char *filename, char **errstr);

/** @brief Encode a frame
 *
 *  Encode frame.
 *  @param evid_handler Video encoding handler.
 *  @param frame Frame data to encode. Colorspace must be YUYV.
 *  @return Zero if OK, 1 if AVI is too long to store one more frame, 2 if
 *  error.
 */
int evid_enc_frame(evid_handler_t *evid_handler, pixel_t *frame);

/** @brief Return file name
 *
 *  Return the file name linked to handler.
 *  @return Pointer to a malloced string containing the file name.
 */
char *evid_filename(evid_handler_t *evid_handler);

/** @brief Destroy video encoding handler
 *
 *  Finish recording, closing AVI stream and destroying handler.
 *  @param evid_handler Video encoding handler.
 *  @param errstr String describing error.
 *  @return Zero if OK, non zero if error, storing at errstr a pointer to a
 *  string describing error. User must free() it after use.
 */
int evid_destroy_handler(evid_handler_t *evid_handler, char **errstr);

/** @brief Return string describing error
 *
 *  If a function of this module returns an error, then you can get a pointer
 *  to a string describing the error. Note that it must be called before
 *  calling any other function of this module, for it to return the correct
 *  error string.
 *  @param evid_handler Video encoding handler.
 *  @return Pointer to a string describing error code
 */
char *evid_strerror(evid_handler_t *evid_handler);

/** @}
 */
#endif
