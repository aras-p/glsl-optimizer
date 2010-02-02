/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef TR_SCREEN_H_
#define TR_SCREEN_H_


#include "pipe/p_screen.h"
#include "os/os_thread.h"


#ifdef __cplusplus
extern "C" {
#endif


struct tr_list {
   struct tr_list *next;
   struct tr_list *prev;
};

/**
 * It often happens that new data is written directly to the user buffers
 * without mapping/unmapping. This flag marks user buffers, so that their
 * contents can be dumpped before being used by the pipe context.
 */
#define TRACE_BUFFER_USAGE_USER  (1 << 31)


struct trace_screen
{
   struct pipe_screen base;

   struct pipe_screen *screen;

   /* remote debugger */
   struct trace_rbug *rbug;

   pipe_mutex list_mutex;
   int num_buffers;
   int num_contexts;
   int num_textures;
   int num_surfaces;
   int num_transfers;
   struct tr_list buffers;
   struct tr_list contexts;
   struct tr_list textures;
   struct tr_list surfaces;
   struct tr_list transfers;
};


/*
 * tr_rbug.c
 */


struct trace_rbug;

struct trace_rbug *
trace_rbug_start(struct trace_screen *tr_scr);

void
trace_rbug_stop(struct trace_rbug *tr_rbug);


/*
 * tr_screen.c
 */


boolean
trace_enabled(void);

struct trace_screen *
trace_screen(struct pipe_screen *screen);

struct pipe_screen *
trace_screen_create(struct pipe_screen *screen);

void
trace_screen_user_buffer_update(struct pipe_screen *screen,
                                struct pipe_buffer *buffer);

#define trace_screen_add_to_list(tr_scr, name, obj) \
   do {                                             \
      pipe_mutex_lock(tr_scr->list_mutex);          \
      insert_at_head(&tr_scr->name, &obj->list);    \
      tr_scr->num_##name++;                         \
      pipe_mutex_unlock(tr_scr->list_mutex);        \
   } while (0)

#define trace_screen_remove_from_list(tr_scr, name, obj) \
   do {                                                  \
      pipe_mutex_lock(tr_scr->list_mutex);               \
      remove_from_list(&obj->list);                      \
      tr_scr->num_##name--;                              \
      pipe_mutex_unlock(tr_scr->list_mutex);             \
   } while (0)


#ifdef __cplusplus
}
#endif

#endif /* TR_SCREEN_H_ */
