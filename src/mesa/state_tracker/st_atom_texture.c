/**************************************************************************
 *
 * Copyright 2007 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  *   Brian Paul
  */


#include "main/macros.h"
#include "main/mtypes.h"
#include "main/samplerobj.h"
#include "main/texobj.h"
#include "program/prog_instruction.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_texture.h"
#include "st_format.h"
#include "st_cb_texture.h"
#include "pipe/p_context.h"
#include "util/u_format.h"
#include "util/u_inlines.h"
#include "cso_cache/cso_context.h"


/**
 * Return swizzle1(swizzle2)
 */
static unsigned
swizzle_swizzle(unsigned swizzle1, unsigned swizzle2)
{
   unsigned i, swz[4];

   for (i = 0; i < 4; i++) {
      unsigned s = GET_SWZ(swizzle1, i);
      switch (s) {
      case SWIZZLE_X:
      case SWIZZLE_Y:
      case SWIZZLE_Z:
      case SWIZZLE_W:
         swz[i] = GET_SWZ(swizzle2, s);
         break;
      case SWIZZLE_ZERO:
         swz[i] = SWIZZLE_ZERO;
         break;
      case SWIZZLE_ONE:
         swz[i] = SWIZZLE_ONE;
         break;
      default:
         assert(!"Bad swizzle term");
         swz[i] = SWIZZLE_X;
      }
   }

   return MAKE_SWIZZLE4(swz[0], swz[1], swz[2], swz[3]);
}


/**
 * Given a user-specified texture base format, the actual gallium texture
 * format and the current GL_DEPTH_MODE, return a texture swizzle.
 *
 * Consider the case where the user requests a GL_RGB internal texture
 * format the driver actually uses an RGBA format.  The A component should
 * be ignored and sampling from the texture should always return (r,g,b,1).
 * But if we rendered to the texture we might have written A values != 1.
 * By sampling the texture with a ".xyz1" swizzle we'll get the expected A=1.
 * This function computes the texture swizzle needed to get the expected
 * values.
 *
 * In the case of depth textures, the GL_DEPTH_MODE state determines the
 * texture swizzle.
 *
 * This result must be composed with the user-specified swizzle to get
 * the final swizzle.
 */
static unsigned
compute_texture_format_swizzle(GLenum baseFormat, GLenum depthMode,
                               enum pipe_format actualFormat)
{
   switch (baseFormat) {
   case GL_RGBA:
      return SWIZZLE_XYZW;
   case GL_RGB:
      if (util_format_has_alpha(actualFormat))
         return MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_ONE);
      else
         return SWIZZLE_XYZW;
   case GL_RG:
      if (util_format_get_nr_components(actualFormat) > 2)
         return MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_ZERO, SWIZZLE_ONE);
      else
         return SWIZZLE_XYZW;
   case GL_RED:
      if (util_format_get_nr_components(actualFormat) > 1)
         return MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_ZERO,
                              SWIZZLE_ZERO, SWIZZLE_ONE);
      else
         return SWIZZLE_XYZW;
   case GL_ALPHA:
      if (util_format_get_nr_components(actualFormat) > 1)
         return MAKE_SWIZZLE4(SWIZZLE_ZERO, SWIZZLE_ZERO,
                              SWIZZLE_ZERO, SWIZZLE_W);
      else
         return SWIZZLE_XYZW;
   case GL_LUMINANCE:
      if (util_format_get_nr_components(actualFormat) > 1)
         return MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_ONE);
      else
         return SWIZZLE_XYZW;
   case GL_LUMINANCE_ALPHA:
      if (util_format_get_nr_components(actualFormat) > 2)
         return MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_W);
      else
         return SWIZZLE_XYZW;
   case GL_INTENSITY:
      if (util_format_get_nr_components(actualFormat) > 1)
         return SWIZZLE_XXXX;
      else
         return SWIZZLE_XYZW;
   case GL_STENCIL_INDEX:
      return SWIZZLE_XYZW;
   case GL_DEPTH_STENCIL:
      /* fall-through */
   case GL_DEPTH_COMPONENT:
      /* Now examine the depth mode */
      switch (depthMode) {
      case GL_LUMINANCE:
         return MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_ONE);
      case GL_INTENSITY:
         return MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X);
      case GL_ALPHA:
         return MAKE_SWIZZLE4(SWIZZLE_ZERO, SWIZZLE_ZERO,
                              SWIZZLE_ZERO, SWIZZLE_X);
      case GL_RED:
         return MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_ZERO,
                              SWIZZLE_ZERO, SWIZZLE_ONE);
      default:
         assert(!"Unexpected depthMode");
         return SWIZZLE_XYZW;
      }
   default:
      assert(!"Unexpected baseFormat");
      return SWIZZLE_XYZW;
   }
}


