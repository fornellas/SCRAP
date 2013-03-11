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
#include "v4l.h"
#define _POSIX_MAPPED_FILES
#include <stdio.h>
#include <linux/videodev.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>

// helper to v4l_print_cap
int print_yn(int yes) {
  if(yes)
    printf("Yes\n");
  else
    printf("No\n");
  return yes;
}

int v4l_print_cap(char *v4l_dev) {
  int fd, c, t;
  struct video_capability v4l_cap;
  struct video_channel v4l_chan;
  struct video_tuner v4l_tuner;

  errno=0;
  fd=open(v4l_dev, O_RDWR);
  if(fd==-1) {
    fprintf(stderr, "  Error opening device: %s\n", strerror(errno));
    return 1;
  }
  // get capabilities
  errno=0;
  if(ioctl(fd, VIDIOCGCAP, &v4l_cap)==-1) {
    fprintf(stderr,"  Error calling capability query: %s\n", strerror(errno));
    errno=0;
    if(close(fd)==-1) {
        fprintf(stderr, "  Error closing file '%s': %s\n",v4l_dev,strerror(errno) );
    }
    return 1;
  }
  // print card capabiities
  printf("Device name:                                  %s\n",v4l_cap.name);
  printf("Can capture to memory:                        ");
  if(!print_yn(v4l_cap.type&VID_TYPE_CAPTURE))
    printf("Warning: This card is not capable of capturing frames. You can only use it to watch TV!\n");
  printf("Has a tuner of some form:                     ");
  print_yn(v4l_cap.type&VID_TYPE_TUNER);
  printf("Has teletext capability:                      ");
  print_yn(v4l_cap.type&VID_TYPE_TELETEXT);
  printf("Can overlay its image onto the frame buffer:  ");
  print_yn(v4l_cap.type&VID_TYPE_OVERLAY);
  printf("Overlay is Chromakeyed:                       ");
  print_yn(v4l_cap.type&VID_TYPE_CHROMAKEY);
  printf("Overlay clipping is supported:                ");
  print_yn(v4l_cap.type&VID_TYPE_CLIPPING);
  printf("Overlay overwrites frame buffer memory:       ");
  print_yn(v4l_cap.type&VID_TYPE_FRAMERAM);
  printf("The hardware supports image scaling:          ");
  print_yn(v4l_cap.type&VID_TYPE_SCALES);
  printf("Image capture is grey scale only:             ");
  print_yn(v4l_cap.type&VID_TYPE_MONOCHROME);
  printf("Capture can be of only part of the image:     ");
  print_yn(v4l_cap.type&VID_TYPE_SUBCAPTURE);
  printf("Number of radio/tv channels:                  %d\n", v4l_cap.channels);
  printf("Number of audio devices:                      %d\n",v4l_cap.audios);
  printf("Maximum capture width:                        %d\n",v4l_cap.maxwidth);
  printf("Maximum capture height:                       %d\n",v4l_cap.maxheight);
  printf("Minimum capture width:                        %d\n",v4l_cap.minwidth);
  printf("Minimum capture height:                       %d\n",v4l_cap.minheight);
  // print channel information
  for(c=0;c<v4l_cap.channels;c++) {
    // set input
    v4l_chan.channel=c;
    // get information
    errno=0;
    if(ioctl(fd, VIDIOCGCHAN, &v4l_chan)==-1) {
      fprintf(stderr, "  Error getting channel %d proprieties: %s\n", c, strerror(errno));
      errno=0;
      if(close(fd)==-1) {
          fprintf(stderr, "  Error closing file '%s': %s\n",v4l_dev,strerror(errno) );
      }
      return 1;
    }
    // print info
    printf("Channel:                                      %d\n",v4l_chan.channel);
    printf("  Name:                                       %s\n",v4l_chan.name);
    printf("  Tuners:                                     %d\n", v4l_chan.tuners);
    // get info
    if(v4l_chan.tuners)
      for(t=0; t<v4l_chan.tuners; t++) {
        // set tuner
        v4l_tuner.tuner=t;
        // get info
        errno=0;
        if(ioctl(fd, VIDIOCGTUNER, &v4l_tuner)==-1) {
          fprintf(stderr, "  Error getting tuner information: %s\n", strerror(errno));
          errno=0;
          if(close(fd)==-1) {
              fprintf(stderr, "  Error closing file '%s': %s\n",v4l_dev,strerror(errno) );
          }
          return 1;
        }
        // print info
        printf("    Number of the tuner:                      %d\n", v4l_tuner.tuner);
        printf("    Canonical name:                           %s\n", v4l_tuner.name);
        printf("    Lowest tunable frequency:                 %ld\n", v4l_tuner.rangelow);
        printf("    Highest tunable frequency:                %ld\n", v4l_tuner.rangehigh);        printf("    PAL tuning is supported:                  ");
        print_yn(v4l_tuner.flags&VIDEO_TUNER_PAL);
        printf("    NTSC tuning is supported:                 ");
        print_yn(v4l_tuner.flags&VIDEO_TUNER_NTSC);
        printf("    SECAM tuning is supported:                ");
        print_yn(v4l_tuner.flags&VIDEO_TUNER_SECAM);
        printf("    Frequency is in a lower range:            ");
        print_yn(v4l_tuner.flags&VIDEO_TUNER_LOW);
        printf("    The norm for this tuner is settable:      ");
        print_yn(v4l_tuner.flags&VIDEO_TUNER_NORM);
        printf("    The tuner is seeing stereo audio:         ");
        print_yn(v4l_tuner.flags&VIDEO_TUNER_STEREO_ON);
        printf("    The tuner is seeing a RDS datastream:     ");
        print_yn(v4l_tuner.flags&VIDEO_TUNER_RDS_ON);
        printf("    The tuner is seeing a MBS datastream:     ");
        print_yn(v4l_tuner.flags&VIDEO_TUNER_MBS_ON);
      }
    //printf("  Channel has tuners:                         ");
    //print_yn(v4l_chan.flags&VIDEO_VC_TUNER);
    printf("  Channel has audio:                          ");
    print_yn(v4l_chan.flags&VIDEO_VC_AUDIO);
    // missing in driver... but exists in documentation...
    //printf("  Channel has norm setting:                   ");
    //print_yn(v4l_chan.flags&VIDEO_VC_NORM);
    if(v4l_chan.type&VIDEO_TYPE_TV)
      printf("  The input is a TV input\n");
    if(v4l_chan.type&VIDEO_TYPE_CAMERA)
      printf("  The input is a camera\n");
  }
  // TODO Audio capture information
  errno=0;
  if(close(fd)==-1) {
	    fprintf(stderr, "  Error closing file '%s': %s\n",v4l_dev,strerror(errno) );
    return 1;
  }
  return 0;
}

