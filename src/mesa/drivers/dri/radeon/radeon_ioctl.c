/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_ioctl.c,v 1.11 2003/01/29 22:04:59 dawes Exp $ */
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

#include "glheader.h"
#include "imports.h"
#include "simple_list.h"
#include "swrast/swrast.h"

#include "radeon_context.h"
#include "radeon_state.h"
#include "radeon_ioctl.h"
#include "radeon_tcl.h"
#include "radeon_sanity.h"

#define STANDALONE_MMIO
#include "radeon_macros.h"  /* for INREG() */

#include "vblank.h"

#define RADEON_TIMEOUT             512
#define RADEON_IDLE_RETRY           16


static void radeonWaitForIdle( radeonContextPtr rmesa );

/* =============================================================
 * Kernel command buffer handling
 */

static void print_state_atom( struct radeon_state_atom *state )
{
   int i;

   fprintf(stderr, "emit %s/%d\n", state->name, state->cmd_size);

   if (RADEON_DEBUG & DEBUG_VERBOSE) 
      for (i = 0 ; i < state->cmd_size ; i++) 
	 fprintf(stderr, "\t%s[%d]: %x\n", state->name, i, state->cmd[i]);

}

static void radeon_emit_state_list( radeonContextPtr rmesa, 
				    struct radeon_state_atom *list )
{
   struct radeon_state_atom *state, *tmp;
   char *dest;
   int i, size, texunits;

   /* It appears that some permutations of state atoms lock up the
    * chip.  Therefore we make sure that state atoms are emitted in a
    * fixed order. First mark all dirty state atoms and then go
    * through all state atoms in a well defined order and emit only
    * the marked ones.
    * FIXME: This requires knowledge of which state atoms exist.
    * FIXME: Is the zbs hack below still needed?
    */
   size = 0;
   foreach_s( state, tmp, list ) {
      if (state->check( rmesa->glCtx )) {
	 size += state->cmd_size;
	 state->dirty = GL_TRUE;
	 move_to_head( &(rmesa->hw.clean), state );
	 if (RADEON_DEBUG & DEBUG_STATE) 
	    print_state_atom( state );
      }
      else if (RADEON_DEBUG & DEBUG_STATE)
	 fprintf(stderr, "skip state %s\n", state->name);
   }
   /* short cut */
   if (!size)
       return;

   dest = radeonAllocCmdBuf( rmesa, size * 4, __FUNCTION__);
   texunits = rmesa->glCtx->Const.MaxTextureUnits;

#define EMIT_ATOM(ATOM) \
do { \
   if (rmesa->hw.ATOM.dirty) { \
      rmesa->hw.ATOM.dirty = GL_FALSE; \
      memcpy( dest, rmesa->hw.ATOM.cmd, rmesa->hw.ATOM.cmd_size * 4); \
      dest += rmesa->hw.ATOM.cmd_size * 4; \
   } \
} while (0)

   EMIT_ATOM (ctx);
   EMIT_ATOM (set);
   EMIT_ATOM (lin);
   EMIT_ATOM (msk);
   EMIT_ATOM (vpt);
   EMIT_ATOM (tcl);
   EMIT_ATOM (msc);
   for (i = 0; i < texunits; ++i) {
       EMIT_ATOM (tex[i]);
       EMIT_ATOM (txr[i]);
   }
   EMIT_ATOM (zbs);
   EMIT_ATOM (mtl);
   for (i = 0; i < 3 + texunits; ++i)
       EMIT_ATOM (mat[i]);
   for (i = 0; i < 8; ++i)
       EMIT_ATOM (lit[i]);
   for (i = 0; i < 6; ++i)
       EMIT_ATOM (ucp[i]);
   EMIT_ATOM (eye);
   EMIT_ATOM (grd);
   EMIT_ATOM (fog);
   EMIT_ATOM (glt);

#undef EMIT_ATOM
}


void radeonEmitState( radeonContextPtr rmesa )
{
   struct radeon_state_atom *state, *tmp;

   if (RADEON_DEBUG & (DEBUG_STATE|DEBUG_PRIMS))
      fprintf(stderr, "%s\n", __FUNCTION__);

   /* Somewhat overkill:
    */
   if (rmesa->lost_context) {
      if (RADEON_DEBUG & (DEBUG_STATE|DEBUG_PRIMS|DEBUG_IOCTL))
	 fprintf(stderr, "%s - lost context\n", __FUNCTION__); 

      foreach_s( state, tmp, &(rmesa->hw.clean) ) 
	 move_to_tail(&(rmesa->hw.dirty), state );

      rmesa->lost_context = 0;
   }
   else if (1) {
      /* This is a darstardly kludge to work around a lockup that I
       * haven't otherwise figured out.
       */
      move_to_tail(&(rmesa->hw.dirty), &(rmesa->hw.zbs) );
   }

   if (!(rmesa->radeonScreen->chipset & RADEON_CHIPSET_TCL)) {
     foreach_s( state, tmp, &(rmesa->hw.dirty) ) {
       if (state->is_tcl) {
	 move_to_head( &(rmesa->hw.clean), state );
       }
     }
   }

   radeon_emit_state_list( rmesa, &rmesa->hw.dirty );
}



