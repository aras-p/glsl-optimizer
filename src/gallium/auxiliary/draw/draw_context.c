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
  */


#include "pipe/p_context.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_cpu_detect.h"
#include "util/u_inlines.h"
#include "util/u_helpers.h"
#include "util/u_prim.h"
#include "util/u_format.h"
#include "draw_context.h"
#include "draw_pipe.h"
#include "draw_prim_assembler.h"
#include "draw_vs.h"
#include "draw_gs.h"

#if HAVE_LLVM
#include "gallivm/lp_bld_init.h"
#include "gallivm/lp_bld_limits.h"
#include "draw_llvm.h"

boolean
draw_get_option_use_llvm(void)
{
   static boolean first = TRUE;
   static boolean value;
   if (first) {
      first = FALSE;
      value = debug_get_bool_option("DRAW_USE_LLVM", TRUE);

#ifdef PIPE_ARCH_X86
      util_cpu_detect();
      /* require SSE2 due to LLVM PR6960. XXX Might be fixed by now? */
      if (!util_cpu_caps.has_sse2)
         value = FALSE;
#endif
   }
   return value;
}
#else
boolean
draw_get_option_use_llvm(void)
{
   return FALSE;
}
#endif


/**
 * Create new draw module context with gallivm state for LLVM JIT.
 */
static struct draw_context *
draw_create_context(struct pipe_context *pipe, void *context,
                    boolean try_llvm)
{
   struct draw_context *draw = CALLOC_STRUCT( draw_context );
   if (draw == NULL)
      goto err_out;

   /* we need correct cpu caps for disabling denorms in draw_vbo() */
   util_cpu_detect();

#if HAVE_LLVM
   if (try_llvm && draw_get_option_use_llvm()) {
      draw->llvm = draw_llvm_create(draw, (LLVMContextRef)context);
   }
#endif

   draw->pipe = pipe;

   if (!draw_init(draw))
      goto err_destroy;

   draw->ia = draw_prim_assembler_create(draw);
   if (!draw->ia)
      goto err_destroy;

   return draw;

err_destroy:
   draw_destroy( draw );
err_out:
   return NULL;
}


/**
 * Create new draw module context, with LLVM JIT.
 */
struct draw_context *
draw_create(struct pipe_context *pipe)
{
   return draw_create_context(pipe, NULL, TRUE);
}


#if HAVE_LLVM
struct draw_context *
draw_create_with_llvm_context(struct pipe_context *pipe,
                              void *context)
{
   return draw_create_context(pipe, context, TRUE);
}
#endif

/**
 * Create a new draw context, without LLVM JIT.
 */
struct draw_context *
draw_create_no_llvm(struct pipe_context *pipe)
{
   return draw_create_context(pipe, NULL, FALSE);
}


boolean draw_init(struct draw_context *draw)
{
   /*
    * Note that several functions compute the clipmask of the predefined
    * formats with hardcoded formulas instead of using these. So modifications
    * here must be reflected there too.
    */

   ASSIGN_4V( draw->plane[0], -1,  0,  0, 1 );
   ASSIGN_4V( draw->plane[1],  1,  0,  0, 1 );
   ASSIGN_4V( draw->plane[2],  0, -1,  0, 1 );
   ASSIGN_4V( draw->plane[3],  0,  1,  0, 1 );
   ASSIGN_4V( draw->plane[4],  0,  0,  1, 1 ); /* yes these are correct */
   ASSIGN_4V( draw->plane[5],  0,  0, -1, 1 ); /* mesa's a bit wonky */
   draw->clip_xy = TRUE;
   draw->clip_z = TRUE;

   draw->pt.user.planes = (float (*) [DRAW_TOTAL_CLIP_PLANES][4]) &(draw->plane[0]);
   draw->pt.user.eltMax = ~0;

   if (!draw_pipeline_init( draw ))
      return FALSE;

   if (!draw_pt_init( draw ))
      return FALSE;

   if (!draw_vs_init( draw ))
      return FALSE;

   if (!draw_gs_init( draw ))
      return FALSE;

   draw->quads_always_flatshade_last = !draw->pipe->screen->get_param(
      draw->pipe->screen, PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION);

   draw->floating_point_depth = false;

   return TRUE;
}

