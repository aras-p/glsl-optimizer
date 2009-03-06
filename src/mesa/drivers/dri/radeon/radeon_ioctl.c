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

#include "main/attrib.h"
#include "main/enable.h"
#include "main/blend.h"
#include "main/bufferobj.h"
#include "main/buffers.h"
#include "main/depth.h"
#include "main/shaders.h"
#include "main/texstate.h"
#include "main/varray.h"
#include "glapi/dispatch.h"
#include "swrast/swrast.h"
#include "main/stencil.h"
#include "main/matrix.h"

#include "main/glheader.h"
#include "main/imports.h"
#include "main/simple_list.h"
#include "swrast/swrast.h"

#include "radeon_context.h"
#include "radeon_common.h"
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


/* =============================================================
 * Kernel command buffer handling
 */

/* The state atoms will be emitted in the order they appear in the atom list,
 * so this step is important.
 */
void radeonSetUpAtomList( r100ContextPtr rmesa )
{
   int i, mtu = rmesa->radeon.glCtx->Const.MaxTextureUnits;

   make_empty_list(&rmesa->radeon.hw.atomlist);
   rmesa->radeon.hw.atomlist.name = "atom-list";

   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.ctx);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.set);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.lin);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.msk);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.vpt);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.tcl);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.msc);
   for (i = 0; i < mtu; ++i) {
       insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.tex[i]);
       insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.txr[i]);
       insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.cube[i]);
   }
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.zbs);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.mtl);
   for (i = 0; i < 3 + mtu; ++i)
      insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.mat[i]);
   for (i = 0; i < 8; ++i)
      insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.lit[i]);
   for (i = 0; i < 6; ++i)
      insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.ucp[i]);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.eye);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.grd);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.fog);
   insert_at_tail(&rmesa->radeon.hw.atomlist, &rmesa->hw.glt);
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
   
   radeonEmitState(&rmesa->radeon);

#if RADEON_OLD_PACKETS
   BEGIN_BATCH(8);
   OUT_BATCH_PACKET3_CLIP(RADEON_CP_PACKET3_3D_RNDR_GEN_INDX_PRIM, 3);
   if (!rmesa->radeon.radeonScreen->kernel_mm) {
     OUT_BATCH_RELOC(rmesa->ioctl.vertex_offset, rmesa->ioctl.bo, rmesa->ioctl.vertex_offset, RADEON_GEM_DOMAIN_GTT, 0, 0);
   } else {
     OUT_BATCH(rmesa->ioctl.vertex_offset);
   }
    
   OUT_BATCH(vertex_nr);
   OUT_BATCH(vertex_format);
   OUT_BATCH(primitive |  RADEON_CP_VC_CNTL_PRIM_WALK_LIST |
	     RADEON_CP_VC_CNTL_COLOR_ORDER_RGBA |
	     RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE |
	     (vertex_nr << RADEON_CP_VC_CNTL_NUM_SHIFT));

   if (rmesa->radeon.radeonScreen->kernel_mm) {
     radeon_cs_write_reloc(rmesa->radeon.cmdbuf.cs,
			   rmesa->ioctl.bo,
			   RADEON_GEM_DOMAIN_GTT,
			   0, 0);
   }
   
   END_BATCH();
   
#else   
   BEGIN_BATCH(4);
   OUT_BATCH_PACKET3_CLIP(RADEON_CP_PACKET3_3D_DRAW_VBUF, 1);
   OUT_BATCH(vertex_format);
   OUT_BATCH(primitive |
	     RADEON_CP_VC_CNTL_PRIM_WALK_LIST |
	     RADEON_CP_VC_CNTL_COLOR_ORDER_RGBA |
	     RADEON_CP_VC_CNTL_MAOS_ENABLE |
	     RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE |
	     (vertex_nr << RADEON_CP_VC_CNTL_NUM_SHIFT));
   END_BATCH();
#endif
}

void radeonFlushElts( GLcontext *ctx )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   BATCH_LOCALS(&rmesa->radeon);
   int nr;
   uint32_t *cmd = (uint32_t *)(rmesa->radeon.cmdbuf.cs->packets + rmesa->tcl.elt_cmd_start);
   int dwords = (rmesa->radeon.cmdbuf.cs->section_ndw - rmesa->radeon.cmdbuf.cs->section_cdw);
   
   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   assert( rmesa->radeon.dma.flush == radeonFlushElts );
   rmesa->radeon.dma.flush = NULL;

   nr = rmesa->tcl.elt_used;