static char dev[]="/dev/video";

void v4l_dev_set_default(char **v4l_dev) {
  (*v4l_dev)=dev;
}

#define MAX_ERROR_MSG_LENGTH 256

typedef struct {
  v4l_cam_par_t *v4l_cam_par;
  int n_cam;
  char *v4l_dev;
  int fd;
  char *map;
  v4l_cam_ident_t camera_last;
  struct video_mbuf vid_mbuf;
  struct video_mmap vid_mmap;
  int buffer_pos;
  char *errstr;
  char errbuf[MAX_ERROR_MSG_LENGTH];
} handler_t;

v4l_handler_t *v4l_create_handler(char *v4l_dev) {
  handler_t *handler;

  handler=(handler_t *)x_malloc(sizeof(handler_t));

  handler->v4l_cam_par=NULL;
  handler->n_cam=-1;

  handler->v4l_dev=(char *)x_malloc(strlen(v4l_dev)+1);
  strcpy(handler->v4l_dev, v4l_dev);

  handler->camera_last=-1;

  handler->errstr=NULL;

  return (v4l_handler_t *)handler;
}

void v4l_cam_set_default(v4l_cam_par_t *v4l_cam_par) {
  v4l_cam_par->width=352;
  v4l_cam_par->height=288;
  v4l_cam_par->frame_par.width=0;
  v4l_cam_par->frame_par.height=0;
  v4l_cam_par->window_x=0;
  v4l_cam_par->window_y=0;
  v4l_cam_par->channel=0;
  v4l_cam_par->norm=3;
  v4l_cam_par->tuner=-1;
  v4l_cam_par->mode=3;
  v4l_cam_par->freq=0;
  v4l_cam_par->gray=0;
  v4l_cam_par->brightness=32768;
  v4l_cam_par->hue=32768;
  v4l_cam_par->colour=32512;
  v4l_cam_par->contrast=27648;
  v4l_cam_par->whiteness=0;
}

