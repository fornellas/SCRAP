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

#include "motion.h"
#include "common.h"
#include <string.h>
#include <time.h>

// remover
#include <stdio.h>

typedef struct {
  motion_par_t motion_par;
  frame_par_t frame_par;
  pixel_t *frame_old;
  pixel_t *frame_deriv;
  time_t last_motion;
} handler_t;

void motion_par_set_defaults(motion_par_t *motion_par) {
  motion_par->algorithm=1;
  motion_par->half_image=0;
  motion_par->deriv_diff_full=1;
  motion_par->threshold=0.5;
  motion_par->noise=10;
  motion_par->force_obsolete_sec=0;
  motion_par->calibrate=0;
}

motion_handler_t *motion_create_handler(motion_par_t *motion_par, frame_par_t *frame_par) {
  handler_t *handler;

  // allocate handler
  handler=(handler_t *)x_malloc(sizeof(handler_t));

  // copy parameters
  memcpy(&handler->motion_par, motion_par, sizeof(motion_par_t));
  handler->motion_par.threshold/=100;
  memcpy(&handler->frame_par, frame_par, sizeof(frame_par_t));

  if(handler->motion_par.algorithm) {
    // allocate old frame
    handler->frame_old=(pixel_t *)x_malloc((size_t)(frame_par->size));
    // and derivate frame
    if(motion_par->algorithm)
      handler->frame_deriv=(pixel_t *)x_malloc((size_t)(frame_par->size));
  }

  // long time ago...
  handler->last_motion=0;

  // return handler
  return (motion_handler_t *)handler;
}

// derivate frame and store at frame_deriv
void derivate_frame(pixel_t *frame, pixel_t *frame_deriv, frame_par_t *frame_par, int half, int full) {
  int x;
  int y;

  if(full)
    for(x=1;x<=frame_par->width;x+=half?2:1) {
      for(y=1;y<=frame_par->height;y++) {
        int p;
        int sum=0;
        int t=0;
        pixel_t c;
        pixel_t e;
        pixel_t *d;

        p=((y-1)*frame_par->width+x-1)*2;
        c=frame[p];
        d=&frame[p];
        if(x>1&&y>1) {
          e=*(d-(frame_par->width-1)*2);
          sum+=c>e?c-e:e-c;
          t++;
        }
        if(y>1) {
          e=*(d-frame_par->width*2);
          sum+=c>e?c-e:e-c;
          t++;
        }
        if(x<frame_par->width&&y>1) {
          e=*(d-(frame_par->width-1)*2);
          sum+=c>e?c-e:e-c;
          t++;
        }
        if(x>1) {
          e=*(d-2);
          sum+=c>e?c-e:e-c;
          t++;
        }
        if(x<frame_par->width) {
          e=*(d+2);
          sum+=c>e?c-e:e-c;
          t++;
        }
        if(x>1&&y<frame_par->height) {
          e=*(d+(frame_par->width-1)*2);
          sum+=c>e?c-e:e-c;
          t++;
        }
        if(y<frame_par->height) {
          e=*(d+(frame_par->width)*2);
          sum+=c>e?c-e:e-c;
          t++;
        }
        if(x<frame_par->width&&y<frame_par->height) {
          e=*(d+(frame_par->width)*2);
          sum+=c>e?c-e:e-c;
          t++;
        }
        frame_deriv[p]=(pixel_t)(sum/t);
        frame_deriv[p+1]=100;
      }
    }
  else
    for(x=1;x<=frame_par->width;x+=half?2:1) {
      for(y=1;y<=frame_par->height;y++) {
        int p;
        int sum=0;
        pixel_t c;
        pixel_t e;
        pixel_t *d;

        p=((y-1)*frame_par->width+x-1)*2;
        c=frame[p];
        d=&frame[p];
        if(y>1) {
          e=*(d-frame_par->width*2);
          sum+=c>e?c-e:e-c;
        } else {
          e=*(d+(frame_par->width)*2);
          sum+=c>e?c-e:e-c;
        }
        if(x<frame_par->width) {
          e=*(d+2);
          sum+=c>e?c-e:e-c;
        } else {
          e=*(d-2);
          sum+=c>e?c-e:e-c;
        }
        frame_deriv[p]=(pixel_t)(sum/2);
        frame_deriv[p+1]=100;
      }
    }
}

