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

#ifndef I915_CONTEXT_H
#define I915_CONTEXT_H


#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "pipe/draw/draw_vertex.h"


#define I915_TEX_UNITS 8

#define I915_DYNAMIC_MODES4       0
#define I915_DYNAMIC_DEPTHSCALE_0 1 /* just the header */
#define I915_DYNAMIC_DEPTHSCALE_1 2 
#define I915_DYNAMIC_IAB          3
#define I915_DYNAMIC_BC_0         4 /* just the header */
#define I915_DYNAMIC_BC_1         5
#define I915_DYNAMIC_BFO_0        6 
#define I915_DYNAMIC_BFO_1        7
#define I915_DYNAMIC_STP_0        8 
#define I915_DYNAMIC_STP_1        9 
#define I915_DYNAMIC_SC_ENA_0     10 
#define I915_DYNAMIC_SC_RECT_0    11 
#define I915_DYNAMIC_SC_RECT_1    12 
#define I915_DYNAMIC_SC_RECT_2    13 
#define I915_MAX_DYNAMIC          14


#define I915_IMMEDIATE_S0         0
#define I915_IMMEDIATE_S1         1
#define I915_IMMEDIATE_S2         2
#define I915_IMMEDIATE_S3         3
#define I915_IMMEDIATE_S4         4
#define I915_IMMEDIATE_S5         5
#define I915_IMMEDIATE_S6         6
#define I915_IMMEDIATE_S7         7
#define I915_MAX_IMMEDIATE        8

/* These must mach the order of LI0_STATE_* bits, as they will be used
 * to generate hardware packets:
 */
#define I915_CACHE_STATIC         0 
#define I915_CACHE_DYNAMIC        1 /* handled specially */
#define I915_CACHE_SAMPLER        2
#define I915_CACHE_MAP            3
#define I915_CACHE_PROGRAM        4
#define I915_CACHE_CONSTANTS      5
#define I915_MAX_CACHE            6

#define I915_MAX_CONSTANT  32



struct i915_cache_context;

/* Use to calculate differences between state emitted to hardware and
 * current driver-calculated state.  
 */
struct i915_state 
{
   unsigned immediate[I915_MAX_IMMEDIATE];
   unsigned dynamic[I915_MAX_DYNAMIC];

   float constants[PIPE_SHADER_TYPES][I915_MAX_CONSTANT][4];
   /** number of constants passed in through a constant buffer */
   uint num_user_constants[PIPE_SHADER_TYPES];
   /** user constants, plus extra constants from shader translation */
   uint num_constants[PIPE_SHADER_TYPES];

   uint *program;
   uint program_len;

   /* texture sampler state */
   unsigned sampler[I915_TEX_UNITS][3];
   unsigned sampler_enable_flags;
   unsigned sampler_enable_nr;

   /* texture image buffers */
   unsigned texbuffer[I915_TEX_UNITS][2];

   /** Describes the current hardware vertex layout */
   struct vertex_info vertex_info;

   unsigned id;			/* track lost context events */
};

struct i915_blend_state {
   unsigned iab;
   unsigned modes4;
   unsigned LIS5;
   unsigned LIS6;
};

struct i915_depth_stencil_state {
   unsigned stencil_modes4;
   unsigned bfo[2];
   unsigned stencil_LIS5;
   unsigned depth_LIS6;
};

struct i915_rasterizer_state {
   int light_twoside : 1;
   unsigned st;
   interp_mode color_interp;

   unsigned LIS4;
   unsigned LIS7;
   unsigned sc[1];

   const struct pipe_rasterizer_state *templ;

   union { float f; unsigned u; } ds[2];
};

struct i915_sampler_state {
   unsigned state[3];
   const struct pipe_sampler_state *templ;
};

struct i915_context
{
   struct pipe_context pipe; 
   struct i915_winsys *winsys;
   struct draw_context *draw;

   /* The most recent drawing state as set by the driver:
    */
   const struct i915_blend_state           *blend;
   const struct i915_sampler_state         *sampler[PIPE_MAX_SAMPLERS];
   const struct i915_depth_stencil_state   *depth_stencil;
   const struct i915_rasterizer_state      *rasterizer;
   const struct pipe_shader_state *fs;

   struct pipe_alpha_test_state alpha_test;
   struct pipe_blend_color blend_color;
   struct pipe_clear_color_state clear_color;
   struct pipe_clip_state clip;
   struct pipe_constant_buffer constants[PIPE_SHADER_TYPES];
   struct pipe_framebuffer_state framebuffer;
   struct pipe_poly_stipple poly_stipple;
   struct pipe_scissor_state scissor;
   struct pipe_mipmap_tree *texture[PIPE_MAX_SAMPLERS];
   struct pipe_viewport_state viewport;
   struct pipe_vertex_buffer vertex_buffer[PIPE_ATTRIB_MAX];

   unsigned dirty;

   unsigned *batch_start;

   struct i915_state current;
   unsigned hardware_dirty;
   
   unsigned debug;
   unsigned pci_id;

   struct {
      unsigned is_i945:1;
   } flags;
};

/* A flag for each state_tracker state object:
 */
#define I915_NEW_VIEWPORT      0x1
#define I915_NEW_RASTERIZER    0x2
#define I915_NEW_FS            0x4
#define I915_NEW_BLEND         0x8
#define I915_NEW_CLIP          0x10
#define I915_NEW_SCISSOR       0x20
#define I915_NEW_STIPPLE       0x40
#define I915_NEW_FRAMEBUFFER   0x80
#define I915_NEW_ALPHA_TEST    0x100
#define I915_NEW_DEPTH_STENCIL 0x200
#define I915_NEW_SAMPLER       0x400
#define I915_NEW_TEXTURE       0x800
#define I915_NEW_CONSTANTS     0x1000


/* Driver's internally generated state flags:
 */
#define I915_NEW_VERTEX_FORMAT    0x10000


/* Dirty flags for hardware emit
 */
#define I915_HW_STATIC            (1<<I915_CACHE_STATIC)
#define I915_HW_DYNAMIC           (1<<I915_CACHE_DYNAMIC)
#define I915_HW_SAMPLER           (1<<I915_CACHE_SAMPLER)
#define I915_HW_MAP               (1<<I915_CACHE_MAP)
#define I915_HW_PROGRAM           (1<<I915_CACHE_PROGRAM)
#define I915_HW_CONSTANTS         (1<<I915_CACHE_CONSTANTS)
#define I915_HW_IMMEDIATE         (1<<(I915_MAX_CACHE+0))
#define I915_HW_INVARIENT         (1<<(I915_MAX_CACHE+1))


/***********************************************************************
 * i915_prim_emit.c: 
 */
struct draw_stage *i915_draw_render_stage( struct i915_context *i915 );


/***********************************************************************
 * i915_state_emit.c: 
 */
void i915_emit_hardware_state(struct i915_context *i915 );



/***********************************************************************
 * i915_clear.c: 
 */
void i915_clear(struct pipe_context *pipe, struct pipe_surface *ps,
		unsigned clearValue);


/***********************************************************************
 * i915_region.c: 
 */
void i915_init_region_functions( struct i915_context *i915 );
void i915_init_surface_functions( struct i915_context *i915 );
void i915_init_state_functions( struct i915_context *i915 );
void i915_init_flush_functions( struct i915_context *i915 );
void i915_init_string_functions( struct i915_context *i915 );



/***********************************************************************
 * Inline conversion functions.  These are better-typed than the
 * macros used previously:
 */
static INLINE struct i915_context *
i915_context( struct pipe_context *pipe )
{
   return (struct i915_context *)pipe;
}



#endif
