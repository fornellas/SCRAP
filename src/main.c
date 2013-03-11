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

/**
 *  \mainpage SCRAP hacker's guide
 *
 *  \section intro Introduction
 *
 *  This is the hacker's guide.
 */

/** @defgroup core Program core
 *  @{
 */

/** @file device.h
 *  @brief Structures and definitions for configuration storage
 *
 *  Defines and some data types used for capture device configuration.
 */

/** @file main.c
 *  @brief Program core
 *
 *  This file contain the program core functions and capture loop.
 */

#include "common.h"
#include "capture_conf.h"
#include "device.h"
#include "enc_video.h"
#include "frame_queue.h"
#include "v4l.h"
#include "load_conf.h"
#include "log.h"
#include "sleep.h"
#include <unistd.h>
#include <error.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <glob.h>
#include <fnmatch.h>
#include <limits.h>
#define BUFFER_SIZE 256

/** @brief Common log handler
 */
log_handler_t *log_handler;
/** @brief Recording prefix for naming saved videos. Can not contain wildcards.
 */
char rec_prefix[]="(recording)";
/** @brief Interrupted recording file name sufix. Can not contain wildcards.
 */
char rec_int_sufix[]="(recording interrupted)";
/** @brief Device array.
 */
device_t **device_v=NULL;
/** @brief Device count.
 */
int device_c=-1;
/** @brief Indicates capture must be done.
 */
int capture=1;
/** @brief Capture indicator lock
 */
pthread_mutex_t capture_lock;
/** @brief Enable performance warnings.
 */
int performance_warn=1;

/** @brief Return capture value.
 */
int get_capture() {
  int c;
  pthread_mutex_lock(&capture_lock);
  c=capture;
  pthread_mutex_unlock(&capture_lock);
  return c;
}

/** @brief Capture thread
 *
 *  This function will be responsable for capturing and queueing frames.
 */
void *capture_thread(void *arg) {
  device_t *device=(device_t *)arg;
  sleep_handler_t *sleep_handler;
  int c;
  pixel_t *frame;
  char *errstr=NULL;
  double fps;
  int next_cam;
  int n_frames;
  // start sleep handler
  sleep_handler=sleep_create_handler(device->framerate, &errstr);
  if(sleep_handler==NULL) {
    log_printf(log_handler, LOG_ERR, "Error with delay function: %s", errstr);
    exit(1);
  }
  // start capture
  log_printf(log_handler, LOG_ACT, "'%s', '%s': Starting frame capture.", device->video_dev, device->camera_v[0].name);
  if(v4l_start_cap(device->v4l_handler, device->camera_v[0].v4l_cam_ident)) {
    log_printf(log_handler, LOG_ERR, "'%s', '%s': Error capturing frame: %s", device->video_dev, device->camera_v[0].name, v4l_strerror(device->v4l_handler));
    exit(1);
  }
  while(get_capture()) {
    for(c=0;c<device->camera_c;c++) {
      // find next cam
      next_cam=c+1;
      if(next_cam==device->camera_c)
        next_cam=0;
      // add frame to queue
      log_printf(log_handler, LOG_ACT, "'%s', '%s': Queueing frame.", device->video_dev, device->camera_v[c].name);
      frame=queue_frame(device->queue, device->camera_v[c].queue_ident, &n_frames);
      if(frame==NULL) {
        // full queue
        if(performance_warn)
          log_printf(log_handler, LOG_WARN, "'%s', '%s': Queue full (%d frames), discarting frame and waiting for a free queue.", device->video_dev, device->camera_v[c].name, n_frames);
        queue_wait_not_full(device->queue);
        if(v4l_sync_frame(device->v4l_handler, device->camera_v[c].v4l_cam_ident, NULL)) {
          log_printf(log_handler, LOG_ERR, "'%s', '%s': Error syncronizing frame: %s", device->video_dev, device->camera_v[c].name, v4l_strerror(device->v4l_handler));
          exit(1);
        }
        continue;
      }
      log_printf(log_handler, LOG_ACT, "'%s', '%s': Queued %d frames.", device->video_dev, device->camera_v[c].name, n_frames);
      // sync frame
      log_printf(log_handler, LOG_ACT, "'%s', '%s': Syncronizing frame.", device->video_dev, device->camera_v[c].name);
      if(v4l_sync_frame(device->v4l_handler, device->camera_v[next_cam].v4l_cam_ident, frame)) {
        log_printf(log_handler, LOG_ERR, "'%s', '%s': Error syncronizing frame: %s", device->video_dev, device->camera_v[c].name, v4l_strerror(device->v4l_handler));
        exit(1);
      }
      // signal frame is ready
      log_printf(log_handler, LOG_ACT, "'%s', '%s': Frame ready to be processed.", device->video_dev, device->camera_v[c].name);
      queue_set_last_frame_ready(device->queue, device->camera_v[c].queue_ident);
    }
    switch(sleep_period(sleep_handler, &fps)) {
      case 1:
        if(performance_warn)
          log_printf(log_handler, LOG_WARN, "'%s': System slow to capture at %.3lf fps , only %.3lf fps last time.", device->video_dev, device->framerate, fps);
        break;
      case 2:
        log_printf(log_handler, LOG_ERR, "'%s': Error sleeping: %s", device->video_dev, sleep_strerror(sleep_handler));
        exit(1);
        break;
    }
  }
  return NULL;
}

/** @brief Close video stream.
 */