/*
 * Called whenever we're starting to draw a new instance.
 * Some internal structures don't want to have to reset internal
 * members on each invocation (because their state might have to persist
 * between multiple primitive restart rendering call) but might have to 
 * for each new instance. 
 * This is particularly the case for primitive id's in geometry shader.
 */
void draw_new_instance(struct draw_context *draw)
{
   draw_geometry_shader_new_instance(draw->gs.geometry_shader);
}


void draw_destroy( struct draw_context *draw )
{
   struct pipe_context *pipe;
   unsigned i, j;

   if (!draw)
      return;

   pipe = draw->pipe;

   /* free any rasterizer CSOs that we may have created.
    */
   for (i = 0; i < 2; i++) {
      for (j = 0; j < 2; j++) {
         if (draw->rasterizer_no_cull[i][j]) {
            pipe->delete_rasterizer_state(pipe, draw->rasterizer_no_cull[i][j]);
         }
      }
   }

   for (i = 0; i < draw->pt.nr_vertex_buffers; i++) {
      pipe_resource_reference(&draw->pt.vertex_buffer[i].buffer, NULL);
   }

   /* Not so fast -- we're just borrowing this at the moment.
    * 
   if (draw->render)
      draw->render->destroy( draw->render );
   */

   draw_prim_assembler_destroy(draw->ia);
   draw_pipeline_destroy( draw );
   draw_pt_destroy( draw );
   draw_vs_destroy( draw );
   draw_gs_destroy( draw );
#ifdef HAVE_LLVM
   if (draw->llvm)
      draw_llvm_destroy( draw->llvm );
#endif

   FREE( draw );
}



void draw_flush( struct draw_context *draw )
{
   draw_do_flush( draw, DRAW_FLUSH_BACKEND );
}


/**
 * Specify the depth stencil format for the draw pipeline. This function
 * determines the Minimum Resolvable Depth factor for polygon offset.
 * This factor potentially depends on the number of Z buffer bits,
 * the rasterization algorithm and the arithmetic performed on Z
 * values between vertex shading and rasterization.
 */
void draw_set_zs_format(struct draw_context *draw, enum pipe_format format)
{
   const struct util_format_description *desc = util_format_description(format);

   draw->floating_point_depth =
      (util_get_depth_format_type(desc) == UTIL_FORMAT_TYPE_FLOAT);

   draw->mrd = util_get_depth_format_mrd(desc);
}


static void update_clip_flags( struct draw_context *draw )
{
   draw->clip_xy = !draw->driver.bypass_clip_xy;
   draw->guard_band_xy = (!draw->driver.bypass_clip_xy &&
                          draw->driver.guard_band_xy);
   draw->clip_z = (!draw->driver.bypass_clip_z &&
                   draw->rasterizer && draw->rasterizer->depth_clip);
   draw->clip_user = draw->rasterizer &&
                     draw->rasterizer->clip_plane_enable != 0;
   draw->guard_band_points_xy = draw->guard_band_xy ||
                                (draw->driver.bypass_clip_points &&
                                (draw->rasterizer &&
                                 draw->rasterizer->point_tri_clip));
}

/**
 * Register new primitive rasterization/rendering state.
 * This causes the drawing pipeline to be rebuilt.
 */
void draw_set_rasterizer_state( struct draw_context *draw,
                                const struct pipe_rasterizer_state *raster,
                                void *rast_handle )
{
   if (!draw->suspend_flushing) {
      draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );

      draw->rasterizer = raster;
      draw->rast_handle = rast_handle;
      update_clip_flags(draw);
   }
}

