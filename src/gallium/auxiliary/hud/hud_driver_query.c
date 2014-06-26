/**************************************************************************
 *
 * Copyright 2013 Marek Olšák <maraeo@gmail.com>
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
 * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/* This file contains code for reading values from pipe queries
 * for displaying on the HUD. To prevent stalls when reading queries, we
 * keep a list of busy queries in a ring. We read only those queries which
 * are idle.
 */

#include "hud/hud_private.h"
#include "pipe/p_screen.h"
#include "os/os_time.h"
#include "util/u_memory.h"
#include <stdio.h>

#define NUM_QUERIES 8

struct query_info {
   struct pipe_context *pipe;
   unsigned query_type;
   unsigned result_index; /* unit depends on query_type */

   /* Ring of queries. If a query is busy, we use another slot. */
   struct pipe_query *query[NUM_QUERIES];
   unsigned head, tail;
   unsigned num_queries;

   uint64_t last_time;
   uint64_t results_cumulative;
   unsigned num_results;
};

static void
query_new_value(struct hud_graph *gr)
{
   struct query_info *info = gr->query_data;
   struct pipe_context *pipe = info->pipe;
   uint64_t now = os_time_get();

   if (info->last_time) {
      pipe->end_query(pipe, info->query[info->head]);

      /* read query results */
      while (1) {
         struct pipe_query *query = info->query[info->tail];
         union pipe_query_result result;
         uint64_t *res64 = (uint64_t *)&result;

         if (pipe->get_query_result(pipe, query, FALSE, &result)) {
            info->results_cumulative += res64[info->result_index];
            info->num_results++;

            if (info->tail == info->head)
               break;

            info->tail = (info->tail+1) % NUM_QUERIES;
         }
         else {
            /* the oldest query is busy */
            if ((info->head+1) % NUM_QUERIES == info->tail) {
               /* all queries are busy, throw away the last query and create
                * a new one */
               fprintf(stderr,
                       "gallium_hud: all queries are busy after %i frames, "
                       "can't add another query\n",
                       NUM_QUERIES);
               pipe->destroy_query(pipe, info->query[info->head]);
               info->query[info->head] =
                     pipe->create_query(pipe, info->query_type, 0);
            }
            else {
               /* the last query is busy, we need to add a new one we can use
                * for this frame */
               info->head = (info->head+1) % NUM_QUERIES;
               if (!info->query[info->head]) {
                  info->query[info->head] =
                        pipe->create_query(pipe, info->query_type, 0);
               }
            }
            break;
         }
      }

      if (info->num_results && info->last_time + gr->pane->period <= now) {
         /* compute the average value across all frames */
         hud_graph_add_value(gr, info->results_cumulative / info->num_results);

         info->last_time = now;
         info->results_cumulative = 0;
         info->num_results = 0;
      }

      pipe->begin_query(pipe, info->query[info->head]);
   }
   else {
      /* initialize */
      info->last_time = now;
      info->query[info->head] = pipe->create_query(pipe, info->query_type, 0);
      pipe->begin_query(pipe, info->query[info->head]);
   }
}

static void
free_query_info(void *ptr)
{
   struct query_info *info = ptr;

   if (info->last_time) {
      struct pipe_context *pipe = info->pipe;
      int i;

      pipe->end_query(pipe, info->query[info->head]);

      for (i = 0; i < Elements(info->query); i++) {
         if (info->query[i]) {
            pipe->destroy_query(pipe, info->query[i]);
         }
      }
   }
   FREE(info);
}

void
hud_pipe_query_install(struct hud_pane *pane, struct pipe_context *pipe,
                       const char *name, unsigned query_type,
                       unsigned result_index,
                       uint64_t max_value, boolean uses_byte_units)
{
   struct hud_graph *gr;
   struct query_info *info;

   gr = CALLOC_STRUCT(hud_graph);
   if (!gr)
      return;

   strcpy(gr->name, name);
   gr->query_data = CALLOC_STRUCT(query_info);
   if (!gr->query_data) {
      FREE(gr);
      return;
   }

   gr->query_new_value = query_new_value;
   gr->free_query_data = free_query_info;

   info = gr->query_data;
   info->pipe = pipe;
   info->query_type = query_type;
   info->result_index = result_index;

   hud_pane_add_graph(pane, gr);
   if (pane->max_value < max_value)
      hud_pane_set_max_value(pane, max_value);
   if (uses_byte_units)
      pane->uses_byte_units = TRUE;
}

boolean
hud_driver_query_install(struct hud_pane *pane, struct pipe_context *pipe,
                         const char *name)
{
   struct pipe_screen *screen = pipe->screen;
   struct pipe_driver_query_info query;
   unsigned num_queries, i;
   boolean found = FALSE;

   if (!screen->get_driver_query_info)
      return FALSE;

   num_queries = screen->get_driver_query_info(screen, 0, NULL);

   for (i = 0; i < num_queries; i++) {
      if (screen->get_driver_query_info(screen, i, &query) &&
          strcmp(query.name, name) == 0) {
         found = TRUE;
         break;
      }
   }

   if (!found)
      return FALSE;

   hud_pipe_query_install(pane, pipe, query.name, query.query_type, 0,
                      query.max_value, query.uses_byte_units);
   return TRUE;
}