void close_video_stream(device_t *device, int c) {
  char *filename;
  char *dest;
  char *errstr;
  int p;
  int i;
  char buffer[BUFFER_SIZE];

  // get file name
  filename=evid_filename(device->camera_v[c].evid_handler);
  log_printf(log_handler, LOG_ACT, "'%s', '%s': Closing video file '%s'.", device->video_dev, device->camera_v[c].name, filename);
  // close avi
  if(evid_destroy_handler(device->camera_v[c].evid_handler, &errstr)) {
    log_printf(log_handler, LOG_ERR, "'%s', '%s': Error closing video file: %s", device->video_dev, device->camera_v[c].name, errstr);
    exit(1);
  }
  // gen dest filename
  dest=x_strdup(filename);
  for(p=strlen(dest)-1;p>-1;p--)
    if(dest[p]=='/')
      break;
  for(i=p+(int)strlen(rec_prefix)+2;i<(int)strlen(dest);i++)
    dest[i-strlen(rec_prefix)-1]=dest[i];
  dest[i-strlen(rec_prefix)-1]='\0';
  // move file
  errno=0;
  if(rename(filename, dest))
    log_printf(log_handler, LOG_ERR, "Error renaming file '%s' to '%s': %s", filename, dest, strerror_r(errno, buffer, BUFFER_SIZE));
  free(filename);
  free(dest);
  device->camera_v[c].evid_handler=NULL;
}

/** @brief Processing thread
 *
 *  Queued frames will be processed by this function.
 */