/* With a little more work, llvmpipe will be able to turn this off and
 * do its own x/y clipping.  
 *
 * Some hardware can turn off clipping altogether - in particular any
 * hardware with a TNL unit can do its own clipping, even if it is
 * relying on the draw module for some other reason.
 * Setting bypass_clip_points to achieve d3d-style point clipping (the driver
 * will need to do the "vp scissoring") _requires_ the driver to implement
 * wide points / point sprites itself (points will still be clipped if rasterizer
 * point_tri_clip isn't set). Only relevant if bypass_clip_xy isn't set.
 */
void draw_set_driver_clipping( struct draw_context *draw,
                               boolean bypass_clip_xy,
                               boolean bypass_clip_z,
                               boolean guard_band_xy,
                               boolean bypass_clip_points)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );

   draw->driver.bypass_clip_xy = bypass_clip_xy;
   draw->driver.bypass_clip_z = bypass_clip_z;
   draw->driver.guard_band_xy = guard_band_xy;
   draw->driver.bypass_clip_points = bypass_clip_points;
   update_clip_flags(draw);
}


/** 
 * Plug in the primitive rendering/rasterization stage (which is the last
 * stage in the drawing pipeline).
 * This is provided by the device driver.
 */
void draw_set_rasterize_stage( struct draw_context *draw,
                               struct draw_stage *stage )
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );

   draw->pipeline.rasterize = stage;
}


/**
 * Set the draw module's clipping state.
 */
void draw_set_clip_state( struct draw_context *draw,
                          const struct pipe_clip_state *clip )
{
   draw_do_flush(draw, DRAW_FLUSH_PARAMETER_CHANGE);

   memcpy(&draw->plane[6], clip->ucp, sizeof(clip->ucp));
}


/**
 * Set the draw module's viewport state.
 */
void draw_set_viewport_states( struct draw_context *draw,
                               unsigned start_slot,
                               unsigned num_viewports,
                               const struct pipe_viewport_state *vps )
{
   const struct pipe_viewport_state *viewport = vps;
   draw_do_flush(draw, DRAW_FLUSH_PARAMETER_CHANGE);

   debug_assert(start_slot < PIPE_MAX_VIEWPORTS);
   debug_assert((start_slot + num_viewports) <= PIPE_MAX_VIEWPORTS);

   memcpy(draw->viewports + start_slot, vps,
          sizeof(struct pipe_viewport_state) * num_viewports);

   draw->identity_viewport = (num_viewports == 1) &&
      (viewport->scale[0] == 1.0f &&
       viewport->scale[1] == 1.0f &&
       viewport->scale[2] == 1.0f &&
       viewport->scale[3] == 1.0f &&
       viewport->translate[0] == 0.0f &&
       viewport->translate[1] == 0.0f &&
       viewport->translate[2] == 0.0f &&
       viewport->translate[3] == 0.0f);
}



void
draw_set_vertex_buffers(struct draw_context *draw,
                        unsigned start_slot, unsigned count,
                        const struct pipe_vertex_buffer *buffers)
{
   assert(start_slot + count <= PIPE_MAX_ATTRIBS);

   util_set_vertex_buffers_count(draw->pt.vertex_buffer,
                                 &draw->pt.nr_vertex_buffers,
                                 buffers, start_slot, count);
}


void
draw_set_vertex_elements(struct draw_context *draw,
                         unsigned count,
                         const struct pipe_vertex_element *elements)
{
   assert(count <= PIPE_MAX_ATTRIBS);

   /* We could improve this by only flushing the frontend and the fetch part
    * of the middle. This would avoid recalculating the emit keys.*/
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );

   memcpy(draw->pt.vertex_element, elements, count * sizeof(elements[0]));
   draw->pt.nr_vertex_elements = count;
}


/**
 * Tell drawing context where to find mapped vertex buffers.
 */
void
draw_set_mapped_vertex_buffer(struct draw_context *draw,
                              unsigned attr, const void *buffer,
                              size_t size)
{
   draw->pt.user.vbuffer[attr].map  = buffer;
   draw->pt.user.vbuffer[attr].size = size;
}


