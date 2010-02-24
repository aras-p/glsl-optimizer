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



#include "radeon_common.h"
#include "radeon_lock.h"
#include "r200_context.h"
#include "r200_ioctl.h"
#include "radeon_reg.h"

#include "vblank.h"

#define R200_TIMEOUT             512
#define R200_IDLE_RETRY           16

static void r200KernelClear(GLcontext *ctx, GLuint flags)
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   __DRIdrawable *dPriv = radeon_get_drawable(&rmesa->radeon);
   GLint cx, cy, cw, ch, ret;
   GLuint i;

   radeonEmitState(&rmesa->radeon);

   LOCK_HARDWARE( &rmesa->radeon );

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


  /* compute region after locking: */
   cx = ctx->DrawBuffer->_Xmin;
   cy = ctx->DrawBuffer->_Ymin;
   cw = ctx->DrawBuffer->_Xmax - cx;
   ch = ctx->DrawBuffer->_Ymax - cy;

   /* Flip top to bottom */
   cx += dPriv->x;
   cy  = dPriv->y + dPriv->h - cy - ch;
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
}
/* ================================================================
 * Buffer clear
 */
static void r200Clear( GLcontext *ctx, GLbitfield mask )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   __DRIdrawable *dPriv = radeon_get_drawable(&rmesa->radeon);
   GLuint flags = 0;
   GLuint color_mask = 0;
   GLuint orig_mask = mask;

   if ( R200_DEBUG & RADEON_IOCTL ) {
	   if (rmesa->radeon.sarea)
	       fprintf( stderr, "r200Clear %x %d\n", mask, rmesa->radeon.sarea->pfCurrentPage);
	   else
	       fprintf( stderr, "r200Clear %x radeon->sarea is NULL\n", mask);
   }

   {
      LOCK_HARDWARE( &rmesa->radeon );
      UNLOCK_HARDWARE( &rmesa->radeon );
      if ( dPriv->numClipRects == 0 )
	 return;
   }

   radeonFlush( ctx );

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

   if ( (mask & BUFFER_BIT_STENCIL) ) {
      flags |= RADEON_STENCIL;
      mask &= ~BUFFER_BIT_STENCIL;
   }

   if ( mask ) {
      if (R200_DEBUG & RADEON_FALLBACKS)
	 fprintf(stderr, "%s: swrast clear, mask: %x\n", __FUNCTION__, mask);
      _swrast_Clear( ctx, mask );
   }

   if ( !flags )
      return;

   if (rmesa->using_hyperz) {
      flags |= RADEON_USE_COMP_ZBUF;
/*      if (rmesa->radeon.radeonScreen->chip_family == CHIP_FAMILY_R200)
	 flags |= RADEON_USE_HIERZ; */
      if (!((flags & RADEON_DEPTH) && (flags & RADEON_STENCIL) &&
	    ((rmesa->radeon.state.stencil.clear & R200_STENCIL_WRITE_MASK) == R200_STENCIL_WRITE_MASK))) {
	  flags |= RADEON_CLEAR_FASTZ;
      }
   }

   if (rmesa->radeon.radeonScreen->kernel_mm)
      radeonUserClear(ctx, orig_mask);
   else {
      r200KernelClear(ctx, flags);
      rmesa->radeon.hw.all_dirty = GL_TRUE;
   }
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

   if (R200_DEBUG & RADEON_IOCTL)
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

   if (R200_DEBUG & RADEON_IOCTL)
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

   if (R200_DEBUG & RADEON_IOCTL)
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
    functions->Finish = radeonFinish;
    functions->Flush = radeonFlush;
}

