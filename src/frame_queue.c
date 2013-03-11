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
#include "frame_queue.h"
#include <pthread.h>
#include <string.h>

// remover
#include <stdio.h>

typedef struct{
  int max_frames;
  frame_par_t frame_par;
  pixel_t **frame;   /* queue_frame
                        qeue_set_last_frame_ready
                        queue_get_first_frame
                        queue_remove_first */
  int start;         /* queue_frame
                        qeue_set_last_frame_ready
                        queue_get_first_frame
                        queue_get_first_frame_time
                        queue_remove_first
                        queue_wait_any_start_frame_ready */
  int n_frames;      /* queue_frame
                        qeue_set_last_frame_ready
                        queue_get_first_frame
                        queue_get_first_frame_time
                        queue_remove_first
                        queue_wait_any_start_frame_ready */
  int *frame_ready;  /* queue_frame
                        qeue_set_last_frame_ready
                        queue_get_first_frame
                        queue_get_first_frame_time
                        queue_remove_first
                        queue_wait_any_start_frame_ready */
  time_t *cap_time;  /* queue_frame
                        queue_get_first_frame_time */
} queue_t;

typedef struct {
  // queues
  queue_t *queue_v;
  pthread_mutex_t *queue_lock_v;
  int queue_c; // 0=one queue

  // full queues count
  int full_queues;                 /* queue_frame
                                      queue_wait_not_full
                                      queue_remove_first */
  pthread_mutex_t full_queues_lock;
  pthread_cond_t full_queues_cond;
  // start frame ready count
  int start_ready;                 /* queue_wait_any_start_frame_ready
                                          qeue_set_last_frame_ready
                                          queue_remove_first */
  pthread_mutex_t start_ready_lock;
  pthread_cond_t start_ready_cond;
  // empty queues count
  int empty_queues;                    /* queue_frame
                                          queue_remove_first */
  pthread_mutex_t empty_queues_lock;
  pthread_cond_t empty_queues_cond;
} handler_t;

void print_queue_stats(queue_handler_t *queue_handler) {
  handler_t *handler=(handler_t *)queue_handler;
  int c;
  int j;
return ;
  fprintf(stderr, "----\nqueue_c %d\nfull_queues %d\nstart_ready %d\nempty_queues %d\n", handler->queue_c, handler->full_queues, handler->start_ready, handler->empty_queues);
  for(c=0;c<=handler->queue_c;c++) {
    fprintf(stderr, "Queue %d:\n  max_frames %d\n  start %d\n  n_frames  %d\n", c, handler->queue_v[c].max_frames, handler->queue_v[c].start, handler->queue_v[c].n_frames);
    for(j=0;j<handler->queue_v[c].max_frames;j++)
      fprintf(stderr, "  frame_ready[%d] %d\n", j, handler->queue_v[c].frame_ready[j]);
  }
  fprintf(stderr, "----\n");
}

void queue_par_set_defaults(int *max_frames) {
  (*max_frames) = 10;
}

queue_handler_t *queue_create_handler() {
  handler_t *handler;

  // alloc handler
  handler=(handler_t *)x_malloc(sizeof(handler_t));
  // start empty queues array
  handler->queue_v=NULL;
  handler->queue_lock_v=NULL;
  handler->queue_c=-1;
  // full queues count
  handler->full_queues=0;
  pthread_mutex_init(&handler->full_queues_lock, NULL);
  pthread_cond_init(&handler->full_queues_cond, NULL);
  // start frames
  handler->start_ready=0;
  pthread_mutex_init(&handler->start_ready_lock, NULL);
  pthread_cond_init(&handler->start_ready_cond, NULL);
  // empty queues count
  handler->empty_queues=0;
  pthread_mutex_init(&handler->empty_queues_lock, NULL);
  pthread_cond_init(&handler->empty_queues_cond, NULL);

  return handler;
}

queue_ident_t queue_add(queue_handler_t * queue_handler, int max_frames, frame_par_t *frame_par) {
  handler_t *handler=(handler_t *)queue_handler;
  int f;

  // lets add a queue...
  handler->queue_c++;
  handler->queue_v=(queue_t *)x_realloc(handler->queue_v, (size_t)(sizeof(queue_t)*(handler->queue_c+1)));
  // ...and its lock
  handler->queue_lock_v=(pthread_mutex_t *)x_realloc(handler->queue_lock_v, (size_t)(sizeof(pthread_mutex_t)*(handler->queue_c+1)));
  pthread_mutex_init(&handler->queue_lock_v[handler->queue_c], NULL);
  // set one more empty queue
  handler->empty_queues++;
  // and finally set up queue defaults
  handler->queue_v[handler->queue_c].max_frames=max_frames;
  memcpy(&(handler->queue_v[handler->queue_c].frame_par), frame_par, sizeof(frame_par_t));
  // allocate frame pointer array
  handler->queue_v[handler->queue_c].frame=(pixel_t **)x_malloc((size_t)(sizeof(pixel_t *)*max_frames));
  // allocate first frame
  handler->queue_v[handler->queue_c].frame[0]=(pixel_t *)x_malloc((size_t)(frame_par->size));
  // and NULL empty ones
  for(f=1;f<max_frames;f++)
    handler->queue_v[handler->queue_c].frame[f]=NULL;
  // queue status
  handler->queue_v[handler->queue_c].start=0;
  handler->queue_v[handler->queue_c].n_frames=0;
  // allocate ready frames array
  handler->queue_v[handler->queue_c].frame_ready=(int *)x_malloc((size_t)(sizeof(int)*max_frames));
  // and set all to non ready
  for(f=0;f<max_frames;f++)
    handler->queue_v[handler->queue_c].frame_ready[f]=0;
  // capture time array
  handler->queue_v[handler->queue_c].cap_time=(time_t *)x_malloc((size_t)(sizeof(time_t)*max_frames));

  // return queue ident
  return handler->queue_c;
}

