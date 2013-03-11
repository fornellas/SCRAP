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

#include "enc_video.h"
#include "common.h"
#include "avilib.h"
#include "xvid.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

typedef struct {
  frame_par_t frame_par;
  evid_xvidpar_t evid_xvidpar;
  char *filename;
  struct {
    avi_t *avi;
    int open;
  } avilib;
  struct {
    void *handle;
    void *bitstream;
  } xvid;
  char *errstr;
} handler_t;

int evid_init(char **errstr) {
  // XviD global initialization
  xvid_gbl_init_t xvid_gbl_init;
  memset(&xvid_gbl_init, 0, sizeof(xvid_gbl_init));
  xvid_gbl_init.version=XVID_VERSION;
  xvid_gbl_init.debug=0;
  xvid_gbl_init.cpu_flags=0;
  if(xvid_global(NULL, XVID_GBL_INIT, &xvid_gbl_init, NULL)) {
    x_asprintf(errstr, "%s", "Error initializing XviD core.\n");
    return 1;
  }
  return 0;
}

void evid_xvidpar_set_defaults(evid_xvidpar_t *evid_xvidpar) {
  evid_xvidpar->quality=6;
  evid_xvidpar->quant=3;
  evid_xvidpar->gray=0;
}

evid_handler_t *evid_create_handler(frame_par_t *frame_par, evid_xvidpar_t *evid_xvidpar, double framerate, char *filename, char **errstr) {
  xvid_enc_create_t xvid_enc_create;
  handler_t *handler;

  handler=(handler_t *)x_malloc(sizeof(handler_t));
  // frame_par
  memcpy(&(handler->frame_par), frame_par, sizeof(frame_par_t));
  // xvid_par
  memcpy(&(handler->evid_xvidpar), evid_xvidpar, sizeof(evid_xvidpar_t));
  // sets file name
  handler->filename=(char *)x_malloc(strlen(filename)+1);
  strcpy(handler->filename, filename);

  // avilib
  if((handler->avilib.avi=AVI_open_output_file(filename))==NULL) {
    x_asprintf(errstr, "%s.\n", AVI_strerror());
    free(handler);
    return NULL;
  }

  AVI_set_video(handler->avilib.avi, frame_par->width, frame_par->height, framerate, "xvid");
  AVI_set_audio(handler->avilib.avi, 0, 44100, 16, WAVE_FORMAT_UNKNOWN);

  // xvid encoder
  memset(&xvid_enc_create, 0, sizeof(xvid_enc_create));
  xvid_enc_create.profile=XVID_PROFILE_S_L0; // XVID_PROFILE_AS_L4; // do not know abot this...
  xvid_enc_create.version=XVID_VERSION;
  xvid_enc_create.width=frame_par->width;
  xvid_enc_create.height=frame_par->height;
  xvid_enc_create.num_zones=0;
  xvid_enc_create.zones=NULL;
  xvid_enc_create.num_plugins=0;
  xvid_enc_create.plugins=NULL;
  xvid_enc_create.num_threads=0;
  xvid_enc_create.max_bframes=0;
  xvid_enc_create.global=0;
  xvid_enc_create.global|=XVID_GLOBAL_CLOSED_GOP;
  xvid_enc_create.fincr=1000;
  xvid_enc_create.fbase=(int)(framerate*1000);
  xvid_enc_create.max_key_interval=(int)(framerate*10);
  xvid_enc_create.frame_drop_ratio=0;
  xvid_enc_create.bquant_ratio=150;
  xvid_enc_create.bquant_offset=100;
  xvid_enc_create.min_quant[0]=2;
  xvid_enc_create.max_quant[0]=31;

  if(xvid_encore(NULL, XVID_ENC_CREATE, &xvid_enc_create, NULL)) {
    x_asprintf(errstr, "Error initializing XviD encore.\n");
    if(AVI_close(handler->avilib.avi)) {
      free((*errstr));
      x_asprintf(errstr, "Error initializing XviD encore / Error closing AVI file: %s.\n", AVI_strerror());
    }
    unlink(filename); // no error checking here
    free(handler);
    return NULL;
  }
  handler->xvid.handle=xvid_enc_create.handle;
  // bitstream
  handler->xvid.bitstream=x_malloc((size_t)(handler->frame_par.width*handler->frame_par.height*PALETTE_DEPTH));

  // errstr
  handler->errstr=NULL;
  return (evid_handler_t *)handler;
}