void v4l_add_cam(v4l_handler_t *v4l_handler, v4l_cam_ident_t *v4l_cam_ident, v4l_cam_par_t *v4l_cam_par) {
  handler_t *handler=(handler_t *)v4l_handler;

  handler->n_cam++;
  handler->v4l_cam_par=(v4l_cam_par_t *)x_realloc(handler->v4l_cam_par, (size_t)(sizeof(v4l_cam_par_t)*(handler->n_cam+1)));

  memcpy(&(handler->v4l_cam_par[handler->n_cam]), v4l_cam_par, sizeof(v4l_cam_par_t));

  (*v4l_cam_ident)=handler->n_cam;
}

// Set v4l capture to specifyed camera
int v4l_set_cam(v4l_handler_t *v4l_handler, v4l_cam_ident_t v4l_cam_ident) {
  handler_t *handler=(handler_t *)v4l_handler;
  struct video_channel v4l_chan;
  struct video_tuner v4l_tun;
  struct video_picture v4l_pict;

  if(handler->camera_last!=v4l_cam_ident) {
    if(v4l_cam_ident>handler->n_cam||v4l_cam_ident<0) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Requested camera %d not configured!\n", v4l_cam_ident);
      return 1;
    }
    handler->camera_last=v4l_cam_ident;
    /* video source
     */
    memset(&v4l_chan, 0, sizeof(struct video_channel));
    v4l_chan.channel=handler->v4l_cam_par[v4l_cam_ident].channel;
    switch(handler->v4l_cam_par[v4l_cam_ident].norm) {
      case 0:
        v4l_chan.norm=VIDEO_MODE_PAL;
        break;
      case 1:
        v4l_chan.norm=VIDEO_MODE_NTSC;
        break;
      case 2:
        v4l_chan.norm=VIDEO_MODE_SECAM;
        break;
      case 3:
        v4l_chan.norm=VIDEO_MODE_AUTO;
        break;
      default:
        return 3;
    }
    // set input
    errno=0;
    if(ioctl(handler->fd, VIDIOCSCHAN, &v4l_chan)==-1) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Fail to change to channel %d: %s\n", handler->v4l_cam_par[v4l_cam_ident].channel, strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
      return 1;
    }
    /* tuner
     */
    if(handler->v4l_cam_par[v4l_cam_ident].tuner!=-1) {
      // set tunner
      v4l_tun.tuner=handler->v4l_cam_par[v4l_cam_ident].tuner;
      v4l_tun.mode=handler->v4l_cam_par[v4l_cam_ident].mode;
      errno=0;
      if(ioctl(handler->fd, VIDIOCSTUNER, &v4l_tun)==-1) {
        free(handler->errstr);
        x_asprintf(&handler->errstr, "Fail to change/set tuner: %s\n", strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
        return 1;
      }
      /* frequency switch
       */
      if(handler->v4l_cam_par[v4l_cam_ident].freq) {
        // set frequency
        errno=0;
        if(ioctl(handler->fd, VIDIOCSFREQ, &handler->v4l_cam_par[v4l_cam_ident].freq)==-1) {
          free(handler->errstr);
          x_asprintf(&handler->errstr, "Fail to set frequency: %s\n", strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
          return 1;
        }
      }
    }
    /* Image proprieties
     */
    v4l_pict.brightness=handler->v4l_cam_par[v4l_cam_ident].brightness;
    v4l_pict.hue=handler->v4l_cam_par[v4l_cam_ident].hue;
    v4l_pict.colour=handler->v4l_cam_par[v4l_cam_ident].colour;
    v4l_pict.contrast=handler->v4l_cam_par[v4l_cam_ident].contrast;
    v4l_pict.whiteness=handler->v4l_cam_par[v4l_cam_ident].whiteness;
    // do not know about this, but keep 0 to make it work...
    v4l_pict.depth=PALETTE_DEPTH*8;
    v4l_pict.palette=handler->v4l_cam_par[v4l_cam_ident].palette;
    // set things
    errno=0;
    if(ioctl(handler->fd, VIDIOCSPICT, &v4l_pict)==-1) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Fail to change image proprieties.\n");
      return 1;
    }
  }
  return 0;
}