static unsigned
get_texture_format_swizzle(const struct st_texture_object *stObj)
{
   const struct gl_texture_image *texImage =
      stObj->base.Image[0][stObj->base.BaseLevel];
   unsigned tex_swizzle;

   if (texImage) {
      tex_swizzle = compute_texture_format_swizzle(texImage->_BaseFormat,
                                                   stObj->base.DepthMode,
                                                   stObj->pt->format);
   }
   else {
      tex_swizzle = SWIZZLE_XYZW;
   }

   /* Combine the texture format swizzle with user's swizzle */
   return swizzle_swizzle(stObj->base._Swizzle, tex_swizzle);
}


/**
 * Return TRUE if the texture's sampler view swizzle is not equal to
 * the texture's swizzle.
 *
 * \param stObj  the st texture object,
 */
static boolean
check_sampler_swizzle(const struct st_texture_object *stObj,
		      struct pipe_sampler_view *sv)
{
   unsigned swizzle = get_texture_format_swizzle(stObj);

   return ((sv->swizzle_r != GET_SWZ(swizzle, 0)) ||
           (sv->swizzle_g != GET_SWZ(swizzle, 1)) ||
           (sv->swizzle_b != GET_SWZ(swizzle, 2)) ||
           (sv->swizzle_a != GET_SWZ(swizzle, 3)));
}


static unsigned last_level(struct st_texture_object *stObj)
{
   unsigned ret = MIN2(stObj->base.MinLevel + stObj->base._MaxLevel,
                       stObj->pt->last_level);
   if (stObj->base.Immutable)
      ret = MIN2(ret, stObj->base.MinLevel + stObj->base.NumLevels - 1);
   return ret;
}

static unsigned last_layer(struct st_texture_object *stObj)
{
   if (stObj->base.Immutable && stObj->pt->array_size > 1)
      return MIN2(stObj->base.MinLayer + stObj->base.NumLayers - 1,
                  stObj->pt->array_size - 1);
   return stObj->pt->array_size - 1;
}

static struct pipe_sampler_view *
st_create_texture_sampler_view_from_stobj(struct pipe_context *pipe,
					  struct st_texture_object *stObj,
                                          const struct gl_sampler_object *samp,
					  enum pipe_format format)
{
   struct pipe_sampler_view templ;
   unsigned swizzle = get_texture_format_swizzle(stObj);

   u_sampler_view_default_template(&templ,
                                   stObj->pt,
                                   format);

   if (stObj->pt->target == PIPE_BUFFER) {
      unsigned base, size;
      unsigned f, n;
      const struct util_format_description *desc
         = util_format_description(templ.format);

      base = stObj->base.BufferOffset;
      if (base >= stObj->pt->width0)
         return NULL;
      size = MIN2(stObj->pt->width0 - base, (unsigned)stObj->base.BufferSize);

      f = ((base * 8) / desc->block.bits) * desc->block.width;
      n = ((size * 8) / desc->block.bits) * desc->block.width;
      if (!n)
         return NULL;
      templ.u.buf.first_element = f;
      templ.u.buf.last_element  = f + (n - 1);
   } else {
      templ.u.tex.first_level = stObj->base.MinLevel + stObj->base.BaseLevel;
      templ.u.tex.last_level = last_level(stObj);
      assert(templ.u.tex.first_level <= templ.u.tex.last_level);
      templ.u.tex.first_layer = stObj->base.MinLayer;
      templ.u.tex.last_layer = last_layer(stObj);
      assert(templ.u.tex.first_layer <= templ.u.tex.last_layer);
      templ.target = gl_target_to_pipe(stObj->base.Target);
   }

