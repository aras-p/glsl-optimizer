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

#ifndef ILO_TRANSFER_H
#define ILO_TRANSFER_H

#include "pipe/p_state.h"

#include "ilo_common.h"

enum ilo_transfer_map_method {
   /* map() / map_gtt() / map_unsynchronized() */
   ILO_TRANSFER_MAP_CPU,
   ILO_TRANSFER_MAP_GTT,
   ILO_TRANSFER_MAP_UNSYNC,

   /* use staging system buffer */
   ILO_TRANSFER_MAP_SW_CONVERT,
   ILO_TRANSFER_MAP_SW_ZS,
};

struct ilo_transfer {
   struct pipe_transfer base;

   enum ilo_transfer_map_method method;
   void *ptr;

   void *staging_sys;
};

struct ilo_context;

static inline struct ilo_transfer *
ilo_transfer(struct pipe_transfer *transfer)
{
   return (struct ilo_transfer *) transfer;
}

void
ilo_init_transfer_functions(struct ilo_context *ilo);

#endif /* ILO_TRANSFER_H */
