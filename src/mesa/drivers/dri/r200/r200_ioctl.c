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
#include "main/attrib.h"
#include "main/enable.h"

#include "radeon_common.h"
#include "radeon_lock.h"
#include "r200_context.h"
#include "r200_state.h"
#include "r200_ioctl.h"
#include "r200_tcl.h"
#include "r200_sanity.h"
#include "radeon_reg.h"

#include "drirenderbuffer.h"
#include "vblank.h"

#define R200_TIMEOUT             512
#define R200_IDLE_RETRY           16

static void
r200_meta_set_passthrough_transform(r200ContextPtr r200)
{
   GLcontext *ctx = r200->radeon.glCtx;

   r200->meta.saved_vp_x = ctx->Viewport.X;
   r200->meta.saved_vp_y = ctx->Viewport.Y;
   r200->meta.saved_vp_width = ctx->Viewport.Width;
   r200->meta.saved_vp_height = ctx->Viewport.Height;
   r200->meta.saved_matrix_mode = ctx->Transform.MatrixMode;

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
r200_meta_restore_transform(r200ContextPtr r200)
{
   _mesa_MatrixMode(GL_PROJECTION);
   _mesa_PopMatrix();
   _mesa_MatrixMode(GL_MODELVIEW);
   _mesa_PopMatrix();

   _mesa_MatrixMode(r200->meta.saved_matrix_mode);

   _mesa_Viewport(r200->meta.saved_vp_x, r200->meta.saved_vp_y,
		  r200->meta.saved_vp_width, r200->meta.saved_vp_height);
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
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
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
  
   r200_meta_set_passthrough_transform(rmesa);
   
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

   r200_meta_restore_transform(rmesa);

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


static void r200UserClear(GLcontext *ctx, GLuint mask)
{
   radeon_clear_tris(ctx, mask);
}

static void r200KernelClear(GLcontext *ctx, GLuint flags)
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = rmesa->radeon.dri.drawable;
   GLint cx, cy, cw, ch, ret;
   GLuint i;

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
   __DRIdrawablePrivate *dPriv = rmesa->radeon.dri.drawable;
   GLuint flags = 0;
   GLuint color_mask = 0;
   GLint ret;
   GLuint orig_mask = mask;

   if ( R200_DEBUG & DEBUG_IOCTL ) {
       fprintf( stderr, "r200Clear %x %d\n", mask, rmesa->radeon.sarea->pfCurrentPage);
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

   if (rmesa->radeon.radeonScreen->kernel_mm)
      r200UserClear(ctx, orig_mask);
   else
      r200KernelClear(ctx, flags);

   rmesa->radeon.hw.all_dirty = GL_TRUE;
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
    functions->Finish = radeonFinish;
    functions->Flush = radeonFlush;
}

