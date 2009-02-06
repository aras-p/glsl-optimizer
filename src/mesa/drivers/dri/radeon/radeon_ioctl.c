/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

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
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include <sched.h>
#include <errno.h> 

#include "main/glheader.h"
#include "main/imports.h"
#include "main/simple_list.h"
#include "swrast/swrast.h"

#include "radeon_context.h"
#include "common_cmdbuf.h"
#include "radeon_cs.h"
#include "radeon_state.h"
#include "radeon_ioctl.h"
#include "radeon_tcl.h"
#include "radeon_sanity.h"

#define STANDALONE_MMIO
#include "radeon_macros.h"  /* for INREG() */

#include "drirenderbuffer.h"
#include "vblank.h"

#define RADEON_TIMEOUT             512
#define RADEON_IDLE_RETRY           16

#define DEBUG_CMDBUF         0

static void radeonSaveHwState( r100ContextPtr rmesa )
{
   struct radeon_state_atom *atom;
   char * dest = rmesa->backup_store.cmd_buf;

   if (RADEON_DEBUG & DEBUG_STATE)
      fprintf(stderr, "%s\n", __FUNCTION__);
   
   rmesa->backup_store.cmd_used = 0;

   foreach( atom, &rmesa->hw.atomlist ) {
      if ( atom->check( rmesa->radeon.glCtx, 0 ) ) {
	 int size = atom->cmd_size * 4;
	 memcpy( dest, atom->cmd, size);
	 dest += size;
	 rmesa->backup_store.cmd_used += size;
	 if (RADEON_DEBUG & DEBUG_STATE)
	    radeon_print_state_atom( atom );
      }
   }

   assert( rmesa->backup_store.cmd_used <= RADEON_CMD_BUF_SZ );
   if (RADEON_DEBUG & DEBUG_STATE)
      fprintf(stderr, "Returning to radeonEmitState\n");
}

/* At this point we were in FlushCmdBufLocked but we had lost our context, so
 * we need to unwire our current cmdbuf, hook the one with the saved state in
 * it, flush it, and then put the current one back.  This is so commands at the
 * start of a cmdbuf can rely on the state being kept from the previous one.
 */
static void radeonBackUpAndEmitLostStateLocked( r100ContextPtr rmesa )
{
   GLuint nr_released_bufs;
   struct radeon_store saved_store;

   if (rmesa->backup_store.cmd_used == 0)
      return;

   if (RADEON_DEBUG & DEBUG_STATE)
      fprintf(stderr, "Emitting backup state on lost context\n");

   rmesa->radeon.lost_context = GL_FALSE;

   nr_released_bufs = rmesa->radeon.dma.nr_released_bufs;
   saved_store = rmesa->store;
   rmesa->radeon.dma.nr_released_bufs = 0;
   rmesa->store = rmesa->backup_store;
   rcommonFlushCmdBufLocked( &rmesa->radeon, __FUNCTION__ );
   rmesa->radeon.dma.nr_released_bufs = nr_released_bufs;
   rmesa->store = saved_store;
}

/* =============================================================
 * Kernel command buffer handling
 */

/* The state atoms will be emitted in the order they appear in the atom list,
 * so this step is important.
 */
void radeonSetUpAtomList( r100ContextPtr rmesa )
{
   int i, mtu = rmesa->radeon.glCtx->Const.MaxTextureUnits;

   make_empty_list(&rmesa->hw.atomlist);
   rmesa->hw.atomlist.name = "atom-list";

   insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.ctx);
   insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.set);
   insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.lin);
   insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.msk);
   insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.vpt);
   insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.tcl);
   insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.msc);
   for (i = 0; i < mtu; ++i) {
       insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.tex[i]);
       insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.txr[i]);
       insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.cube[i]);
   }
   insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.zbs);
   insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.mtl);
   for (i = 0; i < 3 + mtu; ++i)
      insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.mat[i]);
   for (i = 0; i < 8; ++i)
      insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.lit[i]);
   for (i = 0; i < 6; ++i)
      insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.ucp[i]);
   insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.eye);
   insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.grd);
   insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.fog);
   insert_at_tail(&rmesa->hw.atomlist, &rmesa->hw.glt);
}

