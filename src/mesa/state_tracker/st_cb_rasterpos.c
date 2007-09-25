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
  *   Brian Paul
  */

#include "main/imports.h"
#include "main/macros.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_cache.h"
#include "st_draw.h"
#include "st_program.h"
#include "st_cb_rasterpos.h"
#include "st_draw.h"
#include "st_format.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "shader/prog_instruction.h"
#include "vf/vf.h"



static void
setup_vertex_attribs(GLcontext *ctx)
{
   struct pipe_context *pipe = ctx->st->pipe;
   const struct cso_vertex_shader *vs = ctx->st->state.vs;
   const struct st_vertex_program *stvp = ctx->st->vp;
   uint slot;

   /* all attributes come from the default attribute buffer */
   {
      struct pipe_vertex_buffer vbuffer;
      vbuffer.buffer = ctx->st->default_attrib_buffer;
      vbuffer.buffer_offset = 0;
      vbuffer.pitch = 0; /* must be zero! */
      vbuffer.max_index = 1;
      pipe->set_vertex_buffer(pipe, 0, &vbuffer);
   }

   for (slot = 0; slot < vs->state.num_inputs; slot++) {
      struct pipe_vertex_element velement;
      const GLuint attr = stvp->index_to_input[slot];

      velement.src_offset = attr * 4 * sizeof(GLfloat);
      velement.vertex_buffer_index = 0;
      velement.dst_offset = 0;
      velement.src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
      pipe->set_vertex_element(pipe, slot, &velement);
   }
}


static void
setup_feedback(GLcontext *ctx)
{
   struct pipe_context *pipe = ctx->st->pipe;
   const struct pipe_shader_state *vs = &ctx->st->state.vs->state;
   struct pipe_feedback_state feedback;
   uint i;

   memset(&feedback, 0, sizeof(feedback));
   feedback.enabled = 1;
   feedback.interleaved = 1;
   feedback.discard = 1;
   feedback.num_attribs = 0;

   /* feedback all results from vertex shader */
   for (i = 0; i < vs->num_outputs; i++) {
      feedback.attrib[feedback.num_attribs] = i;
      feedback.size[feedback.num_attribs] = 4;
      feedback.num_attribs++;
   }

   pipe->set_feedback_state(pipe, &feedback);
}





/**
 * Clip a point against the view volume.
 *
 * \param v vertex vector describing the point to clip.
 * 
 * \return zero if outside view volume, or one if inside.
 */
static GLuint
viewclip_point( const GLfloat v[] )
{
   if (   v[0] > v[3] || v[0] < -v[3]
       || v[1] > v[3] || v[1] < -v[3]
       || v[2] > v[3] || v[2] < -v[3] ) {
      return 0;
   }
   else {
      return 1;
   }
}


/**
 * Clip a point against the far/near Z clipping planes.
 *
 * \param v vertex vector describing the point to clip.
 * 
 * \return zero if outside view volume, or one if inside.
 */
static GLuint
viewclip_point_z( const GLfloat v[] )
{
   if (v[2] > v[3] || v[2] < -v[3] ) {
      return 0;
   }
   else {
      return 1;
   }
}


/**
 * Clip a point against the user clipping planes.
 * 
 * \param ctx GL context.
 * \param v vertex vector describing the point to clip.
 * 
 * \return zero if the point was clipped, or one otherwise.
 */
static GLuint
userclip_point( GLcontext *ctx, const GLfloat v[] )
{
   GLuint p;

   for (p = 0; p < ctx->Const.MaxClipPlanes; p++) {
      if (ctx->Transform.ClipPlanesEnabled & (1 << p)) {
	 GLfloat dot = v[0] * ctx->Transform._ClipUserPlane[p][0]
		     + v[1] * ctx->Transform._ClipUserPlane[p][1]
		     + v[2] * ctx->Transform._ClipUserPlane[p][2]
		     + v[3] * ctx->Transform._ClipUserPlane[p][3];
         if (dot < 0.0F) {
            return 0;
         }
      }
   }

   return 1;
}


/**
 * Update the current raster position.
 * Do clip testing, etc. here.
 */