// Check v4l camera settings
int v4l_check_cam_settings(v4l_handler_t *v4l_handler, v4l_cam_ident_t v4l_cam_ident) {
  handler_t *handler=(handler_t *)v4l_handler;
  struct video_channel v4l_chan;
  struct video_tuner v4l_tun;
  struct video_picture v4l_pict;
  unsigned long freq;

  /* video source
   */
  // get settings
  memset(&v4l_chan, 0, sizeof(struct video_channel));
  v4l_chan.channel=handler->v4l_cam_par[v4l_cam_ident].channel;
  errno=0;
  if(ioctl(handler->fd, VIDIOCGCHAN, &v4l_chan)==-1) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Fail to get video source parameters.\n");
    return 1;
  }
  // check
  if(v4l_chan.channel!=handler->v4l_cam_par[v4l_cam_ident].channel) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Card did not changed channel: requested %d, got %d.\n", handler->v4l_cam_par[v4l_cam_ident].channel, v4l_chan.channel);
    return 1;
  }
  if(v4l_chan.norm!=handler->v4l_cam_par[v4l_cam_ident].norm) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Card did not changed norm: requested %d, got %d.\n", handler->v4l_cam_par[v4l_cam_ident].norm, v4l_chan.norm);
    return 1;
  }
  /* tuner
   */
  if(handler->v4l_cam_par[v4l_cam_ident].tuner!=-1) {
    // check if channel has a tuner
    if(v4l_chan.tuners<1) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Request to set tuner for this channel, but channel do not have a tuner.\n");
      return 1;
    }
    // check if channel has asked tuner number
    if(handler->v4l_cam_par[v4l_cam_ident].tuner>=v4l_chan.tuners) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Request to set tuner number %d for this channel, but channel do not have such tuner.\n", handler->v4l_cam_par[v4l_cam_ident].tuner);
      return 1;
    }
    // get settings
    errno=0;
    if(ioctl(handler->fd, VIDIOCGTUNER, &v4l_tun)==-1) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Fail to get tuner information: %s\n", strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
      return 1;
    }
    // check settings
    if(v4l_tun.tuner!=handler->v4l_cam_par[v4l_cam_ident].tuner) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Card did switch to specified tuner: requested %d, got %d.\n", handler->v4l_cam_par[v4l_cam_ident].tuner, v4l_tun.tuner );
      return 1;
    }
    if(v4l_tun.mode!=handler->v4l_cam_par[v4l_cam_ident].mode) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Tuner %d did not switch to requested mode: requested %d, got %d.\n", handler->v4l_cam_par[v4l_cam_ident].tuner, handler->v4l_cam_par[v4l_cam_ident].tuner, v4l_tun.tuner );
      return 1;
    }
    /* frequency switch
     */
    if(handler->v4l_cam_par[v4l_cam_ident].freq) {
      // no frequency tuning allowed
      if(v4l_tun.rangehigh>v4l_tun.rangelow) {
        free(handler->errstr);
        x_asprintf(&handler->errstr, "Asked to tune frequency, but v4l driver says board's highest tunable frequency is lower than board's lowest tunable frequency. Probably this tuner do not support frequency tuning.\n");
        return 1;
      }
      // frequency out of range
      if(handler->v4l_cam_par[v4l_cam_ident].freq<v4l_tun.rangelow||handler->v4l_cam_par[v4l_cam_ident].freq>v4l_tun.rangehigh) {
        free(handler->errstr);
        x_asprintf(&handler->errstr, "Frequency out of range: requested %ld, range is %ld to %ld.\n", handler->v4l_cam_par[v4l_cam_ident].freq, v4l_tun.rangelow, v4l_tun.rangehigh);
        return 1;
      }
      // get frequency
      errno=0;
      if(ioctl(handler->fd, VIDIOCGFREQ, &freq)==-1) {
        free(handler->errstr);
        x_asprintf(&handler->errstr, "Fail to get frequency: %s\n", strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
        return 1;
      }
      // check setting
      if(freq!=handler->v4l_cam_par[v4l_cam_ident].freq) {
        free(handler->errstr);
        x_asprintf(&handler->errstr, "Tuner %d did not switch frequency: requested %ld, got %ld.\n", handler->v4l_cam_par[v4l_cam_ident].tuner, handler->v4l_cam_par[v4l_cam_ident].freq, freq);
        return 1;
      }
    }
  }
  /* Image proprieties
   */
  // get settings
  errno=0;
  if(ioctl(handler->fd, VIDIOCGPICT, &v4l_pict)==-1) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Fail to get image proprieties.\n");
    return 1;
  }
  // check settings
  if(v4l_pict.brightness!=handler->v4l_cam_par[v4l_cam_ident].brightness) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Fail to set brightness: requested %hd, got %hd.\n", handler->v4l_cam_par[v4l_cam_ident].brightness, v4l_pict.brightness);
    return 1;
  }
  if(v4l_pict.hue!=handler->v4l_cam_par[v4l_cam_ident].hue) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Fail to set hue: requested %hd, got %hd.\n", handler->v4l_cam_par[v4l_cam_ident].hue, v4l_pict.hue);
    return 1;
  }
  if(v4l_pict.colour!=handler->v4l_cam_par[v4l_cam_ident].colour) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Fail to set colour: requested %hd, got %hd.\n", handler->v4l_cam_par[v4l_cam_ident].colour, v4l_pict.colour);
    return 1;
  }
  if(v4l_pict.contrast!=handler->v4l_cam_par[v4l_cam_ident].contrast) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Fail to set contrast: requested %hd, got %hd.\n", handler->v4l_cam_par[v4l_cam_ident].contrast, v4l_pict.contrast);
    return 1;
  }
  if(v4l_pict.whiteness!=handler->v4l_cam_par[v4l_cam_ident].whiteness) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Fail to set whiteness: requested %hd, got %hd.\n", handler->v4l_cam_par[v4l_cam_ident].whiteness, v4l_pict.whiteness);
    return 1;
  }
  // check strange value for depth
  if(v4l_pict.depth!=PALETTE_DEPTH*8) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Unexpected value in capture depth variable: %hd. Internal error, contact the author.\n", v4l_pict.depth);
    return 1;
  }
  // palette
  if(handler->v4l_cam_par[v4l_cam_ident].palette!=v4l_pict.palette) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Fail to set palette: requested %hd, got %hd.\n", handler->v4l_cam_par[v4l_cam_ident].palette, v4l_pict.palette);
    return 1;
  }
  return 0;
}

