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

/** @defgroup load_conf Load configuration
 *  @{
 */
/** @file load_conf.h
 *  @brief Capture device configuration file parsing
 *
 *  Function to load all configuration from a given file.
 */
/** @file load_conf.c
 *  @brief Capture device configuration file parsing functions implementations
 *
 *  Implementation of the functions described at load_conf.h.
 */

#ifndef PARSE_CONF_H
#define PARSE_CONF_H
#include "common.h"
#include "device.h"

/** @brief Load capture device configuration file
 *
 *  Reads configuration file and load capture/camera settings into *device.
 *  @param file Configuration file path.
 *  @param device Device structure to store data.
 *  @return True if error, also print error message to device->log.
 */
int load_config(char *file, device_t *device);

/** @}
 */
#endif
