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
#include "read_conf.h"
#include <stdio.h>
#include <ctype.h>
#include <linux/types.h>

int goto_next(FILE *conf_file, int *line_count) {
  char c;
  while((c=(char)getc(conf_file))!=EOF) {
    // skip spaces
    if(c==' '||c=='\t')
      continue;
    // skip empty lines
    if(c=='\n') {
      (*line_count)++;
      continue;
    }
    // skip commented lines
    if(c=='#') {
      for(;;) {
        c=(char)getc(conf_file);
        if(c==EOF)
          return 1;
        if(c=='\n') {
          (*line_count)++;
          break;
        }
        if(!isprint(c)) {
          fprintf(stderr, "Non printable char found at line %d.\n", *line_count);
          return 2;
        }
      }
      continue;
    }
    if(ungetc((int)c, conf_file)==EOF) {
      fprintf(stderr, "Error in ungetc().\n");
      return 2;
    } else
      return 0;
  }
  return 1;
}

int read_section(FILE *conf_file, int *line_count, char *sec_name) {
  int i;
  char c;
  // find section beginning
  if((i=goto_next(conf_file, line_count)))
    return i;
  // section start
  c=getc(conf_file);
  if(c!='[') {
    fprintf(stderr, "Expected a section start ([Section]) but got garbage at line %d.\n", *line_count);
    return 2;
  }
  // section name
  sec_name[0]='\0';
  for(i=0;i<MAX_NAME_LENGTH;i++) {
    c=getc(conf_file);
    if(c==EOF) {
      fprintf(stderr, "Unexpected EOF at line %d.\n", *line_count);
      return 2;
    }
    if(c=='#') {
      fprintf(stderr, "Missing ] at line %d.\n", *line_count);
      return 2;
    }
    if(c=='\n') {
      fprintf(stderr, "Unexpected new line at line %d.\n", *line_count);
      return 2;
    }
    if(c==']') {
      sec_name[i]='\0';
      break;
    }
    if(!isprint(c)) {
      fprintf(stderr, "Non printable char found at line %d.\n", *line_count);
      return 2;
    }
    sec_name[i]=c;
  }
  if(c!=']') {
    fprintf(stderr, "Section name too long at line %d.\n", *line_count);
    return 2;
  }
  if(i==0) {
    fprintf(stderr, "Empty section name at line %d.\n", *line_count);
    return 2;
  }
  // read until new line
  i=(*line_count);
  switch(goto_next(conf_file, line_count)) {
    case 0:
      if(i==(*line_count)) {
        fprintf(stderr, "Garbage after section [%s] at line %d.\n", sec_name, *line_count);
        return 2;
      }
      break;
    case 1:
      fprintf(stderr, "Empty section [%s] at line %d.\n", sec_name, *line_count);
      return 2;
      break;
    case 2:
      return 2;
      break;
    default:
      fprintf(stderr, "Internal error.\n");
      return 2;
      break;
  }
  if(i==*line_count) {
    fprintf(stderr, "Garbage at line %d.\n", *line_count);
    return 2;
  }
  return 0;
}

int read_var(FILE *conf_file, int *line_count, char *var_name, char *var_value) {
  int i;
  char c;
  // find section beginning
  if((i=goto_next(conf_file, line_count)))
    return i;
  // get var name
  var_name[0]='\0';
  for(i=0;i<MAX_NAME_LENGTH;i++) {
    c=getc(conf_file);
    if(i==0&&c=='[') {
      if(ungetc((int)c, conf_file)==EOF) {
        fprintf(stderr, "Error in ungetc().\n");
        return 2;
      }
      return 3;
    }
    if(c==EOF) {
      fprintf(stderr, "Unexpected EOF at line %d.\n", *line_count);
      return 2;
    }
    if(c=='#') {
      fprintf(stderr, "Comment out of place at line %d.", *line_count);
      return 2;
    }
    if(c==' '||c=='\t'||c=='\n') {
      fprintf(stderr, "No space, tab or new line allowed in variable name, line %d.\n", *line_count);
      return 2;
    }
    if(!isprint(c)) {
      fprintf(stderr, "Non printable char found at line %d.\n", *line_count);
      return 2;
    }
    if(c=='=') {
      var_name[i]='\0';
      break;
    }
    var_name[i]=c;
  }
  // get var value
  var_value[0]='\0';
  switch((c=getc(conf_file))) {
    case EOF:
    case '#':
      fprintf(stderr, "Missing variable ´%s´ value at line %d.\n", var_name, *line_count);
      return 2;
      break;
//     case ' ':
//     case '\n':
//     case '\t':
//       fprintf(stderr, "No space, tab or new line allowed without quotes in variable ´%s´ value, line %d.\n", var_name, *line_count);
//       return 2;
//       break;
    default:
      // parse without quotes
      if(!isprint(c)) {
        fprintf(stderr, "Non printable char found at line %d.\n", *line_count);
        return 2;
      }
      var_value[0]=c;
      for(i=1;i<MAX_VALUE_LENGTH;i++) {
        c=getc(conf_file);
        if(c==EOF||c=='\n') {
          if(c=='\n')
            (*line_count)++;
          var_value[i]='\0';
          return 0;
        }
        if(!isprint(c)) {
          fprintf(stderr, "Non printable char found at line %d.\n", *line_count);
          return 2;
        }
        var_value[i]=c;
      }
      if(var_value[i]!='\0') {
        fprintf(stderr, "Variable ´%s´ value too long at line %d.\n", var_name, *line_count);
        return 2;
      }
      break;
  }
  return 0;
}

