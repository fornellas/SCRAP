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

/** @defgroup log Log functions
 *  @{
 */

/** @file log.h
 *  @brief Log functions
 *
 *  Log functions.
 */

/** @file log.c
 *  @brief Log functions implementations.
 *
 *  Implementatio of the functions described at log.h.
 */

#ifndef LOG_H
#define LOG_H

#define LOG_ACT 2
#define LOG_WARN 1
#define LOG_MOTION -1
#define LOG_ERR 0

#define LOG_DEFAULT_LEVEL 0

/** @brief Log handler
  */
typedef void log_handler_t;

/** @brief Create new log handler
 *
 *  Initialize log; call before other functions of this module. If error,
 *  exit() is called.
 *  @param verbose_level Verbose level.
 *  @param motion Enable output for motion reports.
 *  @param path Path of the file to output log. If NULL, log will go to stdout.
 *  @param errstr String describing error.
 *  @return Pointer to new handler, or NULL if error, storing at errstr a
 *  pointer to a string describing error. User must free() it after use.
 */
log_handler_t *log_create_handler(int verbose_level, int motion, const char *path, char **errstr);

/** @brief Print log messages
 *
 *  Print log messages to correct log stream. All messages are prefixed with
 *  with date & time, and a newline is added to the end of the string.
 *  In case of error, error() is called.
 *  @param log Log handler.
 *  @param msg_level Log level of the message.
 *  @param fmt Message formatting.
 *  @param ... printf like parameters
 */
void log_printf(log_handler_t *log_handler, int msg_log_level, const char *fmt, ...);

/** @brief Destroy log handler
 *
 *  Destroy log handler, closing output file. If error, exit is called.
 *  @param errstr String describing error.
 *  @param log_handler Log handler.
 *  @return Zero if OK, non zero if error, storing at errstr a pointer to a
 *  string describing error. User must free() it after use.
 */
int log_destroy_handler(log_handler_t *log_handler, char **errstr);

/** @}
 */
#endif
