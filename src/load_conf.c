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
#include "device.h"
#include "load_conf.h"
#include "read_conf.h"
#include "v4l.h"
#include "motion.h"
#include "frame_queue.h"
#include "write.h"
// #include "enc_jpeg.h"
#include "enc_video.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <linux/types.h>
#include <stdlib.h>
#include <error.h>

#if MAX_VALUE_LENGTH < 12
  #error MAX_VALUE_LENGTH too low
#endif

FILE *conf_file;

// safe exit
void load_config_return() {
  errno=0;
  if(fclose(conf_file)==EOF)
    fprintf(stderr, "Error closing file: %s\n", strerror(errno));
}

int load_config(char *file, device_t *device) {
  char sec_name[MAX_NAME_LENGTH]="\0", var_name[MAX_NAME_LENGTH]="\0", var_value[MAX_VALUE_LENGTH]="\0";
  int line_count=1, s, v, e, c;
  int split_record_min;
  camera_t config_cam_global;
  char *dev=NULL;

  // open file for reading
  errno=0;
  conf_file=fopen(file, "r");
  if(conf_file==NULL) {
    fprintf(stderr, "Error opening file '%s' for reading: %s\n", file, strerror(errno));
    return 1;
  }

  // "Zero" config
  device->framerate=2;
  v4l_dev_set_default(&dev);
  strcpy(device->video_dev, dev);
  device->camera_c=0;
  device->camera_v=NULL;
  v4l_cam_set_default(&config_cam_global.v4l_cam_par);
  motion_par_set_defaults(&config_cam_global.motion_par);
  queue_par_set_defaults(&config_cam_global.queue_max_frames);
  config_cam_global.frame_buffer_size=0;
  write_par_set_defaults(&config_cam_global.write_par);
//  ejpeg_quality_set_defaults(&config_cam_global.jpeg_quality);
  evid_xvidpar_set_defaults(&config_cam_global.evid_xvidpar);
  config_cam_global.avi_framerate=-1;
  strcpy(config_cam_global.output_dir, "./");
  config_cam_global.split_record_sec=7200;
  config_cam_global.archive_days=1;
  config_cam_global.archive_size=1024;

  // For each section
  while((s=read_section(conf_file, &line_count, sec_name))==0) {
    if(!strcmp( "Capture", sec_name)) {
      e=0;
      while(!(v=read_var(conf_file, &line_count, var_name, var_value))) {
        if(!strcmp(var_name, "video_dev")) {
          strcpy(device->video_dev, var_value);
        } else if(!strcmp(var_name, "framerate")) {
          if(store_var(&device->framerate, var_value, T_DOUBLE_LOW(0.001))) {
            e=1; break;}
        } else {
          fprintf(stderr, "Invalid variable '%s' in section [%s] at line %d.\n", var_name, sec_name, line_count-1);
          load_config_return();
          return 1;
        }
      }
      if(e) {
        fprintf(stderr, "Invalid value '%s' to '%s' at line %d in section [%s].\n", var_value, var_name, line_count-1, sec_name);
        load_config_return();
        return 1;
      }
    } else if(!strcmp("GlobalCamera", sec_name)) {
      e=0;
      if(device->camera_c) {
        fprintf(stderr, "Section [GlobalCamera] (line %d) should come before any [Camera] sections.\n", line_count);
        load_config_return();
        return 1;
      }
      while(!(v=read_var(conf_file, &line_count, var_name, var_value))) {
        if(!strcmp(var_name, "channel")) {
          if(store_var(&config_cam_global.v4l_cam_par.channel, var_value, T_INT_LOW(0))) {
            e=1; break;}
        } else if(!strcmp(var_name, "norm")) {
          if(store_var(&config_cam_global.v4l_cam_par.norm, var_value, T___U16_HIGH(3))) {
            e=1; break;}
        } else if(!strcmp(var_name, "tuner")) {
          if(store_var(&config_cam_global.v4l_cam_par.tuner, var_value, T_INT_LOW(-1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "mode")) {
          if(store_var(&config_cam_global.v4l_cam_par.mode, var_value, T___U16_HIGH(3))) {
            e=1; break;}
        } else if(!strcmp(var_name, "freq")) {
          if(store_var(&config_cam_global.v4l_cam_par.freq, var_value, T_UNSIGNED_LONG_LOW(0))) {
            e=1; break;}
        } else if(!strcmp(var_name, "width")) {
          if(store_var(&config_cam_global.v4l_cam_par.width, var_value, T_INT_LOW(1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "height")) {
          if(store_var(&config_cam_global.v4l_cam_par.height, var_value, T_INT_LOW(1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "window_x")) {
          if(store_var(&config_cam_global.v4l_cam_par.window_x, var_value, T_INT_LOW(0))) {
            e=1; break;}
        } else if(!strcmp(var_name, "window_y")) {
          if(store_var(&config_cam_global.v4l_cam_par.window_y, var_value, T_INT_LOW(0))) {
            e=1; break;}
        } else if(!strcmp(var_name, "window_width")) {
          if(store_var(&config_cam_global.v4l_cam_par.frame_par.width, var_value, T_INT_LOW(1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "window_height")) {
          if(store_var(&config_cam_global.v4l_cam_par.frame_par.height, var_value, T_INT_LOW(1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "gray")) {
          if(store_var(&config_cam_global.v4l_cam_par.gray, var_value, T_INT)) {
            e=1; break;}
        } else if(!strcmp(var_name, "brightness")) {
          if(store_var(&config_cam_global.v4l_cam_par.brightness, var_value, T___U16)) {
            e=1; break;}
        } else if(!strcmp(var_name, "hue")) {
          if(store_var(&config_cam_global.v4l_cam_par.hue, var_value, T___U16)) {
            e=1; break;}
        } else if(!strcmp(var_name, "colour")) {
          if(store_var(&config_cam_global.v4l_cam_par.colour, var_value, T___U16)) {
            e=1; break;}
        } else if(!strcmp(var_name, "contrast")) {
          if(store_var(&config_cam_global.v4l_cam_par.contrast, var_value, T___U16)) {
            e=1; break;}
        } else if(!strcmp(var_name, "whiteness")) {
          if(store_var(&config_cam_global.v4l_cam_par.whiteness, var_value, T___U16)) {
            e=1; break;}
        } else if(!strcmp(var_name, "motion_detection_algorithm")) {
          if(store_var(&config_cam_global.motion_par.algorithm, var_value, T_INT_BOTH(0, 2))) {
            e=1; break;}
        } else if(!strcmp(var_name, "half_image")) {
          if(store_var(&config_cam_global.motion_par.half_image, var_value, T_INT_BOTH(0, 1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "deriv_diff_full")) {
          if(store_var(&config_cam_global.motion_par.deriv_diff_full, var_value, T_INT_BOTH(0, 1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "force_obsolete_sec")) {
          int sec;
          if(store_var(&sec, var_value, T_INT_LOW(0))) {
            e=1; break;} else {
            config_cam_global.motion_par.force_obsolete_sec=(time_t)sec;}
        } else if(!strcmp(var_name, "threshold")) {
          if(store_var(&config_cam_global.motion_par.threshold, var_value, T_DOUBLE_BOTH(0, 100))) {
            e=1; break;}
        } else if(!strcmp(var_name, "noise")) {
          if(store_var(&config_cam_global.motion_par.noise, var_value, T_UNSIGNED_CHAR)) {
            e=1; break;}
        } else if(!strcmp(var_name, "calibrate")) {
          if(store_var(&config_cam_global.motion_par.calibrate, var_value, T_INT_BOTH(0,2))) {
            e=1; break;}
        } else if(!strcmp(var_name, "frame_buffer_size")) {
          if(store_var(&config_cam_global.frame_buffer_size, var_value, T_INT_LOW(1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "font")) {
          if(store_var(&config_cam_global.write_par.font, var_value, T_INT_BOTH(0, 2))) {
            e=1; break;}
        } else if(!strcmp(var_name, "font_transparency")) {
          if(store_var(&config_cam_global.write_par.transparency, var_value, T_INT_BOTH(0, 100))) {
            e=1; break;}
//        } else if(!strcmp(var_name, "jpeg_quality")) {
//          if(store_var(&config_cam_global.jpeg_quality, var_value, T_INT_BOTH(0, 100))) {
//            e=1; break;}
        } else if(!strcmp(var_name, "xvid_quality")) {
          if(store_var(&config_cam_global.evid_xvidpar.quality, var_value, T_INT_BOTH(0, 6))) {
            e=1; break;}
        } else if(!strcmp(var_name, "xvid_quant")) {
          if(store_var(&config_cam_global.evid_xvidpar.quant, var_value, T_INT_BOTH(1, 31))) {
            e=1; break;}
        } else if(!strcmp(var_name, "avi_framerate")) {
          if(store_var(&config_cam_global.avi_framerate, var_value, T_DOUBLE_LOW(0.001))) {
            e=1; break;}
        } else if(!strcmp(var_name, "output_dir")) {
          strcpy(config_cam_global.output_dir, var_value);
        } else if(!strcmp(var_name, "split_record_min")) {
          if(store_var(&split_record_min, var_value, T_INT_BOTH(1,240))) {
            e=1; break;}
            config_cam_global.split_record_sec=(time_t)(split_record_min*60);
        } else if(!strcmp(var_name, "archive_days")) {
          if(store_var(&config_cam_global.archive_days, var_value, T_INT_LOW(0))) {
            e=1; break;}
        } else if(!strcmp(var_name, "archive_size")) {
          if(store_var(&config_cam_global.archive_size, var_value, T_INT_LOW(1))) {
            e=1; break;}
        } else {
          fprintf(stderr, "Invalid variable '%s' in section [%s] at line %d.\n", var_name, sec_name, line_count-1);
          load_config_return();
          return 1;
        }
      }
      if(e) {
        fprintf(stderr, "Invalid value '%s' to '%s' at line %d in section [%s].\n", var_value, var_name, line_count-1, sec_name);
        load_config_return();
        return 1;
      }
    } else if(!strcmp("Camera", sec_name)) {
      e=0;
      device->camera_v=(camera_t *)x_realloc(device->camera_v, (size_t)(sizeof(camera_t)*(device->camera_c+1)));
      // set camera defaults, from global
      sprintf(device->camera_v[device->camera_c].name, "Camera #%d", device->camera_c);
      memcpy(&device->camera_v[device->camera_c].v4l_cam_par, &config_cam_global.v4l_cam_par, sizeof(v4l_cam_par_t));
      memcpy(&device->camera_v[device->camera_c].motion_par, &config_cam_global.motion_par, sizeof(motion_par_t));
      device->camera_v[device->camera_c].queue_max_frames=config_cam_global.queue_max_frames;
      device->camera_v[device->camera_c].frame_buffer_size=config_cam_global.frame_buffer_size;
      memcpy(&device->camera_v[device->camera_c].write_par, &config_cam_global.write_par, sizeof(write_par_t));
//      device->camera_v[device->camera_c].jpeg_quality=config_cam_global.jpeg_quality;
      memcpy(&device->camera_v[device->camera_c].evid_xvidpar, &config_cam_global.evid_xvidpar, sizeof(evid_xvidpar_t));
      device->camera_v[device->camera_c].avi_framerate=config_cam_global.avi_framerate;
      strcpy(device->camera_v[device->camera_c].output_dir, config_cam_global.output_dir);
      device->camera_v[device->camera_c].split_record_sec=config_cam_global.split_record_sec;
      device->camera_v[device->camera_c].archive_days=config_cam_global.archive_days;
      device->camera_v[device->camera_c].archive_size=config_cam_global.archive_size;
      device->camera_v[device->camera_c].dump_raw_video[0]='\0';
      while(!(v=read_var(conf_file, &line_count, var_name, var_value))) {
        if(!strcmp(var_name, "name")) {
          strcpy(device->camera_v[device->camera_c].name, var_value);
        } else if(!strcmp(var_name, "channel")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.channel, var_value, T_INT_LOW(0))) {
            e=1; break;}
        } else if(!strcmp(var_name, "norm")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.norm, var_value, T___U16_HIGH(3))) {
            e=1; break;}
        } else if(!strcmp(var_name, "tuner")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.tuner, var_value, T_INT_LOW(-1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "mode")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.mode, var_value, T___U16_HIGH(3))) {
            e=1; break;}
        } else if(!strcmp(var_name, "freq")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.freq, var_value, T_UNSIGNED_LONG_LOW(0))) {
            e=1; break;}
        } else if(!strcmp(var_name, "width")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.width, var_value, T_INT_LOW(1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "height")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.height, var_value, T_INT_LOW(1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "window_x")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.window_x, var_value, T_INT_LOW(0))) {
            e=1; break;}
        } else if(!strcmp(var_name, "window_y")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.window_y, var_value, T_INT_LOW(0))) {
            e=1; break;}
        } else if(!strcmp(var_name, "window_width")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.frame_par.width, var_value, T_INT_LOW(1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "window_height")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.frame_par.height, var_value, T_INT_LOW(1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "gray")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.gray, var_value, T_INT)) {
            e=1; break;}
        } else if(!strcmp(var_name, "brightness")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.brightness, var_value, T___U16)) {
            e=1; break;}
        } else if(!strcmp(var_name, "hue")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.hue, var_value, T___U16)) {
            e=1; break;}
        } else if(!strcmp(var_name, "colour")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.colour, var_value, T___U16)) {
            e=1; break;}
        } else if(!strcmp(var_name, "contrast")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.contrast, var_value, T___U16)) {
            e=1; break;}
        } else if(!strcmp(var_name, "whiteness")) {
          if(store_var(&device->camera_v[device->camera_c].v4l_cam_par.whiteness, var_value, T___U16)) {
            e=1; break;}
        } else if(!strcmp(var_name, "threshold")) {
          if(store_var(&device->camera_v[device->camera_c].motion_par.threshold, var_value, T_DOUBLE_BOTH(0, 100))) {
            e=1; break;}
        } else if(!strcmp(var_name, "motion_detection_algorithm")) {
          if(store_var(&device->camera_v[device->camera_c].motion_par.algorithm, var_value, T_INT_BOTH(0, 2))) {
            e=1; break;}
        } else if(!strcmp(var_name, "half_image")) {
          if(store_var(&device->camera_v[device->camera_c].motion_par.half_image, var_value, T_INT_BOTH(0, 1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "deriv_diff_full")) {
          if(store_var(&device->camera_v[device->camera_c].motion_par.deriv_diff_full, var_value, T_INT_BOTH(0, 1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "force_obsolete_sec")) {
          int sec;
          if(store_var(&sec, var_value, T_INT_LOW(0))) {
            e=1; break;} else {
            device->camera_v[device->camera_c].motion_par.force_obsolete_sec=(time_t)sec;}
        } else if(!strcmp(var_name, "noise")) {
          if(store_var(&device->camera_v[device->camera_c].motion_par.noise, var_value, T_UNSIGNED_CHAR)) {
            e=1; break;}
        } else if(!strcmp(var_name, "calibrate")) {
          if(store_var(&device->camera_v[device->camera_c].motion_par.calibrate, var_value, T_INT_BOTH(0,2))) {
            e=1; break;}
        } else if(!strcmp(var_name, "frame_buffer_size")) {
          if(store_var(&device->camera_v[device->camera_c].frame_buffer_size, var_value, T_INT_LOW(1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "font")) {
          if(store_var(&device->camera_v[device->camera_c].write_par.font, var_value, T_INT_BOTH(0, 2))) {
            e=1; break;}
        } else if(!strcmp(var_name, "font_transparency")) {
          if(store_var(&device->camera_v[device->camera_c].write_par.font, var_value, T_INT_BOTH(0, 100))) {
            e=1; break;}
//        } else if(!strcmp(var_name, "jpeg_quality")) {
//          if(store_var(&device->camera_v[device->camera_c].jpeg_quality, var_value, T_INT_BOTH(0, 100))) {
//            e=1; break;}
        } else if(!strcmp(var_name, "xvid_quality")) {
          if(store_var(&device->camera_v[device->camera_c].evid_xvidpar.quality, var_value, T_INT_BOTH(0, 6))) {
            e=1; break;}
        } else if(!strcmp(var_name, "xvid_quant")) {
          if(store_var(&device->camera_v[device->camera_c].evid_xvidpar.quant, var_value, T_INT_BOTH(1, 31))) {
            e=1; break;}
        } else if(!strcmp(var_name, "avi_framerate")) {
          if(store_var(&device->camera_v[device->camera_c].avi_framerate, var_value, T_DOUBLE_LOW(0.001))) {
            e=1; break;}
        } else if(!strcmp(var_name, "output_dir")) {
          strcpy(device->camera_v[device->camera_c].output_dir, var_value);
        } else if(!strcmp(var_name, "split_record_min")) {
          if(store_var(&split_record_min, var_value, T_INT_BOTH(1,240))) {
            e=1; break;}
            device->camera_v[device->camera_c].split_record_sec=(time_t)(split_record_min*60);
        } else if(!strcmp(var_name, "archive_days")) {
          if(store_var(&device->camera_v[device->camera_c].archive_days, var_value, T_INT_LOW(0))) {
            e=1; break;}
        } else if(!strcmp(var_name, "archive_size")) {
          if(store_var(&device->camera_v[device->camera_c].archive_size, var_value, T_INT_LOW(1))) {
            e=1; break;}
        } else if(!strcmp(var_name, "dump_raw_video")) {
          strcpy(device->camera_v[device->camera_c].dump_raw_video, var_value);
        } else {
          fprintf(stderr, "Invalid variable '%s' in section [%s] at line %d.\n", var_name, sec_name, line_count-1);
          load_config_return();
          return 1;
        }
      }
      if(e) {
        fprintf(stderr, "Invalid value '%s' to '%s' at line %d in section [%s].\n", var_value, var_name, line_count-1, sec_name);
        load_config_return();
        return 1;
      }
      device->camera_c++;
    } else {
      fprintf(stderr, "Unknow section [%s] at line %d.", sec_name, line_count);
      load_config_return();
      return 1;
    }
    // EOF
    if(v==1)
      break;
    // Parse error
    if(v==2) {
      load_config_return();
      return 1;
    }
    // New section
    if(v==3)
      continue;
  }
  // Section read error
  if(s==2) {
    load_config_return();
    return 1;
  }
  // Parsing finished, no more sections
  // Missing cameras
  if(device->camera_c==0) {
    fprintf(stderr, "You must specify at least one 'camera' option.\n");
    return 1;
  }
  // window settings
  for(c=0;c<device->camera_c;c++) {
    if(device->camera_v[c].v4l_cam_par.width==0)
      device->camera_v[c].v4l_cam_par.width=352;
    if(device->camera_v[c].v4l_cam_par.height==0)
      device->camera_v[c].v4l_cam_par.height=288;
    if(device->camera_v[c].v4l_cam_par.frame_par.width==0)
      device->camera_v[c].v4l_cam_par.frame_par.width=device->camera_v[c].v4l_cam_par.width-device->camera_v[c].v4l_cam_par.window_x;
    if(device->camera_v[c].v4l_cam_par.frame_par.height==0)
      device->camera_v[c].v4l_cam_par.frame_par.height=device->camera_v[c].v4l_cam_par.height-device->camera_v[c].v4l_cam_par.window_y;
    if(device->camera_v[c].v4l_cam_par.window_x+device->camera_v[c].v4l_cam_par.frame_par.width>device->camera_v[c].v4l_cam_par.width||device->camera_v[c].v4l_cam_par.window_y+device->camera_v[c].v4l_cam_par.frame_par.height>device->camera_v[c].v4l_cam_par.height) {
      fprintf(stderr, "Capture window out of range");
      load_config_return();
      return 1;
    }
    // set frame par
    device->camera_v[c].v4l_cam_par.frame_par.size=device->camera_v[c].v4l_cam_par.frame_par.width*device->camera_v[c].v4l_cam_par.frame_par.height*PALETTE_DEPTH;
    memcpy(&device->camera_v[c].frame_par, &device->camera_v[c].v4l_cam_par.frame_par, sizeof(frame_par_t));
    // xvid greyscale
    device->camera_v[c].evid_xvidpar.gray=config_cam_global.v4l_cam_par.gray;
    // frame buffer
    if(device->camera_v[c].frame_buffer_size)
      device->camera_v[c].queue_max_frames=device->camera_v[c].frame_buffer_size*1048576/device->camera_v[c].frame_par.size;
    device->camera_v[c].frame_buffer_size=device->camera_v[c].frame_par.size*device->camera_v[c].queue_max_frames;
    // avi framerate
    if(device->camera_v[c].avi_framerate==-1)
      device->camera_v[c].avi_framerate=device->framerate;
  }
  load_config_return();
  return 0;
}