#if RADEON_OLD_PACKETS
   if (rmesa->radeon.radeonScreen->kernel_mm) {
     dwords -= 2;
   }
#endif

#if RADEON_OLD_PACKETS
   cmd[1] |= (dwords + 3) << 16;
   cmd[5] |= nr << RADEON_CP_VC_CNTL_NUM_SHIFT;
#else
   cmd[1] |= (dwords + 2) << 16;
   cmd[3] |= nr << RADEON_CP_VC_CNTL_NUM_SHIFT;
#endif

   rmesa->radeon.cmdbuf.cs->cdw += dwords;
   rmesa->radeon.cmdbuf.cs->section_cdw += dwords;

#if RADEON_OLD_PACKETS
   if (rmesa->radeon.radeonScreen->kernel_mm) {
      radeon_cs_write_reloc(rmesa->radeon.cmdbuf.cs,
			    rmesa->ioctl.bo,
			    RADEON_GEM_DOMAIN_GTT,
			    0, 0);
   }
#endif

   END_BATCH();

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
   int align_min_nr;
   BATCH_LOCALS(&rmesa->radeon);

   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s %d prim %x\n", __FUNCTION__, min_nr, primitive);

   assert((primitive & RADEON_CP_VC_CNTL_PRIM_WALK_IND));
   
   radeonEmitState(&rmesa->radeon);
   
   rmesa->tcl.elt_cmd_start = rmesa->radeon.cmdbuf.cs->cdw;

   /* round up min_nr to align the state */
   align_min_nr = (min_nr + 1) & ~1;

#if RADEON_OLD_PACKETS
   BEGIN_BATCH_NO_AUTOSTATE(2+ELTS_BUFSZ(align_min_nr)/4);
   OUT_BATCH_PACKET3_CLIP(RADEON_CP_PACKET3_3D_RNDR_GEN_INDX_PRIM, 0);
   if (!rmesa->radeon.radeonScreen->kernel_mm) {
     OUT_BATCH_RELOC(rmesa->ioctl.vertex_offset, rmesa->ioctl.bo, rmesa->ioctl.vertex_offset, RADEON_GEM_DOMAIN_GTT, 0, 0);
   } else {
     OUT_BATCH(rmesa->ioctl.vertex_offset);
   }
   OUT_BATCH(0xffff);
   OUT_BATCH(vertex_format);
   OUT_BATCH(primitive | 
	     RADEON_CP_VC_CNTL_PRIM_WALK_IND |
	     RADEON_CP_VC_CNTL_COLOR_ORDER_RGBA |
	     RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE);

#else
   BEGIN_BATCH_NO_AUTOSTATE(ELTS_BUFSZ(align_min_nr)/4);
   OUT_BATCH_PACKET3_CLIP(RADEON_CP_PACKET3_DRAW_INDX, 0);
   OUT_BATCH(vertex_format);
   OUT_BATCH(primitive | 
	     RADEON_CP_VC_CNTL_PRIM_WALK_IND |
	     RADEON_CP_VC_CNTL_COLOR_ORDER_RGBA |
	     RADEON_CP_VC_CNTL_MAOS_ENABLE |
	     RADEON_CP_VC_CNTL_VTX_FMT_RADEON_MODE);
#endif


   rmesa->tcl.elt_cmd_offset = rmesa->radeon.cmdbuf.cs->cdw;
   rmesa->tcl.elt_used = min_nr;

   retval = (GLushort *)(rmesa->radeon.cmdbuf.cs->packets + rmesa->tcl.elt_cmd_offset);
   
   if (RADEON_DEBUG & DEBUG_PRIMS)
      fprintf(stderr, "%s: header prim %x \n",
	      __FUNCTION__, primitive);

   assert(!rmesa->radeon.dma.flush);
   rmesa->radeon.glCtx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
   rmesa->radeon.dma.flush = radeonFlushElts;

   return retval;
}

