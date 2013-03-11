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

/** @defgroup read_conf Configuration file parsing
 *  @{
 */

/** @file read_conf.h
 *  @brief Configuration file reading functions
 *
 *  This file contain functions to help reading "INI" configuration files.
 */

/** @file read_conf.c
 *  @brief Configuration file reading functions implementations
 *
 *  Implementation of the functions described in read_conf.h
 */

#ifndef READ_CONF_H
#define READ_CONF_H

#include <linux/types.h>
#include <stdio.h>

/** @brief Find next section
 *
 *  Reads stream conf_file, find next section and stores its name in sec_name.
 *  line_count is incremented with the number of lines readed.
 *  @param conf_file Configuration file stream.
 *  @param line_count Lines readed.
 *  @param sec_name Where to store section name.
 *  @return Returns 0 if OK, section found; 1 if EOF, and nothing readed; 2 if
 *  parse error.
 */
int read_section(FILE *conf_file, int *line_count, char *sec_name);

/** @brief Find next variable
 *
 *  Reads strem conf_file, find next section variable, and store its name in
 *  var_name, and its contents in var_value. line_count is incremented with the
 *  number of lines readed.
 *  @param conf_file Configuration file stream.
 *  @param line_count Lines readed.
 *  @param var_name Where to store variable name.
 *  @param var_value Where to store variable value.
 *  @return Returns 0 if OK; 1 if EOF, and nothing readed; 2 if parse error or
 *  3 if new section found.
 */
int read_var(FILE *conf_file, int *line_count, char *var_name, char *var_value);

// Commented lines are not implemented yet.
/** store_var macro: int */
#define T_INT                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
/** store_var macro: int, with low limit */
#define T_INT_LOW(l)                0, 1, l, 0, 0, 0, 0, 0, 0, 0, 0, 0
// #define T_INT_HIGH(h)               0, 2, 0, h, 0, 0, 0, 0, 0, 0, 0, 0
/** store_var macro: int, with low and high limit */
#define T_INT_BOTH(l, h)            0, 3, l, h, 0, 0, 0, 0, 0, 0, 0, 0
/** store_var macro: double */
#define T_DOUBLE                    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
/** store_var macro: double, with low limit */
#define T_DOUBLE_LOW(l)             1, 1, 0, 0, l, 0, 0, 0, 0, 0, 0, 0
// #define T_DOUBLE_HIGH(h)            1, 2, 0, 0, 0, h, 0, 0, 0, 0, 0, 0
/** store_var macro: double, with low and high limit */
#define T_DOUBLE_BOTH(l, h)         1, 3, 0, 0, l, h, 0, 0, 0, 0, 0, 0
/** store_var macro: __u16 */
#define T___U16                     2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
// #define T___U16_LOW(l)              2, 1, 0, 0, 0, 0, l, 0, 0, 0, 0, 0
/** store_var macro: __u16, with high limit */
#define T___U16_HIGH(h)             2, 2, 0, 0, 0, 0, 0, h, 0, 0, 0, 0
// #define T___U16_BOTH(l, h)          2, 3, 0, 0, 0, 0, l, h, 0, 0, 0, 0
/** store_var macro: unsigned char */
#define T_UNSIGNED_CHAR             3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
// #define T_UNSIGNED_CHAR_LOW(l)      3, 1, 0, 0, 0, 0, 0, 0, l, 0, 0, 0
// #define T_UNSIGNED_CHAR_HIGH(h)     3, 2, 0, 0, 0, 0, 0, 0, 0, h, 0, 0
// #define T_UNSIGNED_CHAR_BOTH(l, h)  3, 3, 0, 0, 0, 0, 0, 0, l, h, 0, 0
/** store_var macro: unsigned long */
#define T_UNSIGNED_LONG             4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
/** store_var macro: unsigned long, with low limit */
#define T_UNSIGNED_LONG_LOW(l)      4, 1, 0, 0, 0, 0, 0, 0, 0, 0, l, 0
// #define T_UNSIGNED_LONG_HIGH(h)     4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, h
/** store_var macro: unsigned long, with low and high limit */
#define T_UNSIGNED_LONG_BOTH(l, h)  4, 3, 0, 0, 0, 0, 0, 0, 0, 0, l, h
/** @brief Parse generic data types from a string
 *
 *  This function takes only the 3 arguments described below.
 *  @param store pointer to a adress where to store the data readed.
 *  @param string pointer to a strintg, containing the data to read.
 *  @param T_[TYPE]_[RANGE] Macro witch specifies data type and rage to read.
 *  @return Returns 0 if value readed, or 1 if parse error or value out of
 *  range. In case of error, an error message is printed to stderr.
 */
int store_var(void *store, char *string, int type, int range, int r_int_low, int r_int_high, double r_double_low, double r_double_high, __u16 r_u16_low, __u16 r_u16_high, unsigned char r_unsigned_char_low, unsigned char r_unsigned_char_high, unsigned long r_unsigned_long_low, unsigned long r_unsigned_long_high);

/** @}
 */
#endif