/* Fire a section of the retained (indexed_verts) buffer as a regular
 * primtive.  
 */
extern void radeonEmitVbufPrim( radeonContextPtr rmesa,
				GLuint vertex_format,
				GLuint primitive,
				GLuint vertex_nr )
{
   drm_radeon_cmd_header_t *cmd;


   assert(!(primitive & RADEON_CP_VC_CNTL_PRIM_WALK_IND));
   
   radeonEmitState( rmesa );

   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s cmd_used/4: %d\n", __FUNCTION__,
	      rmesa->store.cmd_used/4);
   
   cmd = (drm_radeon_cmd_header_t *)radeonAllocCmdBuf( rmesa, VBUF_BUFSZ,
						       __FUNCTION__ );
#if RADEON_OLD_PACKETS
   cmd[0].i = 0;
   cmd[0].header.cmd_type = RADEON_CMD_PACKET3_CLIP;
   cmd[1].i = RADEON_CP_PACKET3_3D_RNDR_GEN_INDX_PRIM | (3 << 16);
   cmd[2].i = rmesa->ioctl.vertex_offset;
   cmd[3].i = vertex_nr;
   cmd[4].i = vertex_format;
   cmd[5].i = (primitive | 
	       RADEON_CP_VC_CNTL_PRIM_WALK_LIST |
	       RADEON_CP_VC_CNTL_COLOR_ORDER_RGBA |
	       RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE |
	       (vertex_nr << RADEON_CP_VC_CNTL_NUM_SHIFT));

   if (RADEON_DEBUG & DEBUG_PRIMS)
      fprintf(stderr, "%s: header 0x%x offt 0x%x vfmt 0x%x vfcntl %x \n",
	      __FUNCTION__,
	      cmd[1].i, cmd[2].i, cmd[4].i, cmd[5].i);
#else
   cmd[0].i = 0;
   cmd[0].header.cmd_type = RADEON_CMD_PACKET3_CLIP;
   cmd[1].i = RADEON_CP_PACKET3_3D_DRAW_VBUF | (1 << 16);
   cmd[2].i = vertex_format;
   cmd[3].i = (primitive | 
	       RADEON_CP_VC_CNTL_PRIM_WALK_LIST |
	       RADEON_CP_VC_CNTL_COLOR_ORDER_RGBA |
	       RADEON_CP_VC_CNTL_MAOS_ENABLE |
	       RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE |
	       (vertex_nr << RADEON_CP_VC_CNTL_NUM_SHIFT));


   if (RADEON_DEBUG & DEBUG_PRIMS)
      fprintf(stderr, "%s: header 0x%x vfmt 0x%x vfcntl %x \n",
	      __FUNCTION__,
	      cmd[1].i, cmd[2].i, cmd[3].i);
#endif
}


void radeonFlushElts( radeonContextPtr rmesa )
{
   int *cmd = (int *)(rmesa->store.cmd_buf + rmesa->store.elts_start);
   int dwords;
#if RADEON_OLD_PACKETS
   int nr = (rmesa->store.cmd_used - (rmesa->store.elts_start + 24)) / 2;
#else
   int nr = (rmesa->store.cmd_used - (rmesa->store.elts_start + 16)) / 2;
#endif

   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   assert( rmesa->dma.flush == radeonFlushElts );
   rmesa->dma.flush = 0;

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
}


