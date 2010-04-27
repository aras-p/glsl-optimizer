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
 

#include "main/macros.h"
#include "shader/prog_instruction.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_texture.h"
#include "st_cb_texture.h"
#include "pipe/p_context.h"
#include "util/u_inlines.h"
#include "cso_cache/cso_context.h"

static boolean check_sampler_swizzle(struct pipe_sampler_view *sv,
				   uint32_t _swizzle)
{
   if ((sv->swizzle_r != GET_SWZ(_swizzle, 0)) ||
       (sv->swizzle_g != GET_SWZ(_swizzle, 1)) ||
       (sv->swizzle_b != GET_SWZ(_swizzle, 2)) ||
       (sv->swizzle_a != GET_SWZ(_swizzle, 3)))
      return true;
   return false;
}

static INLINE struct pipe_sampler_view *
st_create_texture_sampler_view_from_stobj(struct pipe_context *pipe,
					  struct st_texture_object *stObj)
					  
{
   struct pipe_sampler_view templ;

   u_sampler_view_default_template(&templ,
                                   stObj->pt,
                                   stObj->pt->format);

   if (stObj->base._Swizzle != SWIZZLE_NOOP) {
      templ.swizzle_r = GET_SWZ(stObj->base._Swizzle, 0);
      templ.swizzle_g = GET_SWZ(stObj->base._Swizzle, 1);
      templ.swizzle_b = GET_SWZ(stObj->base._Swizzle, 2);
      templ.swizzle_a = GET_SWZ(stObj->base._Swizzle, 3);
   }

   return pipe->create_sampler_view(pipe, stObj->pt, &templ);
}


static INLINE struct pipe_sampler_view *
st_get_texture_sampler_view_from_stobj(struct st_texture_object *stObj,
				       struct pipe_context *pipe)

{
   if (!stObj || !stObj->pt) {
      return NULL;
   }

   if (!stObj->sampler_view) {
      stObj->sampler_view = st_create_texture_sampler_view_from_stobj(pipe, stObj);
   }

   return stObj->sampler_view;
}

static void 
update_textures(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;
   struct gl_vertex_program *vprog = st->ctx->VertexProgram._Current;
   struct gl_fragment_program *fprog = st->ctx->FragmentProgram._Current;
   const GLbitfield samplersUsed = (vprog->Base.SamplersUsed |
                                    fprog->Base.SamplersUsed);
   GLuint su;

   st->state.num_textures = 0;

   /* loop over sampler units (aka tex image units) */
   for (su = 0; su < st->ctx->Const.MaxTextureImageUnits; su++) {
      struct pipe_sampler_view *sampler_view = NULL;

      if (samplersUsed & (1 << su)) {
         struct gl_texture_object *texObj;
         struct st_texture_object *stObj;
         GLboolean retval;
         GLuint texUnit;

         if (fprog->Base.SamplersUsed & (1 << su))
            texUnit = fprog->Base.SamplerUnits[su];
         else
            texUnit = vprog->Base.SamplerUnits[su];

         texObj = st->ctx->Texture.Unit[texUnit]._Current;

         if (!texObj) {
            texObj = st_get_default_texture(st);
         }
         stObj = st_texture_object(texObj);

         retval = st_finalize_texture(st->ctx, st->pipe, texObj);
         if (!retval) {
            /* out of mem */
            continue;
         }

         st->state.num_textures = su + 1;

	 /* if sampler view has changed dereference it */
	 if (stObj->sampler_view)
	    if (check_sampler_swizzle(stObj->sampler_view, stObj->base._Swizzle))
	       pipe_sampler_view_reference(&stObj->sampler_view, NULL);

         sampler_view = st_get_texture_sampler_view_from_stobj(stObj, pipe);
      }
      pipe_sampler_view_reference(&st->state.sampler_views[su], sampler_view);
   }

   cso_set_fragment_sampler_views(st->cso_context,
                                  st->state.num_textures,
                                  st->state.sampler_views);
   if (st->ctx->Const.MaxVertexTextureImageUnits > 0) {
      cso_set_vertex_sampler_views(st->cso_context,
                                   MIN2(st->state.num_textures,
                                        st->ctx->Const.MaxVertexTextureImageUnits),
                                   st->state.sampler_views);
   }
}


const struct st_tracked_state st_update_texture = {
   "st_update_texture",					/* name */
   {							/* dirty */
      _NEW_TEXTURE,					/* mesa */
      ST_NEW_FRAGMENT_PROGRAM,				/* st */
   },
   update_textures					/* update */
};




static void 
finalize_textures(struct st_context *st)
{
   struct gl_fragment_program *fprog = st->ctx->FragmentProgram._Current;
   const GLboolean prev_missing_textures = st->missing_textures;
   GLuint su;

   st->missing_textures = GL_FALSE;

   for (su = 0; su < st->ctx->Const.MaxTextureCoordUnits; su++) {
      if (fprog->Base.SamplersUsed & (1 << su)) {
         const GLuint texUnit = fprog->Base.SamplerUnits[su];
         struct gl_texture_object *texObj
            = st->ctx->Texture.Unit[texUnit]._Current;

         if (texObj) {
            GLboolean retval;

            retval = st_finalize_texture(st->ctx, st->pipe, texObj);
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
