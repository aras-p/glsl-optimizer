/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

/** @file brw_state_pool.c
 * Implements the state pool allocator.
 *
 * For the 965, we create two state pools for state cache entries.  Objects
 * will be allocated into the pools depending on which state base address
 * their pointer is relative to in other 965 state.
 *
 * The state pools are relatively simple: As objects are allocated, increment
 * the offset to allocate space.  When the pool is "full" (rather, close to
 * full), we reset the pool and reset the state cache entries that point into
 * the pool.
 */

#include "pipe/p_winsys.h"
#include "brw_context.h"
#include "brw_state.h"

boolean brw_pool_alloc( struct brw_mem_pool *pool,
			  unsigned size,
			  unsigned align,
			  unsigned *offset_return)
{
   unsigned fixup = ALIGN(pool->offset, align) - pool->offset;

   size = ALIGN(size, 4);

   if (pool->offset + fixup + size >= pool->size) {
      printf("%s failed\n", __FUNCTION__);
      assert(0);
      exit(0);
   }

   pool->offset += fixup;
   *offset_return = pool->offset;
   pool->offset += size;

   return TRUE;
}

static
void brw_invalidate_pool( struct brw_mem_pool *pool )
{
   if (BRW_DEBUG & DEBUG_STATE)
      printf("\n\n\n %s \n\n\n", __FUNCTION__);

   pool->offset = 0;

   brw_clear_all_caches(pool->brw);
}


static void brw_init_pool( struct brw_context *brw,
			   unsigned pool_id,
			   unsigned size )
{
   struct brw_mem_pool *pool = &brw->pool[pool_id];

   pool->size = size;
   pool->brw = brw;

   pool->buffer = brw->pipe.winsys->buffer_create(brw->pipe.winsys,
						  4096, 0, 0);

   brw->pipe.winsys->buffer_data(brw->pipe.winsys,
				 pool->buffer,
				 size,
				 NULL,
				 0 /*  DRM_BO_FLAG_MEM_TT */);
}

static void brw_destroy_pool( struct brw_context *brw,
			      unsigned pool_id )
{
   struct brw_mem_pool *pool = &brw->pool[pool_id];

   pool->brw->pipe.winsys->buffer_reference( pool->brw->pipe.winsys,
					     &pool->buffer,
					     NULL );
}


void brw_pool_check_wrap( struct brw_context *brw,
			  struct brw_mem_pool *pool )
{
   if (pool->offset > (pool->size * 3) / 4) {
      brw->state.dirty.brw |= BRW_NEW_CONTEXT;
   }

}

void brw_init_pools( struct brw_context *brw )
{
   brw_init_pool(brw, BRW_GS_POOL, 0x80000);
   brw_init_pool(brw, BRW_SS_POOL, 0x80000);
}

void brw_destroy_pools( struct brw_context *brw )
{
   brw_destroy_pool(brw, BRW_GS_POOL);
   brw_destroy_pool(brw, BRW_SS_POOL);
}


void brw_invalidate_pools( struct brw_context *brw )
{
   brw_invalidate_pool(&brw->pool[BRW_GS_POOL]);
   brw_invalidate_pool(&brw->pool[BRW_SS_POOL]);
}
