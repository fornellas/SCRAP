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

/** @defgroup v4l video4linux capture interface
 *  @{
 */

/** @file v4l.h
 *  @brief video4linux interface functions
 *
 *  This file contain capture functions to work with a video4linux device.
 */

/** @file v4l.c
 *  @brief video4linux interface functions
 *
 *  Implementation of the functions described at v4l.h.
 */

#ifndef V4L_H
#define V4L_H
#include "common.h"
#include <linux/types.h>

/** @brief Print v4l capabilities
 *
 *  Print to stout all information avaliable about device, whose path is
 *  dev_path, that the v4l driver can tell.
 *  @param v4l_dev v4l device path.
 *  @return Return 0 if device was opened and information readed, or non 0
 *  if fails.
 */
int v4l_print_cap(char *v4l_dev);

/** @brief Set default v4l device
 *
 *  Store a pointer to a static string containing the default device in v4l_dev
 */
void v4l_dev_set_default(char **v4l_dev);

/** @brief v4l handler
 */
typedef void v4l_handler_t;

/** @brief Create a new v4l handler
 *
 *  Create a new v4l handler. If out of memory, exit() is called.
 *  @param v4l_dev v4l device (e.g. /dev/video).
 *  @return Adress of the new handler.
 */
v4l_handler_t *v4l_create_handler(char *v4l_dev);

/** @brief v4l camera setup identifier
 */
typedef int v4l_cam_ident_t;

/** @brief Camera setup
 */
typedef struct {
  // capture frame
  int width;
  int height;
  // capture window
  int window_x;
  int window_y;
  frame_par_t frame_par;
  // video source
  int channel;
  __u16 norm;
  // tuner
  int tuner;
  __u16 mode;
  unsigned long freq;
  // color/bw
  int gray;
  // image proprieties
  __u16 brightness;
  __u16 hue;
  __u16 colour;
  __u16 contrast;
  __u16 whiteness; // grayscale only
  int palette;
  // capture stuff
  char name[MAX_VALUE_LENGTH];
} v4l_cam_par_t;

/** @brief Set default camera setup
 */
void v4l_cam_set_default(v4l_cam_par_t *v4l_cam_par);

/** @brief Add new camera setup
 *
 *  Adds a new camera setup to handler. If out of memory, exit() is called.
 *  @param v4l_handler v4l handler.
 *  @param v4l_cam_ident Camera setup identifier.
 *  @param v4l_cam_par Camera setup
 */
void v4l_add_cam(v4l_handler_t *v4l_handler, v4l_cam_ident_t *v4l_cam_ident, v4l_cam_par_t *v4l_cam_par);

/** @brief Initialize v4l device
 *
 *  Initialize v4l device. Should also check for invalid parameters in handler.
 *  This function MUST be called first and once, before the call of any other
 *  functions below.
 *  @param v4l_handler v4l handler.
 *  @return Return 0 if success and non 0 if error.
 */
int v4l_init_dev(v4l_handler_t *v4l_handler);

/** @brief Start frame capture
 *
 *  Start the capture of the first frame using camera setup v4l_cam_ident. MUST
 *  be called only *once* and *before* v4l_sync_frame().
 *  @param v4l_handler v4l handler.
 *  @param v4l_cam_ident Camera setup identifier.
 *  @return Return 0 if success and non 0 if error.
 */
int v4l_start_cap(v4l_handler_t *v4l_handler, v4l_cam_ident_t v4l_cam_ident);

/** @brief Sync frame
 *
 *  Syncronize thread to capturing frame and also starts capture for the next
 *  frame. Captured frame is stored at frame.
 *  @param v4l_handler v4l handler.
 *  @param v4l_cam_ident_next Camera setup identifier to start next capture.
 *  @param frame Pointer to where to store captured frame data. If NULL, no
 *  no data stored, only sync frame.
 *  @return Return 0 if success and non 0 if error.
 */
int v4l_sync_frame(v4l_handler_t *v4l_handler, v4l_cam_ident_t v4l_cam_ident_next, pixel_t *frame);

/** @brief Destroy handler
 *
 *  Close the v4l device (opened with v4l_init_dev()) and destroy its handler.
 *  @param v4l_handler v4l handler.
 *  @param v4l_cam_ident Camera setup identifier.
 *  @return Return 0 if success and non 0 if error.
 */
int v4l_destroy_handler(v4l_handler_t *v4l_handler);

/** @brief Return string describing error
 *
 *  If a function of this module returns an error, then you can get a pointer
 *  to a string describing the error. Note that it must be called before
 *  calling any other function of this module, for it to return the correct
 *  error string.
 *  @param v4l_handler v4l handler.
 *  @return Pointer to a string describing error code.
 */
char *v4l_strerror(v4l_handler_t *v4l_handler);

/** @}
 */
#endif
