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

#ifndef CONF_H
#define CONF_H
#include "common.h"
#include "motion.h"
#include "v4l.h"
#include "write.h"
#include "enc_video.h"
#include "frame_queue.h"
#include <stdio.h>

/** @brief Camera settings/data structure
 *
 *  Holds configuration and data for each configured camera on device.
 */
typedef struct {
  // name
  char name[MAX_VALUE_LENGTH];
  // frame parameters
  frame_par_t frame_par;
  // v4l
  v4l_cam_par_t v4l_cam_par;
  v4l_cam_ident_t v4l_cam_ident;
  // motion
  motion_par_t motion_par;
  motion_handler_t *motion_handler;
  // queue
  int queue_max_frames;
  int frame_buffer_size;
  queue_ident_t queue_ident;
  // write
  write_par_t write_par;
  write_handler_t *write_handler;
  // jpeg
  int jpeg_quality;
  // video
  evid_xvidpar_t evid_xvidpar;
  evid_handler_t *evid_handler;
  time_t last_video_open;
  double avi_framerate;
  // raw video
  char dump_raw_video[MAX_VALUE_LENGTH];
  FILE *dump_raw_video_stream;
  // out dir
  char output_dir[MAX_VALUE_LENGTH];
  // archiving
  time_t split_record_sec;
  // days of archive
  int archive_days;
  // archive size
  int archive_size;
} camera_t;

/** @brief Device configuration
 *
 * Store capture device configuration.
 */
typedef struct {
  // framerate
  double framerate;
  // v4l
  char video_dev[MAX_VALUE_LENGTH];
  v4l_handler_t *v4l_handler;
  // cameras
  int camera_c;
  camera_t *camera_v;
  // queues
  queue_handler_t *queue;
  // configuration file path
  char *conf_path;
} device_t;

#endif
