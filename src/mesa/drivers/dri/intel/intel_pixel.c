/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * next paragraph) shall be included in all copies or substantial portionsalloc
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

#include "main/enums.h"
#include "main/state.h"
#include "main/context.h"
#include "main/enable.h"
#include "main/matrix.h"
#include "swrast/swrast.h"
#include "shader/arbprogram.h"
#include "shader/program.h"

#include "intel_context.h"
#include "intel_pixel.h"
#include "intel_regions.h"

#define FILE_DEBUG_FLAG DEBUG_PIXEL

static GLenum
effective_func(GLenum func, GLboolean src_alpha_is_one)
{
   if (src_alpha_is_one) {
      if (func == GL_SRC_ALPHA)
	 return GL_ONE;
      if (func == GL_ONE_MINUS_SRC_ALPHA)
	 return GL_ZERO;
   }

   return func;
}

/**
 * Check if any fragment operations are in effect which might effect
 * glDraw/CopyPixels.
 */
GLboolean
intel_check_blit_fragment_ops(GLcontext * ctx, GLboolean src_alpha_is_one)
{
   if (ctx->NewState)
      _mesa_update_state(ctx);

   if (ctx->FragmentProgram._Enabled) {
      DBG("fallback due to fragment program\n");
      return GL_FALSE;
   }

   if (ctx->Color.BlendEnabled &&
       (effective_func(ctx->Color.BlendSrcRGB, src_alpha_is_one) != GL_ONE ||
	effective_func(ctx->Color.BlendDstRGB, src_alpha_is_one) != GL_ZERO ||
	ctx->Color.BlendEquationRGB != GL_FUNC_ADD ||
	effective_func(ctx->Color.BlendSrcA, src_alpha_is_one) != GL_ONE ||
	effective_func(ctx->Color.BlendDstA, src_alpha_is_one) != GL_ZERO ||
	ctx->Color.BlendEquationA != GL_FUNC_ADD)) {
      DBG("fallback due to blend\n");
      return GL_FALSE;
   }

   if (ctx->Texture._EnabledUnits) {
      DBG("fallback due to texturing\n");
      return GL_FALSE;
   }

   if (!(ctx->Color.ColorMask[0] &&
	 ctx->Color.ColorMask[1] &&
	 ctx->Color.ColorMask[2] &&
	 ctx->Color.ColorMask[3])) {
      DBG("fallback due to color masking\n");
      return GL_FALSE;
   }

   if (ctx->Color.AlphaEnabled) {
      DBG("fallback due to alpha\n");
      return GL_FALSE;
   }

   if (ctx->Depth.Test) {
      DBG("fallback due to depth test\n");
      return GL_FALSE;
   }

   if (ctx->Fog.Enabled) {
      DBG("fallback due to fog\n");
      return GL_FALSE;
   }

   if (ctx->_ImageTransferState) {
      DBG("fallback due to image transfer\n");
      return GL_FALSE;
   }

   if (ctx->Stencil.Enabled) {
      DBG("fallback due to image stencil\n");
      return GL_FALSE;
   }

   if (ctx->RenderMode != GL_RENDER) {
      DBG("fallback due to render mode\n");
      return GL_FALSE;
   }

   return GL_TRUE;
}


GLboolean
intel_check_meta_tex_fragment_ops(GLcontext * ctx)
{
   if (ctx->NewState)
      _mesa_update_state(ctx);

   /* Some of _ImageTransferState (scale, bias) could be done with
    * fragment programs on i915.
    */
   return !(ctx->_ImageTransferState || ctx->Fog.Enabled ||     /* not done yet */
            ctx->Texture._EnabledUnits || ctx->FragmentProgram._Enabled);
}

/* The intel_region struct doesn't really do enough to capture the
 * format of the pixels in the region.  For now this code assumes that
 * the region is a display surface and hence is either ARGB8888 or
 * RGB565.
 * XXX FBO: If we'd pass in the intel_renderbuffer instead of region, we'd
 * know the buffer's pixel format.
 *
 * \param format  as given to glDraw/ReadPixels
 * \param type  as given to glDraw/ReadPixels
 */