int store_var(void *store, char *string, int type, int range, int r_int_low, int r_int_high, double r_double_low, double r_double_high, __u16 r_u16_low, __u16 r___u16_high, unsigned char r_unsigned_char_low, unsigned char r_unsigned_char_high, unsigned long r_unsigned_long_low, unsigned long r_unsigned_long_high) {
  int r;
  switch(type) {
    // int
    case 0:
      r=sscanf(string, "%d", (int *)store);
      if(r==0||r==EOF) {
        fprintf(stderr, "Parse error in variable.\n");
        return 1;
      }
      switch(range) {
        // low
        case 1:
          if(*((int *)store)<r_int_low) {
            fprintf(stderr, "Value out of range. Should be >%d.\n", r_int_low);
            return 1;
          }
          break;
        // high
        case 2:
          return 1;
          break;
        // both
        case 3:
          if(*((int *)store)<r_int_low||*((int *)store)>r_int_high) {
            fprintf(stderr, "Value out of range. Should be >%d, <%d.\n", r_int_low, r_int_high);
            return 1;
          }
          break;
      }
      break;
    // double
    case 1:
      r=sscanf(string, "%lf", (double *)store);
      if(r==0||r==EOF) {
        fprintf(stderr, "Parse error in variable.\n");
        return 1;
      }
      switch(range) {
        // low
        case 1:
          if(*((double *)store)<r_double_low) {
            fprintf(stderr, "Value out of range. Should be >%f.\n", r_double_low);
            return 1;
          }
          break;
        // high
        case 2:
          return 1;
          break;
        // both
        case 3:
          if(*((double *)store)<r_double_low||*((double *)store)>r_double_high) {
            fprintf(stderr, "Value out of range. Should be >%f, <%f.\n", r_double_low, r_double_high);
            return 1;
          }
          break;
      }
      break;
    // __u16
    case 2:
      r=sscanf(string, "%hd", (__u16 *)store);
      if(r==0||r==EOF) {
        fprintf(stderr, "Parse error in variable.\n");
        return 1;
      }
      switch(range) {
        // low
        case 1:
          return 1;
          break;
        // high
        case 2:
          if(*((__u16 *)store)>r___u16_high) {
            fprintf(stderr, "Value out of range. Should be <%hd.\n", r___u16_high);
            return 1;
          }
          break;
        // both
        case 3:
          return 1;
          break;
      }
      break;
    // unsigned char
    case 3:
      r=sscanf(string, "%hhd", (unsigned char *)store);
      if(r==0||r==EOF) {
        fprintf(stderr, "Parse error in variable.\n");
        return 1;
      }
      switch(range) {
        // low
        case 1:
          return 1;
          break;
        // high
        case 2:
          return 1;
          break;
        // both
        case 3:
          return 1;
          break;
      }
      break;
    // unsigned long
    case 4:
      r=sscanf(string, "%lu", (unsigned long *)store);
      if(r==0||r==EOF) {
        fprintf(stderr, "Parse error in variable.\n");
        return 1;
      }
      switch(range) {
        // low
        case 1:
          if(*((unsigned long *)store)<r_unsigned_long_low) {
            fprintf(stderr, "Value out of range. Should be >%lu.\n", r_unsigned_long_low);
            return 1;
          }
          break;
        // high
        case 2:
          return 1;
          break;
        // both
        case 3:
          if(*((unsigned long *)store)<r_unsigned_long_low||*((unsigned long *)store)>r_unsigned_long_high) {
            fprintf(stderr, "Value out of range. Should be >%lu, <%lu.\n", r_unsigned_long_low, r_unsigned_long_high);
            return 1;
          }
          break;
      }
      break;
    default:
      fprintf(stderr, "Unknow data type.\n");
      return 1;
      break;
  }
  return 0;
}