void
draw_set_mapped_constant_buffer(struct draw_context *draw,
                                unsigned shader_type,
                                unsigned slot,
                                const void *buffer,
                                unsigned size )
{
   debug_assert(shader_type == PIPE_SHADER_VERTEX ||
                shader_type == PIPE_SHADER_GEOMETRY);
   debug_assert(slot < PIPE_MAX_CONSTANT_BUFFERS);

   draw_do_flush(draw, DRAW_FLUSH_PARAMETER_CHANGE);

   switch (shader_type) {
   case PIPE_SHADER_VERTEX:
      draw->pt.user.vs_constants[slot] = buffer;
      draw->pt.user.vs_constants_size[slot] = size;
      break;
   case PIPE_SHADER_GEOMETRY:
      draw->pt.user.gs_constants[slot] = buffer;
      draw->pt.user.gs_constants_size[slot] = size;
      break;
   default:
      assert(0 && "invalid shader type in draw_set_mapped_constant_buffer");
   }
}


/**
 * Tells the draw module to draw points with triangles if their size
 * is greater than this threshold.
 */
void
draw_wide_point_threshold(struct draw_context *draw, float threshold)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->pipeline.wide_point_threshold = threshold;
}


/**
 * Should the draw module handle point->quad conversion for drawing sprites?
 */
void
draw_wide_point_sprites(struct draw_context *draw, boolean draw_sprite)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->pipeline.wide_point_sprites = draw_sprite;
}


/**
 * Tells the draw module to draw lines with triangles if their width
 * is greater than this threshold.
 */
void
draw_wide_line_threshold(struct draw_context *draw, float threshold)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->pipeline.wide_line_threshold = roundf(threshold);
}


/**
 * Tells the draw module whether or not to implement line stipple.
 */
void
draw_enable_line_stipple(struct draw_context *draw, boolean enable)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->pipeline.line_stipple = enable;
}


/**
 * Tells draw module whether to convert points to quads for sprite mode.
 */
void
draw_enable_point_sprites(struct draw_context *draw, boolean enable)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->pipeline.point_sprite = enable;
}


void
draw_set_force_passthrough( struct draw_context *draw, boolean enable )
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->force_passthrough = enable;
}



/**
 * Allocate an extra vertex/geometry shader vertex attribute, if it doesn't
 * exist already.
 *
 * This is used by some of the optional draw module stages such
 * as wide_point which may need to allocate additional generic/texcoord
 * attributes.
 */
int
draw_alloc_extra_vertex_attrib(struct draw_context *draw,
                               uint semantic_name, uint semantic_index)
{
   int slot;
   uint num_outputs;
   uint n;

   slot = draw_find_shader_output(draw, semantic_name, semantic_index);
   if (slot >= 0) {
      return slot;
   }

   num_outputs = draw_current_shader_outputs(draw);
   n = draw->extra_shader_outputs.num;

   assert(n < Elements(draw->extra_shader_outputs.semantic_name));

   draw->extra_shader_outputs.semantic_name[n] = semantic_name;
   draw->extra_shader_outputs.semantic_index[n] = semantic_index;
   draw->extra_shader_outputs.slot[n] = num_outputs + n;
   draw->extra_shader_outputs.num++;

   return draw->extra_shader_outputs.slot[n];
}


/**
 * Remove all extra vertex attributes that were allocated with
 * draw_alloc_extra_vertex_attrib().
 */
void
draw_remove_extra_vertex_attribs(struct draw_context *draw)
{
   draw->extra_shader_outputs.num = 0;
}


/**
 * If a geometry shader is present, return its info, else the vertex shader's
 * info.
 */
struct tgsi_shader_info *
draw_get_shader_info(const struct draw_context *draw)
{

   if (draw->gs.geometry_shader) {
      return &draw->gs.geometry_shader->info;
   } else {
      return &draw->vs.vertex_shader->info;
   }
}

