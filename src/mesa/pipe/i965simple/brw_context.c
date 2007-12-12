/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.

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

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */


#include "brw_context.h"
#include "brw_defines.h"
#include "brw_draw.h"
#include "brw_vs.h"
#include "brw_tex_layout.h"
#include "brw_winsys.h"

#include "pipe/p_winsys.h"
#include "pipe/p_context.h"
#include "pipe/p_util.h"

/***************************************
 * Mesa's Driver Functions
 ***************************************/

#ifndef BRW_DEBUG
int BRW_DEBUG = (0);
#endif

static void brw_destroy(struct pipe_context *pipe)
{
   struct brw_context *brw = brw_context(pipe);

   FREE(brw);
}

static void brw_clear(struct pipe_context *pipe, struct pipe_surface *ps,
                      unsigned clearValue)
{
   int x, y, w, h;
   /* FIXME: corny... */

   x = 0;
   y = 0;
   w = ps->width;
   h = ps->height;

   pipe->surface_fill(pipe, ps, x, y, w, h, clearValue);
}


static int
brw_get_param(struct pipe_context *pipe, int param)
{
   switch (param) {
   case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
      return 8;
   case PIPE_CAP_NPOT_TEXTURES:
      return 1;
   case PIPE_CAP_TWO_SIDED_STENCIL:
      return 1;
   case PIPE_CAP_GLSL:
      return 0;
   case PIPE_CAP_S3TC:
      return 0;
   case PIPE_CAP_ANISOTROPIC_FILTER:
      return 0;
   case PIPE_CAP_POINT_SPRITE:
      return 0;
   case PIPE_CAP_MAX_RENDER_TARGETS:
      return 1;
   case PIPE_CAP_OCCLUSION_QUERY:
      return 0;
   case PIPE_CAP_TEXTURE_SHADOW_MAP:
      return 1;
   case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
      return 11; /* max 1024x1024 */
   case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      return 8;  /* max 128x128x128 */
   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
      return 11; /* max 1024x1024 */
   default:
      return 0;
   }
}


static float
brw_get_paramf(struct pipe_context *pipe, int param)
{
   switch (param) {
   case PIPE_CAP_MAX_LINE_WIDTH:
      /* fall-through */
   case PIPE_CAP_MAX_LINE_WIDTH_AA:
      return 7.5;

   case PIPE_CAP_MAX_POINT_WIDTH:
      /* fall-through */
   case PIPE_CAP_MAX_POINT_WIDTH_AA:
      return 255.0;

   case PIPE_CAP_MAX_TEXTURE_ANISOTROPY:
      return 4.0;

   case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
      return 16.0;

   default:
      return 0;
   }
}

static boolean
brw_is_format_supported( struct pipe_context *pipe,
                          enum pipe_format format, uint type )
{
#if 0
   /* XXX: This is broken -- rewrite if still needed. */
   static const unsigned tex_supported[] = {
      PIPE_FORMAT_U_R8_G8_B8_A8,
      PIPE_FORMAT_U_A8_R8_G8_B8,
      PIPE_FORMAT_U_R5_G6_B5,
      PIPE_FORMAT_U_L8,
      PIPE_FORMAT_U_A8,
      PIPE_FORMAT_U_I8,
      PIPE_FORMAT_U_L8_A8,
      PIPE_FORMAT_YCBCR,
      PIPE_FORMAT_YCBCR_REV,
      PIPE_FORMAT_S8_Z24,
   };


   /* Actually a lot more than this - add later:
    */
   static const unsigned render_supported[] = {
      PIPE_FORMAT_U_A8_R8_G8_B8,
      PIPE_FORMAT_U_R5_G6_B5,
   };

   /*
    */
   static const unsigned z_stencil_supported[] = {
      PIPE_FORMAT_U_Z16,
      PIPE_FORMAT_U_Z32,
      PIPE_FORMAT_S8_Z24,
   };

   switch (type) {
   case PIPE_RENDER_FORMAT:
      *numFormats = Elements(render_supported);
      return render_supported;

   case PIPE_TEX_FORMAT:
      *numFormats = Elements(tex_supported);
      return render_supported;

   case PIPE_Z_STENCIL_FORMAT:
      *numFormats = Elements(render_supported);
      return render_supported;

   default:
      *numFormats = 0;
      return NULL;
   }
#else
   switch (format) {
   case PIPE_FORMAT_A8R8G8B8_UNORM:
   case PIPE_FORMAT_R5G6B5_UNORM:
   case PIPE_FORMAT_S8Z24_UNORM:
      return TRUE;
   default:
      return FALSE;
   };
   return FALSE;
#endif
}




struct pipe_context *brw_create(struct pipe_winsys *pipe_winsys,
                                struct brw_winsys *brw_winsys,
                                unsigned pci_id)
{
   struct brw_context *brw;

   pipe_winsys->printf(pipe_winsys,
                       "%s: creating brw_context with pci id 0x%x\n",
                       __FUNCTION__, pci_id);

   brw = CALLOC_STRUCT(brw_context);
   if (brw == NULL)
      return NULL;

   brw->winsys = brw_winsys;
   brw->pipe.winsys = pipe_winsys;

   brw->pipe.destroy = brw_destroy;
   brw->pipe.is_format_supported = brw_is_format_supported;
   brw->pipe.get_param = brw_get_param;
   brw->pipe.get_paramf = brw_get_paramf;
   brw->pipe.clear = brw_clear;
   brw->pipe.texture_create  = brw_texture_create;
   brw->pipe.texture_release = brw_texture_release;

   brw_init_surface_functions(brw);
   brw_init_state_functions(brw);
   brw_init_flush_functions(brw);
   brw_init_string_functions(brw);
   brw_init_draw_functions( brw );


   brw_init_state( brw );

   brw->pci_id = pci_id;
   brw->dirty = ~0;
   brw->hardware_dirty = ~0;
   brw->batch_start = NULL;

   memset(&brw->wm.bind, ~0, sizeof(brw->wm.bind));

   return &brw->pipe;
}

