/**************************************************************************
 *
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2009 Intel Corporation.
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

#include "main/arrayobj.h"
#include "main/attrib.h"
#include "main/blend.h"
#include "main/bufferobj.h"
#include "main/buffers.h"
#include "main/depth.h"
#include "main/enable.h"
#include "main/matrix.h"
#include "main/macros.h"
#include "main/polygon.h"
#include "main/shaders.h"
#include "main/stencil.h"
#include "main/texstate.h"
#include "main/varray.h"
#include "main/viewport.h"
#include "shader/arbprogram.h"
#include "shader/program.h"
#include "dri_metaops.h"

void
meta_set_passthrough_transform(struct dri_metaops *meta)
{
   GLcontext *ctx = meta->ctx;

   meta->saved_vp_x = ctx->Viewport.X;
   meta->saved_vp_y = ctx->Viewport.Y;
   meta->saved_vp_width = ctx->Viewport.Width;
   meta->saved_vp_height = ctx->Viewport.Height;
   meta->saved_matrix_mode = ctx->Transform.MatrixMode;

   meta->internal_viewport_call = GL_TRUE;
   _mesa_Viewport(0, 0, ctx->DrawBuffer->Width, ctx->DrawBuffer->Height);
   meta->internal_viewport_call = GL_FALSE;

   _mesa_MatrixMode(GL_PROJECTION);
   _mesa_PushMatrix();
   _mesa_LoadIdentity();
   _mesa_Ortho(0, ctx->DrawBuffer->Width, 0, ctx->DrawBuffer->Height, 1, -1);

   _mesa_MatrixMode(GL_MODELVIEW);
   _mesa_PushMatrix();
   _mesa_LoadIdentity();
}

void
meta_restore_transform(struct dri_metaops *meta)
{
   _mesa_MatrixMode(GL_PROJECTION);
   _mesa_PopMatrix();
   _mesa_MatrixMode(GL_MODELVIEW);
   _mesa_PopMatrix();

   _mesa_MatrixMode(meta->saved_matrix_mode);

   meta->internal_viewport_call = GL_TRUE;
   _mesa_Viewport(meta->saved_vp_x, meta->saved_vp_y,
		  meta->saved_vp_width, meta->saved_vp_height);
   meta->internal_viewport_call = GL_FALSE;
}


/**
 * Set up a vertex program to pass through the position and first texcoord
 * for pixel path.
 */
void
meta_set_passthrough_vertex_program(struct dri_metaops *meta)
{
   GLcontext *ctx = meta->ctx;
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

   assert(meta->saved_vp == NULL);

   _mesa_reference_vertprog(ctx, &meta->saved_vp,
			    ctx->VertexProgram.Current);
   if (meta->passthrough_vp == NULL) {
      GLuint prog_name;
      _mesa_GenPrograms(1, &prog_name);
      _mesa_BindProgram(GL_VERTEX_PROGRAM_ARB, prog_name);
      _mesa_ProgramStringARB(GL_VERTEX_PROGRAM_ARB,
			     GL_PROGRAM_FORMAT_ASCII_ARB,
			     strlen(vp), (const GLubyte *)vp);
      _mesa_reference_vertprog(ctx, &meta->passthrough_vp,
			       ctx->VertexProgram.Current);
      _mesa_DeletePrograms(1, &prog_name);
   }

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);
   _mesa_reference_vertprog(ctx, &ctx->VertexProgram.Current,
			    meta->passthrough_vp);
   ctx->Driver.BindProgram(ctx, GL_VERTEX_PROGRAM_ARB,
			   &meta->passthrough_vp->Base);

   meta->saved_vp_enable = ctx->VertexProgram.Enabled;
   _mesa_Enable(GL_VERTEX_PROGRAM_ARB);
}

/**
 * Restores the previous vertex program after
 * meta_set_passthrough_vertex_program()
 */
