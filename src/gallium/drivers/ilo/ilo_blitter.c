/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2013 LunarG, Inc.
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

#include "util/u_blitter.h"

#include "ilo_context.h"
#include "ilo_blitter.h"

static bool
ilo_blitter_pipe_create(struct ilo_blitter *blitter)
{
   if (blitter->pipe_blitter)
      return true;

   blitter->pipe_blitter = util_blitter_create(&blitter->ilo->base);

   return (blitter->pipe_blitter != NULL);
}

/**
 * Create a blitter.  Because the use of util_blitter, this must be called
 * after the context is initialized.
 */
struct ilo_blitter *
ilo_blitter_create(struct ilo_context *ilo)
{
   struct ilo_blitter *blitter;

   blitter = CALLOC_STRUCT(ilo_blitter);
   if (!blitter)
      return NULL;

   blitter->ilo = ilo;

   if (!ilo_blitter_pipe_create(blitter)) {
      FREE(blitter);
      return NULL;
   }

   return blitter;
}

void
ilo_blitter_destroy(struct ilo_blitter *blitter)
{
   if (blitter->pipe_blitter)
      util_blitter_destroy(blitter->pipe_blitter);

   FREE(blitter);
}
