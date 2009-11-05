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
#include "pipe/p_error.h"

struct brw_winsys;
struct pipe_fence_handle;

/* Not sure why the winsys needs this:
 */
#define BRW_BATCH_SIZE (32*1024)


/* Need a tiny bit of information inside the abstract buffer struct:
 */
struct brw_winsys_buffer {
   unsigned *offset;
   unsigned size;
};


/* Should be possible to validate usages above against buffer creation
 * types, below:
 */
enum brw_buffer_type
{
   BRW_BUFFER_TYPE_TEXTURE,
   BRW_BUFFER_TYPE_SCANOUT, /**< a texture used for scanning out from */
   BRW_BUFFER_TYPE_VERTEX,
   BRW_BUFFER_TYPE_CURBE,
   BRW_BUFFER_TYPE_QUERY,
   BRW_BUFFER_TYPE_SHADER_CONSTANTS,
   BRW_BUFFER_TYPE_SHADER_SCRATCH,
   BRW_BUFFER_TYPE_BATCH,
   BRW_BUFFER_TYPE_GENERAL_STATE,
   BRW_BUFFER_TYPE_SURFACE_STATE,
   BRW_BUFFER_TYPE_PIXEL,       /* image uploads, pbo's, etc */
   BRW_BUFFER_TYPE_GENERIC,     /* unknown */
   BRW_BUFFER_TYPE_MAX		/* Count of possible values */
};


/* Describe the usage of a particular buffer in a relocation.  The DRM
 * winsys will translate these back to GEM read/write domain flags.
 */
enum brw_buffer_usage {
   BRW_USAGE_STATE,         /* INSTRUCTION, 0 */
   BRW_USAGE_QUERY_RESULT,	 /* INSTRUCTION, INSTRUCTION */
   BRW_USAGE_RENDER_TARGET, /* RENDER,      0 */
   BRW_USAGE_DEPTH_BUFFER,	 /* RENDER,      RENDER */
   BRW_USAGE_BLIT_SOURCE,	 /* RENDER,      0 */
   BRW_USAGE_BLIT_DEST,     /* RENDER,      RENDER */
   BRW_USAGE_SAMPLER,	 /* SAMPLER,     0 */
   BRW_USAGE_VERTEX,	 /* VERTEX,      0 */
   BRW_USAGE_SCRATCH,	 /* 0,           0 */
   BRW_USAGE_MAX
};

enum brw_buffer_data_type {
   BRW_DATA_GS_CC_VP,
   BRW_DATA_GS_CC_UNIT,
   BRW_DATA_GS_WM_PROG,
   BRW_DATA_GS_SAMPLER_DEFAULT_COLOR,
   BRW_DATA_GS_SAMPLER,
   BRW_DATA_GS_WM_UNIT,
   BRW_DATA_GS_SF_PROG,
   BRW_DATA_GS_SF_VP,
   BRW_DATA_GS_SF_UNIT,
   BRW_DATA_GS_VS_UNIT,
   BRW_DATA_GS_VS_PROG,
   BRW_DATA_GS_GS_UNIT,
   BRW_DATA_GS_GS_PROG,
   BRW_DATA_GS_CLIP_VP,
   BRW_DATA_GS_CLIP_UNIT,
   BRW_DATA_GS_CLIP_PROG,
   BRW_DATA_SS_SURFACE,
   BRW_DATA_SS_SURF_BIND,
   BRW_DATA_OTHER,
   BRW_DATA_MAX
};

struct brw_winsys_screen {


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

   /* delta -- added to b2->offset, and written into buffer
    * offset -- location above value is written to within buffer
    */
   int (*bo_emit_reloc)( struct brw_winsys_buffer *buffer,
			 enum brw_buffer_usage usage,
			 unsigned delta,
			 unsigned offset,
			 struct brw_winsys_buffer *b2);

   int (*bo_exec)( struct brw_winsys_buffer *buffer,
		   unsigned bytes_used );

   int (*bo_subdata)(struct brw_winsys_buffer *buffer,
                     enum brw_buffer_data_type data_type,
                     size_t offset,
                     size_t size,
                     const void *data);

   boolean (*bo_is_busy)(struct brw_winsys_buffer *buffer);
   boolean (*bo_references)(struct brw_winsys_buffer *a,
			    struct brw_winsys_buffer *b);

   /* XXX: couldn't this be handled by returning true/false on
    * bo_emit_reloc?
    */
   enum pipe_error (*check_aperture_space)( struct brw_winsys_screen *iws,
					    struct brw_winsys_buffer **buffers,
					    unsigned count );

   /**
    * Map a buffer.
    */
   void *(*bo_map)(struct brw_winsys_buffer *buffer,
                   enum brw_buffer_data_type data_type,
		   boolean write);

   /**
    * Unmap a buffer.
    */
   void (*bo_unmap)(struct brw_winsys_buffer *buffer);
   /*@}*/

   /**
    * Destroy the winsys.
    */
   void (*destroy)(struct brw_winsys_screen *iws);
};


/**
 * Create brw pipe_screen.
 */
struct pipe_screen *brw_create_screen(struct brw_winsys_screen *iws, unsigned pci_id);

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
boolean brw_texture_get_winsys_buffer(struct pipe_texture *texture,
				      struct brw_winsys_buffer **buffer,
				      unsigned *stride);

/**
 * Wrap a brw_winsys buffer with a texture blanket.
 *
 * TODO UGLY
 */
struct pipe_texture * 
brw_texture_blanket_winsys_buffer(struct pipe_screen *screen,
				  const struct pipe_texture *template,
				  const unsigned pitch,
				  struct brw_winsys_buffer *buffer);




#endif