void
meta_restore_vertex_program(struct dri_metaops *meta)
{
   GLcontext *ctx = meta->ctx;

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);
   _mesa_reference_vertprog(ctx, &ctx->VertexProgram.Current,
			    meta->saved_vp);
   _mesa_reference_vertprog(ctx, &meta->saved_vp, NULL);
   ctx->Driver.BindProgram(ctx, GL_VERTEX_PROGRAM_ARB,
			   &ctx->VertexProgram.Current->Base);

   if (!meta->saved_vp_enable)
      _mesa_Disable(GL_VERTEX_PROGRAM_ARB);
}

/**
 * Binds the given program string to GL_FRAGMENT_PROGRAM_ARB, caching the
 * program object.
 */
void
meta_set_fragment_program(struct dri_metaops *meta,
			  struct gl_fragment_program **prog,
			  const char *prog_string)
{
   GLcontext *ctx = meta->ctx;
   assert(meta->saved_fp == NULL);

   _mesa_reference_fragprog(ctx, &meta->saved_fp,
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

   meta->saved_fp_enable = ctx->FragmentProgram.Enabled;
   _mesa_Enable(GL_FRAGMENT_PROGRAM_ARB);
}

/**
 * Restores the previous fragment program after
 * meta_set_fragment_program()
 */
void
meta_restore_fragment_program(struct dri_metaops *meta)
{
   GLcontext *ctx = meta->ctx;

   FLUSH_VERTICES(ctx, _NEW_PROGRAM);
   _mesa_reference_fragprog(ctx, &ctx->FragmentProgram.Current,
			    meta->saved_fp);
   _mesa_reference_fragprog(ctx, &meta->saved_fp, NULL);
   ctx->Driver.BindProgram(ctx, GL_FRAGMENT_PROGRAM_ARB,
			   &ctx->FragmentProgram.Current->Base);

   if (!meta->saved_fp_enable)
      _mesa_Disable(GL_FRAGMENT_PROGRAM_ARB);
}

static const float default_texcoords[4][2] = { { 0.0, 0.0 },
					       { 1.0, 0.0 },
					       { 1.0, 1.0 },
					       { 0.0, 1.0 } };

void
meta_set_default_texrect(struct dri_metaops *meta)
{
   GLcontext *ctx = meta->ctx;
   struct gl_client_array *old_texcoord_array;

   meta->saved_active_texture = ctx->Texture.CurrentUnit;
   if (meta->saved_array_vbo == NULL) {
      _mesa_reference_buffer_object(ctx, &meta->saved_array_vbo,
				    ctx->Array.ArrayBufferObj);
   }

   old_texcoord_array = &ctx->Array.ArrayObj->TexCoord[0];
   meta->saved_texcoord_type = old_texcoord_array->Type;
   meta->saved_texcoord_size = old_texcoord_array->Size;
   meta->saved_texcoord_stride = old_texcoord_array->Stride;
   meta->saved_texcoord_enable = old_texcoord_array->Enabled;
   meta->saved_texcoord_ptr = old_texcoord_array->Ptr;
   _mesa_reference_buffer_object(ctx, &meta->saved_texcoord_vbo,
				 old_texcoord_array->BufferObj);

   _mesa_ClientActiveTextureARB(GL_TEXTURE0);

   if (meta->texcoord_vbo == NULL) {
      GLuint vbo_name;

      _mesa_GenBuffersARB(1, &vbo_name);
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, vbo_name);
      _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(default_texcoords),
			  default_texcoords, GL_STATIC_DRAW_ARB);
      _mesa_reference_buffer_object(ctx, &meta->texcoord_vbo,
				    ctx->Array.ArrayBufferObj);
   } else {
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB,
			  meta->texcoord_vbo->Name);
   }
   _mesa_TexCoordPointer(2, GL_FLOAT, 2 * sizeof(GLfloat), NULL);

   _mesa_Enable(GL_TEXTURE_COORD_ARRAY);
}