GLboolean
intel_check_blit_format(struct intel_region * region,
                        GLenum format, GLenum type)
{
   if (region->cpp == 4 &&
       (type == GL_UNSIGNED_INT_8_8_8_8_REV ||
        type == GL_UNSIGNED_BYTE) && format == GL_BGRA) {
      return GL_TRUE;
   }

   if (region->cpp == 2 &&
       type == GL_UNSIGNED_SHORT_5_6_5_REV && format == GL_BGR) {
      return GL_TRUE;
   }

   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s: bad format for blit (cpp %d, type %s format %s)\n",
              __FUNCTION__, region->cpp,
              _mesa_lookup_enum_by_nr(type), _mesa_lookup_enum_by_nr(format));

   return GL_FALSE;
}

void
intel_meta_set_passthrough_transform(struct intel_context *intel)
{
   GLcontext *ctx = &intel->ctx;

   intel->meta.saved_vp_x = ctx->Viewport.X;
   intel->meta.saved_vp_y = ctx->Viewport.Y;
   intel->meta.saved_vp_width = ctx->Viewport.Width;
   intel->meta.saved_vp_height = ctx->Viewport.Height;
   intel->meta.saved_matrix_mode = ctx->Transform.MatrixMode;

   _mesa_Viewport(0, 0, ctx->DrawBuffer->Width, ctx->DrawBuffer->Height);

   _mesa_MatrixMode(GL_PROJECTION);
   _mesa_PushMatrix();
   _mesa_LoadIdentity();
   _mesa_Ortho(0, ctx->DrawBuffer->Width, 0, ctx->DrawBuffer->Height, 1, -1);

   _mesa_MatrixMode(GL_MODELVIEW);
   _mesa_PushMatrix();
   _mesa_LoadIdentity();
}

void
intel_meta_restore_transform(struct intel_context *intel)
{
   _mesa_MatrixMode(GL_PROJECTION);
   _mesa_PopMatrix();
   _mesa_MatrixMode(GL_MODELVIEW);
   _mesa_PopMatrix();

   _mesa_MatrixMode(intel->meta.saved_matrix_mode);

   _mesa_Viewport(intel->meta.saved_vp_x, intel->meta.saved_vp_y,
		  intel->meta.saved_vp_width, intel->meta.saved_vp_height);
}

/**
 * Set up a vertex program to pass through the position and first texcoord
 * for pixel path.
 */
void
intel_meta_set_passthrough_vertex_program(struct intel_context *intel)
{
   GLcontext *ctx = &intel->ctx;
   static const char *vp =
      "!!ARBvp1.0\n"
      "TEMP vertexClip;\n"
      "DP4 vertexClip.x, state.matrix.mvp.row[0], vertex.position;\n"
      "DP4 vertexClip.y, state.matrix.mvp.row[1], vertex.position;\n"
      "DP4 vertexClip.z, state.matrix.mvp.row[2], vertex.position;\n"
      "DP4 vertexClip.w, state.matrix.mvp.row[3], vertex.position;\n"
      "MOV result.position, vertexClip;\n"
      "MOV result.texcoord[0], vertex.texcoord[0];\n"
      "MOV result.color, vertex.color;\n"
      "END\n";

   assert(intel->meta.saved_vp == NULL);

   _mesa_reference_vertprog(ctx, &intel->meta.saved_vp,
			    ctx->VertexProgram.Current);
   if (intel->meta.passthrough_vp == NULL) {
      GLuint prog_name;
      _mesa_GenPrograms(1, &prog_name);
      _mesa_BindProgram(GL_VERTEX_PROGRAM_ARB, prog_name);
      _mesa_ProgramStringARB(GL_VERTEX_PROGRAM_ARB,
			     GL_PROGRAM_FORMAT_ASCII_ARB,
			     strlen(vp), (const GLubyte *)vp);
      _mesa_reference_vertprog(ctx, &intel->meta.passthrough_vp,
			       ctx->VertexProgram.Current);
      _mesa_DeletePrograms(1, &prog_name);
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);
   _mesa_reference_vertprog(ctx, &ctx->VertexProgram.Current,
			    intel->meta.passthrough_vp);
   ctx->Driver.BindProgram(ctx, GL_VERTEX_PROGRAM_ARB,
			   &intel->meta.passthrough_vp->Base);

   intel->meta.saved_vp_enable = ctx->VertexProgram.Enabled;
   _mesa_Enable(GL_VERTEX_PROGRAM_ARB);
}