   if (swizzle != SWIZZLE_NOOP) {
      templ.swizzle_r = GET_SWZ(swizzle, 0);
      templ.swizzle_g = GET_SWZ(swizzle, 1);
      templ.swizzle_b = GET_SWZ(swizzle, 2);
      templ.swizzle_a = GET_SWZ(swizzle, 3);
   }

   return pipe->create_sampler_view(pipe, stObj->pt, &templ);
}


static struct pipe_sampler_view *
st_get_texture_sampler_view_from_stobj(struct st_context *st,
                                       struct st_texture_object *stObj,
                                       const struct gl_sampler_object *samp,
				       enum pipe_format format)
{
   struct pipe_sampler_view **sv;

   if (!stObj || !stObj->pt) {
      return NULL;
   }

   sv = st_texture_get_sampler_view(st, stObj);

   if (stObj->base.StencilSampling &&
       util_format_is_depth_and_stencil(format))
      format = util_format_stencil_only(format);

   /* if sampler view has changed dereference it */
   if (*sv) {
      if (check_sampler_swizzle(stObj, *sv) ||
	  (format != (*sv)->format) ||
          gl_target_to_pipe(stObj->base.Target) != (*sv)->target ||
          stObj->base.MinLevel + stObj->base.BaseLevel != (*sv)->u.tex.first_level ||
          last_level(stObj) != (*sv)->u.tex.last_level ||
          stObj->base.MinLayer != (*sv)->u.tex.first_layer ||
          last_layer(stObj) != (*sv)->u.tex.last_layer) {
	 pipe_sampler_view_reference(sv, NULL);
      }
   }

   if (!*sv) {
      *sv = st_create_texture_sampler_view_from_stobj(st->pipe, stObj, samp, format);

   } else if ((*sv)->context != st->pipe) {
      /* Recreate view in correct context, use existing view as template */
      struct pipe_sampler_view *new_sv =
         st->pipe->create_sampler_view(st->pipe, stObj->pt, *sv);
      pipe_sampler_view_reference(sv, NULL);
      *sv = new_sv;
   }

   return *sv;
}

static GLboolean
update_single_texture(struct st_context *st,
                      struct pipe_sampler_view **sampler_view,
		      GLuint texUnit)
{
   struct gl_context *ctx = st->ctx;
   const struct gl_sampler_object *samp;
   struct gl_texture_object *texObj;
   struct st_texture_object *stObj;
   enum pipe_format view_format;
   GLboolean retval;

   samp = _mesa_get_samplerobj(ctx, texUnit);

   texObj = ctx->Texture.Unit[texUnit]._Current;

   if (!texObj) {
      texObj = _mesa_get_fallback_texture(ctx, TEXTURE_2D_INDEX);
      samp = &texObj->Sampler;
   }
   stObj = st_texture_object(texObj);

   retval = st_finalize_texture(ctx, st->pipe, texObj);
   if (!retval) {
      /* out of mem */
      return GL_FALSE;
   }

   /* Determine the format of the texture sampler view */
   if (texObj->Target == GL_TEXTURE_BUFFER) {
      view_format =
         st_mesa_format_to_pipe_format(st, stObj->base._BufferObjectFormat);
   }
   else {
      view_format =
         stObj->surface_based ? stObj->surface_format : stObj->pt->format;

      /* If sRGB decoding is off, use the linear format */
      if (samp->sRGBDecode == GL_SKIP_DECODE_EXT) {
         view_format = util_format_linear(view_format);
      }
   }

   *sampler_view = st_get_texture_sampler_view_from_stobj(st, stObj, samp,
							  view_format);
   return GL_TRUE;
}



