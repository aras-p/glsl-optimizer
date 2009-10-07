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

#ifndef I915_SCREEN_H
#define I915_SCREEN_H

#include "pipe/p_state.h"
#include "pipe/p_screen.h"


struct intel_winsys;


/**
 * Subclass of pipe_screen
 */
struct i915_screen
{
   struct pipe_screen base;

   struct intel_winsys *iws;

   boolean is_i945;
   uint pci_id;
};

/**
 * Subclass of pipe_transfer
 */
struct i915_transfer
{
   struct pipe_transfer base;

   unsigned offset;
};


/*
 * Cast wrappers
 */


static INLINE struct i915_screen *
i915_screen(struct pipe_screen *pscreen)
{
   return (struct i915_screen *) pscreen;
}

static INLINE struct i915_transfer *
i915_transfer(struct pipe_transfer *transfer)
{
   return (struct i915_transfer *)transfer;
}


#endif /* I915_SCREEN_H */