static INLINE void radeonEmitAtoms(r100ContextPtr r100, GLboolean dirty)
{
   BATCH_LOCALS(&r100->radeon);
   struct radeon_state_atom *atom;
   int dwords;

   /* Emit actual atoms */
   foreach(atom, &r100->hw.atomlist) {
     if ((atom->dirty || r100->hw.all_dirty) == dirty) {
       dwords = (*atom->check) (r100->radeon.glCtx, atom);
       if (dwords) {
	  if (DEBUG_CMDBUF && RADEON_DEBUG & DEBUG_STATE) {
	     radeon_print_state_atom(atom);
	  }
	 if (atom->emit) {
	   (*atom->emit)(r100->radeon.glCtx, atom);
	 } else {
	   BEGIN_BATCH_NO_AUTOSTATE(dwords);
	   OUT_BATCH_TABLE(atom->cmd, dwords);
	   END_BATCH();
	 }
	 atom->dirty = GL_FALSE;
       } else {
	  if (DEBUG_CMDBUF && RADEON_DEBUG & DEBUG_STATE) {
	     fprintf(stderr, "  skip state %s\n",
		     atom->name);
	  }
       }
     }
   }
   
   COMMIT_BATCH();
}

void radeonEmitState( r100ContextPtr rmesa )
{
   struct radeon_state_atom *atom;
   char *dest;
   uint32_t dwords;

   if (RADEON_DEBUG & (DEBUG_STATE|DEBUG_PRIMS))
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (rmesa->save_on_next_emit) {
      radeonSaveHwState(rmesa);
      rmesa->save_on_next_emit = GL_FALSE;
   }

   if (rmesa->radeon.cmdbuf.cs->cdw)
       return;

   /* this code used to return here but now it emits zbs */

   /* To avoid going across the entire set of states multiple times, just check
    * for enough space for the case of emitting all state, and inline the
    * radeonAllocCmdBuf code here without all the checks.
    */
   rcommonEnsureCmdBufSpace(&rmesa->radeon, rmesa->hw.max_state_size, __FUNCTION__);
   dest = rmesa->store.cmd_buf + rmesa->store.cmd_used;

   /* We always always emit zbs, this is due to a bug found by keithw in
      the hardware and rediscovered after Erics changes by me.
      if you ever touch this code make sure you emit zbs otherwise
      you get tcl lockups on at least M7/7500 class of chips - airlied */
   rmesa->hw.zbs.dirty=1;

   if (!rmesa->radeon.cmdbuf.cs->cdw) {
     if (RADEON_DEBUG & DEBUG_STATE)
       fprintf(stderr, "Begin reemit state\n");
     
     radeonEmitAtoms(rmesa, GL_FALSE);
   }

   if (RADEON_DEBUG & DEBUG_STATE)
     fprintf(stderr, "Begin dirty state\n");

   radeonEmitAtoms(rmesa, GL_TRUE);
   rmesa->hw.is_dirty = GL_FALSE;
   rmesa->hw.all_dirty = GL_FALSE;

}

/* Fire a section of the retained (indexed_verts) buffer as a regular
 * primtive.  
 */
extern void radeonEmitVbufPrim( r100ContextPtr rmesa,
				GLuint vertex_format,
				GLuint primitive,
				GLuint vertex_nr )
{
   BATCH_LOCALS(&rmesa->radeon);

   assert(!(primitive & RADEON_CP_VC_CNTL_PRIM_WALK_IND));
   
   radeonEmitState( rmesa );

   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s cmd_used/4: %d\n", __FUNCTION__,
	      rmesa->store.cmd_used/4);
   
   BEGIN_BATCH(3);
   OUT_BATCH_PACKET3_CLIP(RADEON_CP_PACKET3_3D_DRAW_VBUF, 0);
   OUT_BATCH(vertex_format);
   OUT_BATCH(primitive |
	     RADEON_CP_VC_CNTL_PRIM_WALK_LIST |
	     RADEON_CP_VC_CNTL_COLOR_ORDER_RGBA |
	     RADEON_CP_VC_CNTL_MAOS_ENABLE |
	     RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE |
	     (vertex_nr << RADEON_CP_VC_CNTL_NUM_SHIFT));
   END_BATCH();
}