void *processing_thread(void *arg) {
  device_t *device=(device_t *)arg;
  int c;
  pixel_t *frame_queue;
  pixel_t **frame_calibrate;
  pixel_t *frame_process;
  double area_changed=-1;
  int motion;
  struct tm cap_time_tm;
  time_t cap_time;
  char *errstr=NULL;
  char *filename;
  char buffer[BUFFER_SIZE];
  time_t last_erase_old_videos=time(NULL);
  time_t now;
  char date[20];
  char *text;
  DIR *dp;
  int filec=0;
  char **filev;
  struct dirent entry;
  struct dirent *result;
  struct stat st;
  int match;
  char *pattern;
  char *dirname;
  struct tm rec_time_tm;
  time_t rec_time;
  int i;
  int r;
  char cam_name[MAX_VALUE_LENGTH*2];

  // initalize frame_calibrate array
  frame_calibrate=(pixel_t **)x_malloc((size_t)(sizeof(pixel_t *)*(device->camera_c+1)));
  for(c=0;c<device->camera_c;c++) {
    if(device->camera_v[c].motion_par.calibrate)
      frame_calibrate[c]=(pixel_t *)x_calloc(1, (size_t)(device->camera_v[c].frame_par.width*device->camera_v[c].frame_par.height*PALETTE_DEPTH));
  }

  while(1) {
    now=time(NULL);
    // at each one second, erase old videos
    if(now-last_erase_old_videos>1) {
      for(c=0;c<device->camera_c;c++) {
        // open dir
        x_asprintf(&dirname, "%s/%s", device->conf_path, device->camera_v[c].output_dir);
        errno=0;
        if(NULL==(dp=opendir(dirname))) {
          log_printf(log_handler, LOG_ERR, "'%s' '%s': Error opening directory '%s' for reading: %s", device->video_dev, device->camera_v[c].name, dirname, strerror_r(errno, buffer, BUFFER_SIZE));
          exit(1);
        }

        // set camera name
        for(r=0;(size_t)r<strlen(device->camera_v[c].name);r++) {
          cam_name[r*2]='\\';
          cam_name[r*2+1]=device->camera_v[c].name[r];
        }
        cam_name[r*2]='\0';

        // list files
        filec=0;
        filev=NULL;
        do {
          errno=0;
          if(readdir_r(dp, &entry, &result)) {
            log_printf(log_handler, LOG_ERR, "'%s' '%s': Error listing directory '%s': %s", device->video_dev, device->camera_v[c].name, dirname, strerror_r(errno, buffer, BUFFER_SIZE));
            exit(1);
          }

          // get file status
          x_asprintf(&filename, "%s/%s/%s", device->conf_path, device->camera_v[c].output_dir, entry.d_name);
          errno=0;
          if(stat(filename, &st)) {
            log_printf(log_handler, LOG_ERR, "'%s' '%s': Error getting file '%s' status: %s", device->video_dev, device->camera_v[c].name, filename, strerror_r(errno, buffer, BUFFER_SIZE));
            exit(1);
          }

          // only files
          if(!S_ISREG(st.st_mode))
            continue;

          // match pattern
          match=0;
          // closed streams
          x_asprintf(&pattern, "[0-9][0-9][0-9][0-9].[0-9][0-9].[0-9][0-9] [0-9][0-9]:[0-9][0-9]:[0-9][0-9] - %s.avi", cam_name);
          if((r=fnmatch(pattern, entry.d_name, 0))) {
            if(r!=FNM_NOMATCH) {
              log_printf(log_handler, LOG_ERR, "'%s' '%s': Error matching pattern for file '%s'. Probably a bug. Please contact the author:\n%s", device->video_dev, device->camera_v[c].name, entry.d_name, AUTHORS);
              exit(1);
            }
          } else
            match=1;
          free(pattern);
          // interrupted streams
          x_asprintf(&pattern, "[0-9][0-9][0-9][0-9].[0-9][0-9].[0-9][0-9] [0-9][0-9]:[0-9][0-9]:[0-9][0-9] - %s %s.avi", cam_name, rec_int_sufix);
          if((r=fnmatch(pattern, entry.d_name, 0))) {
            if(r!=FNM_NOMATCH) {
              log_printf(log_handler, LOG_ERR, "'%s' '%s': Error matching pattern for file '%s'. Probably a bug. Please contact the author:\n%s", device->video_dev, device->camera_v[c].name, entry.d_name, AUTHORS);
              exit(1);
            }
          } else
            match=1;
          free(pattern);
          // recording streams
          x_asprintf(&pattern, "%s [0-9][0-9][0-9][0-9].[0-9][0-9].[0-9][0-9] [0-9][0-9]:[0-9][0-9]:[0-9][0-9] - %s.avi", rec_prefix, cam_name);
          if((r=fnmatch(pattern, entry.d_name, 0))) {
            if(r!=FNM_NOMATCH) {
              log_printf(log_handler, LOG_ERR, "'%s' '%s': Error matching pattern for file '%s'. Probably a bug. Please contact the author:\n%s", device->video_dev, device->camera_v[c].name, entry.d_name, AUTHORS);
              exit(1);
            }
          } else
            match=1;
          free(pattern);
          // add to list
          if(match) {
            filec++;
            filev=(char **)x_realloc(filev, sizeof(char *)*filec);
            filev[filec-1]=x_strdup(filename);
          }
          free(filename);
        } while(NULL!=result);

        // close dir
        errno=0;
        if(closedir(dp)) {
          log_printf(log_handler, LOG_ERR, "'%s' '%s': Error closing directory '%s': %s", device->video_dev, device->camera_v[c].name, dirname, strerror_r(errno, buffer, BUFFER_SIZE));
          exit(1);
        }
        free(dirname);

        // sort filenames
        qsort(filev, filec, sizeof(char *), &alphasort);

        // erase by date
        if(device->camera_v[c].archive_days) {
          rec_time_tm.tm_isdst=daylight;
          for(r=0;r<filec;r++) {
            // isolate filename
            for(i=strlen(filev[r])-1;i>-1;i--)
              if(filev[r][i]=='/')
                break;
            i++;
            // skip current recordings
            if(!strncmp(rec_prefix, &filev[r][i], strlen(rec_prefix)))
              continue;
            // extract recording date information
            if(6!=sscanf(&filev[r][i], "%d.%d.%d %d:%d:%d %*s", &rec_time_tm.tm_year, &rec_time_tm.tm_mon, &rec_time_tm.tm_mday, &rec_time_tm.tm_hour, &rec_time_tm.tm_min, &rec_time_tm.tm_sec)) {
              log_printf(log_handler, LOG_ERR, "'%s' '%s': Error extracting date from filename '%s'. You may have found a bug, please contact the author:\n%s", device->video_dev, device->camera_v[c].name, &filev[r][i], AUTHORS);
              exit(1);
            } else {
              rec_time_tm.tm_mon--;
              rec_time_tm.tm_year-=1900;
              if(-1==(rec_time=mktime(&rec_time_tm))) {
                log_printf(log_handler, LOG_ERR, "'%s' '%s': Error converting date format of '%s'. This may be a bug. Please contact the author:\n%s", device->video_dev, device->camera_v[c].name, &filev[r][i], AUTHORS);
                exit(1);
              } else {
                // file is obsolete
                if(time(NULL)-rec_time>(time_t)device->camera_v[c].archive_days*86400) {
                  log_printf(log_handler, LOG_ACT, "'%s' '%s': Erasing file older than %d day(s): '%s'.", device->video_dev, device->camera_v[c].name, device->camera_v[c].archive_days, &filev[r][i]);
                  // remove file
                  errno=0;
                  if(unlink(filev[r]))
                    log_printf(log_handler, LOG_ERR, "'%s' '%s': Error removing '%s': %s", device->video_dev, device->camera_v[c].name, filev[r], strerror_r(errno, buffer, BUFFER_SIZE));
                  else {
                    free(filev[r]);
                    for(i=r+1;i<filec;i++)
                      filev[i-1]=filev[i];
                    filec--;
                    filev=(char **)x_realloc(filev, sizeof(char *)*filec);
                    r--;
                  }
                } else
                  break;
              }
            }
          }
        }

        // erase by max size
        if(device->camera_v[c].archive_size) {
          unsigned long int sum=0;
          // sum size
          for(r=0;r<filec;r++) {
            errno=0;
            if(stat(filev[r], &st)==-1) {
              log_printf(log_handler, LOG_ERR, "'%s' '%s': Error getting status of file '%s': %s", device->video_dev, device->camera_v[c].name, filev[r], strerror_r(errno, buffer, BUFFER_SIZE));
              continue;
            }
            sum+=(unsigned long int)st.st_size;
          }

          // for each file
          for(r=0;r<filec;r++) {
            // isolate filename
            for(i=strlen(filev[r])-1;i>-1;i--)
              if(filev[r][i]=='/')
                break;
            i++;
            // skip current recordings
            if(!strncmp(rec_prefix, &filev[r][i], strlen(rec_prefix)))
              continue;
            // erase if too big
            if(sum>(unsigned long int)device->camera_v[c].archive_size*1048576) {
              log_printf(log_handler, LOG_ACT, "'%s' '%s': Erasing file to free space: '%s'.", device->video_dev, device->camera_v[c].name, &filev[r][i]);
              errno=0;
              if(stat(filev[r], &st)==-1)
                log_printf(log_handler, LOG_ERR, "'%s' '%s': Error getting status of file '%s': %s", device->video_dev, device->camera_v[c].name, filev[r], strerror_r(errno, buffer, BUFFER_SIZE));
              else {
                errno=0;
                if(unlink(filev[r]))
                  log_printf(log_handler, LOG_ERR, "'%s' '%s': Error removing '%s': ", device->video_dev, device->camera_v[c].name, filev[r], strerror_r(errno, buffer, BUFFER_SIZE));
                else {
                  free(filev[r]);
                  for(i=r+1;i<filec;i++)
                    filev[i-1]=filev[i];
                  filec--;
                  filev=(char **)x_realloc(filev, sizeof(char *)*filec);
                  r--;
                  sum-=(unsigned long int)st.st_size;
                }
              }
            } else
              break;
          }
        }

        // free memory
        for(r=0;r<filec;r++)
          free(filev[r]);
        free(filev);
      }
      // reset time
      last_erase_old_videos=now;
    }

    // save jpeg screenshots
    // TODO

    // process queued frames
    for(c=0;c<device->camera_c;c++) {
      // get frame pointer
      log_printf(log_handler, LOG_ACT, "'%s', '%s': Getting frame to process.", device->video_dev, device->camera_v[c].name);
      frame_queue=queue_get_first_frame(device->queue, device->camera_v[c].queue_ident);
      // frame not ready
      if(frame_queue==NULL) {
        log_printf(log_handler, LOG_ACT, "'%s', '%s': No frames ready, waiting.", device->video_dev, device->camera_v[c].name);
        queue_wait_any_start_frame_ready(device->queue);
        continue;
      }
      // get cap time
      cap_time=queue_get_first_frame_time(device->queue, device->camera_v[c].queue_ident);
      localtime_r(&cap_time, &cap_time_tm);
      // detects motion
      if(device->camera_v[c].motion_par.algorithm)
        log_printf(log_handler, LOG_ACT, "'%s', '%s': Detecting motion.", device->video_dev, device->camera_v[c].name);
      if((motion=motion_detect(device->camera_v[c].motion_handler, frame_queue, cap_time, &area_changed, frame_calibrate[c]))||device->camera_v[c].motion_par.calibrate) {
        // motion detected! lets say it
        if(device->camera_v[c].motion_par.algorithm) {
          if(motion)
            log_printf(log_handler, LOG_MOTION, "'%s', '%s': Motion detected (%.2lf%% changed).", device->video_dev, device->camera_v[c].name, area_changed);
          // no motion here... lets report it in a different log level
          else
            log_printf(log_handler, LOG_MOTION, "'%s', '%s': No motion detected (%.2lf%% changed).", device->video_dev, device->camera_v[c].name, area_changed);
        }

        // set frame to be processed
        frame_process=device->camera_v[c].motion_par.calibrate?frame_calibrate[c]:frame_queue;

        // set date string
        strftime(date, 20, "%Y.%m.%d %H:%M:%S", &cap_time_tm);

        // time stamp frame
        if(device->camera_v[c].write_par.font) {
          log_printf(log_handler, LOG_ACT, "'%s', '%s': Time stamping frame.", device->video_dev, device->camera_v[c].name);
          x_asprintf(&text, "%s - %s", device->camera_v[c].name, date);
          if(write_frame(device->camera_v[c].write_handler, frame_process, date)) {
            log_printf(log_handler, LOG_ACT, "'%s', '%s': Error time stamping frame: %s", device->video_dev, device->camera_v[c].name, write_strerror(device->camera_v[c].write_handler));
            exit(1);
          }
          free(text);
        }

        // close avi stream if time's up
        if(device->camera_v[c].evid_handler!=NULL) {
          if(cap_time-device->camera_v[c].last_video_open>=device->camera_v[c].split_record_sec) {
            close_video_stream(device, c);
          }
        }
        // open avi stream if not opened
        if(device->camera_v[c].evid_handler==NULL) {
          // set open time
          device->camera_v[c].last_video_open=cap_time;
          // set name
          x_asprintf(&filename, "%s/%s/%s %s - %s.avi", device->conf_path, device->camera_v[c].output_dir, rec_prefix, date, device->camera_v[c].name);
          log_printf(log_handler, LOG_ACT, "'%s', '%s': Opening new video file: '%s'.", device->video_dev, device->camera_v[c].name, filename);
          // open stream
          if((device->camera_v[c].evid_handler=evid_create_handler(&device->camera_v[c].frame_par, &device->camera_v[c].evid_xvidpar, device->camera_v[c].avi_framerate, filename, &errstr))==NULL) {
            log_printf(log_handler, LOG_ERR, "'%s', '%s': Error opening file '%s' to save video: %s", device->video_dev, device->camera_v[c].name, filename, errstr);
            free(errstr);
            exit(1);
          }
          free(filename);
        }
        // encode video frame
        log_printf(log_handler, LOG_ACT, "'%s', '%s': Encoding frame.", device->video_dev, device->camera_v[c].name);
        switch(evid_enc_frame(device->camera_v[c].evid_handler, frame_process)) {
          case 1:
            // avi too long to store it
            log_printf(log_handler, LOG_WARN, "'%s', '%s': Maximum AVI size reached, closing it.", device->video_dev, device->camera_v[c].name);
            close_video_stream(device, c);
            break;
          case 2:
            log_printf(log_handler, LOG_ERR, "'%s', '%s': Error encoding frame to file '%s': %s", device->video_dev, device->camera_v[c].name, filename, evid_strerror(device->camera_v[c].evid_handler));
            exit(1);
            break;
        }

        // dump raw video
        if(device->camera_v[c].dump_raw_video[0]!='\0') {
          if(1!=fwrite(frame_process, (size_t)device->camera_v[c].frame_par.size, 1, device->camera_v[c].dump_raw_video_stream))
            log_printf(log_handler, LOG_ERR, "'%s', '%s': Error dumping raw video to file '%s'.", device->video_dev, device->camera_v[c].name, device->camera_v[c].dump_raw_video);
          // flush stream
          errno=0;
          if(fflush(device->camera_v[c].dump_raw_video_stream)) {
            log_printf(log_handler, LOG_ERR, "'%s', '%s': Error flushing stream of raw video dump: %s", device->video_dev, device->camera_v[c].name, strerror_r(errno, buffer, BUFFER_SIZE));
            exit(1);
          }

        }
      } else {
        // no motion here...
        if(device->camera_v[c].motion_par.algorithm)
          log_printf(log_handler, LOG_ACT, "'%s', '%s': No motion detected (%.2lf%% changed).", device->video_dev, device->camera_v[c].name, area_changed);
      }
      // discart frame
      log_printf(log_handler, LOG_ACT, "'%s', '%s': Frame processed, removing from queue.", device->video_dev, device->camera_v[c].name);
      queue_remove_first(device->queue, device->camera_v[c].queue_ident);
    }
  }

  return NULL;
}

