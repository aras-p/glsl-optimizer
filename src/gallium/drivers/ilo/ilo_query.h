/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef ILO_QUERY_H
#define ILO_QUERY_H

#include "ilo_common.h"

struct intel_bo;
struct ilo_context;

/**
 * Queries can be bound to various places in the driver.  While bound, it tells
 * the driver to collect the data indicated by the type of the query.
 */
struct ilo_query {
   unsigned type;
   bool active;

   struct list_head list;

   /* storage for the collected data */
   union pipe_query_result data;

   /* for queries that need to read hardware statistics */
   struct intel_bo *bo;
   int reg_read, reg_total;
   int reg_cmd_size; /* in dwords, as expected by ilo_cp */
};

void
ilo_init_query_functions(struct ilo_context *ilo);

bool
ilo_query_alloc_bo(struct ilo_query *q, int reg_count, int repeat_count,
                   struct intel_winsys *winsys);

#endif /* ILO_QUERY_H */
