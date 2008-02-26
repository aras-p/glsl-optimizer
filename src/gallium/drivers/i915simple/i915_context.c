/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "i915_context.h"
#include "i915_winsys.h"
#include "i915_state.h"
#include "i915_batch.h"
#include "i915_texture.h"
#include "i915_reg.h"

#include "draw/draw_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "pipe/p_util.h"


/**
 * Query format support for creating a texture, drawing surface, etc.
 * \param format  the format to test
 * \param type  one of PIPE_TEXTURE, PIPE_SURFACE
 */
static boolean
i915_is_format_supported( struct pipe_context *pipe,
                          enum pipe_format format, uint type )
{
   static const enum pipe_format tex_supported[] = {
      PIPE_FORMAT_R8G8B8A8_UNORM,
      PIPE_FORMAT_A8R8G8B8_UNORM,
      PIPE_FORMAT_R5G6B5_UNORM,
      PIPE_FORMAT_U_L8,
      PIPE_FORMAT_U_A8,
      PIPE_FORMAT_U_I8,
      PIPE_FORMAT_U_A8_L8,
      PIPE_FORMAT_YCBCR,
      PIPE_FORMAT_YCBCR_REV,
      PIPE_FORMAT_S8Z24_UNORM,
      PIPE_FORMAT_NONE  /* list terminator */
   };
   static const enum pipe_format surface_supported[] = {
      PIPE_FORMAT_A8R8G8B8_UNORM,
      PIPE_FORMAT_R5G6B5_UNORM,
      PIPE_FORMAT_S8Z24_UNORM,
      /*PIPE_FORMAT_R16G16B16A16_SNORM,*/
      PIPE_FORMAT_NONE  /* list terminator */
   };
   const enum pipe_format *list;
   uint i;

   switch (type) {
   case PIPE_TEXTURE:
      list = tex_supported;
      break;
   case PIPE_SURFACE:
      list = surface_supported;
      break;
   default:
      assert(0);
   }

   for (i = 0; list[i] != PIPE_FORMAT_NONE; i++) {
      if (list[i] == format)
         return TRUE;
   }

   return FALSE;
}


static int
i915_get_param(struct pipe_context *pipe, int param)
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
i915_get_paramf(struct pipe_context *pipe, int param)
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


static void i915_destroy( struct pipe_context *pipe )
{
   struct i915_context *i915 = i915_context( pipe );

   draw_destroy( i915->draw );

   FREE( i915 );
}




static boolean
i915_draw_elements( struct pipe_context *pipe,
                    struct pipe_buffer *indexBuffer,
                    unsigned indexSize,
                    unsigned prim, unsigned start, unsigned count)
{
   struct i915_context *i915 = i915_context( pipe );
   struct draw_context *draw = i915->draw;
   unsigned i;

   if (i915->dirty)
      i915_update_derived( i915 );

   /*
    * Map vertex buffers
    */
   for (i = 0; i < PIPE_ATTRIB_MAX; i++) {
      if (i915->vertex_buffer[i].buffer) {
         void *buf
            = pipe->winsys->buffer_map(pipe->winsys,
                                       i915->vertex_buffer[i].buffer,
                                       PIPE_BUFFER_USAGE_CPU_READ);
         draw_set_mapped_vertex_buffer(draw, i, buf);
      }
   }
   /* Map index buffer, if present */
   if (indexBuffer) {
      void *mapped_indexes
         = pipe->winsys->buffer_map(pipe->winsys, indexBuffer,
                                    PIPE_BUFFER_USAGE_CPU_READ);
      draw_set_mapped_element_buffer(draw, indexSize, mapped_indexes);
   }
   else {
      /* no index/element buffer */
      draw_set_mapped_element_buffer(draw, 0, NULL);
   }


   draw_set_mapped_constant_buffer(draw,
                                i915->current.constants[PIPE_SHADER_VERTEX]);

   /* draw! */
   draw_arrays(i915->draw, prim, start, count);

   /*
    * unmap vertex/index buffers
    */
   for (i = 0; i < PIPE_ATTRIB_MAX; i++) {
      if (i915->vertex_buffer[i].buffer) {
         pipe->winsys->buffer_unmap(pipe->winsys, i915->vertex_buffer[i].buffer);
         draw_set_mapped_vertex_buffer(draw, i, NULL);
      }
   }
   if (indexBuffer) {
      pipe->winsys->buffer_unmap(pipe->winsys, indexBuffer);
      draw_set_mapped_element_buffer(draw, 0, NULL);
   }

   return TRUE;
}


static boolean i915_draw_arrays( struct pipe_context *pipe,
				 unsigned prim, unsigned start, unsigned count)
{
   return i915_draw_elements(pipe, NULL, 0, prim, start, count);
}



struct pipe_context *i915_create( struct pipe_winsys *pipe_winsys,
				  struct i915_winsys *i915_winsys,
				  unsigned pci_id )
{
   struct i915_context *i915;
   unsigned is_i945 = 0;

   switch (pci_id) {
   case PCI_CHIP_I915_G:
   case PCI_CHIP_I915_GM:
      break;

   case PCI_CHIP_I945_G:
   case PCI_CHIP_I945_GM:
   case PCI_CHIP_I945_GME:
   case PCI_CHIP_G33_G:
   case PCI_CHIP_Q33_G:
   case PCI_CHIP_Q35_G:
      is_i945 = 1;
      break;

   default:
      pipe_winsys->printf(pipe_winsys, 
			  "%s: unknown pci id 0x%x, cannot create context\n", 
			  __FUNCTION__, pci_id);
      return NULL;
   }

   i915 = CALLOC_STRUCT(i915_context);
   if (i915 == NULL)
      return NULL;

   i915->winsys = i915_winsys;
   i915->pipe.winsys = pipe_winsys;

   i915->pipe.destroy = i915_destroy;
   i915->pipe.is_format_supported = i915_is_format_supported;
   i915->pipe.get_param = i915_get_param;
   i915->pipe.get_paramf = i915_get_paramf;

   i915->pipe.clear = i915_clear;


   i915->pipe.draw_arrays = i915_draw_arrays;
   i915->pipe.draw_elements = i915_draw_elements;

   /*
    * Create drawing context and plug our rendering stage into it.
    */
   i915->draw = draw_create();
   assert(i915->draw);
   if (GETENV("I915_VBUF")) {
      draw_set_rasterize_stage(i915->draw, i915_draw_vbuf_stage(i915));
   }
   else {
      draw_set_rasterize_stage(i915->draw, i915_draw_render_stage(i915));
   }

   i915_init_surface_functions(i915);
   i915_init_state_functions(i915);
   i915_init_flush_functions(i915);
   i915_init_string_functions(i915);
   i915_init_texture_functions(i915);

   draw_install_aaline_stage(i915->draw, &i915->pipe);
   draw_install_aapoint_stage(i915->draw, &i915->pipe);

   i915->pci_id = pci_id;
   i915->flags.is_i945 = is_i945;

   i915->dirty = ~0;
   i915->hardware_dirty = ~0;

   /* Batch stream debugging is a bit hacked up at the moment:
    */
   i915->batch_start = NULL;

   /*
    * XXX we could plug GL selection/feedback into the drawing pipeline
    * by specifying a different setup/render stage.
    */

   return &i915->pipe;
}