void radeonFlushElts( GLcontext *ctx )
{
  r100ContextPtr rmesa = R100_CONTEXT(ctx);
   int *cmd = (int *)(rmesa->store.cmd_buf + rmesa->store.elts_start);
   int dwords;
#if RADEON_OLD_PACKETS
   int nr = (rmesa->store.cmd_used - (rmesa->store.elts_start + 24)) / 2;
#else
   int nr = (rmesa->store.cmd_used - (rmesa->store.elts_start + 16)) / 2;
#endif

   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   assert( rmesa->radeon.dma.flush == radeonFlushElts );
   rmesa->radeon.dma.flush = NULL;

   /* Cope with odd number of elts:
    */
   rmesa->store.cmd_used = (rmesa->store.cmd_used + 2) & ~2;
   dwords = (rmesa->store.cmd_used - rmesa->store.elts_start) / 4;

#if RADEON_OLD_PACKETS
   cmd[1] |= (dwords - 3) << 16;
   cmd[5] |= nr << RADEON_CP_VC_CNTL_NUM_SHIFT;
#else
   cmd[1] |= (dwords - 3) << 16;
   cmd[3] |= nr << RADEON_CP_VC_CNTL_NUM_SHIFT;
#endif

   if (RADEON_DEBUG & DEBUG_SYNC) {
      fprintf(stderr, "%s: Syncing\n", __FUNCTION__);
      radeonFinish( rmesa->radeon.glCtx );
   }
}


GLushort *radeonAllocEltsOpenEnded( r100ContextPtr rmesa,
				    GLuint vertex_format,
				    GLuint primitive,
				    GLuint min_nr )
{
   GLushort *retval;

   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s %d\n", __FUNCTION__, min_nr);

   assert((primitive & RADEON_CP_VC_CNTL_PRIM_WALK_IND));
   
   radeonEmitState( rmesa );
   
   rmesa->tcl.elt_dma_bo = radeon_bo_open(rmesa->radeon.radeonScreen->bom,
					  0, R200_ELT_BUF_SZ, 4,
					  RADEON_GEM_DOMAIN_GTT, 0);
   rmesa->tcl.elt_dma_offset = 0;
   rmesa->tcl.elt_used = min_nr * 2;

   radeon_bo_map(rmesa->tcl.elt_dma_bo, 1);
   retval = rmesa->tcl.elt_dma_bo->ptr + rmesa->tcl.elt_dma_offset;

   if (RADEON_DEBUG & DEBUG_PRIMS)
      fprintf(stderr, "%s: header vfmt 0x%x prim %x \n",
	      __FUNCTION__,
	      vertex_format, primitive);

   assert(!rmesa->radeon.dma.flush);
   rmesa->radeon.glCtx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
   rmesa->radeon.dma.flush = radeonFlushElts;

   //   rmesa->store.elts_start = ((char *)cmd) - rmesa->store.cmd_buf;

   return retval;
}



void radeonEmitVertexAOS( r100ContextPtr rmesa,
			  GLuint vertex_size,
			  GLuint offset )
{
#if RADEON_OLD_PACKETS
   rmesa->ioctl.vertex_size = vertex_size;
   rmesa->ioctl.vertex_offset = offset;
#else
   BATCH_LOCALS(&rmesa->radeon);

   if (RADEON_DEBUG & (DEBUG_PRIMS|DEBUG_IOCTL))
      fprintf(stderr, "%s:  vertex_size 0x%x offset 0x%x \n",
	      __FUNCTION__, vertex_size, offset);

   BEGIN_BATCH(5);
   OUT_BATCH_PACKET3(RADEON_CP_PACKET3_3D_LOAD_VBPNTR, 2);
   OUT_BATCH(1);
   OUT_BATCH(vertex_size | (vertex_size << 8));
   OUT_BATCH_RELOC(offset, bo, offset, RADEON_GEM_DOMAIN_GTT, 0, 0);
   END_BATCH();
}
#endif
}
		       

