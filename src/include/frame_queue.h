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

/** @defgroup frame_queue Frame queue
 *  @{
 */

/** @file frame_queue.h
 *  @brief Frame queue functions
 *
 *  These functions manage the frame queue for each camera.
 */

/** @file frame_queue.c
 *  @brief Frame queue functions implementations.
 *
 *  Implementation of the functions described at frame_queue.h.
 */

#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H

#include "common.h"
#include <time.h>

/** @brief Set default frames per queue
 */
void queue_par_set_defaults(int *max_frames);

/** @brief Queue handler
 */
typedef void queue_handler_t;

/** @brief Create a new queue handler
 *
 *  Create & initialize a new queue handler.
 *  @return Adress of the new handler.
 */
queue_handler_t *queue_create_handler();

/** @brief Queue identifier
 */
typedef int queue_ident_t;

/** @brief Add a new queue
 *
 *  Add a new queue to specified handler. Should be used always before next
 *  functions.
 *  @param queue_handler Queue handler.
 *  @param max_frames Maximum number of frames in queue.
 *  @param frame_par Frame parameters.
 *  @return Identifier for added queue.
 */
queue_ident_t queue_add(queue_handler_t *queue_handler, int max_frames, frame_par_t *frame_par);

/** @brief Queue frame
 *
 *  Queue one more frame on queue_ident handled by queue_handler.
 *  @param queue_handler Queue handler.
 *  @param queue_ident Queue identifier.
 *  @param n_frames If non NULL, store in this adress the number of frames in
 *  queue.
 *  @return Pointer to allocated frame, or NULL if queue is full.
 */
pixel_t *queue_frame(queue_handler_t *queue_handler, queue_ident_t queue_ident, int *n_frames);

/** @brief Wait until any queue is not full
 *
 *  Hold thread execution until any of the queues handled by queue_handler is
 *  not full.
 *  @param queue_handler Queue handler.
 */
void queue_wait_not_full(queue_handler_t *queue_handler);

/** @brief Set last frame ready
 *
 *  Set last frame of queue is ready to be processed.
 *  @param queue_handler Queue handler.
 *  @param queue_ident Queue identifier.
 */
void queue_set_last_frame_ready(queue_handler_t *queue_handler, queue_ident_t queue_ident);

/** @brief Get first frame
 *
 *  Retuns first frame of the queue_number.
 *  @param queue_handler Queue handler.
 *  @param queue_ident Queue identifier.
 *  @return Pointer to first frame if it is ready to be processed, or NULL if
 *  not.
 */
pixel_t *queue_get_first_frame(queue_handler_t *queue_handler, queue_ident_t queue_ident);

/** @brief Get capture time
 *
 *  Get queue first frame capture time.
 *  @param queue_handler Queue handler.
 *  @param queue_ident Queue identifier.
 *  @return
 */
time_t queue_get_first_frame_time(queue_handler_t *queue_handler, queue_ident_t queue_ident);

/** @brief Remove the first frame of queue
 *
 *  Remove the sfirst frame of queue.
 *  @param queue_handler Queue handler.
 *  @param queue_ident Queue identifier.
 */
void queue_remove_first(queue_handler_t *queue_handler, queue_ident_t queue_ident);

/** @brief Wait for frame ready
 *
 *  Holds thread execution, until any queue handled by queue_handler have a
 *  first frame ready to be processed.
 *  @param queue_handler Queue handler.
 */
void queue_wait_any_start_frame_ready(queue_handler_t *queue_handler);

/** @brief Wait for all queues empty
 *
 *  Holds thread execution, until all queues handled by queue_handler are
 *  empty.
 *  @param queue_handler Queue handler.
 */
void queue_wait_all_empty(queue_handler_t *queue_handler);

/** @brief Destroy queue handler
 *
 *  Destroy queue handler, including all queues handled by it.
 *  @param queue_handler Queue handler.
 *  @return Zero if OK, or error number if not.
 */
int queue_destroy_handler(queue_handler_t *queue_handler);

/** @}
 */
#endif