void radeonEmitVertexAOS( r100ContextPtr rmesa,
			  GLuint vertex_size,
			  struct radeon_bo *bo,
			  GLuint offset )
{
#if RADEON_OLD_PACKETS
   rmesa->ioctl.vertex_offset = offset;
   rmesa->ioctl.bo = bo;
#else
   BATCH_LOCALS(&rmesa->radeon);

   if (RADEON_DEBUG & (DEBUG_PRIMS|DEBUG_IOCTL))
      fprintf(stderr, "%s:  vertex_size 0x%x offset 0x%x \n",
	      __FUNCTION__, vertex_size, offset);

   BEGIN_BATCH(7);
   OUT_BATCH_PACKET3(RADEON_CP_PACKET3_3D_LOAD_VBPNTR, 2);
   OUT_BATCH(1);
   OUT_BATCH(vertex_size | (vertex_size << 8));
   OUT_BATCH_RELOC(offset, bo, offset, RADEON_GEM_DOMAIN_GTT, 0, 0);
   END_BATCH();

#endif
}
		       

void radeonEmitAOS( r100ContextPtr rmesa,
		    GLuint nr,
		    GLuint offset )
{
#if RADEON_OLD_PACKETS
   assert( nr == 1 );
   rmesa->ioctl.bo = rmesa->tcl.aos[0].bo;
   rmesa->ioctl.vertex_offset = 
     (rmesa->tcl.aos[0].offset + offset * rmesa->tcl.aos[0].stride * 4);
#else
   BATCH_LOCALS(&rmesa->radeon);
   uint32_t voffset;
   //   int sz = AOS_BUFSZ(nr);
   int sz = 1 + (nr >> 1) * 3 + (nr & 1) * 2;
   int i;

   if (RADEON_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   BEGIN_BATCH(sz+2+(nr * 2));
   OUT_BATCH_PACKET3(RADEON_CP_PACKET3_3D_LOAD_VBPNTR, sz - 1);
   OUT_BATCH(nr);

   if (!rmesa->radeon.radeonScreen->kernel_mm) {
      for (i = 0; i + 1 < nr; i += 2) {
	 OUT_BATCH((rmesa->tcl.aos[i].components << 0) |
		   (rmesa->tcl.aos[i].stride << 8) |
		   (rmesa->tcl.aos[i + 1].components << 16) |
		   (rmesa->tcl.aos[i + 1].stride << 24));
			
	 voffset =  rmesa->tcl.aos[i + 0].offset +
	    offset * 4 * rmesa->tcl.aos[i + 0].stride;
	 OUT_BATCH_RELOC(voffset,
			 rmesa->tcl.aos[i].bo,
			 voffset,
			 RADEON_GEM_DOMAIN_GTT,
			 0, 0);
	 voffset =  rmesa->tcl.aos[i + 1].offset +
	    offset * 4 * rmesa->tcl.aos[i + 1].stride;
	 OUT_BATCH_RELOC(voffset,
			 rmesa->tcl.aos[i+1].bo,
			 voffset,
			 RADEON_GEM_DOMAIN_GTT,
			 0, 0);
      }
      
      if (nr & 1) {
	 OUT_BATCH((rmesa->tcl.aos[nr - 1].components << 0) |
		   (rmesa->tcl.aos[nr - 1].stride << 8));
	 voffset =  rmesa->tcl.aos[nr - 1].offset +
	    offset * 4 * rmesa->tcl.aos[nr - 1].stride;
	 OUT_BATCH_RELOC(voffset,
			 rmesa->tcl.aos[nr - 1].bo,
			 voffset,
			 RADEON_GEM_DOMAIN_GTT,
			 0, 0);
      }
   } else {
      for (i = 0; i + 1 < nr; i += 2) {
	 OUT_BATCH((rmesa->tcl.aos[i].components << 0) |
		   (rmesa->tcl.aos[i].stride << 8) |
		   (rmesa->tcl.aos[i + 1].components << 16) |
		   (rmesa->tcl.aos[i + 1].stride << 24));
	 
	 voffset =  rmesa->tcl.aos[i + 0].offset +
	    offset * 4 * rmesa->tcl.aos[i + 0].stride;
	 OUT_BATCH(voffset);
	 voffset =  rmesa->tcl.aos[i + 1].offset +
	    offset * 4 * rmesa->tcl.aos[i + 1].stride;
	 OUT_BATCH(voffset);
      }
      
      if (nr & 1) {
	 OUT_BATCH((rmesa->tcl.aos[nr - 1].components << 0) |
		   (rmesa->tcl.aos[nr - 1].stride << 8));
	 voffset =  rmesa->tcl.aos[nr - 1].offset +
	    offset * 4 * rmesa->tcl.aos[nr - 1].stride;
	 OUT_BATCH(voffset);
      }
      for (i = 0; i + 1 < nr; i += 2) {
	 voffset =  rmesa->tcl.aos[i + 0].offset +
	    offset * 4 * rmesa->tcl.aos[i + 0].stride;
	 radeon_cs_write_reloc(rmesa->radeon.cmdbuf.cs,
			       rmesa->tcl.aos[i+0].bo,
			       RADEON_GEM_DOMAIN_GTT,
			       0, 0);
	 voffset =  rmesa->tcl.aos[i + 1].offset +
	    offset * 4 * rmesa->tcl.aos[i + 1].stride;
	 radeon_cs_write_reloc(rmesa->radeon.cmdbuf.cs,
			       rmesa->tcl.aos[i+1].bo,
			       RADEON_GEM_DOMAIN_GTT,
			       0, 0);
      }
      if (nr & 1) {
	 voffset =  rmesa->tcl.aos[nr - 1].offset +
	    offset * 4 * rmesa->tcl.aos[nr - 1].stride;
	 radeon_cs_write_reloc(rmesa->radeon.cmdbuf.cs,
			       rmesa->tcl.aos[nr-1].bo,
			       RADEON_GEM_DOMAIN_GTT,
			       0, 0);
      }
   }
   END_BATCH();

#endif
}

/* ================================================================
 * Buffer clear
 */
#define RADEON_MAX_CLEARS	256



static void
r100_meta_set_passthrough_transform(r100ContextPtr r100)
{
   GLcontext *ctx = r100->radeon.glCtx;

   r100->meta.saved_vp_x = ctx->Viewport.X;
   r100->meta.saved_vp_y = ctx->Viewport.Y;
   r100->meta.saved_vp_width = ctx->Viewport.Width;
   r100->meta.saved_vp_height = ctx->Viewport.Height;
   r100->meta.saved_matrix_mode = ctx->Transform.MatrixMode;

   _mesa_Viewport(0, 0, ctx->DrawBuffer->Width, ctx->DrawBuffer->Height);

   _mesa_MatrixMode(GL_PROJECTION);
   _mesa_PushMatrix();
   _mesa_LoadIdentity();
   _mesa_Ortho(0, ctx->DrawBuffer->Width, 0, ctx->DrawBuffer->Height, 1, -1);

   _mesa_MatrixMode(GL_MODELVIEW);
   _mesa_PushMatrix();
   _mesa_LoadIdentity();
}

static void
r100_meta_restore_transform(r100ContextPtr r100)
{
   _mesa_MatrixMode(GL_PROJECTION);
   _mesa_PopMatrix();
   _mesa_MatrixMode(GL_MODELVIEW);
   _mesa_PopMatrix();

   _mesa_MatrixMode(r100->meta.saved_matrix_mode);

   _mesa_Viewport(r100->meta.saved_vp_x, r100->meta.saved_vp_y,
		  r100->meta.saved_vp_width, r100->meta.saved_vp_height);
}

/**
 * Perform glClear where mask contains only color, depth, and/or stencil.
 *
 * The implementation is based on calling into Mesa to set GL state and
 * performing normal triangle rendering.  The intent of this path is to
 * have as generic a path as possible, so that any driver could make use of
 * it.
 */
static void radeon_clear_tris(GLcontext *ctx, GLbitfield mask)
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   GLfloat vertices[4][3];
   GLfloat color[4][4];
   GLfloat dst_z;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   int i;
   GLboolean saved_fp_enable = GL_FALSE, saved_vp_enable = GL_FALSE;
   GLboolean saved_shader_program = 0;
   unsigned int saved_active_texture;
   
   assert((mask & ~(BUFFER_BIT_BACK_LEFT | BUFFER_BIT_FRONT_LEFT |
		    BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL)) == 0);

   _mesa_PushAttrib(GL_COLOR_BUFFER_BIT |
		    GL_CURRENT_BIT |
		    GL_DEPTH_BUFFER_BIT |
		    GL_ENABLE_BIT |
		    GL_STENCIL_BUFFER_BIT |
		    GL_TRANSFORM_BIT |
		    GL_CURRENT_BIT);
   _mesa_PushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
   saved_active_texture = ctx->Texture.CurrentUnit;
  
  /* Disable existing GL state we don't want to apply to a clear. */
   _mesa_Disable(GL_ALPHA_TEST);
   _mesa_Disable(GL_BLEND);
   _mesa_Disable(GL_CULL_FACE);
   _mesa_Disable(GL_FOG);
   _mesa_Disable(GL_POLYGON_SMOOTH);
   _mesa_Disable(GL_POLYGON_STIPPLE);
   _mesa_Disable(GL_POLYGON_OFFSET_FILL);
   _mesa_Disable(GL_LIGHTING);
   _mesa_Disable(GL_CLIP_PLANE0);
   _mesa_Disable(GL_CLIP_PLANE1);
   _mesa_Disable(GL_CLIP_PLANE2);
   _mesa_Disable(GL_CLIP_PLANE3);
   _mesa_Disable(GL_CLIP_PLANE4);
   _mesa_Disable(GL_CLIP_PLANE5);
   if (ctx->Extensions.ARB_fragment_program && ctx->FragmentProgram.Enabled) {
      saved_fp_enable = GL_TRUE;
      _mesa_Disable(GL_FRAGMENT_PROGRAM_ARB);
   }
   if (ctx->Extensions.ARB_vertex_program && ctx->VertexProgram.Enabled) {
      saved_vp_enable = GL_TRUE;
      _mesa_Disable(GL_VERTEX_PROGRAM_ARB);
   }
   if (ctx->Extensions.ARB_shader_objects && ctx->Shader.CurrentProgram) {
      saved_shader_program = ctx->Shader.CurrentProgram->Name;
      _mesa_UseProgramObjectARB(0);
   }
   
   if (ctx->Texture._EnabledUnits != 0) {
      int i;
      
      for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
	 _mesa_ActiveTextureARB(GL_TEXTURE0 + i);
	 _mesa_Disable(GL_TEXTURE_1D);
	 _mesa_Disable(GL_TEXTURE_2D);
	 _mesa_Disable(GL_TEXTURE_3D);
	 if (ctx->Extensions.ARB_texture_cube_map)
	    _mesa_Disable(GL_TEXTURE_CUBE_MAP_ARB);
	 if (ctx->Extensions.NV_texture_rectangle)
	    _mesa_Disable(GL_TEXTURE_RECTANGLE_NV);
	 if (ctx->Extensions.MESA_texture_array) {
	    _mesa_Disable(GL_TEXTURE_1D_ARRAY_EXT);
	    _mesa_Disable(GL_TEXTURE_2D_ARRAY_EXT);
	 }
      }
   }
  
   r100_meta_set_passthrough_transform(rmesa);
   
   for (i = 0; i < 4; i++) {
      color[i][0] = ctx->Color.ClearColor[0];
      color[i][1] = ctx->Color.ClearColor[1];
      color[i][2] = ctx->Color.ClearColor[2];
      color[i][3] = ctx->Color.ClearColor[3];
   }

   /* convert clear Z from [0,1] to NDC coord in [-1,1] */
   dst_z = -1.0 + 2.0 * ctx->Depth.Clear;
   
   /* Prepare the vertices, which are the same regardless of which buffer we're
    * drawing to.
    */
   vertices[0][0] = fb->_Xmin;
   vertices[0][1] = fb->_Ymin;
   vertices[0][2] = dst_z;
   vertices[1][0] = fb->_Xmax;
   vertices[1][1] = fb->_Ymin;
   vertices[1][2] = dst_z;
   vertices[2][0] = fb->_Xmax;
   vertices[2][1] = fb->_Ymax;
   vertices[2][2] = dst_z;
   vertices[3][0] = fb->_Xmin;
   vertices[3][1] = fb->_Ymax;
   vertices[3][2] = dst_z;

   _mesa_ColorPointer(4, GL_FLOAT, 4 * sizeof(GLfloat), &color);
   _mesa_VertexPointer(3, GL_FLOAT, 3 * sizeof(GLfloat), &vertices);
   _mesa_Enable(GL_COLOR_ARRAY);
   _mesa_Enable(GL_VERTEX_ARRAY);

   while (mask != 0) {
      GLuint this_mask = 0;

      if (mask & BUFFER_BIT_BACK_LEFT)
	 this_mask = BUFFER_BIT_BACK_LEFT;
      else if (mask & BUFFER_BIT_FRONT_LEFT)
	 this_mask = BUFFER_BIT_FRONT_LEFT;

      /* Clear depth/stencil in the same pass as color. */
      this_mask |= (mask & (BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL));

      /* Select the current color buffer and use the color write mask if
       * we have one, otherwise don't write any color channels.
       */
      if (this_mask & BUFFER_BIT_FRONT_LEFT)
	 _mesa_DrawBuffer(GL_FRONT_LEFT);
      else if (this_mask & BUFFER_BIT_BACK_LEFT)
	 _mesa_DrawBuffer(GL_BACK_LEFT);
      else
	 _mesa_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

      /* Control writing of the depth clear value to depth. */
      if (this_mask & BUFFER_BIT_DEPTH) {
	 _mesa_DepthFunc(GL_ALWAYS);
	 _mesa_Enable(GL_DEPTH_TEST);
      } else {
	 _mesa_Disable(GL_DEPTH_TEST);
	 _mesa_DepthMask(GL_FALSE);
      }

      /* Control writing of the stencil clear value to stencil. */
      if (this_mask & BUFFER_BIT_STENCIL) {
	 _mesa_Enable(GL_STENCIL_TEST);
	 _mesa_StencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	 _mesa_StencilFuncSeparate(GL_FRONT, GL_ALWAYS, ctx->Stencil.Clear,
				   ctx->Stencil.WriteMask[0]);
      } else {
	 _mesa_Disable(GL_STENCIL_TEST);
      }

      CALL_DrawArrays(ctx->Exec, (GL_TRIANGLE_FAN, 0, 4));

      mask &= ~this_mask;
   }

   r100_meta_restore_transform(rmesa);

   _mesa_ActiveTextureARB(GL_TEXTURE0 + saved_active_texture);
   if (saved_fp_enable)
      _mesa_Enable(GL_FRAGMENT_PROGRAM_ARB);
   if (saved_vp_enable)
      _mesa_Enable(GL_VERTEX_PROGRAM_ARB);

   if (saved_shader_program)
      _mesa_UseProgramObjectARB(saved_shader_program);

   _mesa_PopClientAttrib();
   _mesa_PopAttrib();
}