int evid_enc_frame(evid_handler_t *evid_handler, pixel_t *frame) {
  xvid_enc_frame_t xvid_enc_frame;
  handler_t *handler=(handler_t *)evid_handler;
  static const int vop_presets[] = {
    0,
    0,
    XVID_VOP_HALFPEL,
    XVID_VOP_HALFPEL | XVID_VOP_INTER4V,
    XVID_VOP_HALFPEL | XVID_VOP_INTER4V,
    XVID_VOP_HALFPEL | XVID_VOP_INTER4V | XVID_VOP_TRELLISQUANT,
    XVID_VOP_HALFPEL | XVID_VOP_INTER4V | XVID_VOP_TRELLISQUANT | XVID_VOP_HQACPRED,
  };
  static const int motion_presets[] = {
    0,
    XVID_ME_ADVANCEDDIAMOND16,
    XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16,
    XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 | XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8,
    XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 | XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 | XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP,
    XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 | XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 | XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP,
    XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_HALFPELREFINE16 | XVID_ME_EXTSEARCH16 | XVID_ME_ADVANCEDDIAMOND8 | XVID_ME_HALFPELREFINE8 | XVID_ME_EXTSEARCH8 | XVID_ME_CHROMA_PVOP | XVID_ME_CHROMA_BVOP,
  };
  int frame_size;

  // XviD
  memset(&xvid_enc_frame, 0, sizeof(xvid_enc_frame));
  xvid_enc_frame.version=XVID_VERSION;
  xvid_enc_frame.vol_flags=0;
  xvid_enc_frame.vol_flags|=XVID_VOL_INTERLACING;
  xvid_enc_frame.quant_intra_matrix=NULL;
  xvid_enc_frame.quant_inter_matrix=NULL;
  xvid_enc_frame.par=XVID_PAR_11_VGA;
  xvid_enc_frame.par_width=1;
  xvid_enc_frame.par_height=1;
  xvid_enc_frame.fincr=0;
  xvid_enc_frame.vop_flags=vop_presets[handler->evid_xvidpar.quality];
  if(handler->evid_xvidpar.gray)
    xvid_enc_frame.vop_flags|=XVID_VOP_GREYSCALE;
  xvid_enc_frame.motion=motion_presets[handler->evid_xvidpar.quality];
  xvid_enc_frame.input.csp=XVID_CSP_YUY2;
  xvid_enc_frame.input.plane[0]=frame;
  xvid_enc_frame.input.stride[0]=handler->frame_par.width*PALETTE_DEPTH;
  xvid_enc_frame.type=XVID_TYPE_AUTO;
  xvid_enc_frame.quant=handler->evid_xvidpar.quant;
  xvid_enc_frame.bitstream=handler->xvid.bitstream;
  xvid_enc_frame.length=-1;

  frame_size=xvid_encore(handler->xvid.handle, XVID_ENC_ENCODE, &xvid_enc_frame, NULL);

  // avilib
  if(AVI_write_frame(handler->avilib.avi, (char *)handler->xvid.bitstream, (long)frame_size, ((xvid_enc_frame.out_flags&XVID_KEYFRAME)?1:0))) {
    free(handler->errstr);
    switch(AVI_errno) {
      case AVI_ERR_SIZELIM:
        return 1;
        break;
      default:
        x_asprintf(&handler->errstr, "Error writing encoded frame to AVI file: %s.\n", AVI_strerror());
        break;
    }
    return 1;
  }
  // update header
  if(AVI_make_header(handler->avilib.avi)) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Error writing encoded frame to AVI file: %s.\n", AVI_strerror());
    return 1;
  }
  return 0;
}

char *evid_filename(evid_handler_t *evid_handler) {
  handler_t *handler=(handler_t *)evid_handler;

  return x_strdup(handler->filename);
}

int evid_destroy_handler(evid_handler_t *evid_handler, char **errstr) {
  handler_t *handler=(handler_t *)evid_handler;

  free(handler->filename);
  // avilib
  if(AVI_close(handler->avilib.avi)) {
    x_asprintf(errstr, "Error closing AVI file: %s.\n", AVI_strerror());
    return 1;
  }
  // xvid
  if(xvid_encore(handler->xvid.handle, XVID_ENC_DESTROY, NULL, NULL)) {
    (*errstr)=NULL;
    return 1;
  }
  // free bitstream
  free(handler->xvid.bitstream);
  // free handle
  free(handler);
  return 0;
}

char *evid_strerror(evid_handler_t *evid_handler) {
  handler_t *handler=(handler_t *)evid_handler;

  return handler->errstr;
}