GLushort *radeonAllocEltsOpenEnded( radeonContextPtr rmesa,
				    GLuint vertex_format,
				    GLuint primitive,
				    GLuint min_nr )
{
   drm_radeon_cmd_header_t *cmd;
   GLushort *retval;

   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s %d\n", __FUNCTION__, min_nr);

   assert((primitive & RADEON_CP_VC_CNTL_PRIM_WALK_IND));
   
   radeonEmitState( rmesa );
   
   cmd = (drm_radeon_cmd_header_t *)radeonAllocCmdBuf( rmesa,
						       ELTS_BUFSZ(min_nr),
						       __FUNCTION__ );
#if RADEON_OLD_PACKETS
   cmd[0].i = 0;
   cmd[0].header.cmd_type = RADEON_CMD_PACKET3_CLIP;
   cmd[1].i = RADEON_CP_PACKET3_3D_RNDR_GEN_INDX_PRIM;
   cmd[2].i = rmesa->ioctl.vertex_offset;
   cmd[3].i = 0xffff;
   cmd[4].i = vertex_format;
   cmd[5].i = (primitive | 
	       RADEON_CP_VC_CNTL_PRIM_WALK_IND |
	       RADEON_CP_VC_CNTL_COLOR_ORDER_RGBA |
	       RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE);

   retval = (GLushort *)(cmd+6);
#else   
   cmd[0].i = 0;
   cmd[0].header.cmd_type = RADEON_CMD_PACKET3_CLIP;
   cmd[1].i = RADEON_CP_PACKET3_3D_DRAW_INDX;
   cmd[2].i = vertex_format;
   cmd[3].i = (primitive | 
	       RADEON_CP_VC_CNTL_PRIM_WALK_IND |
	       RADEON_CP_VC_CNTL_COLOR_ORDER_RGBA |
	       RADEON_CP_VC_CNTL_MAOS_ENABLE |
	       RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE);

   retval = (GLushort *)(cmd+4);
#endif

   if (RADEON_DEBUG & DEBUG_PRIMS)
      fprintf(stderr, "%s: header 0x%x vfmt 0x%x prim %x \n",
	      __FUNCTION__,
	      cmd[1].i, vertex_format, primitive);

   assert(!rmesa->dma.flush);
   rmesa->glCtx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
   rmesa->dma.flush = radeonFlushElts;

   rmesa->store.elts_start = ((char *)cmd) - rmesa->store.cmd_buf;

   return retval;
}



void radeonEmitVertexAOS( radeonContextPtr rmesa,
			  GLuint vertex_size,
			  GLuint offset )
{
#if RADEON_OLD_PACKETS
   rmesa->ioctl.vertex_size = vertex_size;
   rmesa->ioctl.vertex_offset = offset;
#else
   drm_radeon_cmd_header_t *cmd;

   if (RADEON_DEBUG & (DEBUG_PRIMS|DEBUG_IOCTL))
      fprintf(stderr, "%s:  vertex_size 0x%x offset 0x%x \n",
	      __FUNCTION__, vertex_size, offset);

   cmd = (drm_radeon_cmd_header_t *)radeonAllocCmdBuf( rmesa, VERT_AOS_BUFSZ,
						  __FUNCTION__ );

   cmd[0].i = 0;
   cmd[0].header.cmd_type = RADEON_CMD_PACKET3;
   cmd[1].i = RADEON_CP_PACKET3_3D_LOAD_VBPNTR | (2 << 16);
   cmd[2].i = 1;
   cmd[3].i = vertex_size | (vertex_size << 8);
   cmd[4].i = offset;
#endif
}
		       