static void radeonUserClear(GLcontext *ctx, GLuint mask)
{
   radeon_clear_tris(ctx, mask);
}

static void radeonKernelClear(GLcontext *ctx, GLuint flags)
{
     r100ContextPtr rmesa = R100_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = rmesa->radeon.dri.drawable;
   drm_radeon_sarea_t *sarea = rmesa->radeon.sarea;
   uint32_t clear;
   GLint ret, i;
   GLint cx, cy, cw, ch;

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
}

static void radeonClear( GLcontext *ctx, GLbitfield mask )
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = rmesa->radeon.dri.drawable;
   GLuint flags = 0;
   GLuint color_mask = 0;
   GLuint orig_mask = mask;

   if ( RADEON_DEBUG & DEBUG_IOCTL ) {
      fprintf( stderr, "radeonClear\n");
   }

   {
      LOCK_HARDWARE( &rmesa->radeon );
      UNLOCK_HARDWARE( &rmesa->radeon );
      if ( dPriv->numClipRects == 0 ) 
	 return;
   }
   
   radeon_firevertices(&rmesa->radeon); 

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

   if (rmesa->radeon.radeonScreen->kernel_mm)
     radeonUserClear(ctx, orig_mask);
   else {
      radeonKernelClear(ctx, flags);
      rmesa->radeon.hw.all_dirty = GL_TRUE;
   }
}

void radeonInitIoctlFuncs( GLcontext *ctx )
{
    ctx->Driver.Clear = radeonClear;
    ctx->Driver.Finish = radeonFinish;
    ctx->Driver.Flush = radeonFlush;
}

