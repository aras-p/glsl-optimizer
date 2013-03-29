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

/* This file contains code for reading CPU load for displaying on the HUD.
 */

#include "hud/hud_private.h"
#include "os/os_time.h"
#include "util/u_memory.h"
#include <stdio.h>
#include <inttypes.h>

static boolean
get_cpu_stats(unsigned cpu_index, uint64_t *busy_time, uint64_t *total_time)
{
   char cpuname[32];
   char line[1024];
   FILE *f;

   if (cpu_index == ALL_CPUS)
      strcpy(cpuname, "cpu");
   else
      sprintf(cpuname, "cpu%u", cpu_index);

   f = fopen("/proc/stat", "r");
   if (!f)
      return FALSE;

   while (!feof(f) && fgets(line, sizeof(line), f)) {
      if (strstr(line, cpuname) == line) {
         uint64_t v[12];
         int i, num;

         num = sscanf(line,
                      "%s %"PRIu64" %"PRIu64" %"PRIu64" %"PRIu64" %"PRIu64
                      " %"PRIu64" %"PRIu64" %"PRIu64" %"PRIu64" %"PRIu64
                      " %"PRIu64" %"PRIu64"",
                      cpuname, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5],
                      &v[6], &v[7], &v[8], &v[9], &v[10], &v[11]);
         if (num < 5) {
            fclose(f);
            return FALSE;
         }

         /* user + nice + system */
         *busy_time = v[0] + v[1] + v[2];
         *total_time = *busy_time;

         /* ... + idle + iowait + irq + softirq + ...  */
         for (i = 3; i < num-1; i++) {
            *total_time += v[i];
         }
         fclose(f);
         return TRUE;
      }
   }
   fclose(f);
   return FALSE;
}

struct cpu_info {
   unsigned cpu_index;
   uint64_t last_cpu_busy, last_cpu_total, last_time;
};

static void
query_cpu_load(struct hud_graph *gr)
{
   struct cpu_info *info = gr->query_data;
   uint64_t now = os_time_get();

   if (info->last_time) {
      if (info->last_time + gr->pane->period <= now) {
         uint64_t cpu_busy, cpu_total, cpu_load;

         get_cpu_stats(info->cpu_index, &cpu_busy, &cpu_total);

         cpu_load = (cpu_busy - info->last_cpu_busy) * 100 /
                    (double)(cpu_total - info->last_cpu_total);
         hud_graph_add_value(gr, cpu_load);

         info->last_cpu_busy = cpu_busy;
         info->last_cpu_total = cpu_total;
         info->last_time = now;
      }
   }
   else {
      /* initialize */
      info->last_time = now;
      get_cpu_stats(info->cpu_index, &info->last_cpu_busy,
                    &info->last_cpu_total);
   }
}

void
hud_cpu_graph_install(struct hud_pane *pane, unsigned cpu_index)
{
   struct hud_graph *gr;
   struct cpu_info *info;
   uint64_t busy, total;

   /* see if the cpu exists */
   if (cpu_index != ALL_CPUS && !get_cpu_stats(cpu_index, &busy, &total)) {
      return;
   }

   gr = CALLOC_STRUCT(hud_graph);
   if (!gr)
      return;

   if (cpu_index == ALL_CPUS)
      strcpy(gr->name, "cpu");
   else
      sprintf(gr->name, "cpu%u", cpu_index);

   gr->query_data = CALLOC_STRUCT(cpu_info);
   if (!gr->query_data) {
      FREE(gr);
      return;
   }

   gr->query_new_value = query_cpu_load;
   gr->free_query_data = free;

   info = gr->query_data;
   info->cpu_index = cpu_index;

   hud_pane_add_graph(pane, gr);
   hud_pane_set_max_value(pane, 100);
}

int
hud_get_num_cpus(void)
{
   uint64_t busy, total;
   int i = 0;

   while (get_cpu_stats(i, &busy, &total))
      i++;

   return i;
}