void
meta_restore_texcoords(struct dri_metaops *meta)
{
   GLcontext *ctx = meta->ctx;

   /* Restore the old TexCoordPointer */
   if (meta->saved_texcoord_vbo) {
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB,
			  meta->saved_texcoord_vbo->Name);
      _mesa_reference_buffer_object(ctx, &meta->saved_texcoord_vbo, NULL);
   } else {
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
   }

   _mesa_TexCoordPointer(meta->saved_texcoord_size,
			 meta->saved_texcoord_type,
			 meta->saved_texcoord_stride,
			 meta->saved_texcoord_ptr);
   if (!meta->saved_texcoord_enable)
      _mesa_Disable(GL_TEXTURE_COORD_ARRAY);

   _mesa_ClientActiveTextureARB(GL_TEXTURE0 +
				meta->saved_active_texture);

   if (meta->saved_array_vbo) {
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB,
			  meta->saved_array_vbo->Name);
      _mesa_reference_buffer_object(ctx, &meta->saved_array_vbo, NULL);
   } else {
      _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
   }
}


/**
 * Perform glClear where mask contains only color, depth, and/or stencil.
 *
 * The implementation is based on calling into Mesa to set GL state and
 * performing normal triangle rendering.  The intent of this path is to
 * have as generic a path as possible, so that any driver could make use of
 * it.
 */

/**
 * Per-context one-time init of things for intl_clear_tris().
 * Basically set up a private array object for vertex/color arrays.
 */
static void
meta_init_clear(struct dri_metaops *meta)
{
   GLcontext *ctx = meta->ctx;
   struct gl_array_object *arraySave = NULL;
   const GLuint arrayBuffer = ctx->Array.ArrayBufferObj->Name;
   const GLuint elementBuffer = ctx->Array.ElementArrayBufferObj->Name;

   /* create new array object */
   meta->clear.arrayObj = _mesa_new_array_object(ctx, ~0);

   /* save current array object, bind new one */
   _mesa_reference_array_object(ctx, &arraySave, ctx->Array.ArrayObj);
   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= _NEW_ARRAY_ALL;
   _mesa_reference_array_object(ctx, &ctx->Array.ArrayObj, meta->clear.arrayObj);

   /* one-time setup of vertex arrays (pos, color) */
   _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
   _mesa_BindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
   _mesa_ColorPointer(4, GL_FLOAT, 4 * sizeof(GLfloat), meta->clear.color);
   _mesa_VertexPointer(3, GL_FLOAT, 3 * sizeof(GLfloat), meta->clear.vertices);
   _mesa_Enable(GL_COLOR_ARRAY);
   _mesa_Enable(GL_VERTEX_ARRAY);

   /* restore original array object */
   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= _NEW_ARRAY_ALL;
   _mesa_reference_array_object(ctx, &ctx->Array.ArrayObj, arraySave);
   _mesa_reference_array_object(ctx, &arraySave, NULL);

   /* restore original buffer objects */
   _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, arrayBuffer);
   _mesa_BindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, elementBuffer);
}



/**
 * Perform glClear where mask contains only color, depth, and/or stencil.
 *
 * The implementation is based on calling into Mesa to set GL state and
 * performing normal triangle rendering.  The intent of this path is to
 * have as generic a path as possible, so that any driver could make use of
 * it.
 */
