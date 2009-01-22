/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

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

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */
 
#include <sched.h>
#include <errno.h>

#include "main/glheader.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/context.h"
#include "swrast/swrast.h"

#include "radeon_cs.h"
#include "r200_context.h"

#include "common_cmdbuf.h"
#include "r200_state.h"
#include "r200_ioctl.h"
#include "r200_tcl.h"
#include "r200_sanity.h"
#include "radeon_reg.h"

#include "drirenderbuffer.h"
#include "vblank.h"

#define R200_TIMEOUT             512
#define R200_IDLE_RETRY           16


/* At this point we were in FlushCmdBufLocked but we had lost our context, so
 * we need to unwire our current cmdbuf, hook the one with the saved state in
 * it, flush it, and then put the current one back.  This is so commands at the
 * start of a cmdbuf can rely on the state being kept from the previous one.
 */
static void r200BackUpAndEmitLostStateLocked( r200ContextPtr rmesa )
{
   GLuint nr_released_bufs;
   struct radeon_store saved_store;

   if (rmesa->backup_store.cmd_used == 0)
      return;

   if (R200_DEBUG & DEBUG_STATE)
      fprintf(stderr, "Emitting backup state on lost context\n");

   rmesa->radeon.lost_context = GL_FALSE;

   nr_released_bufs = rmesa->dma.nr_released_bufs;
   saved_store = rmesa->store;
   rmesa->dma.nr_released_bufs = 0;
   rmesa->store = rmesa->backup_store;
   rcommonFlushCmdBufLocked( &rmesa->radeon, __FUNCTION__ );
   rmesa->dma.nr_released_bufs = nr_released_bufs;
   rmesa->store = saved_store;
}

/* ================================================================
 * Buffer clear
 */