void radeonEmitAOS( r100ContextPtr rmesa,
		    struct radeon_dma_region **component,
		    GLuint nr,
		    GLuint offset )
{
#if RADEON_OLD_PACKETS
   assert( nr == 1 );
   assert( component[0]->aos_size == component[0]->aos_stride );
   rmesa->ioctl.vertex_size = component[0]->aos_size;
   rmesa->ioctl.vertex_offset = 
      (component[0]->aos_start + offset * component[0]->aos_stride * 4);
#else
   drm_radeon_cmd_header_t *cmd;
   int sz = AOS_BUFSZ(nr);
   int i;
   int *tmp;

   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);


   cmd = (drm_radeon_cmd_header_t *)radeonAllocCmdBuf( rmesa, sz,
						  __FUNCTION__ );
   cmd[0].i = 0;
   cmd[0].header.cmd_type = RADEON_CMD_PACKET3;
   cmd[1].i = RADEON_CP_PACKET3_3D_LOAD_VBPNTR | (((sz / sizeof(int))-3) << 16);
   cmd[2].i = nr;
   tmp = &cmd[0].i;
   cmd += 3;

   for (i = 0 ; i < nr ; i++) {
      if (i & 1) {
	 cmd[0].i |= ((component[i]->aos_stride << 24) | 
		      (component[i]->aos_size << 16));
	 cmd[2].i = (component[i]->aos_start + 
		     offset * component[i]->aos_stride * 4);
	 cmd += 3;
      }
      else {
	 cmd[0].i = ((component[i]->aos_stride << 8) | 
		     (component[i]->aos_size << 0));
	 cmd[1].i = (component[i]->aos_start + 
		     offset * component[i]->aos_stride * 4);
      }
   }

   if (RADEON_DEBUG & DEBUG_VERTS) {
      fprintf(stderr, "%s:\n", __FUNCTION__);
      for (i = 0 ; i < sz ; i++)
	 fprintf(stderr, "   %d: %x\n", i, tmp[i]);
   }
#endif
}


#if 0
/* using already shifted color_fmt! */
void radeonEmitBlit( r100ContextPtr rmesa, /* FIXME: which drmMinor is required? */
		   GLuint color_fmt,
		   GLuint src_pitch,
		   GLuint src_offset,
		   GLuint dst_pitch,
		   GLuint dst_offset,
		   GLint srcx, GLint srcy,
		   GLint dstx, GLint dsty,
		   GLuint w, GLuint h )
{
   drm_radeon_cmd_header_t *cmd;

   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s src %x/%x %d,%d dst: %x/%x %d,%d sz: %dx%d\n",
	      __FUNCTION__, 
	      src_pitch, src_offset, srcx, srcy,
	      dst_pitch, dst_offset, dstx, dsty,
	      w, h);

   assert( (src_pitch & 63) == 0 );
   assert( (dst_pitch & 63) == 0 );
   assert( (src_offset & 1023) == 0 ); 
   assert( (dst_offset & 1023) == 0 ); 
   assert( w < (1<<16) );
   assert( h < (1<<16) );

   cmd = (drm_radeon_cmd_header_t *)radeonAllocCmdBuf( rmesa, 8 * sizeof(int),
						  __FUNCTION__ );


   cmd[0].i = 0;
   cmd[0].header.cmd_type = RADEON_CMD_PACKET3;
   cmd[1].i = RADEON_CP_PACKET3_CNTL_BITBLT_MULTI | (5 << 16);
   cmd[2].i = (RADEON_GMC_SRC_PITCH_OFFSET_CNTL |
	       RADEON_GMC_DST_PITCH_OFFSET_CNTL |
	       RADEON_GMC_BRUSH_NONE |
	       color_fmt |
	       RADEON_GMC_SRC_DATATYPE_COLOR |
	       RADEON_ROP3_S |
	       RADEON_DP_SRC_SOURCE_MEMORY |
	       RADEON_GMC_CLR_CMP_CNTL_DIS |
	       RADEON_GMC_WR_MSK_DIS );

   cmd[3].i = ((src_pitch/64)<<22) | (src_offset >> 10);
   cmd[4].i = ((dst_pitch/64)<<22) | (dst_offset >> 10);
   cmd[5].i = (srcx << 16) | srcy;
   cmd[6].i = (dstx << 16) | dsty; /* dst */
   cmd[7].i = (w << 16) | h;
}


void radeonEmitWait( r100ContextPtr rmesa, GLuint flags )
{
   drm_radeon_cmd_header_t *cmd;

   assert( !(flags & ~(RADEON_WAIT_2D|RADEON_WAIT_3D)) );

   cmd = (drm_radeon_cmd_header_t *)radeonAllocCmdBuf( rmesa, 1 * sizeof(int),
					   __FUNCTION__ );
   cmd[0].i = 0;
   cmd[0].wait.cmd_type = RADEON_CMD_WAIT;
   cmd[0].wait.flags = flags;
}
#endif