/**
 * Prepare outputs slots from the draw module
 *
 * Certain parts of the draw module can emit additional
 * outputs that can be quite useful to the backends, a good
 * example of it is the process of decomposing primitives
 * into wireframes (aka. lines) which normally would lose
 * the face-side information, but using this method we can
 * inject another shader output which passes the original
 * face side information to the backend.
 */
void
draw_prepare_shader_outputs(struct draw_context *draw)
{
   draw_remove_extra_vertex_attribs(draw);
   draw_prim_assembler_prepare_outputs(draw->ia);
   draw_unfilled_prepare_outputs(draw, draw->pipeline.unfilled);
   if (draw->pipeline.aapoint)
      draw_aapoint_prepare_outputs(draw, draw->pipeline.aapoint);
   if (draw->pipeline.aaline)
      draw_aaline_prepare_outputs(draw, draw->pipeline.aaline);
}

/**
 * Ask the draw module for the location/slot of the given vertex attribute in
 * a post-transformed vertex.
 *
 * With this function, drivers that use the draw module should have no reason
 * to track the current vertex/geometry shader.
 *
 * Note that the draw module may sometimes generate vertices with extra
 * attributes (such as texcoords for AA lines).  The driver can call this
 * function to find those attributes.
 *
 * -1 is returned if the attribute is not found since this is
 * an undefined situation. Note, that zero is valid and can
 * be used by any of the attributes, because position is not
 * required to be attribute 0 or even at all present.
 */
int
draw_find_shader_output(const struct draw_context *draw,
                        uint semantic_name, uint semantic_index)
{
   const struct tgsi_shader_info *info = draw_get_shader_info(draw);
   uint i;

   for (i = 0; i < info->num_outputs; i++) {
      if (info->output_semantic_name[i] == semantic_name &&
          info->output_semantic_index[i] == semantic_index)
         return i;
   }

   /* Search the extra vertex attributes */
   for (i = 0; i < draw->extra_shader_outputs.num; i++) {
      if (draw->extra_shader_outputs.semantic_name[i] == semantic_name &&
          draw->extra_shader_outputs.semantic_index[i] == semantic_index) {
         return draw->extra_shader_outputs.slot[i];
      }
   }

   return -1;
}


/**
 * Return total number of the shader outputs.  This function is similar to
 * draw_current_shader_outputs() but this function also counts any extra
 * vertex/geometry output attributes that may be filled in by some draw
 * stages (such as AA point, AA line).
 *
 * If geometry shader is present, its output will be returned,
 * if not vertex shader is used.
 */
uint
draw_num_shader_outputs(const struct draw_context *draw)
{
   const struct tgsi_shader_info *info = draw_get_shader_info(draw);
   uint count;

   count = info->num_outputs;
   count += draw->extra_shader_outputs.num;

   return count;
}


/**
 * Return total number of the vertex shader outputs.  This function
 * also counts any extra vertex output attributes that may
 * be filled in by some draw stages (such as AA point, AA line,
 * front face).
 */
uint
draw_total_vs_outputs(const struct draw_context *draw)
{
   const struct tgsi_shader_info *info = &draw->vs.vertex_shader->info;

   return info->num_outputs + draw->extra_shader_outputs.num;;
}

/**
 * Return total number of the geometry shader outputs. This function
 * also counts any extra geometry output attributes that may
 * be filled in by some draw stages (such as AA point, AA line, front
 * face).
 */
uint
draw_total_gs_outputs(const struct draw_context *draw)
{   
   const struct tgsi_shader_info *info;

   if (!draw->gs.geometry_shader)
      return 0;

   info = &draw->gs.geometry_shader->info;

   return info->num_outputs + draw->extra_shader_outputs.num;
}


/**
 * Provide TGSI sampler objects for vertex/geometry shaders that use
 * texture fetches.  This state only needs to be set once per context.
 * This might only be used by software drivers for the time being.
 */