static void r200Clear( GLcontext *ctx, GLbitfield mask )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = rmesa->radeon.dri.drawable;
   GLuint flags = 0;
   GLuint color_mask = 0;
   GLint ret, i;
   GLint cx, cy, cw, ch;

   if ( R200_DEBUG & DEBUG_IOCTL ) {
      fprintf( stderr, "r200Clear\n");
   }

   {
      LOCK_HARDWARE( &rmesa->radeon );
      UNLOCK_HARDWARE( &rmesa->radeon );
      if ( dPriv->numClipRects == 0 ) 
	 return;
   }

   r200Flush( ctx );

   if ( mask & BUFFER_BIT_FRONT_LEFT ) {
      flags |= RADEON_FRONT;
      color_mask = rmesa->hw.msk.cmd[MSK_RB3D_PLANEMASK];
      mask &= ~BUFFER_BIT_FRONT_LEFT;
   }

   if ( mask & BUFFER_BIT_BACK_LEFT ) {
      flags |= RADEON_BACK;
      color_mask = rmesa->hw.msk.cmd[MSK_RB3D_PLANEMASK];
      mask &= ~BUFFER_BIT_BACK_LEFT;
   }

   if ( mask & BUFFER_BIT_DEPTH ) {
      flags |= RADEON_DEPTH;
      mask &= ~BUFFER_BIT_DEPTH;
   }

   if ( (mask & BUFFER_BIT_STENCIL) && rmesa->radeon.state.stencil.hwBuffer ) {
      flags |= RADEON_STENCIL;
      mask &= ~BUFFER_BIT_STENCIL;
   }

   if ( mask ) {
      if (R200_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "%s: swrast clear, mask: %x\n", __FUNCTION__, mask);
      _swrast_Clear( ctx, mask );
   }

   if ( !flags ) 
      return;

   if (rmesa->using_hyperz) {
      flags |= RADEON_USE_COMP_ZBUF;
/*      if (rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_R200)
	 flags |= RADEON_USE_HIERZ; */
      if (!(rmesa->radeon.state.stencil.hwBuffer) ||
	 ((flags & RADEON_DEPTH) && (flags & RADEON_STENCIL) &&
	    ((rmesa->radeon.state.stencil.clear & R200_STENCIL_WRITE_MASK) == R200_STENCIL_WRITE_MASK))) {
	  flags |= RADEON_CLEAR_FASTZ;
      }
   }

   LOCK_HARDWARE( &rmesa->radeon );

   /* compute region after locking: */
   cx = ctx->DrawBuffer->_Xmin;
   cy = ctx->DrawBuffer->_Ymin;
   cw = ctx->DrawBuffer->_Xmax - cx;
   ch = ctx->DrawBuffer->_Ymax - cy;

   /* Flip top to bottom */
   cx += dPriv->x;
   cy  = dPriv->y + dPriv->h - cy - ch;

   /* Throttle the number of clear ioctls we do.
    */
   while ( 1 ) {
      drm_radeon_getparam_t gp;
      int ret;
      int clear;

      gp.param = RADEON_PARAM_LAST_CLEAR;
      gp.value = (int *)&clear;
      ret = drmCommandWriteRead( rmesa->radeon.dri.fd,
		      DRM_RADEON_GETPARAM, &gp, sizeof(gp) );

      if ( ret ) {
	 fprintf( stderr, "%s: drmRadeonGetParam: %d\n", __FUNCTION__, ret );
	 exit(1);
      }

      /* Clear throttling needs more thought.
       */
      if ( rmesa->radeon.sarea->last_clear - clear <= 25 ) {
	 break;
      }
      
      if (rmesa->radeon.do_usleeps) {
	 UNLOCK_HARDWARE( &rmesa->radeon );
	 DO_USLEEP( 1 );
	 LOCK_HARDWARE( &rmesa->radeon );
      }
   }

   /* Send current state to the hardware */
   rcommonFlushCmdBufLocked( &rmesa->radeon, __FUNCTION__ );

   for ( i = 0 ; i < dPriv->numClipRects ; ) {
      GLint nr = MIN2( i + RADEON_NR_SAREA_CLIPRECTS, dPriv->numClipRects );
      drm_clip_rect_t *box = dPriv->pClipRects;
      drm_clip_rect_t *b = rmesa->radeon.sarea->boxes;
      drm_radeon_clear_t clear;
      drm_radeon_clear_rect_t depth_boxes[RADEON_NR_SAREA_CLIPRECTS];
      GLint n = 0;

      if (cw != dPriv->w || ch != dPriv->h) {
         /* clear subregion */
	 for ( ; i < nr ; i++ ) {
	    GLint x = box[i].x1;
	    GLint y = box[i].y1;
	    GLint w = box[i].x2 - x;
	    GLint h = box[i].y2 - y;

	    if ( x < cx ) w -= cx - x, x = cx;
	    if ( y < cy ) h -= cy - y, y = cy;
	    if ( x + w > cx + cw ) w = cx + cw - x;
	    if ( y + h > cy + ch ) h = cy + ch - y;
	    if ( w <= 0 ) continue;
	    if ( h <= 0 ) continue;

	    b->x1 = x;
	    b->y1 = y;
	    b->x2 = x + w;
	    b->y2 = y + h;
	    b++;
	    n++;
	 }
      } else {
         /* clear whole window */
	 for ( ; i < nr ; i++ ) {
	    *b++ = box[i];
	    n++;
	 }
      }

      rmesa->radeon.sarea->nbox = n;

      clear.flags       = flags;
      clear.clear_color = rmesa->radeon.state.color.clear;
      clear.clear_depth = rmesa->radeon.state.depth.clear;	/* needed for hyperz */
      clear.color_mask  = rmesa->hw.msk.cmd[MSK_RB3D_PLANEMASK];
      clear.depth_mask  = rmesa->radeon.state.stencil.clear;
      clear.depth_boxes = depth_boxes;

      n--;
      b = rmesa->radeon.sarea->boxes;
      for ( ; n >= 0 ; n-- ) {
	 depth_boxes[n].f[CLEAR_X1] = (float)b[n].x1;
	 depth_boxes[n].f[CLEAR_Y1] = (float)b[n].y1;
	 depth_boxes[n].f[CLEAR_X2] = (float)b[n].x2;
	 depth_boxes[n].f[CLEAR_Y2] = (float)b[n].y2;
	 depth_boxes[n].f[CLEAR_DEPTH] = ctx->Depth.Clear;
      }

      ret = drmCommandWrite( rmesa->radeon.dri.fd, DRM_RADEON_CLEAR,
			     &clear, sizeof(clear));


      if ( ret ) {
	 UNLOCK_HARDWARE( &rmesa->radeon );
	 fprintf( stderr, "DRM_RADEON_CLEAR: return = %d\n", ret );
	 exit( 1 );
      }
   }

   UNLOCK_HARDWARE( &rmesa->radeon );
   rmesa->hw.all_dirty = GL_TRUE;
}


void r200Flush( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );

   if (R200_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (rmesa->swtcl.flush)
      rmesa->swtcl.flush( ctx );

   if (rmesa->tcl.flush)
      rmesa->tcl.flush( ctx );

   r200EmitState( rmesa );

   if (rmesa->radeon.cmdbuf.cs->cdw)
      rcommonFlushCmdBuf( &rmesa->radeon, __FUNCTION__ );
}

/* Make sure all commands have been sent to the hardware and have
 * completed processing.
 */
void r200Finish( GLcontext *ctx )
{
   r200Flush( ctx );
   radeon_common_finish(ctx);
}