int v4l_init_dev(v4l_handler_t *v4l_handler) {
  handler_t *handler=(handler_t *)v4l_handler;
  int i;
  struct video_capability v4l_cap;

  // open video device file descriptor
  errno=0;
  handler->fd=open(handler->v4l_dev, O_RDWR);
  if(handler->fd==-1) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Error opening device: %s\n", strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
    return 1;
  }
  // check if card can capture
  // get capabilities
  errno=0;
  if(ioctl(handler->fd, VIDIOCGCAP, &v4l_cap)==-1) {
    free(handler->errstr);
    x_asprintf(&handler->errstr,"Capability query failed: %s\n", strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
    errno=0;
    if(close(handler->fd)==-1) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Fail to close file '%s': %s\n", handler->v4l_dev,strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH) );
    }
    return 1;
  }
  // check capture
  if(!(v4l_cap.type&VID_TYPE_CAPTURE)) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "This card is not capable of capturing frames. You can only use it to watch TV!\n");
    errno=0;
    if(close(handler->fd)==-1) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Fail to close file '%s': %s\n", handler->v4l_dev,strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH) );
    }
    return 1;
  }
  // check palette && frame size
  for(i=0;i<=handler->n_cam;i++) {
    // palette
    if(v4l_cap.type&VID_TYPE_MONOCHROME||handler->v4l_cam_par[i].gray) {
      if(!handler->v4l_cam_par[i].gray)
        free(handler->errstr);
        x_asprintf(&handler->errstr, "Asked for colored capture on camera %d, but device is monochrome!\n", i);
    }
    handler->v4l_cam_par[i].palette=VIDEO_PALETTE_YUV422;
    // frame size
    if(handler->v4l_cam_par[i].width>v4l_cap.maxwidth    ||handler->v4l_cam_par[i].width<v4l_cap.minwidth    ||handler->v4l_cam_par[i].height>v4l_cap.maxheight    ||handler->v4l_cam_par[i].height<v4l_cap.minheight) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Asked frame size (%dx%d) on camera %d is out of range device support (%dx%d - %dx%d).\n", handler->v4l_cam_par[i].width, handler->v4l_cam_par[i].height, i, v4l_cap.minwidth, v4l_cap.minheight, v4l_cap.maxwidth, v4l_cap.maxheight);
      errno=0;
      if(close(handler->fd)==-1) {
        free(handler->errstr);
        x_asprintf(&handler->errstr, "Fail to close file '%s': %s\n", handler->v4l_dev,strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH) );
      }
      return 1;
    }
    // camera settings
    if(v4l_set_cam(handler, i)) {
      errno=0;
      if(close(handler->fd)==-1) {
        free(handler->errstr);
        x_asprintf(&handler->errstr, "Fail to close file '%s': %s\n", handler->v4l_dev,strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH) );
      }
      return 1;
    }
    if(v4l_check_cam_settings(handler, i)) {
      errno=0;
      if(close(handler->fd)==-1) {
        free(handler->errstr);
        x_asprintf(&handler->errstr, "Fail to close file '%s': %s\n", handler->v4l_dev,strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH) );
      }
      return 1;
    }
  }
  // map device
  if(ioctl(handler->fd, VIDIOCGMBUF, &handler->vid_mbuf)==-1) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Device do not support mmap() interface to capture images. You might have an obsolete board/driver.\n");
    errno=0;
    if(close(handler->fd)==-1) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Fail to close file '%s': %s\n", handler->v4l_dev,strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH) );
    }
    return 1;
  }
  if(handler->vid_mbuf.frames<1) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Device do not support at least 1 frame in buffer!\n");
    errno=0;
    if(close(handler->fd)==-1) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Fail to close file '%s': %s\n", handler->v4l_dev,strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH) );
    }
    return 1;
  }

  if((handler->map=(char *)mmap(0, handler->vid_mbuf.size, PROT_READ|PROT_WRITE, MAP_SHARED, handler->fd, 0))==MAP_FAILED) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "mmap() failed!\n");
    errno=0;
    if(close(handler->fd)==-1) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Fail to close file '%s': %s\n", handler->v4l_dev,strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH) );
    }
    return 1;
  }
  handler->buffer_pos=-1;
  return 0;
}

