/**************************************************************************
 *
 * Copyright © 2009 Jakob Bornecrantz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef INTEL_WINSYS_H
#define INTEL_WINSYS_H

#include "pipe/p_compiler.h"

struct intel_winsys;
struct intel_buffer;
struct intel_batchbuffer;
struct pipe_texture;
struct pipe_fence_handle;

enum intel_buffer_usage
{
   /* use on textures */
   INTEL_USAGE_RENDER    = 0x01,
   INTEL_USAGE_SAMPLER   = 0x02,
   INTEL_USAGE_2D_TARGET = 0x04,
   INTEL_USAGE_2D_SOURCE = 0x08,
   /* use on vertex */
   INTEL_USAGE_VERTEX    = 0x10
};

enum intel_buffer_type
{
   INTEL_NEW_TEXTURE,
   INTEL_NEW_SCANOUT, /**< a texture used for scanning out from */
   INTEL_NEW_VERTEX
};

enum intel_buffer_tile
{
   INTEL_TILE_NONE,
   INTEL_TILE_X,
   INTEL_TILE_Y
};

struct intel_batchbuffer {

   struct intel_winsys *iws;

   /**
    * Values exported to speed up the writing the batchbuffer,
    * instead of having to go trough a accesor function for
    * each dword written.
    */
   /*{@*/
   uint8_t *map;
   uint8_t *ptr;
   size_t size;

   size_t relocs;
   size_t max_relocs;
   /*@}*/
};

struct intel_winsys {

   /**
    * Batchbuffer functions.
    */
   /*@{*/
   /**
    * Create a new batchbuffer.
    */
   struct intel_batchbuffer *(*batchbuffer_create)(struct intel_winsys *iws);

   /**
    * Emit a relocation to a buffer.
    * Target position in batchbuffer is the same as ptr.
    *
    * @batch
    * @reloc buffer address to be inserted into target.
    * @usage how is the hardware going to use the buffer.
    * @offset add this to the reloc buffers address
    * @target buffer where to write the address, null for batchbuffer.
    */
   int (*batchbuffer_reloc)(struct intel_batchbuffer *batch,
                            struct intel_buffer *reloc,
                            enum intel_buffer_usage usage,
                            unsigned offset);

   /**
    * Flush a bufferbatch.
    */
   void (*batchbuffer_flush)(struct intel_batchbuffer *batch,
                             struct pipe_fence_handle **fence);

   /**
    * Destroy a batchbuffer.
    */
   void (*batchbuffer_destroy)(struct intel_batchbuffer *batch);
   /*@}*/


   /**
    * Buffer functions.
    */
   /*@{*/
   /**
    * Create a buffer.
    */
   struct intel_buffer *(*buffer_create)(struct intel_winsys *iws,
                                         unsigned size, unsigned alignment,
                                         enum intel_buffer_type type);

   /**
    * Fence a buffer with a fence reg.
    * Not to be confused with pipe_fence_handle.
    */
   int (*buffer_set_fence_reg)(struct intel_winsys *iws,
                               struct intel_buffer *buffer,
                               unsigned stride,
                               enum intel_buffer_tile tile);

   /**
    * Map a buffer.
    */
   void *(*buffer_map)(struct intel_winsys *iws,
                       struct intel_buffer *buffer,
                       boolean write);

   /**
    * Unmap a buffer.
    */
   void (*buffer_unmap)(struct intel_winsys *iws,
                        struct intel_buffer *buffer);

   /**
    * Write to a buffer.
    *
    * Arguments follows pipe_buffer_write.
    */
   int (*buffer_write)(struct intel_winsys *iws,
                       struct intel_buffer *dst,
                       size_t offset,
                       size_t size,
                       const void *data);

   void (*buffer_destroy)(struct intel_winsys *iws,
                          struct intel_buffer *buffer);
   /*@}*/


   /**
    * Fence functions.
    */
   /*@{*/
   /**
    * Reference fence and set ptr to fence.
    */
   void (*fence_reference)(struct intel_winsys *iws,
                           struct pipe_fence_handle **ptr,
                           struct pipe_fence_handle *fence);

   /**
    * Check if a fence has finished.
    */
   int (*fence_signalled)(struct intel_winsys *iws,
                          struct pipe_fence_handle *fence);

   /**
    * Wait on a fence to finish.
    */
   int (*fence_finish)(struct intel_winsys *iws,
                       struct pipe_fence_handle *fence);
   /*@}*/


   /**
    * Destroy the winsys.
    */
   void (*destroy)(struct intel_winsys *iws);
};


/**
 * Create i915 pipe_screen.
 */
struct pipe_screen *i915_create_screen(struct intel_winsys *iws, unsigned pci_id);


/**
 * Get the intel_winsys buffer backing the texture.
 *
 * TODO UGLY
 */
boolean i915_get_texture_buffer_intel(struct pipe_texture *texture,
                                      struct intel_buffer **buffer,
                                      unsigned *stride);

/**
 * Wrap a intel_winsys buffer with a texture blanket.
 *
 * TODO UGLY
 */
struct pipe_texture * i915_texture_blanket_intel(struct pipe_screen *screen,
                                                 struct pipe_texture *tmplt,
                                                 unsigned pitch,
                                                 struct intel_buffer *buffer);

#endif