/**
 * Restores the previous vertex program after
 * intel_meta_set_passthrough_vertex_program()
 */
void
intel_meta_restore_vertex_program(struct intel_context *intel)
{
   GLcontext *ctx = &intel->ctx;

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);
   _mesa_reference_vertprog(ctx, &ctx->VertexProgram.Current,
			    intel->meta.saved_vp);
   _mesa_reference_vertprog(ctx, &intel->meta.saved_vp, NULL);
   ctx->Driver.BindProgram(ctx, GL_VERTEX_PROGRAM_ARB,
			   &ctx->VertexProgram.Current->Base);

   if (!intel->meta.saved_vp_enable)
      _mesa_Disable(GL_VERTEX_PROGRAM_ARB);
}

/**
 * Binds the given program string to GL_FRAGMENT_PROGRAM_ARB, caching the
 * program object.
 */
void
intel_meta_set_fragment_program(struct intel_context *intel,
				struct gl_fragment_program **prog,
				const char *prog_string)
{
   GLcontext *ctx = &intel->ctx;
   assert(intel->meta.saved_fp == NULL);

   _mesa_reference_fragprog(ctx, &intel->meta.saved_fp,
			    ctx->FragmentProgram.Current);
   if (*prog == NULL) {
      GLuint prog_name;
      _mesa_GenPrograms(1, &prog_name);
      _mesa_BindProgram(GL_FRAGMENT_PROGRAM_ARB, prog_name);
      _mesa_ProgramStringARB(GL_FRAGMENT_PROGRAM_ARB,
			     GL_PROGRAM_FORMAT_ASCII_ARB,
			     strlen(prog_string), (const GLubyte *)prog_string);
      _mesa_reference_fragprog(ctx, prog, ctx->FragmentProgram.Current);
      /* Note that DeletePrograms unbinds the program on us */
      _mesa_DeletePrograms(1, &prog_name);
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);
   _mesa_reference_fragprog(ctx, &ctx->FragmentProgram.Current, *prog);
   ctx->Driver.BindProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, &((*prog)->Base));

   intel->meta.saved_fp_enable = ctx->FragmentProgram.Enabled;
   _mesa_Enable(GL_FRAGMENT_PROGRAM_ARB);
}

/**
 * Restores the previous fragment program after
 * intel_meta_set_fragment_program()
 */
void
intel_meta_restore_fragment_program(struct intel_context *intel)
{
   GLcontext *ctx = &intel->ctx;

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);
   _mesa_reference_fragprog(ctx, &ctx->FragmentProgram.Current,
			    intel->meta.saved_fp);
   _mesa_reference_fragprog(ctx, &intel->meta.saved_fp, NULL);
   ctx->Driver.BindProgram(ctx, GL_FRAGMENT_PROGRAM_ARB,
			   &ctx->FragmentProgram.Current->Base);

   if (!intel->meta.saved_fp_enable)
      _mesa_Disable(GL_FRAGMENT_PROGRAM_ARB);
}

void
intelInitPixelFuncs(struct dd_function_table *functions)
{
   functions->Accum = _swrast_Accum;
   if (!getenv("INTEL_NO_BLIT")) {
      functions->Bitmap = intelBitmap;
      functions->CopyPixels = intelCopyPixels;
      functions->DrawPixels = intelDrawPixels;
#ifdef I915
      functions->ReadPixels = intelReadPixels;
#endif
   }
}

void
intel_free_pixel_state(struct intel_context *intel)
{
   GLcontext *ctx = &intel->ctx;

   _mesa_reference_vertprog(ctx, &intel->meta.passthrough_vp, NULL);
   _mesa_reference_fragprog(ctx, &intel->meta.bitmap_fp, NULL);
}