int v4l_start_cap(v4l_handler_t *v4l_handler, v4l_cam_ident_t v4l_cam_ident) {
  handler_t *handler=(handler_t *)v4l_handler;
  int b;

  // no double call
  if(handler->buffer_pos!=-1) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "v4l_start_cap() should be called only once.\n");
    return 1;
  }

  // Switch camera
  if(v4l_set_cam(handler, v4l_cam_ident))
    return 1;

  handler->vid_mmap.width=handler->v4l_cam_par[v4l_cam_ident].width;
  handler->vid_mmap.height=handler->v4l_cam_par[v4l_cam_ident].height;
  handler->vid_mmap.format=handler->v4l_cam_par[v4l_cam_ident].palette;

  // start capture to buffer
  for(b=0;b<handler->vid_mbuf.frames;b++) {
    handler->vid_mmap.frame=b;
    errno=0;
    if(ioctl(handler->fd, VIDIOCMCAPTURE, &handler->vid_mmap)==-1) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Fail to start capture: %s\n", strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
      return 1;
    }
    // if more than one camera, lets stop at the first frame
    if(handler->n_cam)
      break;
  }
  handler->buffer_pos=0;

  // set last camera
  handler->camera_last=v4l_cam_ident;

  return 0;
}

int v4l_sync_frame(v4l_handler_t *v4l_handler, v4l_cam_ident_t v4l_cam_ident_next, pixel_t *frame) {
  handler_t *handler=(handler_t *)v4l_handler;
  int old_pos=handler->buffer_pos;
  int h, m, f;

  // sync current frame
  errno=0;
  if(ioctl(handler->fd, VIDIOCSYNC, &handler->buffer_pos)==-1) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Fail to sync frame: %s\n", strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
    return 1;
  }

  // if more than one camera and buffer avaliable, we start next capture
  if(handler->n_cam&&handler->vid_mbuf.frames>1) {
    // Switch camera
    if(v4l_set_cam(handler, v4l_cam_ident_next))
      return 1;
    handler->buffer_pos=handler->buffer_pos?0:1;
    handler->vid_mmap.frame=handler->buffer_pos;
    errno=0;
    if(ioctl(handler->fd, VIDIOCMCAPTURE, &handler->vid_mmap)==-1) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Fail to start capture: %s\n", strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
      return 1;
    }
  }

  // crop frame
  if(frame!=NULL) {
    m=handler->v4l_cam_par[handler->camera_last].width*handler->v4l_cam_par[handler->camera_last].window_y*PALETTE_DEPTH;
    f=0;
    for(h=0; h<handler->v4l_cam_par[handler->camera_last].frame_par.height;h++) {
      // copy line
      m+=handler->v4l_cam_par[handler->camera_last].window_x*PALETTE_DEPTH;
      memcpy(&frame[f], &handler->map[m+handler->vid_mbuf.offsets[old_pos]], (size_t)handler->v4l_cam_par[handler->camera_last].frame_par.width*PALETTE_DEPTH);
      m+=(handler->v4l_cam_par[handler->camera_last].width-handler->v4l_cam_par[handler->camera_last].window_x)*PALETTE_DEPTH;
      f+=handler->v4l_cam_par[handler->camera_last].frame_par.width*PALETTE_DEPTH;
    }
  }

  // if we had not enought buffer, start next capture now
  if(handler->n_cam) {
    if(handler->vid_mbuf.frames<=1) {
      // Switch camera
      if(v4l_set_cam(handler, v4l_cam_ident_next))
        return 1;
      // start capture
      handler->vid_mmap.frame=handler->buffer_pos;
      errno=0;
      if(ioctl(handler->fd, VIDIOCMCAPTURE, &handler->vid_mmap)==-1) {
        free(handler->errstr);
        x_asprintf(&handler->errstr, "Fail to start capture: %s\n", strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
        return 1;
      }
    }
  // if only one camera
  } else {
    handler->vid_mmap.frame=handler->buffer_pos;
    errno=0;
    if(ioctl(handler->fd, VIDIOCMCAPTURE, &handler->vid_mmap)==-1) {
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Fail to start capture: %s\n", strerror_r(errno, handler->errbuf, MAX_ERROR_MSG_LENGTH));
      return 1;
    }
    handler->buffer_pos=handler->buffer_pos==handler->vid_mbuf.frames-1?0:handler->buffer_pos+1;
  }

  // set last cam
  handler->camera_last=v4l_cam_ident_next;

  return 0;
}

int v4l_destroy_handler(v4l_handler_t *v4l_handler) {
  handler_t *handler=(handler_t *)v4l_handler;
  free(handler->v4l_cam_par);
  free(handler->v4l_dev);
  free(handler->errstr);
  return 1;
}

char *v4l_strerror(v4l_handler_t *v4l_handler) {
  handler_t *handler=(handler_t *)v4l_handler;
  return handler->errstr;
}