/** @brief Clean program exit
 *
 *  Function to be called if program is terminated.
 */
void clean_exit(void) {
  int d;
  int c;

  // disable capture
  pthread_mutex_lock(&capture_lock);
  capture--;
  if(capture<0)
    return;
  pthread_mutex_unlock(&capture_lock);

  // wait until all frames in memory are processed
  log_printf(log_handler, LOG_WARN, "Waiting for frames in memory to be processed.");
  for(d=0;d<=device_c;d++)
    for(c=0;c<device_v[d]->camera_c;c++) {
      queue_wait_all_empty(device_v[d]->queue);
      log_printf(log_handler, LOG_WARN, "'%s' '%s': No more frames.", device_v[d]->video_dev, device_v[d]->camera_v[c].name);
    }

  // close
  log_printf(log_handler, LOG_WARN, "Closing video files.");
  for(d=0;d<=device_c;d++)
    for(c=0;c<device_v[d]->camera_c;c++) {
      if(device_v[d]->camera_v[c].evid_handler!=NULL)
        close_video_stream(device_v[d], c);
    }

  log_printf(log_handler, LOG_WARN, "Exiting.");
  _exit(0);
}

/** @brief Clean exit handler.
 */
void atexit_handler(void) {
  clean_exit();
}

/** @brief Signal handler.
 */