pixel_t *queue_frame(queue_handler_t *queue_handler, queue_ident_t queue_ident, int *n_frames) {
  handler_t *handler=(handler_t *)queue_handler;
  int end;
// fprintf(stderr, "----\nqueue_frame\n");

  // lets lock queue
  pthread_mutex_lock(&handler->queue_lock_v[queue_ident]);

  // if we have a full queue, exit
  if(handler->queue_v[queue_ident].n_frames==handler->queue_v[queue_ident].max_frames) {
    // store frame count
    if(n_frames!=NULL)
      (*n_frames)=handler->queue_v[queue_ident].n_frames;
    // release queue
    pthread_mutex_unlock(&handler->queue_lock_v[queue_ident]);
print_queue_stats(handler);
    return NULL;
  }

  // here, a non empty queue
  if(handler->queue_v[queue_ident].n_frames) {
    // find end
    end=handler->queue_v[queue_ident].start+handler->queue_v[queue_ident].n_frames;
    if(end>=handler->queue_v[queue_ident].max_frames)
      end-=handler->queue_v[queue_ident].max_frames;
    // lets allocate this frame
    handler->queue_v[queue_ident].frame[end]=(pixel_t *)x_malloc((size_t)(handler->queue_v[queue_ident].frame_par.size));
  // if we have an empty queue, easier job
  } else {
    // set end
    end=handler->queue_v[queue_ident].start;
    // one less queue empty
    pthread_mutex_lock(&handler->empty_queues_lock);
    handler->empty_queues--;
    pthread_cond_signal(&handler->empty_queues_cond);
    pthread_mutex_unlock(&handler->empty_queues_lock);
  }
  // sum one more frame
  handler->queue_v[queue_ident].n_frames++;
  // store time
  handler->queue_v[queue_ident].cap_time[end]=time(NULL);
  // if this queue is full now
  if(handler->queue_v[queue_ident].max_frames==handler->queue_v[queue_ident].n_frames) {
    // lets sum it
    pthread_mutex_lock(&handler->full_queues_lock);
    handler->full_queues++;
    pthread_cond_signal(&handler->full_queues_cond);
    pthread_mutex_unlock(&handler->full_queues_lock);
  }
  // store frame count
  if(n_frames!=NULL)
    (*n_frames)=handler->queue_v[queue_ident].n_frames;
  // release queue
  pthread_mutex_unlock(&handler->queue_lock_v[queue_ident]);
print_queue_stats(handler);
  // and return frame
  return handler->queue_v[queue_ident].frame[end];
}

void queue_wait_not_full(queue_handler_t *queue_handler) {
  handler_t *handler=(handler_t *)queue_handler;
// fprintf(stderr, "----\nqueue_wait_not_full (start)\n");
print_queue_stats(handler);
  pthread_mutex_lock(&handler->full_queues_lock);
  while(handler->full_queues==handler->queue_c+1) {
    pthread_cond_wait(&handler->full_queues_cond, &handler->full_queues_lock);
  }
  pthread_mutex_unlock(&handler->full_queues_lock);
// fprintf(stderr, "----\nqueue_wait_not_full (exit)\n");
print_queue_stats(handler);
}

void queue_set_last_frame_ready(queue_handler_t *queue_handler, queue_ident_t queue_ident) {
  handler_t *handler=(handler_t *)queue_handler;
  int end;
// fprintf(stderr, "----\nqeue_set_last_frame_ready\n");
  // lets lock queue
  pthread_mutex_lock(&handler->queue_lock_v[queue_ident]);

  // find end
  end=handler->queue_v[queue_ident].start+handler->queue_v[queue_ident].n_frames-1;
  if(end>=handler->queue_v[queue_ident].max_frames)
    end-=handler->queue_v[queue_ident].max_frames;
  // set ready
  handler->queue_v[queue_ident].frame_ready[end]=1;

  // if it is the only frame, signal it is ready
  if(handler->queue_v[queue_ident].n_frames==1) {
    pthread_mutex_lock(&handler->start_ready_lock);
    handler->start_ready++;
    pthread_cond_signal(&handler->start_ready_cond);
    pthread_mutex_unlock(&handler->start_ready_lock);
  }

  // release queue
  pthread_mutex_unlock(&handler->queue_lock_v[queue_ident]);
print_queue_stats(handler);
}

