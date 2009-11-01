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

#ifndef BRW_WINSYS_H
#define BRW_WINSYS_H

#include "pipe/p_compiler.h"

struct brw_winsys;
struct pipe_fence_handle;

/* This currently just wraps dri_bo:
 */
struct brw_winsys_buffer {
   struct brw_winsys_screen *sws;
   void *bo;
   unsigned offset;
   unsigned size;
};

enum brw_buffer_usage {
   I915_GEM_DOMAIN_RENDER,
   I915_GEM_DOMAIN_SAMPLER,
   I915_GEM_DOMAIN_VERTEX,
   I915_GEM_DOMAIN_INSTRUCTION,


   /* XXX: migrate from domains to explicit usage cases, eg below:
    */

   /* use on textures */
   BRW_USAGE_RENDER    = 0x01,
   BRW_USAGE_SAMPLER   = 0x02,
   BRW_USAGE_2D_TARGET = 0x04,
   BRW_USAGE_2D_SOURCE = 0x08,
   /* use on vertex */
   BRW_USAGE_VERTEX    = 0x10,
};

enum brw_buffer_type
{
   BRW_BUFFER_TYPE_TEXTURE,
   BRW_BUFFER_TYPE_SCANOUT, /**< a texture used for scanning out from */
   BRW_BUFFER_TYPE_VERTEX,
   BRW_BUFFER_TYPE_CURBE,
   BRW_BUFFER_TYPE_QUERY,
   BRW_BUFFER_TYPE_SHADER_CONSTANTS,
   BRW_BUFFER_TYPE_WM_SCRATCH,
   BRW_BUFFER_TYPE_BATCH,
   BRW_BUFFER_TYPE_STATE_CACHE,
};


/* AKA winsys context:
 */
struct brw_batchbuffer {

   struct brw_winsys *iws;
   struct brw_winsys_buffer *buf;

   /**
    * Values exported to speed up the writing the batchbuffer,
    * instead of having to go trough a accesor function for
    * each dword written.
    */
   /*{@*/
   uint8_t *map;
   uint8_t *ptr;
   size_t size;
   struct {
      uint8_t *end_ptr;
   } emit;


   size_t relocs;
   size_t max_relocs;
   /*@}*/
};

struct brw_winsys_screen {

   /**
    * Batchbuffer functions.
    */
   /*@{*/
   /**
    * Create a new batchbuffer.
    */
   struct brw_batchbuffer *(*batchbuffer_create)(struct brw_winsys_screen *iws);

   /**
    * Emit a relocation to a buffer.
    * Target position in batchbuffer is the same as ptr.
    */
   int (*batchbuffer_reloc)(struct brw_batchbuffer *batch,
			    unsigned offset,
                            struct brw_winsys_buffer *reloc,
			    unsigned pre_add,
                            enum brw_buffer_usage usage);

   /**
    * Flush a bufferbatch.
    */
   void (*batchbuffer_flush)(struct brw_batchbuffer *batch,
                             struct pipe_fence_handle **fence);

   /**
    * Destroy a batchbuffer.
    */
   void (*batchbuffer_destroy)(struct brw_batchbuffer *batch);
   /*@}*/


   /**
    * Buffer functions.
    */

   /*@{*/
   /**
    * Create a buffer.
    */
   struct brw_winsys_buffer *(*bo_alloc)( struct brw_winsys_screen *sws,
					  enum brw_buffer_type type,
					  unsigned size,
					  unsigned alignment );

   /* Reference and unreference buffers:
    */
   void (*bo_reference)( struct brw_winsys_buffer *buffer );
   void (*bo_unreference)( struct brw_winsys_buffer *buffer );
   void (*bo_emit_reloc)( struct brw_winsys_buffer *buffer,
			  unsigned domain,
			  unsigned a,
			  unsigned b,
			  unsigned offset,
			  struct brw_winsys_buffer *b2);

   void (*bo_subdata)(struct brw_winsys_buffer *buffer,
		      size_t offset,
		      size_t size,
		      const void *data);

   boolean (*bo_is_busy)(struct brw_winsys_buffer *buffer);
   boolean (*bo_references)(struct brw_winsys_buffer *a,
			    struct brw_winsys_buffer *b);

   /* XXX: couldn't this be handled by returning true/false on
    * bo_emit_reloc?
    */
   boolean (*check_aperture_space)( struct brw_winsys_screen *iws,
				    struct brw_winsys_buffer **buffers,
				    unsigned count );

   /**
    * Map a buffer.
    */
   void *(*bo_map)(struct brw_winsys_buffer *buffer,
		   boolean write);

   /**
    * Unmap a buffer.
    */
   void (*bo_unmap)(struct brw_winsys_buffer *buffer);
   /*@}*/


   /**
    * Fence functions.
    */
   /*@{*/
   /**
    * Reference fence and set ptr to fence.
    */
   void (*fence_reference)(struct brw_winsys *iws,
                           struct pipe_fence_handle **ptr,
                           struct pipe_fence_handle *fence);

   /**
    * Check if a fence has finished.
    */
   int (*fence_signalled)(struct brw_winsys *iws,
                          struct pipe_fence_handle *fence);

   /**
    * Wait on a fence to finish.
    */
   int (*fence_finish)(struct brw_winsys *iws,
                       struct pipe_fence_handle *fence);
   /*@}*/


   /**
    * Destroy the winsys.
    */
   void (*destroy)(struct brw_winsys *iws);
};


/**
 * Create brw pipe_screen.
 */
struct pipe_screen *brw_create_screen(struct brw_winsys *iws, unsigned pci_id);

/**
 * Create a brw pipe_context.
 */
struct pipe_context *brw_create_context(struct pipe_screen *screen);

/**
 * Get the brw_winsys buffer backing the texture.
 *
 * TODO UGLY
 */
struct pipe_texture;
boolean brw_get_texture_buffer_brw(struct pipe_texture *texture,
				    struct brw_winsys_buffer **buffer,
				    unsigned *stride);

/**
 * Wrap a brw_winsys buffer with a texture blanket.
 *
 * TODO UGLY
 */
struct pipe_texture * brw_texture_blanket_ws(struct pipe_screen *screen,
					     const struct pipe_texture *tmplt,
					     const unsigned *stride,
					     struct brw_winsys_buffer *buffer);




#endif