void radeonEmitAOS( radeonContextPtr rmesa,
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
   int sz = AOS_BUFSZ;
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

/* using already shifted color_fmt! */
void radeonEmitBlit( radeonContextPtr rmesa, /* FIXME: which drmMinor is required? */
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


void radeonEmitWait( radeonContextPtr rmesa, GLuint flags )
{
   if (rmesa->dri.drmMinor >= 6) {
      drm_radeon_cmd_header_t *cmd;

      assert( !(flags & ~(RADEON_WAIT_2D|RADEON_WAIT_3D)) );
      
      cmd = (drm_radeon_cmd_header_t *)radeonAllocCmdBuf( rmesa, 1 * sizeof(int),
						   __FUNCTION__ );
      cmd[0].i = 0;
      cmd[0].wait.cmd_type = RADEON_CMD_WAIT;
      cmd[0].wait.flags = flags;
   }
}


static int radeonFlushCmdBufLocked( radeonContextPtr rmesa, 
				    const char * caller )
{
   int ret, i;
   drm_radeon_cmd_buffer_t cmd;

   if (RADEON_DEBUG & DEBUG_IOCTL) {
      fprintf(stderr, "%s from %s\n", __FUNCTION__, caller); 

      if (RADEON_DEBUG & DEBUG_VERBOSE) 
	 for (i = 0 ; i < rmesa->store.cmd_used ; i += 4 )
	    fprintf(stderr, "%d: %x\n", i/4, 
		    *(int *)(&rmesa->store.cmd_buf[i]));
   }

   if (RADEON_DEBUG & DEBUG_DMA)
      fprintf(stderr, "%s: Releasing %d buffers\n", __FUNCTION__,
	      rmesa->dma.nr_released_bufs);


   if (RADEON_DEBUG & DEBUG_SANITY) {
      if (rmesa->state.scissor.enabled) 
	 ret = radeonSanityCmdBuffer( rmesa, 
				      rmesa->state.scissor.numClipRects,
				      rmesa->state.scissor.pClipRects);
      else
	 ret = radeonSanityCmdBuffer( rmesa, 
				      rmesa->numClipRects,
				      rmesa->pClipRects);
      if (ret) {
	 fprintf(stderr, "drmSanityCommandWrite: %d\n", ret);	 
	 goto out;
      }
   }


   cmd.bufsz = rmesa->store.cmd_used;
   cmd.buf = rmesa->store.cmd_buf;

   if (rmesa->state.scissor.enabled) {
      cmd.nbox = rmesa->state.scissor.numClipRects;
      cmd.boxes = rmesa->state.scissor.pClipRects;
   } else {
      cmd.nbox = rmesa->numClipRects;
      cmd.boxes = rmesa->pClipRects;
   }

   ret = drmCommandWrite( rmesa->dri.fd,
			  DRM_RADEON_CMDBUF,
			  &cmd, sizeof(cmd) );

   if (ret)
      fprintf(stderr, "drmCommandWrite: %d\n", ret);

 out:
   rmesa->store.primnr = 0;
   rmesa->store.statenr = 0;
   rmesa->store.cmd_used = 0;
   rmesa->dma.nr_released_bufs = 0;
   /* Set lost_context so that the first state emit on the new buffer is a full
    * one.  This is because the context might get lost while preparing the next
    * buffer, and when we lock and find out, we don't have the information to
    * recreate the state.  This function should always be called before the new
    * buffer is begun, so it's sufficient to just set lost_context here.
    *
    * The alternative to this would be to copy out the state on unlock
    * (approximately) and if we did lose the context, dispatch a cmdbuf to reset
    * the state to that old copy before continuing with the accumulated command
    * buffer.
    */
   rmesa->lost_context = 1;

   return ret;
}


/* Note: does not emit any commands to avoid recursion on
 * radeonAllocCmdBuf.
 */
void radeonFlushCmdBuf( radeonContextPtr rmesa, const char *caller )
{
   int ret;

	      
   LOCK_HARDWARE( rmesa );

   ret = radeonFlushCmdBufLocked( rmesa, caller );

   UNLOCK_HARDWARE( rmesa );

   if (ret) {
      fprintf(stderr, "drm_radeon_cmd_buffer_t: %d (exiting)\n", ret);
      exit(ret);
   }
}

/* =============================================================
 * Hardware vertex buffer handling
 */


void radeonRefillCurrentDmaRegion( radeonContextPtr rmesa )
{
   struct radeon_dma_buffer *dmabuf;
   int fd = rmesa->dri.fd;
   int index = 0;
   int size = 0;
   drmDMAReq dma;
   int ret;

   if (RADEON_DEBUG & (DEBUG_IOCTL|DEBUG_DMA))
      fprintf(stderr, "%s\n", __FUNCTION__);  

   if (rmesa->dma.flush) {
      rmesa->dma.flush( rmesa );
   }

   if (rmesa->dma.current.buf)
      radeonReleaseDmaRegion( rmesa, &rmesa->dma.current, __FUNCTION__ );

   if (rmesa->dma.nr_released_bufs > 4)
      radeonFlushCmdBuf( rmesa, __FUNCTION__ );

   dma.context = rmesa->dri.hwContext;
   dma.send_count = 0;
   dma.send_list = NULL;
   dma.send_sizes = NULL;
   dma.flags = 0;
   dma.request_count = 1;
   dma.request_size = RADEON_BUFFER_SIZE;
   dma.request_list = &index;
   dma.request_sizes = &size;
   dma.granted_count = 0;

   LOCK_HARDWARE(rmesa);	/* no need to validate */

   ret = drmDMA( fd, &dma );
      
   if (ret != 0) {
      /* Free some up this way?
       */
      if (rmesa->dma.nr_released_bufs) {
	 radeonFlushCmdBufLocked( rmesa, __FUNCTION__ );
      }
      
      if (RADEON_DEBUG & DEBUG_DMA)
	 fprintf(stderr, "Waiting for buffers\n");

      radeonWaitForIdleLocked( rmesa );
      ret = drmDMA( fd, &dma );

      if ( ret != 0 ) {
	 UNLOCK_HARDWARE( rmesa );
	 fprintf( stderr, "Error: Could not get dma buffer... exiting\n" );
	 exit( -1 );
      }
   }

   UNLOCK_HARDWARE(rmesa);

   if (RADEON_DEBUG & DEBUG_DMA)
      fprintf(stderr, "Allocated buffer %d\n", index);

   dmabuf = CALLOC_STRUCT( radeon_dma_buffer );
   dmabuf->buf = &rmesa->radeonScreen->buffers->list[index];
   dmabuf->refcount = 1;

   rmesa->dma.current.buf = dmabuf;
   rmesa->dma.current.address = dmabuf->buf->address;
   rmesa->dma.current.end = dmabuf->buf->total;
   rmesa->dma.current.start = 0;
   rmesa->dma.current.ptr = 0;

   rmesa->c_vertexBuffers++;
}

void radeonReleaseDmaRegion( radeonContextPtr rmesa,
			     struct radeon_dma_region *region,
			     const char *caller )
{
   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s from %s\n", __FUNCTION__, caller); 
   
   if (!region->buf)
      return;

   if (rmesa->dma.flush)
      rmesa->dma.flush( rmesa );

   if (--region->buf->refcount == 0) {
      drm_radeon_cmd_header_t *cmd;

      if (RADEON_DEBUG & (DEBUG_IOCTL|DEBUG_DMA))
	 fprintf(stderr, "%s -- DISCARD BUF %d\n", __FUNCTION__,
		 region->buf->buf->idx);  
      
      cmd = (drm_radeon_cmd_header_t *)radeonAllocCmdBuf( rmesa, sizeof(*cmd), 
						     __FUNCTION__ );
      cmd->dma.cmd_type = RADEON_CMD_DMA_DISCARD;
      cmd->dma.buf_idx = region->buf->buf->idx;
      FREE(region->buf);
      rmesa->dma.nr_released_bufs++;
   }

   region->buf = 0;
   region->start = 0;
}

/* Allocates a region from rmesa->dma.current.  If there isn't enough
 * space in current, grab a new buffer (and discard what was left of current)
 */
void radeonAllocDmaRegion( radeonContextPtr rmesa, 
			   struct radeon_dma_region *region,
			   int bytes,
			   int alignment )
{
   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s %d\n", __FUNCTION__, bytes);

   if (rmesa->dma.flush)
      rmesa->dma.flush( rmesa );

   if (region->buf)
      radeonReleaseDmaRegion( rmesa, region, __FUNCTION__ );

   alignment--;
   rmesa->dma.current.start = rmesa->dma.current.ptr = 
      (rmesa->dma.current.ptr + alignment) & ~alignment;

   if ( rmesa->dma.current.ptr + bytes > rmesa->dma.current.end ) 
      radeonRefillCurrentDmaRegion( rmesa );

   region->start = rmesa->dma.current.start;
   region->ptr = rmesa->dma.current.start;
   region->end = rmesa->dma.current.start + bytes;
   region->address = rmesa->dma.current.address;
   region->buf = rmesa->dma.current.buf;
   region->buf->refcount++;

   rmesa->dma.current.ptr += bytes; /* bug - if alignment > 7 */
   rmesa->dma.current.start = 
      rmesa->dma.current.ptr = (rmesa->dma.current.ptr + 0x7) & ~0x7;  
}

void radeonAllocDmaRegionVerts( radeonContextPtr rmesa, 
				struct radeon_dma_region *region,
				int numverts,
				int vertsize,
				int alignment )
{
   radeonAllocDmaRegion( rmesa, region, vertsize * numverts, alignment );
}

/* ================================================================
 * SwapBuffers with client-side throttling
 */

static uint32_t radeonGetLastFrame (radeonContextPtr rmesa) 
{
   unsigned char *RADEONMMIO = rmesa->radeonScreen->mmio.map;
   int ret;
   uint32_t frame;

   if (rmesa->dri.screen->drmMinor >= 4) {
      drm_radeon_getparam_t gp;

      gp.param = RADEON_PARAM_LAST_FRAME;
      gp.value = (int *)&frame;
      ret = drmCommandWriteRead( rmesa->dri.fd, DRM_RADEON_GETPARAM,
				 &gp, sizeof(gp) );
   } 
   else
      ret = -EINVAL;

   if ( ret == -EINVAL ) {
      frame = INREG( RADEON_LAST_FRAME_REG );
      ret = 0;
   } 
   if ( ret ) {
      fprintf( stderr, "%s: drm_radeon_getparam_t: %d\n", __FUNCTION__, ret );
      exit(1);
   }

   return frame;
}

static void radeonEmitIrqLocked( radeonContextPtr rmesa )
{
   drm_radeon_irq_emit_t ie;
   int ret;

   ie.irq_seq = &rmesa->iw.irq_seq;
   ret = drmCommandWriteRead( rmesa->dri.fd, DRM_RADEON_IRQ_EMIT, 
			      &ie, sizeof(ie) );
   if ( ret ) {
      fprintf( stderr, "%s: drm_radeon_irq_emit_t: %d\n", __FUNCTION__, ret );
      exit(1);
   }
}


static void radeonWaitIrq( radeonContextPtr rmesa )
{
   int ret;

   do {
      ret = drmCommandWrite( rmesa->dri.fd, DRM_RADEON_IRQ_WAIT,
			     &rmesa->iw, sizeof(rmesa->iw) );
   } while (ret && (errno == EINTR || errno == EAGAIN));

   if ( ret ) {
      fprintf( stderr, "%s: drmRadeonIrqWait: %d\n", __FUNCTION__, ret );
      exit(1);
   }
}


static void radeonWaitForFrameCompletion( radeonContextPtr rmesa )
{
   drm_radeon_sarea_t *sarea = rmesa->sarea;

   if (rmesa->do_irqs) {
      if (radeonGetLastFrame(rmesa) < sarea->last_frame) {
	 if (!rmesa->irqsEmitted) {
	    while (radeonGetLastFrame (rmesa) < sarea->last_frame)
	       ;
	 }
	 else {
	    UNLOCK_HARDWARE( rmesa ); 
	    radeonWaitIrq( rmesa );	
	    LOCK_HARDWARE( rmesa ); 
	 }
	 rmesa->irqsEmitted = 10;
      }

      if (rmesa->irqsEmitted) {
	 radeonEmitIrqLocked( rmesa );
	 rmesa->irqsEmitted--;
      }
   } 
   else {
      while (radeonGetLastFrame (rmesa) < sarea->last_frame) {
	 UNLOCK_HARDWARE( rmesa ); 
	 if (rmesa->do_usleeps) 
	    DO_USLEEP( 1 );
	 LOCK_HARDWARE( rmesa ); 
      }
   }
}

/* Copy the back color buffer to the front color buffer.
 */
void radeonCopyBuffer( const __DRIdrawablePrivate *dPriv )
{
   radeonContextPtr rmesa;
   GLint nbox, i, ret;
   GLboolean   missed_target;
   int64_t ust;

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   rmesa = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;

   if ( RADEON_DEBUG & DEBUG_IOCTL ) {
      fprintf( stderr, "\n%s( %p )\n\n", __FUNCTION__, (void *) rmesa->glCtx );
   }

   RADEON_FIREVERTICES( rmesa );
   LOCK_HARDWARE( rmesa );

   /* Throttle the frame rate -- only allow one pending swap buffers
    * request at a time.
    */
   radeonWaitForFrameCompletion( rmesa );
   UNLOCK_HARDWARE( rmesa );
   driWaitForVBlank( dPriv, & rmesa->vbl_seq, rmesa->vblank_flags, & missed_target );
   LOCK_HARDWARE( rmesa );

   nbox = dPriv->numClipRects; /* must be in locked region */

   for ( i = 0 ; i < nbox ; ) {
      GLint nr = MIN2( i + RADEON_NR_SAREA_CLIPRECTS , nbox );
      drm_clip_rect_t *box = dPriv->pClipRects;
      drm_clip_rect_t *b = rmesa->sarea->boxes;
      GLint n = 0;

      for ( ; i < nr ; i++ ) {
	 *b++ = box[i];
	 n++;
      }
      rmesa->sarea->nbox = n;

      ret = drmCommandNone( rmesa->dri.fd, DRM_RADEON_SWAP );

      if ( ret ) {
	 fprintf( stderr, "DRM_RADEON_SWAP_BUFFERS: return = %d\n", ret );
	 UNLOCK_HARDWARE( rmesa );
	 exit( 1 );
      }
   }

   UNLOCK_HARDWARE( rmesa );
   rmesa->swap_count++;
   (*rmesa->get_ust)( & ust );
   if ( missed_target ) {
      rmesa->swap_missed_count++;
      rmesa->swap_missed_ust = ust - rmesa->swap_ust;
   }

   rmesa->swap_ust = ust;
}

void radeonPageFlip( const __DRIdrawablePrivate *dPriv )
{
   radeonContextPtr rmesa;
   GLint ret;
   GLboolean   missed_target;

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   rmesa = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;

   if ( RADEON_DEBUG & DEBUG_IOCTL ) {
      fprintf(stderr, "%s: pfCurrentPage: %d\n", __FUNCTION__,
	      rmesa->sarea->pfCurrentPage);
   }

   RADEON_FIREVERTICES( rmesa );
   LOCK_HARDWARE( rmesa );

   /* Need to do this for the perf box placement:
    */
   if (dPriv->numClipRects)
   {
      drm_clip_rect_t *box = dPriv->pClipRects;
      drm_clip_rect_t *b = rmesa->sarea->boxes;
      b[0] = box[0];
      rmesa->sarea->nbox = 1;
   }

   /* Throttle the frame rate -- only allow a few pending swap buffers
    * request at a time.
    */
   radeonWaitForFrameCompletion( rmesa );
   UNLOCK_HARDWARE( rmesa );
   driWaitForVBlank( dPriv, & rmesa->vbl_seq, rmesa->vblank_flags, & missed_target );
   if ( missed_target ) {
      rmesa->swap_missed_count++;
      (void) (*rmesa->get_ust)( & rmesa->swap_missed_ust );
   }
   LOCK_HARDWARE( rmesa );

   ret = drmCommandNone( rmesa->dri.fd, DRM_RADEON_FLIP );

   UNLOCK_HARDWARE( rmesa );

   if ( ret ) {
      fprintf( stderr, "DRM_RADEON_FLIP: return = %d\n", ret );
      exit( 1 );
   }

   rmesa->swap_count++;
   (void) (*rmesa->get_ust)( & rmesa->swap_ust );

   if ( rmesa->sarea->pfCurrentPage == 1 ) {
	 rmesa->state.color.drawOffset = rmesa->radeonScreen->frontOffset;
	 rmesa->state.color.drawPitch  = rmesa->radeonScreen->frontPitch;
   } else {
	 rmesa->state.color.drawOffset = rmesa->radeonScreen->backOffset;
	 rmesa->state.color.drawPitch  = rmesa->radeonScreen->backPitch;
   }

   RADEON_STATECHANGE( rmesa, ctx );
   rmesa->hw.ctx.cmd[CTX_RB3D_COLOROFFSET] = rmesa->state.color.drawOffset
					   + rmesa->radeonScreen->fbLocation;
   rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH]  = rmesa->state.color.drawPitch;
}


/* ================================================================
 * Buffer clear
 */
#define RADEON_MAX_CLEARS	256

static void radeonClear( GLcontext *ctx, GLbitfield mask, GLboolean all,
			 GLint cx, GLint cy, GLint cw, GLint ch )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;
   drm_radeon_sarea_t *sarea = rmesa->sarea;
   unsigned char *RADEONMMIO = rmesa->radeonScreen->mmio.map;
   uint32_t clear;
   GLuint flags = 0;
   GLuint color_mask = 0;
   GLint ret, i;

   if ( RADEON_DEBUG & DEBUG_IOCTL ) {
      fprintf( stderr, "%s:  all=%d cx=%d cy=%d cw=%d ch=%d\n",
	       __FUNCTION__, all, cx, cy, cw, ch );
   }

   RADEON_FIREVERTICES( rmesa ); 

   if ( mask & DD_FRONT_LEFT_BIT ) {
      flags |= RADEON_FRONT;
      color_mask = rmesa->hw.msk.cmd[MSK_RB3D_PLANEMASK];
      mask &= ~DD_FRONT_LEFT_BIT;
   }

   if ( mask & DD_BACK_LEFT_BIT ) {
      flags |= RADEON_BACK;
      color_mask = rmesa->hw.msk.cmd[MSK_RB3D_PLANEMASK];
      mask &= ~DD_BACK_LEFT_BIT;
   }

   if ( mask & DD_DEPTH_BIT ) {
      if ( ctx->Depth.Mask ) flags |= RADEON_DEPTH; /* FIXME: ??? */
      mask &= ~DD_DEPTH_BIT;
   }

   if ( (mask & DD_STENCIL_BIT) && rmesa->state.stencil.hwBuffer ) {
      flags |= RADEON_STENCIL;
      mask &= ~DD_STENCIL_BIT;
   }

   if ( mask ) {
      if (RADEON_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "%s: swrast clear, mask: %x\n", __FUNCTION__, mask);
      _swrast_Clear( ctx, mask, all, cx, cy, cw, ch );
   }

   if ( !flags ) 
      return;


   /* Flip top to bottom */
   cx += dPriv->x;
   cy  = dPriv->y + dPriv->h - cy - ch;

   /* We have to emit state along with the clear, since the kernel relies on
    * some of it.  The EmitState that was above RADEON_FIREVERTICES was an
    * attempt to do that, except that another context may come in and cause us
    * to lose our context while we're unlocked.
    */
   radeonEmitState( rmesa );

   LOCK_HARDWARE( rmesa );

   /* Throttle the number of clear ioctls we do.
    */
   while ( 1 ) {
      int ret;

      if (rmesa->dri.screen->drmMinor >= 4) {
	drm_radeon_getparam_t gp;

	gp.param = RADEON_PARAM_LAST_CLEAR;
	gp.value = (int *)&clear;
	ret = drmCommandWriteRead( rmesa->dri.fd,
				   DRM_RADEON_GETPARAM, &gp, sizeof(gp) );
      } else
	ret = -EINVAL;

      if ( ret == -EINVAL ) {
	 clear = INREG( RADEON_LAST_CLEAR_REG );
	 ret = 0;
      }
      if ( ret ) {
	 fprintf( stderr, "%s: drm_radeon_getparam_t: %d\n", __FUNCTION__, ret );
	 exit(1);
      }
      if ( RADEON_DEBUG & DEBUG_IOCTL ) {
	 fprintf( stderr, "%s( %d )\n", __FUNCTION__, (int)clear );
	 if ( ret ) fprintf( stderr, " ( RADEON_LAST_CLEAR register read directly )\n" );
      }

      if ( sarea->last_clear - clear <= RADEON_MAX_CLEARS ) {
	 break;
      }

      if ( rmesa->do_usleeps ) {
	 UNLOCK_HARDWARE( rmesa );
	 DO_USLEEP( 1 );
	 LOCK_HARDWARE( rmesa );
      }
   }

   /* Send current state to the hardware */
   radeonFlushCmdBufLocked( rmesa, __FUNCTION__ );

   for ( i = 0 ; i < dPriv->numClipRects ; ) {
      GLint nr = MIN2( i + RADEON_NR_SAREA_CLIPRECTS, dPriv->numClipRects );
      drm_clip_rect_t *box = dPriv->pClipRects;
      drm_clip_rect_t *b = rmesa->sarea->boxes;
      drm_radeon_clear_t clear;
      drm_radeon_clear_rect_t depth_boxes[RADEON_NR_SAREA_CLIPRECTS];
      GLint n = 0;

      if ( !all ) {
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
	 for ( ; i < nr ; i++ ) {
	    *b++ = box[i];
	    n++;
	 }
      }

      rmesa->sarea->nbox = n;

      clear.flags       = flags;
      clear.clear_color = rmesa->state.color.clear;
      clear.clear_depth = rmesa->state.depth.clear;
      clear.color_mask  = rmesa->hw.msk.cmd[MSK_RB3D_PLANEMASK];
      clear.depth_mask  = rmesa->state.stencil.clear;
      clear.depth_boxes = depth_boxes;

      n--;
      b = rmesa->sarea->boxes;
      for ( ; n >= 0 ; n-- ) {
	 depth_boxes[n].f[CLEAR_X1] = (float)b[n].x1;
	 depth_boxes[n].f[CLEAR_Y1] = (float)b[n].y1;
	 depth_boxes[n].f[CLEAR_X2] = (float)b[n].x2;
	 depth_boxes[n].f[CLEAR_Y2] = (float)b[n].y2;
	 depth_boxes[n].f[CLEAR_DEPTH] = 
	    (float)rmesa->state.depth.clear;
      }

      ret = drmCommandWrite( rmesa->dri.fd, DRM_RADEON_CLEAR,
			     &clear, sizeof(drm_radeon_clear_t));

      if ( ret ) {
	 UNLOCK_HARDWARE( rmesa );
	 fprintf( stderr, "DRM_RADEON_CLEAR: return = %d\n", ret );
	 exit( 1 );
      }
   }

   UNLOCK_HARDWARE( rmesa );
}


