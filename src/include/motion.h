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

/** @defgroup motion Motion detection
 *  @{
 */

/** @file motion.h
 *  @brief Motion detection function
 *
 *  Function to detect motion.
 */

/** @file motion.c
 *  @brief Motion detection function implementation
 *
 *  Implementatio of the functions described at motion.h.
 */

#ifndef MOTION_H
#define MOTION_H

#include "common.h"
#include <time.h>

/** @brief Motion detection parameters
 */
typedef struct {
  /** @brief Motion detection algorithm
   *  0=none
   *  1=difference
   *  2=derivate difference
   */
  int algorithm;
  /** @brief Process only half frame */
  int half_image;
  /** @brief Full derivate difference algorithm */
  int deriv_diff_full;
  /** @brief Area changed threshold [0-100]% */
  double threshold;
  /** @brief Image noise level [0-255] */
  pixel_t noise;
  /** @brief Force comparation frame obsolete after this time */
  time_t force_obsolete_sec;
  /** @brief Calibrate
   *  0=normal
   *  1=set area_changed and fill frame_calibrate with only pixels changed
   *  2=set area_changed and fill frame_calibrate with difference frame
   */
  int calibrate;
} motion_par_t;

/** @brief Set default motion detection parameters
 *  @param motion_par Pointer to structure to fill.
 */
void motion_par_set_defaults(motion_par_t *motion_par);

/** @brief Motion handler
 */
typedef void motion_handler_t;

/** @brief Create a new motion detection handler
 *
 *  Create & initialize a new motion detection handler.
 *  @param motion_par Motion detection parameters.
 *  @param frame_par Frame parameters.
 *  @return Adress of the new handler.
 */
motion_handler_t *motion_create_handler(motion_par_t *motion_par, frame_par_t *frame_par);

/** @brief Detects motion
 *
 *  This function reads data of frame and detects motion. If handler was
 *  initialized with motion_par->calibrate set, then frame_calibrate will be
 *  filled with requested frame; other case, remain untouched. Frames must come
 *  in YUYV format.
 *  In case of no motion algorithm set, this function will always return 1, and
 *  no parameters are readed or written, except motion_handler.
 *
 *  @param motion_handler Motion detection handler.
 *  @param frame Current frame.
 *  @param cap_time Current frame capture time.
 *  @param area_changed If non NULL, filled with percent of area changed.
 *  @param frame_calibrate Calibration frame.
 *  @return True if motion detected. The first call of this function, will
 *  always return 0 if motion detection is set.
 */
int motion_detect(motion_handler_t *motion_handler, pixel_t *frame, time_t cap_time, double *area_changed, pixel_t *frame_calibrate);

/** @brief Destroy motion detection handler
 *
 *  Destroy a motion detection handler.
 *  @param motion_handler Motion detection handler.
 *  @return Zero if OK.
 */
int motion_destroy_handler(motion_handler_t *);

/** @}
 */
#endif
