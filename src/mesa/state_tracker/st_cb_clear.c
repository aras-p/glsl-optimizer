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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */

#include "main/glheader.h"
#include "main/macros.h"
#include "shader/prog_instruction.h"
#include "st_atom.h"
#include "st_cache.h"
#include "st_context.h"
#include "st_cb_accum.h"
#include "st_cb_clear.h"
#include "st_cb_fbo.h"
#include "st_draw.h"
#include "st_program.h"
#include "st_public.h"
#include "st_mesa_to_tgsi.h"

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"





static GLuint
color_value(enum pipe_format pipeFormat, const GLfloat color[4])
{
   GLubyte r, g, b, a;

   UNCLAMPED_FLOAT_TO_UBYTE(r, color[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(g, color[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(b, color[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(a, color[3]);

   switch (pipeFormat) {
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      return (r << 24) | (g << 16) | (b << 8) | a;
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      return (a << 24) | (r << 16) | (g << 8) | b;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return (b << 24) | (g << 16) | (r << 8) | a;
   case PIPE_FORMAT_R5G6B5_UNORM:
      return ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
   default:
      assert(0);
      return 0;
   }
}
 

static uint
depth_value(enum pipe_format pipeFormat, GLfloat value)
{
   switch (pipeFormat) {
   case PIPE_FORMAT_Z16_UNORM:
      return (uint) (value * 0xffff);
   case PIPE_FORMAT_Z32_UNORM:
      /* special-case to avoid overflow */
      if (value == 1.0)
         return 0xffffffff;
      else
         return (uint) (value * 0xffffffff);
   case PIPE_FORMAT_S8Z24_UNORM:
      return (uint) (value * 0xffffff);
   case PIPE_FORMAT_Z24S8_UNORM:
      return ((uint) (value * 0xffffff)) << 8;
   default:
      assert(0);
      return 0;
   }
}


static GLboolean
is_depth_stencil_format(enum pipe_format pipeFormat)
{
   switch (pipeFormat) {
   case PIPE_FORMAT_S8Z24_UNORM:
   case PIPE_FORMAT_Z24S8_UNORM:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}



/**
 * Create a simple fragment shader that just passes through the fragment color.
 */
static struct st_fragment_program *
make_frag_shader(struct st_context *st)
{
   GLcontext *ctx = st->ctx;
   struct st_fragment_program *stfp;
   struct gl_program *p;
   GLuint interpMode[16];
   GLuint i;

   /* XXX temporary */
   for (i = 0; i < 16; i++)
      interpMode[i] = TGSI_INTERPOLATE_LINEAR;

   p = ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);
   if (!p)
      return NULL;

   p->NumInstructions = 2;
   p->Instructions = _mesa_alloc_instructions(2);
   if (!p->Instructions) {
      ctx->Driver.DeleteProgram(ctx, p);
      return NULL;
   }
   _mesa_init_instructions(p->Instructions, 2);
   /* MOV result.color, fragment.color; */
   p->Instructions[0].Opcode = OPCODE_MOV;
   p->Instructions[0].DstReg.File = PROGRAM_OUTPUT;
   p->Instructions[0].DstReg.Index = FRAG_RESULT_COLR;
   p->Instructions[0].SrcReg[0].File = PROGRAM_INPUT;
   p->Instructions[0].SrcReg[0].Index = FRAG_ATTRIB_COL0;
   /* END; */
   p->Instructions[1].Opcode = OPCODE_END;

   p->InputsRead = FRAG_BIT_COL0;
   p->OutputsWritten = (1 << FRAG_RESULT_COLR);

   stfp = (struct st_fragment_program *) p;
   st_translate_fragment_program(st, stfp, NULL,
                                 stfp->tokens, ST_MAX_SHADER_TOKENS);

   return stfp;
}


/**
 * Create a simple vertex shader that just passes through the
 * vertex position and color.
 */
static struct st_vertex_program *
make_vertex_shader(struct st_context *st)
{
   GLcontext *ctx = st->ctx;
   struct st_vertex_program *stvp;
   struct gl_program *p;

   p = ctx->Driver.NewProgram(ctx, GL_VERTEX_PROGRAM_ARB, 0);
   if (!p)
      return NULL;

   p->NumInstructions = 3;
   p->Instructions = _mesa_alloc_instructions(3);
   if (!p->Instructions) {
      ctx->Driver.DeleteProgram(ctx, p);
      return NULL;
   }
   _mesa_init_instructions(p->Instructions, 3);
   /* MOV result.pos, vertex.pos; */
   p->Instructions[0].Opcode = OPCODE_MOV;
   p->Instructions[0].DstReg.File = PROGRAM_OUTPUT;
   p->Instructions[0].DstReg.Index = VERT_RESULT_HPOS;
   p->Instructions[0].SrcReg[0].File = PROGRAM_INPUT;
   p->Instructions[0].SrcReg[0].Index = VERT_ATTRIB_POS;
   /* MOV result.color, vertex.color; */
   p->Instructions[1].Opcode = OPCODE_MOV;
   p->Instructions[1].DstReg.File = PROGRAM_OUTPUT;
   p->Instructions[1].DstReg.Index = VERT_RESULT_COL0;
   p->Instructions[1].SrcReg[0].File = PROGRAM_INPUT;
   p->Instructions[1].SrcReg[0].Index = VERT_ATTRIB_COLOR0;
   /* END; */
   p->Instructions[2].Opcode = OPCODE_END;

   p->InputsRead = VERT_BIT_POS | VERT_BIT_COLOR0;
   p->OutputsWritten = ((1 << VERT_RESULT_COL0) |
                        (1 << VERT_RESULT_HPOS));

   stvp = (struct st_vertex_program *) p;
   st_translate_vertex_program(st, stvp, NULL,
                               stvp->tokens, ST_MAX_SHADER_TOKENS);
   assert(stvp->cso);

   return stvp;
}



/**
 * Draw a screen-aligned quadrilateral.
 * Coords are window coords with y=0=bottom.  These coords will be transformed
 * by the vertex shader and viewport transform (which will flip Y if needed).
 */
static void
draw_quad(GLcontext *ctx,
          float x0, float y0, float x1, float y1, GLfloat z,
          const GLfloat color[4])
{
   GLfloat verts[4][2][4]; /* four verts, two attribs, XYZW */
   GLuint i;

   /* positions */
   verts[0][0][0] = x0;
   verts[0][0][1] = y0;

   verts[1][0][0] = x1;
   verts[1][0][1] = y0;

   verts[2][0][0] = x1;
   verts[2][0][1] = y1;

   verts[3][0][0] = x0;
   verts[3][0][1] = y1;

   /* same for all verts: */
   for (i = 0; i < 4; i++) {
      verts[i][0][2] = z;
      verts[i][0][3] = 1.0;
      verts[i][1][0] = color[0];
      verts[i][1][1] = color[1];
      verts[i][1][2] = color[2];
      verts[i][1][3] = color[3];
   }

   st_draw_vertices(ctx, PIPE_PRIM_QUADS, 4, (float *) verts, 2, GL_FALSE);
}



/**
 * Do glClear by drawing a quadrilateral.
 * The vertices of the quad will be computed from the
 * ctx->DrawBuffer->_X/Ymin/max fields.
 */
static void
clear_with_quad(GLcontext *ctx,
                GLboolean color, GLboolean depth, GLboolean stencil)
{
   struct st_context *st = ctx->st;
   struct pipe_context *pipe = st->pipe;
   const GLfloat x0 = ctx->DrawBuffer->_Xmin;
   const GLfloat y0 = ctx->DrawBuffer->_Ymin;
   const GLfloat x1 = ctx->DrawBuffer->_Xmax;
   const GLfloat y1 = ctx->DrawBuffer->_Ymax;


   /* blend state: RGBA masking */
   {
      struct pipe_blend_state blend;
      const struct cso_blend *cso;
      memset(&blend, 0, sizeof(blend));
      if (color) {
         if (ctx->Color.ColorMask[0])
            blend.colormask |= PIPE_MASK_R;
         if (ctx->Color.ColorMask[1])
            blend.colormask |= PIPE_MASK_G;
         if (ctx->Color.ColorMask[2])
            blend.colormask |= PIPE_MASK_B;
         if (ctx->Color.ColorMask[3])
            blend.colormask |= PIPE_MASK_A;
         if (st->ctx->Color.DitherFlag)
            blend.dither = 1;
      }
      cso = st_cached_blend_state(st, &blend);
      pipe->bind_blend_state(pipe, cso->data);
   }

   /* depth_stencil state: always pass/set to ref value */
   {
      struct pipe_depth_stencil_alpha_state depth_stencil;
      const struct cso_depth_stencil_alpha *cso;
      memset(&depth_stencil, 0, sizeof(depth_stencil));
      if (depth) {
         depth_stencil.depth.enabled = 1;
         depth_stencil.depth.writemask = 1;
         depth_stencil.depth.func = PIPE_FUNC_ALWAYS;
      }

      if (stencil) {
         depth_stencil.stencil[0].enabled = 1;
         depth_stencil.stencil[0].func = PIPE_FUNC_ALWAYS;
         depth_stencil.stencil[0].fail_op = PIPE_STENCIL_OP_REPLACE;
         depth_stencil.stencil[0].zpass_op = PIPE_STENCIL_OP_REPLACE;
         depth_stencil.stencil[0].zfail_op = PIPE_STENCIL_OP_REPLACE;
         depth_stencil.stencil[0].ref_value = ctx->Stencil.Clear;
         depth_stencil.stencil[0].value_mask = 0xff;
         depth_stencil.stencil[0].write_mask = ctx->Stencil.WriteMask[0] & 0xff;
      }
      cso = st_cached_depth_stencil_alpha_state(st, &depth_stencil);
      pipe->bind_depth_stencil_alpha_state(pipe, cso->data);
   }

   /* rasterizer state: nothing */
   {
      struct pipe_rasterizer_state raster;
      const struct cso_rasterizer *cso;
      memset(&raster, 0, sizeof(raster));
#if 0
      /* don't do per-pixel scissor; we'll just draw a PIPE_PRIM_QUAD
       * that matches the scissor bounds.
       */
      if (ctx->Scissor.Enabled)
         raster.scissor = 1;
#endif
      cso = st_cached_rasterizer_state(st, &raster);
      pipe->bind_rasterizer_state(pipe, cso->data);
   }

   /* fragment shader state: color pass-through program */
   {
      static struct st_fragment_program *stfp = NULL;
      if (!stfp) {
         stfp = make_frag_shader(st);
      }
      pipe->bind_fs_state(pipe, stfp->fs->data);
   }

   /* vertex shader state: color/position pass-through */
   {
      static struct st_vertex_program *stvp = NULL;
      if (!stvp) {
         stvp = make_vertex_shader(st);
      }
      pipe->bind_vs_state(pipe, stvp->cso->data);
   }

   /* viewport state: viewport matching window dims */
   {
      const float width = ctx->DrawBuffer->Width;
      const float height = ctx->DrawBuffer->Height;
      struct pipe_viewport_state vp;
      vp.scale[0] =  0.5 * width;
      vp.scale[1] = -0.5 * height;
      vp.scale[2] = 1.0;
      vp.scale[3] = 1.0;
      vp.translate[0] = 0.5 * width;
      vp.translate[1] = 0.5 * height;
      vp.translate[2] = 0.0;
      vp.translate[3] = 0.0;
      pipe->set_viewport_state(pipe, &vp);
   }

   /* draw quad matching scissor rect (XXX verify coord round-off) */
   draw_quad(ctx, x0, y0, x1, y1, ctx->Depth.Clear, ctx->Color.ClearColor);

   /* Restore pipe state */
   pipe->bind_blend_state(pipe, st->state.blend->data);
   pipe->bind_depth_stencil_alpha_state(pipe, st->state.depth_stencil->data);
   pipe->bind_fs_state(pipe, st->state.fs->data);
   pipe->bind_vs_state(pipe, st->state.vs->cso->data);
   pipe->bind_rasterizer_state(pipe, st->state.rasterizer->data);
   pipe->set_viewport_state(pipe, &st->state.viewport);
   /* OR:
   st_invalidate_state(ctx, _NEW_COLOR | _NEW_DEPTH | _NEW_STENCIL);
   */
}


/**
 * Determine if we need to clear the depth buffer by drawing a quad.
 */
static INLINE GLboolean
check_clear_color_with_quad(GLcontext *ctx)
{
   return !(ctx->Color.ColorMask[0] &&
            ctx->Color.ColorMask[1] &&
            ctx->Color.ColorMask[2] &&
            ctx->Color.ColorMask[3] &&
            !ctx->Scissor.Enabled);
}


/**
 * Determine if we need to clear the depth buffer by drawing a quad.
 */
static INLINE GLboolean
check_clear_depth_with_quad(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   const struct st_renderbuffer *strb = st_renderbuffer(rb);
   const GLboolean isDS = is_depth_stencil_format(strb->surface->format);
   return  ctx->Scissor.Enabled
      || (isDS && 
	  strb->surface->status == PIPE_SURFACE_STATUS_DEFINED &&
	  ctx->DrawBuffer->Visual.stencilBits > 0);
}


/**
 * Determine if we need to clear the stencil buffer by drawing a quad.
 */
static INLINE GLboolean
check_clear_stencil_with_quad(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   const struct st_renderbuffer *strb = st_renderbuffer(rb);
   const GLboolean isDS = is_depth_stencil_format(strb->surface->format);
   const GLuint stencilMax = (1 << rb->StencilBits) - 1;
   const GLboolean maskStencil
      = (ctx->Stencil.WriteMask[0] & stencilMax) != stencilMax;
   return maskStencil
      || ctx->Scissor.Enabled
      || (isDS && ctx->DrawBuffer->Visual.depthBits > 0);
}




static void
clear_color_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   struct st_renderbuffer *strb = st_renderbuffer(rb);

   if (ctx->Color.ColorMask[0] &&
       ctx->Color.ColorMask[1] &&
       ctx->Color.ColorMask[2] &&
       ctx->Color.ColorMask[3] &&
       !ctx->Scissor.Enabled)
   {
      /* clear whole buffer w/out masking */
      GLuint clearValue
         = color_value(strb->surface->format, ctx->Color.ClearColor);
      ctx->st->pipe->clear(ctx->st->pipe, strb->surface, clearValue);
   }
   else {
      /* masking or scissoring */
      clear_with_quad(ctx, GL_TRUE, GL_FALSE, GL_FALSE);
   }
}


static void
clear_depth_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   /*
   const GLboolean isDS = is_depth_stencil_format(strb->surface->format);
   */

   assert(strb->surface->format);

   if (check_clear_depth_with_quad(ctx, rb)) {
      /* scissoring or we have a combined depth/stencil buffer */
      clear_with_quad(ctx, GL_FALSE, GL_TRUE, GL_FALSE);
   }
   else {
      /* simple clear of whole buffer */
      uint clearValue = depth_value(strb->surface->format, ctx->Depth.Clear);
      ctx->st->pipe->clear(ctx->st->pipe, strb->surface, clearValue);
   }
}


static void
clear_stencil_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   const GLboolean isDS = is_depth_stencil_format(strb->surface->format);
   const GLuint stencilMax = (1 << rb->StencilBits) - 1;
   GLboolean maskStencil
      = (ctx->Stencil.WriteMask[0] & stencilMax) != stencilMax;

   if (maskStencil ||
       ctx->Scissor.Enabled ||
       (isDS && ctx->DrawBuffer->Visual.depthBits > 0)) {
      /* masking or scissoring or combined depth/stencil buffer */
      clear_with_quad(ctx, GL_FALSE, GL_FALSE, GL_TRUE);
   }
   else {
      /* simple clear of whole buffer */
      GLuint clearValue = ctx->Stencil.Clear;
      ctx->st->pipe->clear(ctx->st->pipe, strb->surface, clearValue);
   }
}


static void
clear_depth_stencil_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   const GLuint stencilMax = (1 << rb->StencilBits) - 1;
   GLboolean maskStencil
      = (ctx->Stencil.WriteMask[0] & stencilMax) != stencilMax;

   assert(is_depth_stencil_format(strb->surface->format));

   if (!maskStencil && !ctx->Scissor.Enabled) {
      /* clear whole buffer w/out masking */
      GLuint clearValue = depth_value(strb->surface->format, ctx->Depth.Clear);

      switch (strb->surface->format) {
      case PIPE_FORMAT_S8Z24_UNORM:
         clearValue |= ctx->Stencil.Clear << 24;
         break;
      case PIPE_FORMAT_Z24S8_UNORM:
         clearValue |= clearValue | ctx->Stencil.Clear;
         break;
      default:
         assert(0);
      }  

      ctx->st->pipe->clear(ctx->st->pipe, strb->surface, clearValue);
   }
   else {
      /* masking or scissoring */
      clear_with_quad(ctx, GL_FALSE, GL_TRUE, GL_TRUE);
   }
}



/**
 * Called via ctx->Driver.Clear()
 * XXX: doesn't pick up the differences between front/back/left/right
 * clears.  Need to sort that out...
 */
static void st_clear(GLcontext *ctx, GLbitfield mask)
{
   static const GLbitfield BUFFER_BITS_DS
      = (BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL);
   struct st_context *st = ctx->st;
   struct gl_renderbuffer *depthRb
      = ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
   struct gl_renderbuffer *stencilRb
      = ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLbitfield cmask = mask & BUFFER_BITS_COLOR;

   /* This makes sure the softpipe has the latest scissor, etc values */
   st_validate_state( st );

   /*
    * XXX TO-DO:
    * If we're going to use clear_with_quad() for any reason, use it to
    * clear as many other buffers as possible.
    * As it is now, we sometimes call clear_with_quad() three times to clear
    * color/depth/stencil individually...
    */

   if (cmask) {
      GLuint b;
      for (b = 0; cmask; b++) {
         if (cmask & (1 << b)) {
            struct gl_renderbuffer *rb
               = ctx->DrawBuffer->Attachment[b].Renderbuffer;
            assert(rb);
            clear_color_buffer(ctx, rb);
            cmask &= ~(1 << b); /* turn off bit */
         }
         assert(b < BUFFER_COUNT);
      }
   }

   if (mask & BUFFER_BIT_ACCUM) {
      st_clear_accum_buffer(ctx,
                       ctx->DrawBuffer->Attachment[BUFFER_ACCUM].Renderbuffer);
   }

   if ((mask & BUFFER_BITS_DS) == BUFFER_BITS_DS && depthRb == stencilRb) {
      /* clearing combined depth + stencil */
      clear_depth_stencil_buffer(ctx, depthRb);
   }
   else {
      /* separate depth/stencil clears */
      if (mask & BUFFER_BIT_DEPTH) {
         clear_depth_buffer(ctx, depthRb);
      }
      if (mask & BUFFER_BIT_STENCIL) {
         clear_stencil_buffer(ctx, stencilRb);
      }
   }
}


void st_init_clear_functions(struct dd_function_table *functions)
{
   functions->Clear = st_clear;
}