void
draw_texture_sampler(struct draw_context *draw,
                     uint shader,
                     struct tgsi_sampler *sampler)
{
   if (shader == PIPE_SHADER_VERTEX) {
      draw->vs.tgsi.sampler = sampler;
   } else {
      debug_assert(shader == PIPE_SHADER_GEOMETRY);
      draw->gs.tgsi.sampler = sampler;
   }
}




void draw_set_render( struct draw_context *draw, 
		      struct vbuf_render *render )
{
   draw->render = render;
}


/**
 * Tell the draw module where vertex indexes/elements are located, and
 * their size (in bytes).
 *
 * Note: the caller must apply the pipe_index_buffer::offset value to
 * the address.  The draw module doesn't do that.
 */
void
draw_set_indexes(struct draw_context *draw,
                 const void *elements, unsigned elem_size,
                 unsigned elem_buffer_space)
{
   assert(elem_size == 0 ||
          elem_size == 1 ||
          elem_size == 2 ||
          elem_size == 4);
   draw->pt.user.elts = elements;
   draw->pt.user.eltSizeIB = elem_size;
   if (elem_size)
      draw->pt.user.eltMax = elem_buffer_space / elem_size;
   else
      draw->pt.user.eltMax = 0;
}


/* Revamp me please:
 */
void draw_do_flush( struct draw_context *draw, unsigned flags )
{
   if (!draw->suspend_flushing)
   {
      assert(!draw->flushing); /* catch inadvertant recursion */

      draw->flushing = TRUE;

      draw_pipeline_flush( draw, flags );

      draw_pt_flush( draw, flags );

      draw->flushing = FALSE;
   }
}


/**
 * Return the number of output attributes produced by the geometry
 * shader, if present.  If no geometry shader, return the number of
 * outputs from the vertex shader.
 * \sa draw_num_shader_outputs
 */
uint
draw_current_shader_outputs(const struct draw_context *draw)
{
   if (draw->gs.geometry_shader)
      return draw->gs.num_gs_outputs;
   return draw->vs.num_vs_outputs;
}


/**
 * Return the index of the shader output which will contain the
 * vertex position.
 */
uint
draw_current_shader_position_output(const struct draw_context *draw)
{
   if (draw->gs.geometry_shader)
      return draw->gs.position_output;
   return draw->vs.position_output;
}


/**
 * Return the index of the shader output which will contain the
 * viewport index.
 */
uint
draw_current_shader_viewport_index_output(const struct draw_context *draw)
{
   if (draw->gs.geometry_shader)
      return draw->gs.geometry_shader->viewport_index_output;
   return 0;
}

/**
 * Returns true if there's a geometry shader bound and the geometry
 * shader writes out a viewport index.
 */
boolean
draw_current_shader_uses_viewport_index(const struct draw_context *draw)
{
   if (draw->gs.geometry_shader)
      return draw->gs.geometry_shader->info.writes_viewport_index;
   return FALSE;
}


/**
 * Return the index of the shader output which will contain the
 * clip vertex position.
 * Note we don't support clipvertex output in the gs. For clipping
 * to work correctly hence we return ordinary position output instead.
 */
uint
draw_current_shader_clipvertex_output(const struct draw_context *draw)
{
   if (draw->gs.geometry_shader)
      return draw->gs.position_output;
   return draw->vs.clipvertex_output;
}

uint
draw_current_shader_clipdistance_output(const struct draw_context *draw, int index)
{
   debug_assert(index < PIPE_MAX_CLIP_OR_CULL_DISTANCE_ELEMENT_COUNT);
   if (draw->gs.geometry_shader)
      return draw->gs.geometry_shader->clipdistance_output[index];
   return draw->vs.clipdistance_output[index];
}


uint
draw_current_shader_num_written_clipdistances(const struct draw_context *draw)
{
   if (draw->gs.geometry_shader)
      return draw->gs.geometry_shader->info.num_written_clipdistance;
   return draw->vs.vertex_shader->info.num_written_clipdistance;
}