void signal_handler(int sig) {
  switch(sig) {
    case SIGTERM:
      log_printf(log_handler, LOG_WARN, "Recieved SIGTERM signal, exiting...");
      break;
    case SIGINT:
      log_printf(log_handler, LOG_WARN, "Recieved SIGINT signal, exiting...");
      break;
    case SIGPIPE:
      log_printf(log_handler, LOG_WARN, "Recieved SIGPIPE signal, exiting...");
      break;
    case SIGHUP:
      log_printf(log_handler, LOG_WARN, "Recieved SIGHUP signal, exiting...");
      break;
    default:
      log_printf(log_handler, LOG_WARN, "Unexpected signal recieved, exiting.");
      break;
  }
  clean_exit();
}

/** @brief Program main function
 *
 *  Program core.
 */
int main (int argc, char *argv[]){
  int c;
  int d;
  char **conf_path=NULL;
  char *print_cap_dev=NULL;
  char *log_path=NULL;
  int verbose=LOG_DEFAULT_LEVEL;
  int daemon=0;
  pthread_t *capture=NULL;
  pthread_t *processing=NULL;
  char *errstr=NULL;
  char buffer[BUFFER_SIZE];
  char *running_dir;
  char *filename;
  int r;
  char cam_name[MAX_VALUE_LENGTH*2];

  // get current dir
  errno=0;
  running_dir=get_current_dir_name();
  if(running_dir==NULL)
    error(1, errno, "Error getting current working directory name: ");

  // parse command line
  opterr=0;
  while((c=getopt(argc, argv,"c::dhl::p::sv::w"))!=-1) {
    switch(c) {
      // configuratin file
      case 'c':
        device_c++;
        conf_path=(char **)x_realloc(conf_path, (size_t)(sizeof(char *)*(device_c+1)));
        conf_path[device_c]=optarg;
        device_v=(device_t **)x_realloc(device_v, (size_t)(sizeof(device_t *)*(device_c+1)));
        device_v[device_c]=(device_t *)x_malloc(sizeof(device_t));
        if(conf_path[device_c]==NULL) {
          if(argc-optind>0) {
            conf_path[device_c]=argv[optind];
            optind++;
          } else {
            error(1, 0, "Missing argument to '-c'.");
          }
        }
        // set up path
        if(conf_path[device_c][0]=='/')
          x_asprintf(&device_v[device_c]->conf_path, "%s", conf_path[device_c]);
        else
          x_asprintf(&device_v[device_c]->conf_path, "%s/%s", running_dir, conf_path[device_c]);
        for(d=strlen(device_v[device_c]->conf_path)-1;d>-1;d--) {
          if(device_v[device_c]->conf_path[d]=='/') {
            device_v[device_c]->conf_path[d]='\0';
            break;
          }
        }
        if(d==0)
          strcpy(device_v[device_c]->conf_path, ".");
        device_v[device_c]->conf_path=(char *)x_realloc(device_v[device_c]->conf_path, strlen(device_v[device_c]->conf_path)+1);
        break;
      // daemon
      case 'd':
        daemon=1;
        break;
      // help
      case 'h':
        printf("%s\n(compiled on: %s)\n%s\n\nUsage: %s [arguments]\nWhere arguments can be:\n-c FILE\n  Configuration file with device capture options.\n  Can be set many times, each one for a different capture device.\n-d\n  Run as a daemon.\n-h\n  This help.\n-l FILE\n  Log file. Default to stdout without -d and null if -d.\n-p DEVICE\n  Print capabilities for cameras in DEVICE. \n-s\n  Dump sample configuration file to standard output.\n-v LEVEL\n  Log verbose level.\n-w\n  Disable performance warnings (slow capture / full frame queue).\n", PACKAGE_STRING, __DATE__, AUTHORS,argv[0]);
        exit(0);
        break;
      // log path
      case 'l':
        if(log_path!=NULL)
          error(1, 0, "Duplicated '-l' argument.");
        log_path=optarg;
        if(log_path==NULL) {
          if(argc-optind>0) {
            log_path=argv[optind];
            optind++;
          } else {
            error(1, 0, "Missing argument to '-l'.");
          }
        }
        break;
      // print device capabilities
      case 'p':
        print_cap_dev=optarg;
        if(print_cap_dev==NULL) {
          if(argc-optind>0) {
            print_cap_dev=argv[optind];
            optind++;
          } else {
            error(1, 0, "Missing argument to '-d'.");
          }
        }
        break;
      // dump conf
      case 's':
        puts(CAPTURE_CONF);
        exit(0);
        break;
      // verbose level
      case 'v':
        if(optarg==NULL) {
          if(argc-optind>0) {
            if(1!=sscanf(argv[optind], "%d", &verbose))
              error(1, 0, "Invalid argument to '-v'.");
            optind++;
          } else {
            error(1, 0, "Missing argument to '-l'.");
          }
        } else {
          if(1!=sscanf(optarg, "%d", &verbose))
            error(1, 0, "Invalid argument to '-v'.");
        }
        if(verbose<0)
          error(1, 0, "Verbose level (-v) must be >=0.");
        break;
      // performance warning
      case 'w':
        performance_warn=0;
        break;
      // invalid arg
      default:
        error(1, 0, "Invalid argument '%c'. Try '%s -h'.", optopt,  argv[0]);
        exit(1);
        break;
    }
  }

  if(device_c==-1&&!print_cap_dev)
    error(1, 0, "No action given.");
  if(argc-optind>0)
    error(1, 0, "Too many arguments. Try '%s -h'.", argv[0]);

  // Usual ad banner
  printf("%s\n(compiled on: %s)\n%s\n\n", PACKAGE_STRING, __DATE__, AUTHORS);

  // print capabilities if -d and exit
  if(print_cap_dev!=NULL) {
    printf("Printing capabilities for '%s'...\n", print_cap_dev);
    if(v4l_print_cap(print_cap_dev))
      error(1, 0, "Error getting capabilities.");
    exit(0);
  }

  // Load configuration file
  for(c=0;c<=device_c;c++) {
    printf("Loading configuration %d of %d: '%s'...\n", c+1, device_c+1, conf_path[c]);
    if(load_config(conf_path[c], device_v[c]))
      error(1, 0, "Fail on parsing configuration file '%s'.", conf_path[c]);
  }
  putchar('\n');

  // print current config
  for(d=0;d<=device_c;d++) {
    printf("Device configuration dump (%d/%d):\n  [Capture]\n    video_dev: '%s'\n    framerate: %.3ffps\n", d+1, device_c+1, device_v[d]->video_dev, device_v[d]->framerate);
    for(c=0;c<device_v[d]->camera_c;c++) {
      printf("  [Camera]\n    name: '%s'\n    channel: %d\n    norm: %d\n    tuner: %d\n      mode: %d\n      freq: %lu\n    width: %d\n    height: %d\n    window_x: %d\n    window_y: %d\n    window_width: %d\n    window_height: %d\n    gray: %s\n    brightness: %d\n    hue: %d\n    colour: %d\n    contrast: %d\n    whiteness: %d\n    motion_detection_algorithm: %d\n    half_image: %s\n    deriv_diff_full: %s\n    force_obsolete_sec: %lds\n    threshold: %.2f%%\n    noise: %hhd\n    calibrate: %d\n    frame_buffer_size: %.1fMb, %d frame(s)\n    font: %d\n    font_transparency: %d%%\n    jpeg_quality: %d%%\n    avi_framerate: %.3ffps\n    xvid_quality: %d\n    xvid_quant: %d\n    output_dir: '%s'\n    split_record_min: %ld minute(s)\n    archive_days: %d day(s)\n    archive_size: %dMb\n    dump_raw_video: '%s'\n", device_v[d]->camera_v[c].name, device_v[d]->camera_v[c].v4l_cam_par.channel, device_v[d]->camera_v[c].v4l_cam_par.norm, device_v[d]->camera_v[c].v4l_cam_par.tuner, device_v[d]->camera_v[c].v4l_cam_par.mode, device_v[d]->camera_v[c].v4l_cam_par.freq, device_v[d]->camera_v[c].v4l_cam_par.width, device_v[d]->camera_v[c].v4l_cam_par.height, device_v[d]->camera_v[c].v4l_cam_par.window_x, device_v[d]->camera_v[c].v4l_cam_par.window_y, device_v[d]->camera_v[c].v4l_cam_par.frame_par.width, device_v[d]->camera_v[c].v4l_cam_par.frame_par.height, device_v[d]->camera_v[c].v4l_cam_par.gray?"Yes.":"No.", device_v[d]->camera_v[c].v4l_cam_par.brightness, device_v[d]->camera_v[c].v4l_cam_par.hue, device_v[d]->camera_v[c].v4l_cam_par.colour, device_v[d]->camera_v[c].v4l_cam_par.contrast, device_v[d]->camera_v[c].v4l_cam_par.whiteness, device_v[d]->camera_v[c].motion_par.algorithm, device_v[d]->camera_v[c].motion_par.half_image?"Yes.":"No.", device_v[d]->camera_v[c].motion_par.deriv_diff_full?"Yes.":"No, partial.", device_v[d]->camera_v[c].motion_par.force_obsolete_sec, device_v[d]->camera_v[c].motion_par.threshold, device_v[d]->camera_v[c].motion_par.noise, device_v[d]->camera_v[c].motion_par.calibrate, (float)device_v[d]->camera_v[c].frame_buffer_size/1024/1024, device_v[d]->camera_v[c].queue_max_frames, device_v[d]->camera_v[c].write_par.font, device_v[d]->camera_v[c].write_par.transparency, device_v[d]->camera_v[c].jpeg_quality, device_v[d]->camera_v[c].avi_framerate, device_v[d]->camera_v[c].evid_xvidpar.quality, device_v[d]->camera_v[c].evid_xvidpar.quant, device_v[d]->camera_v[c].output_dir, device_v[d]->camera_v[c].split_record_sec/60, device_v[d]->camera_v[c].archive_days, device_v[d]->camera_v[c].archive_size, device_v[d]->camera_v[c].dump_raw_video);
    }
  }
  printf("Finish.\n\n");

  // open log stream
  printf("Verbose level set to %d.\n", verbose);
  switch(verbose) {
    case LOG_WARN:
      printf("Only errors and warnings will be reported.\n");
      break;
    case LOG_ERR:
      printf("Only errors will be reported.\n");
      break;
    default:
      printf("Every action done will be reported (TOO noisy).\n");
      break;
  }
  r=0;
  for(d=0;d<=device_c;d++)
    for(c=0;c<device_v[d]->camera_c;c++)
      if(device_v[d]->camera_v[c].motion_par.calibrate)
        r=1;
  if(log_path==NULL) {
    if(daemon) {
      printf("Logging redirected to '/dev/null', no more messages here.\n\n");
      errno=0;
      if((log_handler=log_create_handler(verbose, r, "/dev/null", &errstr))==NULL) {
        fprintf(stderr, "Error creating log handler: %s\n?", errstr);
        free(errstr);
        exit(1);
      }
    } else {
      printf("Logging to standard output.\n\n");
      if((log_handler=log_create_handler(verbose, r, NULL, &errstr))==NULL) {
        fprintf(stderr, "Error creating log handler: %s\n?", errstr);
        free(errstr);
        exit(1);
      }
    }
  } else {
    printf("Logging to file '%s'. No more messages here.\n\n", log_path);
    if((log_handler=log_create_handler(verbose, r, log_path, &errstr))==NULL) {
      fprintf(stderr, "Error creating log handler: %s\n?", errstr);
      free(errstr);
      exit(1);
    }
  }

  // fork if daemon
  if(daemon) {
    pid_t pid;
    pid=fork();
    if(pid==(pid_t)0);
    else if (pid<(pid_t)0) {
      // The fork failed
      fprintf(stderr, "Failed while trying to fork to run daemon mode.\n");
      return 1;
    } else  {
      // parent
      printf("Daemon started.\n");
      return 0;
    }
  }

  // rename interrupted records
  for(d=0;d<=device_c;d++) {
    glob_t pglob;

    for(c=0;c<device_v[d]->camera_c;c++) {
      // we must do this trick to escape from wildcards in filename
      for(r=0;r<(int)strlen(device_v[d]->camera_v[c].name);r++) {
        cam_name[r*2]='\\';
        cam_name[r*2+1]=device_v[d]->camera_v[c].name[r];
      }
      cam_name[r*2]='\0';
      x_asprintf(&filename, "%s/%s/%s [0-9][0-9][0-9][0-9].[0-9][0-9].[0-9][0-9] [0-9][0-9]:[0-9][0-9]:[0-9][0-9] - %s.avi", device_v[d]->conf_path, device_v[d]->camera_v[c].output_dir, rec_prefix, cam_name);

      // look for files
      errno=0;
      r=glob(filename, GLOB_ERR, NULL, &pglob);
      free(filename);
      if(r&&r!=GLOB_NOMATCH)
        log_printf(log_handler, LOG_ERR, "Error listing directory '%s': %s", device_v[d]->camera_v[c].output_dir, strerror_r(errno, buffer, BUFFER_SIZE));
      else {
        int f;
        // rename each one found
        for(f=0;f<(int)pglob.gl_pathc;f++) {
          int p;
          int i;
          char *dest;
          log_printf(log_handler, LOG_WARN, "Found interrupted recording from last session: '%s'.", pglob.gl_pathv[f]);
          // gen dest filename
          x_asprintf(&filename, "%s", pglob.gl_pathv[f]);
          for(p=strlen(filename)-1;p>-1;p--)
            if(filename[p]=='/')
              break;
          for(i=p+(int)strlen(rec_prefix)+2;i<(int)strlen(filename);i++)
            filename[i-strlen(rec_prefix)-1]=filename[i];
          filename[i-strlen(rec_prefix)-5]='\0';
          x_asprintf(&dest, "%s %s.avi", filename, rec_int_sufix);

          // move file
          errno=0;
          if(rename(pglob.gl_pathv[f], dest))
            log_printf(log_handler, LOG_ERR, "Error renaming file '%s' to '%s': %s", pglob.gl_pathv[f], filename, strerror_r(errno, buffer, BUFFER_SIZE));
          free(filename);
          free(dest);
        }
        globfree(&pglob);
      }
    }
  }

  // init things
  for(d=0;d<=device_c;d++) {
    // v4l handler
    device_v[d]->v4l_handler=v4l_create_handler(device_v[d]->video_dev);
    // add cameras
    for(c=0;c<device_v[d]->camera_c;c++) {
      v4l_add_cam(device_v[d]->v4l_handler, &device_v[d]->camera_v[c].v4l_cam_ident, &device_v[d]->camera_v[c].v4l_cam_par);
    }
    // init dev
    log_printf(log_handler, LOG_ACT, "'%s': Initializing capture device.", device_v[d]->video_dev);
    if(v4l_init_dev(device_v[d]->v4l_handler)) {
      log_printf(log_handler, LOG_ERR, "'%s': Error intializing device: %s", device_v[d]->video_dev, v4l_strerror(device_v[d]->v4l_handler));
      exit(1);
    }
    // init queue
    log_printf(log_handler, LOG_ACT, "'%s': Initializing queue.", device_v[d]->video_dev);
    device_v[d]->queue=queue_create_handler();
    for(c=0;c<device_v[d]->camera_c;c++) {
      log_printf(log_handler, LOG_ACT, "'%s', '%s': Adding queue.", device_v[d]->video_dev, device_v[d]->camera_v[c].name);
      device_v[d]->camera_v[c].queue_ident=queue_add(device_v[d]->queue, device_v[d]->camera_v[c].queue_max_frames, &device_v[d]->camera_v[c].v4l_cam_par.frame_par);
    }
    // open dump file
    for(c=0;c<device_v[d]->camera_c;c++) {
      if(device_v[d]->camera_v[c].dump_raw_video[0]!='\0') {
        log_printf(log_handler, LOG_ACT, "'%s', '%s': Opening file '%s' for raw video dumping.", device_v[d]->video_dev, device_v[d]->camera_v[c].name, device_v[d]->camera_v[c].dump_raw_video);
        errno=0;
        device_v[d]->camera_v[c].dump_raw_video_stream=fopen(device_v[d]->camera_v[c].dump_raw_video, "a");
        if(device_v[d]->camera_v[c].dump_raw_video_stream==NULL) {
          log_printf(log_handler, LOG_ERR, "'%s', '%s': Error opening file '%s' for raw video dump: %s.\n", device_v[d]->video_dev, device_v[d]->camera_v[c].name, device_v[d]->camera_v[c].dump_raw_video, strerror(errno));
          exit(1);
        }
      }
    }
    // start motion handler
    for(c=0;c<device_v[d]->camera_c;c++) {
      log_printf(log_handler, LOG_ACT, "'%s', '%s': Initializing motion detection.", device_v[d]->video_dev, device_v[d]->camera_v[c].name);
      device_v[d]->camera_v[c].motion_handler=motion_create_handler(&device_v[d]->camera_v[c].motion_par, &device_v[d]->camera_v[c].frame_par);
    }
    // start write handler
    for(c=0;c<device_v[d]->camera_c;c++)
      device_v[d]->camera_v[c].write_handler=write_create_handler(&device_v[d]->camera_v[c].write_par, &device_v[d]->camera_v[c].frame_par);
    // empty video handler
    for(c=0;c<device_v[d]->camera_c;c++)
      device_v[d]->camera_v[c].evid_handler=NULL;
  }

  // initialize video encoding module
  if(evid_init(&errstr)) {
    log_printf(log_handler, LOG_ERR, "Error initalizing video encoding module: %s", errstr);
    free(errstr);
    exit(1);
  }

  // initialize time vars
  tzset();

  // initialize quit mutex
  pthread_mutex_init(&capture_lock, NULL);

  // set a clean exit
  errno=0;
  if(atexit(atexit_handler))
    error(1, errno, "Error calling atexit().\n");
  // and make clean exit with some signals too
  errno=0;
  if(SIG_ERR==signal(SIGTERM, signal_handler))
    error(1, errno, "Error calling signal().\n");
  errno=0;
  if(SIG_ERR==signal(SIGINT, signal_handler))
    error(1, errno, "Error calling signal().\n");
  errno=0;
  if(SIG_ERR==signal(SIGPIPE, signal_handler))
    error(1, errno, "Error calling signal().\n");
  errno=0;
  if(SIG_ERR==signal(SIGHUP, signal_handler))
    error(1, errno, "Error calling signal().\n");

  // start threads
  capture=(pthread_t *)x_malloc((size_t)(sizeof(pthread_t)*(device_c+1)));
  processing=(pthread_t *)x_malloc((size_t)(sizeof(pthread_t)*(device_c+1)));
  for(d=0;d<=device_c;d++) {
    log_printf(log_handler, LOG_ACT, "'%s': Opening capture/processing threads.", device_v[d]->video_dev);
    if(EAGAIN==pthread_create(&capture[d], NULL, &capture_thread, device_v[d])) {
      log_printf(log_handler, LOG_ERR, "'%s': Error creating capture thread: Not enough system resources to create a process for the new thread, or more than PTHREAD_THREADS_MAX threads are already active.", device_v[d]->video_dev);
      exit(1);
    }
    if(EAGAIN==pthread_create(&processing[d], NULL, &processing_thread, device_v[d])) {
      log_printf(log_handler, LOG_ERR, "'%s': Error creating processing thread: Not enough system resources to create a process for the new thread, or more than PTHREAD_THREADS_MAX threads are already active.", device_v[d]->video_dev);
      exit(1);
    }
  }

  log_printf(log_handler, LOG_ACT, "All initialization done.");

  while(1)
    sleep(UINT_MAX);
  // should never reach here
  return 0;
}

/** @}
 */