static void
update_rasterpos(GLcontext *ctx,
                 const float clipPos[4],
                 const float color0[4],
                 const float color1[4],
                 const float *tex)
{
   uint i;
   float d, ndc[3];

   /* clip to view volume */
   if (ctx->Transform.RasterPositionUnclipped) {
      /* GL_IBM_rasterpos_clip: only clip against Z */
      if (viewclip_point_z(clipPos) == 0) {
         ctx->Current.RasterPosValid = GL_FALSE;
         return;
      }
   }
   else if (viewclip_point(clipPos) == 0) {
      /* Normal OpenGL behaviour */
      ctx->Current.RasterPosValid = GL_FALSE;
      return;
   }

   /* clip to user clipping planes */
   if (ctx->Transform.ClipPlanesEnabled && !userclip_point(ctx, clipPos)) {
      ctx->Current.RasterPosValid = GL_FALSE;
      return;
   }


   /*
    * update current raster position
    */
   /* ndc = clip / W */
   d = (clipPos[3] == 0.0F) ? 1.0F : 1.0F / clipPos[3];
   ndc[0] = clipPos[0] * d;
   ndc[1] = clipPos[1] * d;
   ndc[2] = clipPos[2] * d;
   /* wincoord = viewport_mapping(ndc) */
   ctx->Current.RasterPos[0] = (ndc[0] * ctx->Viewport._WindowMap.m[MAT_SX]
                                + ctx->Viewport._WindowMap.m[MAT_TX]);
   ctx->Current.RasterPos[1] = (ndc[1] * ctx->Viewport._WindowMap.m[MAT_SY]
                                + ctx->Viewport._WindowMap.m[MAT_TY]);
   ctx->Current.RasterPos[2] = (ndc[2] * ctx->Viewport._WindowMap.m[MAT_SZ]
                                + ctx->Viewport._WindowMap.m[MAT_TZ])
                               / ctx->DrawBuffer->_DepthMaxF;
   ctx->Current.RasterPos[3] = clipPos[3];

   /* compute raster distance */
   if (ctx->Fog.FogCoordinateSource == GL_FOG_COORDINATE_EXT)
      ctx->Current.RasterDistance = ctx->Current.Attrib[VERT_ATTRIB_FOG][0];
   else {
#if 0
      /* XXX we don't have an eye coord! */
      ctx->Current.RasterDistance =
         SQRTF( eye[0]*eye[0] + eye[1]*eye[1] + eye[2]*eye[2] );
#endif
   }

   /* colors and texcoords */
   COPY_4FV(ctx->Current.RasterColor, color0);
   COPY_4FV(ctx->Current.RasterSecondaryColor, color1);
   for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
      COPY_4FV(ctx->Current.RasterTexCoords + i, tex + i *4);
   }

   ctx->Current.RasterPosValid = GL_TRUE;
}



static void
st_RasterPos(GLcontext *ctx, const GLfloat v[4])
{
   const struct st_context *st = ctx->st;
   struct pipe_context *pipe = st->pipe;
   float *buf_map;
   struct pipe_feedback_buffer fb_buf;

   st_validate_state(ctx->st);

   /* setup vertex buffers */
   setup_vertex_attribs(ctx);

   /*
    * Load the default attribute buffer with current attribs.
    */
   {
      struct pipe_buffer_handle *buf = st->default_attrib_buffer;
      const unsigned size = sizeof(ctx->Current.Attrib);
      const void *data = ctx->Current.Attrib;
      /* colors, texcoords, etc */
      pipe->winsys->buffer_data(pipe->winsys, buf, size, data);
      /* position */
      pipe->winsys->buffer_subdata(pipe->winsys, buf,
                                   0, /* offset */
                                   4 * sizeof(float), /* size */
                                   v); /* data */
   }


   /* setup feedback state */
   setup_feedback(ctx);

   /* setup vertex feedback buffer */
   {
      fb_buf.size = 8 * 4 * sizeof(float);
      fb_buf.buffer = pipe->winsys->buffer_create(pipe->winsys, 0);
      fb_buf.start_offset = 0;
      pipe->winsys->buffer_data(pipe->winsys, fb_buf.buffer,
                                fb_buf.size,
                                NULL); /* data */
      pipe->set_feedback_buffer(pipe, 0, &fb_buf);
   }


   /* draw a point */
   pipe->draw_arrays(pipe, GL_POINTS, 0, 1);

   /* get feedback */
   buf_map = (float *) pipe->winsys->buffer_map(pipe->winsys, fb_buf.buffer,
                                                PIPE_BUFFER_FLAG_READ);

   /* extract values and update rasterpos state */
   {
      const GLuint *outputMapping = st->vertex_result_to_slot;
      const float *pos, *color0, *color1, *tex0;
      float *buf = buf_map;

      assert(outputMapping[VERT_RESULT_HPOS] != ~0);
      pos = buf;
      buf += 4;

      if (outputMapping[VERT_RESULT_COL0] != ~0) {
         color0 = buf;
         buf += 4;
      }
      else {
         color0 = ctx->Current.Attrib[VERT_ATTRIB_COLOR0];
      }

      if (outputMapping[VERT_RESULT_COL1] != ~0) {
         color1 = buf;
         buf += 4;
      }
      else {
         color1 = ctx->Current.Attrib[VERT_ATTRIB_COLOR1];
      }

      if (outputMapping[VERT_RESULT_TEX0] != ~0) {
         tex0 = buf;
         buf += 4;
      }
      else {
         tex0 = ctx->Current.Attrib[VERT_ATTRIB_TEX0];
      }

      update_rasterpos(ctx, pos, color0, color1, tex0);
   }

   /* free vertex feedback buffer */
   pipe->winsys->buffer_unmap(pipe->winsys, fb_buf.buffer);
   pipe->winsys->buffer_reference(pipe->winsys, &fb_buf.buffer, NULL);

   /* restore pipe state */
   pipe->set_feedback_state(pipe, &st->state.feedback);
}


void st_init_rasterpos_functions(struct dd_function_table *functions)
{
   functions->RasterPos = st_RasterPos;
}