uint
draw_current_shader_culldistance_output(const struct draw_context *draw, int index)
{
   debug_assert(index < PIPE_MAX_CLIP_OR_CULL_DISTANCE_ELEMENT_COUNT);
   if (draw->gs.geometry_shader)
      return draw->gs.geometry_shader->culldistance_output[index];
   return draw->vs.vertex_shader->culldistance_output[index];
}

uint
draw_current_shader_num_written_culldistances(const struct draw_context *draw)
{
   if (draw->gs.geometry_shader)
      return draw->gs.geometry_shader->info.num_written_culldistance;
   return draw->vs.vertex_shader->info.num_written_culldistance;
}

/**
 * Return a pointer/handle for a driver/CSO rasterizer object which
 * disabled culling, stippling, unfilled tris, etc.
 * This is used by some pipeline stages (such as wide_point, aa_line
 * and aa_point) which convert points/lines into triangles.  In those
 * cases we don't want to accidentally cull the triangles.
 *
 * \param scissor  should the rasterizer state enable scissoring?
 * \param flatshade  should the rasterizer state use flat shading?
 * \return  rasterizer CSO handle
 */
void *
draw_get_rasterizer_no_cull( struct draw_context *draw,
                             boolean scissor,
                             boolean flatshade )
{
   if (!draw->rasterizer_no_cull[scissor][flatshade]) {
      /* create now */
      struct pipe_context *pipe = draw->pipe;
      struct pipe_rasterizer_state rast;

      memset(&rast, 0, sizeof(rast));
      rast.scissor = scissor;
      rast.flatshade = flatshade;
      rast.front_ccw = 1;
      rast.half_pixel_center = draw->rasterizer->half_pixel_center;
      rast.bottom_edge_rule = draw->rasterizer->bottom_edge_rule;
      rast.clip_halfz = draw->rasterizer->clip_halfz;

      draw->rasterizer_no_cull[scissor][flatshade] =
         pipe->create_rasterizer_state(pipe, &rast);
   }
   return draw->rasterizer_no_cull[scissor][flatshade];
}

void
draw_set_mapped_so_targets(struct draw_context *draw,
                           int num_targets,
                           struct draw_so_target *targets[PIPE_MAX_SO_BUFFERS])
{
   int i;

   for (i = 0; i < num_targets; i++)
      draw->so.targets[i] = targets[i];
   for (i = num_targets; i < PIPE_MAX_SO_BUFFERS; i++)
      draw->so.targets[i] = NULL;

   draw->so.num_targets = num_targets;
}

void
draw_set_sampler_views(struct draw_context *draw,
                       unsigned shader_stage,
                       struct pipe_sampler_view **views,
                       unsigned num)
{
   unsigned i;

   debug_assert(shader_stage < PIPE_SHADER_TYPES);
   debug_assert(num <= PIPE_MAX_SHADER_SAMPLER_VIEWS);

   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );

   for (i = 0; i < num; ++i)
      draw->sampler_views[shader_stage][i] = views[i];
   for (i = num; i < PIPE_MAX_SHADER_SAMPLER_VIEWS; ++i)
      draw->sampler_views[shader_stage][i] = NULL;

   draw->num_sampler_views[shader_stage] = num;
}

void
draw_set_samplers(struct draw_context *draw,
                  unsigned shader_stage,
                  struct pipe_sampler_state **samplers,
                  unsigned num)
{
   unsigned i;

   debug_assert(shader_stage < PIPE_SHADER_TYPES);
   debug_assert(num <= PIPE_MAX_SAMPLERS);

   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );

   for (i = 0; i < num; ++i)
      draw->samplers[shader_stage][i] = samplers[i];
   for (i = num; i < PIPE_MAX_SAMPLERS; ++i)
      draw->samplers[shader_stage][i] = NULL;

   draw->num_samplers[shader_stage] = num;

