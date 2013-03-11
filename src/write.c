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
#include "write.h"
#include "font1.xpm"
#include "font2.xpm"
#include <string.h>
#include <stdio.h>

void write_par_set_defaults(write_par_t *write_par) {
  write_par->font=1;
  write_par->transparency=70;
}

typedef struct {
  frame_par_t frame_par;
  write_par_t write_par;
  char *errstr;
} handler_t;

write_handler_t *write_create_handler(write_par_t *write_par, frame_par_t *frame_par) {
  handler_t *handler;

  handler=(handler_t *)x_malloc(sizeof(handler_t));

  // frame_par
  memcpy(&(handler->frame_par), frame_par, sizeof(frame_par_t));
  // xvid_par
  memcpy(&(handler->write_par), write_par, sizeof(write_par_t));
  // errstr
  handler->errstr=NULL;
  return (write_handler_t *)handler;
}

#define FONT_WIDTH 8
#define FONT_HEIGHT 14
#define DRAW_X 8
#define DRAW_Y 8

int write_frame(write_handler_t *write_handler, pixel_t *frame, const char *text) {
  handler_t *handler=(handler_t *)write_handler;
  int c, w, h, j, f;
  pixel_t p;
  int len=strlen(text);
  static char **font_xpm;
  // frame is too small
  if(len*FONT_WIDTH+DRAW_X+len>handler->frame_par.width||FONT_HEIGHT+DRAW_Y>handler->frame_par.height) {
    free(handler->errstr);
    x_asprintf(&handler->errstr, "Error: frame is too small to draw text!\n");
    return 1;
  }
  switch(handler->write_par.font) {
    case 1:
      font_xpm=font1_xpm;
      break;
    case 2:
      font_xpm=font2_xpm;
      break;
    default:
      return 1;
  }
  // draw each char
  for(c=0;c<len;c++) {
    if(text[c]>96&&text[c]<123) {
      // lowercase
      j=(int)text[c]-97;
    } else if(text[c]>64&&text[c]<91) {
      // uppercase
      j=(int)text[c]-64+25;
    } else if(text[c]==32) {
      // space
      j=52;
    } else if(text[c]>47&&text[c]<58) {
      // numbers
      j=(int)text[c]-47+52;
    } else if(text[c]==35) {
      // #
      j=66;
    } else if(text[c]==58) {
      // #
      j=67;
    } else if(text[c]==45) {
      // -
      j=63;
    } else if(text[c]==95) {
      // _
      j=64;
    } else if(text[c]==46) {
      // .
      j=65;
    } else {
      // invalid char
      free(handler->errstr);
      x_asprintf(&handler->errstr, "Error: invalid char '%c' in text '%s'.\n", text[c], text);
      return 1;
//       j=52;
    }
    for(h=0; h<FONT_HEIGHT;h++) {
      for(w=0;w<FONT_WIDTH;w++) {
        if(font_xpm[4+h][w+FONT_WIDTH*j]==' ') {
          // transparent pixel
          continue;
        } else if(font_xpm[4+h][w+FONT_WIDTH*j]=='.') {
          // black pixel
          p=0;
        } else if(font_xpm[4+h][w+FONT_WIDTH*j]=='+') {
          // white pixel
          p=255;
        } else {
          free(handler->errstr);
          x_asprintf(&handler->errstr, "Internal error, invalid XPM (%c).\n", font_xpm[4+h][w+FONT_WIDTH*j]);
          return 1;
        }
        f=((DRAW_Y+h)*handler->frame_par.width+DRAW_X+w+ FONT_WIDTH*c+c)*PALETTE_DEPTH;
        frame[f]=(pixel_t)(((int)frame[f]*handler->write_par.transparency+p*(100-handler->write_par.transparency))/100);
      }
    }
  }
  return 0;
}

char *write_strerror(write_handler_t *write_handler) {
  handler_t *handler=(handler_t *)write_handler;
  return handler->errstr;
}
