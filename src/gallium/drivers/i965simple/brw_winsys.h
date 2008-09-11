/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/**
 * \file
 * This is the interface that i965simple requires any window system
 * hosting it to implement.  This is the only include file in i965simple
 * which is public.
 *
 */

#ifndef BRW_WINSYS_H
#define BRW_WINSYS_H


#include "pipe/p_defines.h"


/* Pipe drivers are (meant to be!) independent of both GL and the
 * window system.  The window system provides a buffer manager and a
 * set of additional hooks for things like command buffer submission,
 * etc.
 *
 * There clearly has to be some agreement between the window system
 * driver and the hardware driver about the format of command buffers,
 * etc.
 */

struct pipe_buffer;
struct pipe_fence_handle;
struct pipe_winsys;
struct pipe_screen;


/* The pipe driver currently understands the following chipsets:
 */
#define PCI_CHIP_I965_G			0x29A2
#define PCI_CHIP_I965_Q			0x2992
#define PCI_CHIP_I965_G_1		0x2982
#define PCI_CHIP_I965_GM                0x2A02
#define PCI_CHIP_I965_GME               0x2A12


/* These are the names of all the state caches managed by the driver.
 * 
 * When data is uploaded to a buffer with buffer_subdata, we use the
 * special version of that function below so that information about
 * what type of data this is can be passed to the winsys backend.
 * That in turn allows the correct flags to be set in the aub file
 * dump to allow human-readable file dumps later on.
 */

enum brw_cache_id {
   BRW_CC_VP,
   BRW_CC_UNIT,
   BRW_WM_PROG,
   BRW_SAMPLER_DEFAULT_COLOR,
   BRW_SAMPLER,
   BRW_WM_UNIT,
   BRW_SF_PROG,
   BRW_SF_VP,
   BRW_SF_UNIT,
   BRW_VS_UNIT,
   BRW_VS_PROG,
   BRW_GS_UNIT,
   BRW_GS_PROG,
   BRW_CLIP_VP,
   BRW_CLIP_UNIT,
   BRW_CLIP_PROG,
   BRW_SS_SURFACE,
   BRW_SS_SURF_BIND,

   BRW_MAX_CACHE
};

#define BRW_CONSTANT_BUFFER BRW_MAX_CACHE

/**
 * Additional winsys interface for i965simple.
 *
 * It is an over-simple batchbuffer mechanism.  Will want to improve the
 * performance of this, perhaps based on the cmdstream stuff.  It
 * would be pretty impossible to implement swz on top of this
 * interface.
 *
 * Will also need additions/changes to implement static/dynamic
 * indirect state.
 */
struct brw_winsys {

   void (*destroy)(struct brw_winsys *);
   
   /**
    * Reserve space on batch buffer.
    *
    * Returns a null pointer if there is insufficient space in the batch buffer
    * to hold the requested number of dwords and relocations.
    *
    * The number of dwords should also include the number of relocations.
    */
   unsigned *(*batch_start)(struct brw_winsys *sws,
                            unsigned dwords,
                            unsigned relocs);

   void (*batch_dword)(struct brw_winsys *sws,
                       unsigned dword);

   /**
    * Emit a relocation to a buffer.
    *
    * Used not only when the buffer addresses are not pinned, but also to
    * ensure refered buffers will not be destroyed until the current batch
    * buffer execution is finished.
    *
    * The access flags is a combination of I915_BUFFER_ACCESS_WRITE and
    * I915_BUFFER_ACCESS_READ macros.
    */
   void (*batch_reloc)(struct brw_winsys *sws,
                       struct pipe_buffer *buf,
                       unsigned access_flags,
                       unsigned delta);


   /* Not used yet, but really want this:
    */
   void (*batch_end)( struct brw_winsys *sws );

   /**
    * Flush the batch buffer.
    *
    * Fence argument must point to NULL or to a previous fence, and the caller
    * must call fence_reference when done with the fence.
    */
   void (*batch_flush)(struct brw_winsys *sws,
                       struct pipe_fence_handle **fence);


   /* A version of buffer_subdata that includes information for the
    * simulator:
    */
   void (*buffer_subdata_typed)(struct brw_winsys *sws, 
				struct pipe_buffer *buf,
				unsigned long offset, 
				unsigned long size, 
				const void *data,
				unsigned data_type);
   

   /* A cheat so we don't have to think about relocations in a couple
    * of places yet:
    */
   unsigned (*get_buffer_offset)( struct brw_winsys *sws,
				  struct pipe_buffer *buf,
				  unsigned flags );

};

#define BRW_BUFFER_ACCESS_WRITE   0x1
#define BRW_BUFFER_ACCESS_READ    0x2

#define BRW_BUFFER_USAGE_LIT_VERTEX  (PIPE_BUFFER_USAGE_CUSTOM << 0)


struct pipe_context *brw_create(struct pipe_screen *,
                                struct brw_winsys *,
                                unsigned pci_id);

static inline boolean brw_batchbuffer_data(struct brw_winsys *winsys,
                                           const void *data,
                                           unsigned bytes)
{
   static const unsigned incr = sizeof(unsigned);
   uint i;
   const unsigned *udata = (const unsigned*)(data);
   unsigned size = bytes/incr;

   winsys->batch_start(winsys, size, 0);
   for (i = 0; i < size; ++i) {
      winsys->batch_dword(winsys, udata[i]);
   }
   winsys->batch_end(winsys);

   return (i == size);
}
#endif