pixel_t *queue_get_first_frame(queue_handler_t *queue_handler, queue_ident_t queue_ident) {
  handler_t *handler=(handler_t *)queue_handler;
  pixel_t *frame;
// fprintf(stderr, "----\nqueue_get_first_frame\n");
  // lets lock queue
  pthread_mutex_lock(&handler->queue_lock_v[queue_ident]);

  if(handler->queue_v[queue_ident].frame_ready[handler->queue_v[queue_ident].start])
    frame=handler->queue_v[queue_ident].frame[handler->queue_v[queue_ident].start];
  else
    frame=NULL;

  // release queue
  pthread_mutex_unlock(&handler->queue_lock_v[queue_ident]);
print_queue_stats(handler);
  return frame;
}

time_t queue_get_first_frame_time(queue_handler_t *queue_handler,  queue_ident_t queue_ident) {
  handler_t *handler=(handler_t *)queue_handler;
  time_t tm;

  // lets lock queue
  pthread_mutex_lock(&handler->queue_lock_v[queue_ident]);

  tm=handler->queue_v[queue_ident].cap_time[handler->queue_v[queue_ident].start];

  // release queue
  pthread_mutex_unlock(&handler->queue_lock_v[queue_ident]);

  return tm;
}

void queue_remove_first(queue_handler_t *queue_handler, queue_ident_t queue_ident) {
  handler_t *handler=(handler_t *)queue_handler;
// fprintf(stderr, "----\nqueue_remove_first\n");
  // lets lock queue
  pthread_mutex_lock(&handler->queue_lock_v[queue_ident]);

  // if this queue is full, will not be anymore
  if(handler->queue_v[queue_ident].max_frames==handler->queue_v[queue_ident].n_frames) {
    // lets count down it
    pthread_mutex_lock(&handler->full_queues_lock);
    handler->full_queues--;
    pthread_cond_signal(&handler->full_queues_cond);
    pthread_mutex_unlock(&handler->full_queues_lock);
  }

  // not ready anymore
  handler->queue_v[queue_ident].frame_ready[handler->queue_v[queue_ident].start]=0;

  // if more than one frame...
  if(handler->queue_v[queue_ident].n_frames>1) {
    free(handler->queue_v[queue_ident].frame[handler->queue_v[queue_ident].start]);
    // step start
    handler->queue_v[queue_ident].start=handler->queue_v[queue_ident].start==handler->queue_v[queue_ident].max_frames-1?0:handler->queue_v[queue_ident].start+1;
  }

  // less one frame
  handler->queue_v[queue_ident].n_frames--;

  // if queue is empty now
  if(!handler->queue_v[queue_ident].n_frames) {
    pthread_mutex_lock(&handler->empty_queues_lock);
    handler->empty_queues++;
    pthread_cond_signal(&handler->empty_queues_cond);
    pthread_mutex_unlock(&handler->empty_queues_lock);
  }

  // if we have remaning frame(s) and it is not ready to be procesed, or no
  // frames left, signal it
  if((handler->queue_v[queue_ident].n_frames&&!handler->queue_v[queue_ident].frame_ready[handler->queue_v[queue_ident].start])||!handler->queue_v[queue_ident].n_frames) {
    pthread_mutex_lock(&handler->start_ready_lock);
    handler->start_ready--;
    pthread_cond_signal(&handler->start_ready_cond);
    pthread_mutex_unlock(&handler->start_ready_lock);
  }
  // release queue
  pthread_mutex_unlock(&handler->queue_lock_v[queue_ident]);
print_queue_stats(handler);
}

void queue_wait_any_start_frame_ready(queue_handler_t *queue_handler) {
  handler_t *handler=(handler_t *)queue_handler;

// fprintf(stderr, "----\nqueue_wait_any_start_frame_ready (start)\n");
print_queue_stats(handler);
  pthread_mutex_lock(&handler->start_ready_lock);
  while(!handler->start_ready)
    pthread_cond_wait(&handler->start_ready_cond, &handler->start_ready_lock);
  pthread_mutex_unlock(&handler->start_ready_lock);
// fprintf(stderr, "----\nqueue_wait_any_start_frame_ready (end)\n");
print_queue_stats(handler);
}

void queue_wait_all_empty(queue_handler_t *queue_handler) {
  handler_t *handler=(handler_t *)queue_handler;
  pthread_mutex_lock(&handler->empty_queues_lock);
  while(handler->empty_queues<handler->queue_c+1) {
    pthread_cond_wait(&handler->empty_queues_cond, &handler->empty_queues_lock);
  }
  pthread_mutex_unlock(&handler->empty_queues_lock);
}
/*
int queue_destroy_handler(queue_handler_t *queue_handler) {

}
*/