static void
update_textures(struct st_context *st,
                unsigned shader_stage,
                const struct gl_program *prog,
                unsigned max_units,
                struct pipe_sampler_view **sampler_views,
                unsigned *num_textures)
{
   const GLuint old_max = *num_textures;
   GLbitfield samplers_used = prog->SamplersUsed;
   GLuint unit;

   if (samplers_used == 0x0 && old_max == 0)
      return;

   *num_textures = 0;

   /* loop over sampler units (aka tex image units) */
   for (unit = 0; unit < max_units; unit++, samplers_used >>= 1) {
      struct pipe_sampler_view *sampler_view = NULL;

      if (samplers_used & 1) {
         const GLuint texUnit = prog->SamplerUnits[unit];
         GLboolean retval;

         retval = update_single_texture(st, &sampler_view, texUnit);
         if (retval == GL_FALSE)
            continue;

         *num_textures = unit + 1;
      }
      else if (samplers_used == 0 && unit >= old_max) {
         /* if we've reset all the old views and we have no more new ones */
         break;
      }

      pipe_sampler_view_reference(&(sampler_views[unit]), sampler_view);
   }

   cso_set_sampler_views(st->cso_context,
                         shader_stage,
                         *num_textures,
                         sampler_views);
}



static void
update_vertex_textures(struct st_context *st)
{
   const struct gl_context *ctx = st->ctx;

   if (ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits > 0) {
      update_textures(st,
                      PIPE_SHADER_VERTEX,
                      &ctx->VertexProgram._Current->Base,
                      ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits,
                      st->state.sampler_views[PIPE_SHADER_VERTEX],
                      &st->state.num_sampler_views[PIPE_SHADER_VERTEX]);
   }
}


static void
update_fragment_textures(struct st_context *st)
{
   const struct gl_context *ctx = st->ctx;

   update_textures(st,
                   PIPE_SHADER_FRAGMENT,
                   &ctx->FragmentProgram._Current->Base,
                   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits,
                   st->state.sampler_views[PIPE_SHADER_FRAGMENT],
                   &st->state.num_sampler_views[PIPE_SHADER_FRAGMENT]);
}


static void
update_geometry_textures(struct st_context *st)
{
   const struct gl_context *ctx = st->ctx;

   if (ctx->GeometryProgram._Current) {
      update_textures(st,
                      PIPE_SHADER_GEOMETRY,
                      &ctx->GeometryProgram._Current->Base,
                      ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxTextureImageUnits,
                      st->state.sampler_views[PIPE_SHADER_GEOMETRY],
                      &st->state.num_sampler_views[PIPE_SHADER_GEOMETRY]);
   }
}


const struct st_tracked_state st_update_fragment_texture = {
   "st_update_texture",					/* name */
   {							/* dirty */
      _NEW_TEXTURE,					/* mesa */
      ST_NEW_FRAGMENT_PROGRAM,				/* st */
   },
   update_fragment_textures				/* update */
};


const struct st_tracked_state st_update_vertex_texture = {
   "st_update_vertex_texture",				/* name */
   {							/* dirty */
      _NEW_TEXTURE,					/* mesa */
      ST_NEW_VERTEX_PROGRAM,				/* st */
   },
   update_vertex_textures				/* update */
};


const struct st_tracked_state st_update_geometry_texture = {
   "st_update_geometry_texture",			/* name */
   {							/* dirty */
      _NEW_TEXTURE,					/* mesa */
      ST_NEW_GEOMETRY_PROGRAM,				/* st */
   },
   update_geometry_textures				/* update */
};



static void
finalize_textures(struct st_context *st)
{
   struct gl_context *ctx = st->ctx;
   struct gl_fragment_program *fprog = ctx->FragmentProgram._Current;
   const GLboolean prev_missing_textures = st->missing_textures;
   GLuint su;

   st->missing_textures = GL_FALSE;

   for (su = 0; su < ctx->Const.MaxTextureCoordUnits; su++) {
      if (fprog->Base.SamplersUsed & (1 << su)) {
         const GLuint texUnit = fprog->Base.SamplerUnits[su];
         struct gl_texture_object *texObj
            = ctx->Texture.Unit[texUnit]._Current;

         if (texObj) {
            GLboolean retval;

            retval = st_finalize_texture(ctx, st->pipe, texObj);
            if (!retval) {
               /* out of mem */
               st->missing_textures = GL_TRUE;
               continue;
            }
         }
      }
   }

   if (prev_missing_textures != st->missing_textures)
      st->dirty.st |= ST_NEW_FRAGMENT_PROGRAM;
}


const struct st_tracked_state st_finalize_textures = {
   "st_finalize_textures",		/* name */
   {					/* dirty */
      _NEW_TEXTURE,			/* mesa */
      0,				/* st */
   },
   finalize_textures			/* update */
};