/* ================================================================
 * Buffer clear
 */
#define RADEON_MAX_CLEARS	256

static void radeonClear( GLcontext *ctx, GLbitfield mask )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = rmesa->radeon.dri.drawable;
   drm_radeon_sarea_t *sarea = rmesa->radeon.sarea;
   uint32_t clear;
   GLuint flags = 0;
   GLuint color_mask = 0;
   GLint ret, i;
   GLint cx, cy, cw, ch;

   if ( RADEON_DEBUG & DEBUG_IOCTL ) {
      fprintf( stderr, "radeonClear\n");
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

   if ( (mask & BUFFER_BIT_STENCIL) && rmesa->radeon.state.stencil.hwBuffer ) {
      flags |= RADEON_STENCIL;
      mask &= ~BUFFER_BIT_STENCIL;
   }

   if ( mask ) {
      if (RADEON_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "%s: swrast clear, mask: %x\n", __FUNCTION__, mask);
      _swrast_Clear( ctx, mask );
   }

   if ( !flags ) 
      return;

   if (rmesa->using_hyperz) {
      flags |= RADEON_USE_COMP_ZBUF;
/*      if (rmesa->radeon.radeonScreen->chipset & RADEON_CHIPSET_TCL) 
         flags |= RADEON_USE_HIERZ; */
      if (!(rmesa->radeon.state.stencil.hwBuffer) ||
	 ((flags & RADEON_DEPTH) && (flags & RADEON_STENCIL) &&
	    ((rmesa->radeon.state.stencil.clear & RADEON_STENCIL_WRITE_MASK) == RADEON_STENCIL_WRITE_MASK))) {
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
      int ret;
      drm_radeon_getparam_t gp;

      gp.param = RADEON_PARAM_LAST_CLEAR;
      gp.value = (int *)&clear;
      ret = drmCommandWriteRead( rmesa->radeon.dri.fd,
				 DRM_RADEON_GETPARAM, &gp, sizeof(gp) );

      if ( ret ) {
	 fprintf( stderr, "%s: drm_radeon_getparam_t: %d\n", __FUNCTION__, ret );
	 exit(1);
      }

      if ( sarea->last_clear - clear <= RADEON_MAX_CLEARS ) {
	 break;
      }

      if ( rmesa->radeon.do_usleeps ) {
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
         /* clear whole buffer */
	 for ( ; i < nr ; i++ ) {
	    *b++ = box[i];
	    n++;
	 }
      }

      rmesa->radeon.sarea->nbox = n;

      clear.flags       = flags;
      clear.clear_color = rmesa->radeon.state.color.clear;
      clear.clear_depth = rmesa->radeon.state.depth.clear;
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
	 depth_boxes[n].f[CLEAR_DEPTH] = 
	    (float)rmesa->radeon.state.depth.clear;
      }

      ret = drmCommandWrite( rmesa->radeon.dri.fd, DRM_RADEON_CLEAR,
			     &clear, sizeof(drm_radeon_clear_t));

      if ( ret ) {
	 UNLOCK_HARDWARE( &rmesa->radeon );
	 fprintf( stderr, "DRM_RADEON_CLEAR: return = %d\n", ret );
	 exit( 1 );
      }
   }

   UNLOCK_HARDWARE( &rmesa->radeon );
   rmesa->hw.all_dirty = GL_TRUE;
}

void radeonFlush( GLcontext *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT( ctx );

   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (rmesa->radeon.dma.flush)
      rmesa->radeon.dma.flush( ctx );

   radeonEmitState( rmesa );
   
   if (rmesa->radeon.cmdbuf.cs->cdw)
      rcommonFlushCmdBuf( &rmesa->radeon, __FUNCTION__ );
}

/* Make sure all commands have been sent to the hardware and have
 * completed processing.
 */
void radeonFinish( GLcontext *ctx )
{
   radeonFlush( ctx );
   radeon_common_finish(ctx);
}


void radeonInitIoctlFuncs( GLcontext *ctx )
{
    ctx->Driver.Clear = radeonClear;
    ctx->Driver.Finish = radeonFinish;
    ctx->Driver.Flush = radeonFlush;
}