#ifdef HAVE_LLVM
   if (draw->llvm)
      draw_llvm_set_sampler_state(draw, shader_stage);
#endif
}

void
draw_set_mapped_texture(struct draw_context *draw,
                        unsigned shader_stage,
                        unsigned sview_idx,
                        uint32_t width, uint32_t height, uint32_t depth,
                        uint32_t first_level, uint32_t last_level,
                        const void *base_ptr,
                        uint32_t row_stride[PIPE_MAX_TEXTURE_LEVELS],
                        uint32_t img_stride[PIPE_MAX_TEXTURE_LEVELS],
                        uint32_t mip_offsets[PIPE_MAX_TEXTURE_LEVELS])
{
#ifdef HAVE_LLVM
   if (draw->llvm)
      draw_llvm_set_mapped_texture(draw,
                                   shader_stage,
                                   sview_idx,
                                   width, height, depth, first_level,
                                   last_level, base_ptr,
                                   row_stride, img_stride, mip_offsets);
#endif
}

/**
 * XXX: Results for PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS because there are two
 * different ways of setting textures, and drivers typically only support one.
 */
int
draw_get_shader_param_no_llvm(unsigned shader, enum pipe_shader_cap param)
{
   switch(shader) {
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_GEOMETRY:
      return tgsi_exec_get_shader_param(param);
   default:
      return 0;
   }
}

/**
 * XXX: Results for PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS because there are two
 * different ways of setting textures, and drivers typically only support one.
 * Drivers requesting a draw context explicitly without llvm must call
 * draw_get_shader_param_no_llvm instead.
 */
int
draw_get_shader_param(unsigned shader, enum pipe_shader_cap param)
{

#ifdef HAVE_LLVM
   if (draw_get_option_use_llvm()) {
      switch(shader) {
      case PIPE_SHADER_VERTEX:
      case PIPE_SHADER_GEOMETRY:
         return gallivm_get_shader_param(param);
      default:
         return 0;
      }
   }
#endif

   return draw_get_shader_param_no_llvm(shader, param);
}

/**
 * Enables or disables collection of statistics.
 *
 * Draw module is capable of generating statistics for the vertex
 * processing pipeline. Collection of that data isn't free and so
 * it's disabled by default. The users of the module can enable
 * (or disable) this functionality through this function.
 * The actual data will be emitted through the VBUF interface,
 * the 'pipeline_statistics' callback to be exact.
 */
void
draw_collect_pipeline_statistics(struct draw_context *draw,
                                 boolean enable)
{
   draw->collect_statistics = enable;
}

/**
 * Computes clipper invocation statistics.
 *
 * Figures out how many primitives would have been
 * sent to the clipper given the specified
 * prim info data.
 */
void
draw_stats_clipper_primitives(struct draw_context *draw,
                              const struct draw_prim_info *prim_info)
{
   if (draw->collect_statistics) {
      unsigned i;
      for (i = 0; i < prim_info->primitive_count; i++) {
         draw->statistics.c_invocations +=
            u_decomposed_prims_for_vertices(prim_info->prim,
                                            prim_info->primitive_lengths[i]);
      }
   }
}


/**
 * Returns true if the draw module will inject the frontface
 * info into the outputs.
 *
 * Given the specified primitive and rasterizer state
 * the function will figure out if the draw module
 * will inject the front-face information into shader
 * outputs. This is done to preserve the front-facing
 * info when decomposing primitives into wireframes.
 */
boolean
draw_will_inject_frontface(const struct draw_context *draw)
{
   unsigned reduced_prim = u_reduced_prim(draw->pt.prim);
   const struct pipe_rasterizer_state *rast = draw->rasterizer;

   if (reduced_prim != PIPE_PRIM_TRIANGLES) {
      return FALSE;
   }

   return (rast &&
           (rast->fill_front != PIPE_POLYGON_MODE_FILL ||
            rast->fill_back != PIPE_POLYGON_MODE_FILL));
}