void
meta_clear_tris(struct dri_metaops *meta, GLbitfield mask)
{
   GLcontext *ctx = meta->ctx;
   GLfloat dst_z;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   int i;
   GLboolean saved_fp_enable = GL_FALSE, saved_vp_enable = GL_FALSE;
   GLuint saved_shader_program = 0;
   unsigned int saved_active_texture;
   struct gl_array_object *arraySave = NULL;

   if (!meta->clear.arrayObj)
      meta_init_clear(meta);

   assert((mask & ~(TRI_CLEAR_COLOR_BITS | BUFFER_BIT_DEPTH |
		    BUFFER_BIT_STENCIL)) == 0);

   _mesa_PushAttrib(GL_COLOR_BUFFER_BIT |
		    GL_DEPTH_BUFFER_BIT |
		    GL_ENABLE_BIT |
		    GL_POLYGON_BIT |
		    GL_STENCIL_BUFFER_BIT |
		    GL_TRANSFORM_BIT |
		    GL_CURRENT_BIT |
		    GL_VIEWPORT_BIT);
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
   _mesa_PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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

   /* save current array object, bind our private one */
   _mesa_reference_array_object(ctx, &arraySave, ctx->Array.ArrayObj);
   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= _NEW_ARRAY_ALL;
   _mesa_reference_array_object(ctx, &ctx->Array.ArrayObj, meta->clear.arrayObj);

   meta_set_passthrough_transform(meta);

   for (i = 0; i < 4; i++) {
      COPY_4FV(meta->clear.color[i], ctx->Color.ClearColor);
   }

   /* convert clear Z from [0,1] to NDC coord in [-1,1] */
   dst_z = -1.0 + 2.0 * ctx->Depth.Clear;

   /* The ClearDepth value is unaffected by DepthRange, so do a default
    * mapping.
    */
   _mesa_DepthRange(0.0, 1.0);

   /* Prepare the vertices, which are the same regardless of which buffer we're
    * drawing to.
    */
   meta->clear.vertices[0][0] = fb->_Xmin;
   meta->clear.vertices[0][1] = fb->_Ymin;
   meta->clear.vertices[0][2] = dst_z;
   meta->clear.vertices[1][0] = fb->_Xmax;
   meta->clear.vertices[1][1] = fb->_Ymin;
   meta->clear.vertices[1][2] = dst_z;
   meta->clear.vertices[2][0] = fb->_Xmax;
   meta->clear.vertices[2][1] = fb->_Ymax;
   meta->clear.vertices[2][2] = dst_z;
   meta->clear.vertices[3][0] = fb->_Xmin;
   meta->clear.vertices[3][1] = fb->_Ymax;
   meta->clear.vertices[3][2] = dst_z;

   while (mask != 0) {
      GLuint this_mask = 0;
      GLuint color_bit;

      color_bit = _mesa_ffs(mask & TRI_CLEAR_COLOR_BITS);
      if (color_bit != 0)
	 this_mask |= (1 << (color_bit - 1));

      /* Clear depth/stencil in the same pass as color. */
      this_mask |= (mask & (BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL));

      /* Select the current color buffer and use the color write mask if
       * we have one, otherwise don't write any color channels.
       */
      if (this_mask & BUFFER_BIT_FRONT_LEFT)
	 _mesa_DrawBuffer(GL_FRONT_LEFT);
      else if (this_mask & BUFFER_BIT_BACK_LEFT)
	 _mesa_DrawBuffer(GL_BACK_LEFT);
      else if (color_bit != 0)
	 _mesa_DrawBuffer(GL_COLOR_ATTACHMENT0 +
			  (color_bit - BUFFER_COLOR0 - 1));
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
	 _mesa_StencilOpSeparate(GL_FRONT_AND_BACK,
				 GL_REPLACE, GL_REPLACE, GL_REPLACE);
	 _mesa_StencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS,
				   ctx->Stencil.Clear & 0x7fffffff,
				   ctx->Stencil.WriteMask[0]);
      } else {
	 _mesa_Disable(GL_STENCIL_TEST);
      }

      _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

      mask &= ~this_mask;
   }

   meta_restore_transform(meta);

   _mesa_ActiveTextureARB(GL_TEXTURE0 + saved_active_texture);
   if (saved_fp_enable)
      _mesa_Enable(GL_FRAGMENT_PROGRAM_ARB);
   if (saved_vp_enable)
      _mesa_Enable(GL_VERTEX_PROGRAM_ARB);

   if (saved_shader_program)
      _mesa_UseProgramObjectARB(saved_shader_program);

   _mesa_PopAttrib();

   /* restore current array object */
   ctx->NewState |= _NEW_ARRAY;
   ctx->Array.NewState |= _NEW_ARRAY_ALL;
   _mesa_reference_array_object(ctx, &ctx->Array.ArrayObj, arraySave);
   _mesa_reference_array_object(ctx, &arraySave, NULL);
}

void meta_init_metaops(GLcontext *ctx, struct dri_metaops *meta)
{
   meta->ctx = ctx;
}

void meta_destroy_metaops(struct dri_metaops *meta)
{
  if (meta->clear.arrayObj)
    _mesa_delete_array_object(meta->ctx, meta->clear.arrayObj);

}