void radeonWaitForIdleLocked( radeonContextPtr rmesa )
{
    int fd = rmesa->dri.fd;
    int to = 0;
    int ret, i = 0;

    rmesa->c_drawWaits++;

    do {
        do {
            ret = drmCommandNone( fd, DRM_RADEON_CP_IDLE);
        } while ( ret && errno == EBUSY && i++ < RADEON_IDLE_RETRY );
    } while ( ( ret == -EBUSY ) && ( to++ < RADEON_TIMEOUT ) );

    if ( ret < 0 ) {
	UNLOCK_HARDWARE( rmesa );
	fprintf( stderr, "Error: Radeon timed out... exiting\n" );
	exit( -1 );
    }
}


static void radeonWaitForIdle( radeonContextPtr rmesa )
{
   LOCK_HARDWARE(rmesa);
   radeonWaitForIdleLocked( rmesa );
   UNLOCK_HARDWARE(rmesa);
}


void radeonFlush( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT( ctx );

   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (rmesa->dma.flush)
      rmesa->dma.flush( rmesa );

   if (!is_empty_list(&rmesa->hw.dirty)) 
      radeonEmitState( rmesa );
   
   if (rmesa->store.cmd_used)
      radeonFlushCmdBuf( rmesa, __FUNCTION__ );
}

/* Make sure all commands have been sent to the hardware and have
 * completed processing.
 */
void radeonFinish( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   radeonFlush( ctx );

   if (rmesa->do_irqs) {
      LOCK_HARDWARE( rmesa );
      radeonEmitIrqLocked( rmesa );
      UNLOCK_HARDWARE( rmesa );
      radeonWaitIrq( rmesa );
   }
   else
      radeonWaitForIdle( rmesa );
}


void radeonInitIoctlFuncs( GLcontext *ctx )
{
    ctx->Driver.Clear = radeonClear;
    ctx->Driver.Finish = radeonFinish;
    ctx->Driver.Flush = radeonFlush;
}

