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

//#include "main/imports.h"	/* CALLOC */
#include "i915_context.h"
#include "i915_winsys.h"
#include "i915_state.h"
#include "i915_batch.h"
#include "i915_tex_layout.h"
#include "i915_reg.h"

#include "pipe/draw/draw_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "pipe/p_util.h"


/**
 * Return list of supported surface/texture formats.
 * If we find texture and drawable support differs, add a selector
 * parameter or another function.
 */
static const unsigned *
i915_supported_formats(struct pipe_context *pipe, 
//			   unsigned type,
			   unsigned *numFormats)
{
#if 0
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
   static const unsigned render_supported[] = {
      PIPE_FORMAT_U_A8_R8_G8_B8,
      PIPE_FORMAT_U_R5_G6_B5,
      PIPE_FORMAT_S8_Z24,
   };
   *numFormats = 3;
   return render_supported;
#endif
}


/**
 * We might want to return max texture levels instead...
 */
static void
i915_max_texture_size(struct pipe_context *pipe, unsigned textureType,
                      unsigned *maxWidth, unsigned *maxHeight, unsigned *maxDepth)
{
   switch (textureType) {
   case PIPE_TEXTURE_1D:
      *maxWidth = 2048;
      break; 
   case PIPE_TEXTURE_2D:
      *maxWidth =
      *maxHeight = 2048;
      break; 
   case PIPE_TEXTURE_3D:
      *maxWidth =
      *maxHeight =
      *maxDepth = 256;
      break; 
   case PIPE_TEXTURE_CUBE:
      *maxWidth =
      *maxHeight = 2048;
      break; 
   default:
      assert(0);
   }
}


static void i915_destroy( struct pipe_context *pipe )
{
   struct i915_context *i915 = i915_context( pipe );

   draw_destroy( i915->draw );

   free( i915 );
}

static void i915_draw_vb( struct pipe_context *pipe,
			     struct vertex_buffer *VB )
{
   struct i915_context *i915 = i915_context( pipe );

   if (i915->dirty)
      i915_update_derived( i915 );

   draw_vb( i915->draw, VB );
}


static void
i915_draw_vertices(struct pipe_context *pipe,
                       unsigned mode,
                       unsigned numVertex, const float *verts,
                       unsigned numAttribs, const unsigned attribs[])
{
   struct i915_context *i915 = i915_context( pipe );

   if (i915->dirty)
      i915_update_derived( i915 );

   draw_vertices(i915->draw, mode, numVertex, verts, numAttribs, attribs);
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
   i915->pipe.supported_formats = i915_supported_formats;
   i915->pipe.max_texture_size = i915_max_texture_size;
   i915->pipe.draw_vb = i915_draw_vb;
   i915->pipe.draw_vertices = i915_draw_vertices;
   i915->pipe.clear = i915_clear;
   i915->pipe.reset_occlusion_counter = NULL; /* no support */
   i915->pipe.get_occlusion_counter = NULL;


   /*
    * Create drawing context and plug our rendering stage into it.
    */
   i915->draw = draw_create();
   assert(i915->draw);
   draw_set_setup_stage(i915->draw, i915_draw_render_stage(i915));

   i915_init_region_functions(i915);
   i915_init_surface_functions(i915);
   i915_init_state_functions(i915);
   i915_init_flush_functions(i915);
   i915_init_string_functions(i915);

   i915->pci_id = pci_id;
   i915->flags.is_i945 = is_i945;

   if (i915->flags.is_i945)
      i915->pipe.mipmap_tree_layout = i945_miptree_layout;
   else
      i915->pipe.mipmap_tree_layout = i945_miptree_layout;


   i915->dirty = ~0;
   i915->hardware_dirty = ~0;

   /* Batch stream debugging is a bit hacked up at the moment:
    */
   i915->batch_start = BEGIN_BATCH(0, 0);

   /*
    * XXX we could plug GL selection/feedback into the drawing pipeline
    * by specifying a different setup/render stage.
    */

   return &i915->pipe;
}