/* This version of AllocateMemoryMESA allocates only GART memory, and
 * only does so after the point at which the driver has been
 * initialized.
 *
 * Theoretically a valid context isn't required.  However, in this
 * implementation, it is, as I'm using the hardware lock to protect
 * the kernel data structures, and the current context to get the
 * device fd.
 */
void *r200AllocateMemoryMESA(__DRIscreen *screen, GLsizei size,
			     GLfloat readfreq, GLfloat writefreq, 
			     GLfloat priority)
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa;
   int region_offset;
   drm_radeon_mem_alloc_t alloc;
   int ret;

   if (R200_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s sz %d %f/%f/%f\n", __FUNCTION__, size, readfreq, 
	      writefreq, priority);

   if (!ctx || !(rmesa = R200_CONTEXT(ctx)) || !rmesa->radeon.radeonScreen->gartTextures.map)
      return NULL;

   if (getenv("R200_NO_ALLOC"))
      return NULL;

   alloc.region = RADEON_MEM_REGION_GART;
   alloc.alignment = 0;
   alloc.size = size;
   alloc.region_offset = &region_offset;

   ret = drmCommandWriteRead( rmesa->radeon.radeonScreen->driScreen->fd,
			      DRM_RADEON_ALLOC,
			      &alloc, sizeof(alloc));
   
   if (ret) {
      fprintf(stderr, "%s: DRM_RADEON_ALLOC ret %d\n", __FUNCTION__, ret);
      return NULL;
   }
   
   {
      char *region_start = (char *)rmesa->radeon.radeonScreen->gartTextures.map;
      return (void *)(region_start + region_offset);
   }
}


/* Called via glXFreeMemoryMESA() */
void r200FreeMemoryMESA(__DRIscreen *screen, GLvoid *pointer)
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa;
   ptrdiff_t region_offset;
   drm_radeon_mem_free_t memfree;
   int ret;

   if (R200_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s %p\n", __FUNCTION__, pointer);

   if (!ctx || !(rmesa = R200_CONTEXT(ctx)) || !rmesa->radeon.radeonScreen->gartTextures.map) {
      fprintf(stderr, "%s: no context\n", __FUNCTION__);
      return;
   }

   region_offset = (char *)pointer - (char *)rmesa->radeon.radeonScreen->gartTextures.map;

   if (region_offset < 0 || 
       region_offset > rmesa->radeon.radeonScreen->gartTextures.size) {
      fprintf(stderr, "offset %d outside range 0..%d\n", region_offset,
	      rmesa->radeon.radeonScreen->gartTextures.size);
      return;
   }

   memfree.region = RADEON_MEM_REGION_GART;
   memfree.region_offset = region_offset;
   
   ret = drmCommandWrite( rmesa->radeon.radeonScreen->driScreen->fd,
			  DRM_RADEON_FREE,
			  &memfree, sizeof(memfree));
   
   if (ret) 
      fprintf(stderr, "%s: DRM_RADEON_FREE ret %d\n", __FUNCTION__, ret);
}

/* Called via glXGetMemoryOffsetMESA() */
GLuint r200GetMemoryOffsetMESA(__DRIscreen *screen, const GLvoid *pointer)
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa;
   GLuint card_offset;

   if (!ctx || !(rmesa = R200_CONTEXT(ctx)) ) {
      fprintf(stderr, "%s: no context\n", __FUNCTION__);
      return ~0;
   }

   if (!r200IsGartMemory( rmesa, pointer, 0 ))
      return ~0;

   card_offset = r200GartOffsetFromVirtual( rmesa, pointer );

   return card_offset - rmesa->radeon.radeonScreen->gart_base;
}

GLboolean r200IsGartMemory( r200ContextPtr rmesa, const GLvoid *pointer,
			   GLint size )
{
   ptrdiff_t offset = (char *)pointer - (char *)rmesa->radeon.radeonScreen->gartTextures.map;
   int valid = (size >= 0 &&
		offset >= 0 &&
		offset + size < rmesa->radeon.radeonScreen->gartTextures.size);

   if (R200_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "r200IsGartMemory( %p ) : %d\n", pointer, valid );
   
   return valid;
}


GLuint r200GartOffsetFromVirtual( r200ContextPtr rmesa, const GLvoid *pointer )
{
   ptrdiff_t offset = (char *)pointer - (char *)rmesa->radeon.radeonScreen->gartTextures.map;

   if (offset < 0 || offset > rmesa->radeon.radeonScreen->gartTextures.size)
      return ~0;
   else
      return rmesa->radeon.radeonScreen->gart_texture_offset + offset;
}



void r200InitIoctlFuncs( struct dd_function_table *functions )
{
    functions->Clear = r200Clear;
    functions->Finish = r200Finish;
    functions->Flush = r200Flush;
}