int motion_detect(motion_handler_t *motion_handler, pixel_t *frame, time_t cap_time, double *area_changed, pixel_t *frame_calibrate) {
  handler_t *handler=(handler_t *)motion_handler;
  pixel_t *frame_process;
  int p;
  int pixels_changed=0;
  double area;
  int motion;
  int obsolete;

  // motion, if no motion detection
  if(!handler->motion_par.algorithm)
    return 1;

  // if first call, lets set initial values
  if(handler->last_motion==0) {
    handler->last_motion=cap_time;
    if(handler->motion_par.algorithm==2)
      derivate_frame(frame, handler->frame_old, &handler->frame_par, handler->motion_par.half_image, handler->motion_par.deriv_diff_full);
    else
      memcpy(handler->frame_old, frame, (size_t)handler->frame_par.size);
    // and if calibrate
    if(handler->motion_par.calibrate) {
      for(p=0;p<handler->frame_par.size;p+=2) {
        frame_calibrate[p]=0;
        frame_calibrate[p+1]=128;
      }
      (*area_changed)=0;
    }
    return 0;
  }

  // derivate input frame
  if(handler->motion_par.algorithm==2) {
    derivate_frame(frame, handler->frame_deriv, &handler->frame_par, handler->motion_par.half_image, handler->motion_par.deriv_diff_full);
    frame_process=handler->frame_deriv;
  // or not
  } else
    frame_process=frame;

  // substract frames
  for(p=0;p<handler->frame_par.size;p+=handler->motion_par.half_image?4:2) {
    pixel_t d;
    int l=0;

    // substract pixels
    d=handler->frame_old[p]>frame_process[p]?handler->frame_old[p]-frame_process[p]:frame_process[p]-handler->frame_old[p];
    // here we have motion
    if(d>=handler->motion_par.noise) {
      pixels_changed++;
      if(handler->motion_par.calibrate==1) {
        frame_calibrate[p]=frame[p];
        frame_calibrate[p+1]=frame[p+1];
        if(handler->motion_par.half_image) {
          frame_calibrate[p+2]=frame[p+2];
          frame_calibrate[p+3]=frame[p+3];
        } else {
          if(p/2%2)
            frame_calibrate[p-1]=frame[p-1];
          else
            frame_calibrate[p+3]=frame[p+3];
          l=0;
        }
      }
    // no motion here
    } else {
      if(handler->motion_par.calibrate==1) {
        frame_calibrate[p]=0;
        if(handler->motion_par.half_image) {
          frame_calibrate[p+1]=128;
          frame_calibrate[p+2]=0;
          frame_calibrate[p+3]=128;
        } else {
          if((p/2%2)&&l) {
            frame_calibrate[p+1]=128;
            frame_calibrate[p-1]=128;
          }
        }
        l=1;
      }
    }
    if(handler->motion_par.calibrate==2) {
      frame_calibrate[p]=d;
      #define CB 50
      #define CR 50
      frame_calibrate[p+1]=p/2%2?CB:CR;
    }
  }

  // store area changed
  area=(double)pixels_changed/(double)(handler->frame_par.size/2);
  if(handler->motion_par.half_image)
    area*=2;
  // export it
  if(area_changed!=NULL)
    (*area_changed)=area*100;

  // motion
  motion=area>=handler->motion_par.threshold;

  // obsolete
  obsolete=0;
  if(handler->motion_par.force_obsolete_sec)
    obsolete=cap_time-handler->last_motion>=handler->motion_par.force_obsolete_sec;

  // store cap_time && copy frame
  if(motion||obsolete) {
    handler->last_motion=cap_time;
    memcpy(handler->frame_old, frame_process, (size_t)handler->frame_par.size);
  }

  // return
  if(motion) {
// output derived frame
// memcpy(frame_calibrate, handler->frame_deriv, (size_t)handler->frame_par.size);
    // auto-brightness to calibrate frame
/*    if(handler->motion_par.calibrate==2) {
      pixel_t max=0;
      float m;
      // find max
      for(p=0;p<handler->frame_par.size;p+=2)
        max=frame_calibrate[p]>max?frame_calibrate[p]:max;
      m=255.0/(float)max;
      // set brightness
      for(p=0;p<handler->frame_par.size;p+=2)
        frame_calibrate[p]=(pixel_t)((float)frame_calibrate[p]*m);
    }*/
    return 1;
  } else
    return 0;
}
