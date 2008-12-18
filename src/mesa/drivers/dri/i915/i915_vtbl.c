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



#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/colormac.h"

#include "tnl/t_context.h"
#include "tnl/t_vertex.h"

#include "intel_batchbuffer.h"
#include "intel_tex.h"
#include "intel_regions.h"
#include "intel_tris.h"

#include "i915_reg.h"
#include "i915_context.h"

#include "glapi/glapi.h"

static void
i915_render_prevalidate(struct intel_context *intel)
{
   struct i915_context *i915 = i915_context(&intel->ctx);

   if (!intel->Fallback)
       i915ValidateFragmentProgram(i915);
}

static void
i915_render_start(struct intel_context *intel)
{
}


static void
i915_reduced_primitive_state(struct intel_context *intel, GLenum rprim)
{
   struct i915_context *i915 = i915_context(&intel->ctx);
   GLuint st1 = i915->state.Stipple[I915_STPREG_ST1];

   st1 &= ~ST1_ENABLE;

   switch (rprim) {
   case GL_QUADS: /* from RASTERIZE(GL_QUADS) in t_dd_tritemp.h */
   case GL_TRIANGLES:
      if (intel->ctx.Polygon.StippleFlag && intel->hw_stipple)
         st1 |= ST1_ENABLE;
      break;
   case GL_LINES:
   case GL_POINTS:
   default:
      break;
   }

   i915->intel.reduced_primitive = rprim;

   if (st1 != i915->state.Stipple[I915_STPREG_ST1]) {
      INTEL_FIREVERTICES(intel);

      I915_STATECHANGE(i915, I915_UPLOAD_STIPPLE);
      i915->state.Stipple[I915_STPREG_ST1] = st1;
   }
}


/* Pull apart the vertex format registers and figure out how large a
 * vertex is supposed to be. 
 */
static GLboolean
i915_check_vertex_size(struct intel_context *intel, GLuint expected)
{
   struct i915_context *i915 = i915_context(&intel->ctx);
   int lis2 = i915->current->Ctx[I915_CTXREG_LIS2];
   int lis4 = i915->current->Ctx[I915_CTXREG_LIS4];
   int i, sz = 0;

   switch (lis4 & S4_VFMT_XYZW_MASK) {
   case S4_VFMT_XY:
      sz = 2;
      break;
   case S4_VFMT_XYZ:
      sz = 3;
      break;
   case S4_VFMT_XYW:
      sz = 3;
      break;
   case S4_VFMT_XYZW:
      sz = 4;
      break;
   default:
      fprintf(stderr, "no xyzw specified\n");
      return 0;
   }

   if (lis4 & S4_VFMT_SPEC_FOG)
      sz++;
   if (lis4 & S4_VFMT_COLOR)
      sz++;
   if (lis4 & S4_VFMT_DEPTH_OFFSET)
      sz++;
   if (lis4 & S4_VFMT_POINT_WIDTH)
      sz++;
   if (lis4 & S4_VFMT_FOG_PARAM)
      sz++;

   for (i = 0; i < 8; i++) {
      switch (lis2 & S2_TEXCOORD_FMT0_MASK) {
      case TEXCOORDFMT_2D:
         sz += 2;
         break;
      case TEXCOORDFMT_3D:
         sz += 3;
         break;
      case TEXCOORDFMT_4D:
         sz += 4;
         break;
      case TEXCOORDFMT_1D:
         sz += 1;
         break;
      case TEXCOORDFMT_2D_16:
         sz += 1;
         break;
      case TEXCOORDFMT_4D_16:
         sz += 2;
         break;
      case TEXCOORDFMT_NOT_PRESENT:
         break;
      default:
         fprintf(stderr, "bad texcoord fmt %d\n", i);
         return GL_FALSE;
      }
      lis2 >>= S2_TEXCOORD_FMT1_SHIFT;
   }

   if (sz != expected)
      fprintf(stderr, "vertex size mismatch %d/%d\n", sz, expected);

   return sz == expected;
}


static void
i915_emit_invarient_state(struct intel_context *intel)
{
   BATCH_LOCALS;

   BEGIN_BATCH(20, IGNORE_CLIPRECTS);

   OUT_BATCH(_3DSTATE_AA_CMD |
             AA_LINE_ECAAR_WIDTH_ENABLE |
             AA_LINE_ECAAR_WIDTH_1_0 |
             AA_LINE_REGION_WIDTH_ENABLE | AA_LINE_REGION_WIDTH_1_0);

   OUT_BATCH(_3DSTATE_DFLT_DIFFUSE_CMD);
   OUT_BATCH(0);

   OUT_BATCH(_3DSTATE_DFLT_SPEC_CMD);
   OUT_BATCH(0);

   OUT_BATCH(_3DSTATE_DFLT_Z_CMD);
   OUT_BATCH(0);

   /* Don't support texture crossbar yet */
   OUT_BATCH(_3DSTATE_COORD_SET_BINDINGS |
             CSB_TCB(0, 0) |
             CSB_TCB(1, 1) |
             CSB_TCB(2, 2) |
             CSB_TCB(3, 3) |
             CSB_TCB(4, 4) | CSB_TCB(5, 5) | CSB_TCB(6, 6) | CSB_TCB(7, 7));

   OUT_BATCH(_3DSTATE_RASTER_RULES_CMD |
             ENABLE_POINT_RASTER_RULE |
             OGL_POINT_RASTER_RULE |
             ENABLE_LINE_STRIP_PROVOKE_VRTX |
             ENABLE_TRI_FAN_PROVOKE_VRTX |
             LINE_STRIP_PROVOKE_VRTX(1) |
             TRI_FAN_PROVOKE_VRTX(2) | ENABLE_TEXKILL_3D_4D | TEXKILL_4D);

   /* Need to initialize this to zero.
    */
   OUT_BATCH(_3DSTATE_LOAD_STATE_IMMEDIATE_1 | I1_LOAD_S(3) | (0));
   OUT_BATCH(0);

   /* XXX: Use this */
   OUT_BATCH(_3DSTATE_SCISSOR_ENABLE_CMD | DISABLE_SCISSOR_RECT);

   OUT_BATCH(_3DSTATE_SCISSOR_RECT_0_CMD);
   OUT_BATCH(0);
   OUT_BATCH(0);

   OUT_BATCH(_3DSTATE_DEPTH_SUBRECT_DISABLE);

   OUT_BATCH(_3DSTATE_LOAD_INDIRECT | 0);       /* disable indirect state */
   OUT_BATCH(0);


   /* Don't support twosided stencil yet */
   OUT_BATCH(_3DSTATE_BACKFACE_STENCIL_OPS | BFO_ENABLE_STENCIL_TWO_SIDE | 0);
   OUT_BATCH(0);

   ADVANCE_BATCH();
}


#define emit(intel, state, size )		     \
   intel_batchbuffer_data(intel->batch, state, size, IGNORE_CLIPRECTS )

static GLuint
get_dirty(struct i915_hw_state *state)
{
   GLuint dirty;

   /* Workaround the multitex hang - if one texture unit state is
    * modified, emit all texture units.
    */
   dirty = state->active & ~state->emitted;
   if (dirty & I915_UPLOAD_TEX_ALL)
      state->emitted &= ~I915_UPLOAD_TEX_ALL;
   dirty = state->active & ~state->emitted;
   return dirty;
}


static GLuint
get_state_size(struct i915_hw_state *state)
{
   GLuint dirty = get_dirty(state);
   GLuint i;
   GLuint sz = 0;

   if (dirty & I915_UPLOAD_INVARIENT)
      sz += 30 * 4;

   if (dirty & I915_UPLOAD_CTX)
      sz += sizeof(state->Ctx);

   if (dirty & I915_UPLOAD_BUFFERS)
      sz += sizeof(state->Buffer);

   if (dirty & I915_UPLOAD_STIPPLE)
      sz += sizeof(state->Stipple);

   if (dirty & I915_UPLOAD_FOG)
      sz += sizeof(state->Fog);

   if (dirty & I915_UPLOAD_TEX_ALL) {
      int nr = 0;
      for (i = 0; i < I915_TEX_UNITS; i++)
         if (dirty & I915_UPLOAD_TEX(i))
            nr++;

      sz += (2 + nr * 3) * sizeof(GLuint) * 2;
   }

   if (dirty & I915_UPLOAD_CONSTANTS)
      sz += state->ConstantSize * sizeof(GLuint);

   if (dirty & I915_UPLOAD_PROGRAM)
      sz += state->ProgramSize * sizeof(GLuint);

   return sz;
}

/* Push the state into the sarea and/or texture memory.
 */
static void
i915_emit_state(struct intel_context *intel)
{
   struct i915_context *i915 = i915_context(&intel->ctx);
   struct i915_hw_state *state = i915->current;
   int i, count, aper_count;
   GLuint dirty;
   dri_bo *aper_array[3 + I915_TEX_UNITS];
   GET_CURRENT_CONTEXT(ctx);
   BATCH_LOCALS;

   /* We don't hold the lock at this point, so want to make sure that
    * there won't be a buffer wrap between the state emits and the primitive
    * emit header.
    *
    * It might be better to talk about explicit places where
    * scheduling is allowed, rather than assume that it is whenever a
    * batchbuffer fills up.
    *
    * Set the space as LOOP_CLIPRECTS now, since that's what our primitives
    * will be emitted under.
    */
   intel_batchbuffer_require_space(intel->batch,
				   get_state_size(state) + INTEL_PRIM_EMIT_SIZE,
				   LOOP_CLIPRECTS);
   count = 0;
 again:
   aper_count = 0;
   dirty = get_dirty(state);

   aper_array[aper_count++] = intel->batch->buf;
   if (dirty & I915_UPLOAD_BUFFERS) {
      aper_array[aper_count++] = state->draw_region->buffer;
      if (state->depth_region)
	 aper_array[aper_count++] = state->depth_region->buffer;
   }

   if (dirty & I915_UPLOAD_TEX_ALL) {
      for (i = 0; i < I915_TEX_UNITS; i++) {
	 if (dirty & I915_UPLOAD_TEX(i)) {
	    if (state->tex_buffer[i]) {
	       aper_array[aper_count++] = state->tex_buffer[i];
	    }
	 }
      }
   }

   if (dri_bufmgr_check_aperture_space(aper_array, aper_count)) {
       if (count == 0) {
	   count++;
	   intel_batchbuffer_flush(intel->batch);
	   goto again;
       } else {
	   _mesa_error(ctx, GL_OUT_OF_MEMORY, "i915 emit state");
	   assert(0);
       }
   }

   /* work out list of buffers to emit */
   
   /* Do this here as we may have flushed the batchbuffer above,
    * causing more state to be dirty!
    */
   dirty = get_dirty(state);
   state->emitted |= dirty;
   assert(get_dirty(state) == 0);

   if (INTEL_DEBUG & DEBUG_STATE)
      fprintf(stderr, "%s dirty: %x\n", __FUNCTION__, dirty);

   if (dirty & I915_UPLOAD_INVARIENT) {
      if (INTEL_DEBUG & DEBUG_STATE)
         fprintf(stderr, "I915_UPLOAD_INVARIENT:\n");
      i915_emit_invarient_state(intel);
   }

   if (dirty & I915_UPLOAD_CTX) {
      if (INTEL_DEBUG & DEBUG_STATE)
         fprintf(stderr, "I915_UPLOAD_CTX:\n");

      emit(intel, state->Ctx, sizeof(state->Ctx));
   }

   if (dirty & I915_UPLOAD_BUFFERS) {
      GLuint count = 9;

      if (INTEL_DEBUG & DEBUG_STATE)
         fprintf(stderr, "I915_UPLOAD_BUFFERS:\n");

      if (state->depth_region)
          count += 3;

      if (intel->constant_cliprect)
          count += 6;

      BEGIN_BATCH(count, IGNORE_CLIPRECTS);
      OUT_BATCH(state->Buffer[I915_DESTREG_CBUFADDR0]);
      OUT_BATCH(state->Buffer[I915_DESTREG_CBUFADDR1]);
      OUT_RELOC(state->draw_region->buffer,
		I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                state->draw_region->draw_offset);

      if (state->depth_region) {
         OUT_BATCH(state->Buffer[I915_DESTREG_DBUFADDR0]);
         OUT_BATCH(state->Buffer[I915_DESTREG_DBUFADDR1]);
         OUT_RELOC(state->depth_region->buffer,
		   I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                   state->depth_region->draw_offset);
      }

      OUT_BATCH(state->Buffer[I915_DESTREG_DV0]);
      OUT_BATCH(state->Buffer[I915_DESTREG_DV1]);
      OUT_BATCH(state->Buffer[I915_DESTREG_SENABLE]);
      OUT_BATCH(state->Buffer[I915_DESTREG_SR0]);
      OUT_BATCH(state->Buffer[I915_DESTREG_SR1]);
      OUT_BATCH(state->Buffer[I915_DESTREG_SR2]);

      if (intel->constant_cliprect) {
	 assert(state->Buffer[I915_DESTREG_DRAWRECT0] != MI_NOOP);
	 OUT_BATCH(state->Buffer[I915_DESTREG_DRAWRECT0]);
	 OUT_BATCH(state->Buffer[I915_DESTREG_DRAWRECT1]);
	 OUT_BATCH(state->Buffer[I915_DESTREG_DRAWRECT2]);
	 OUT_BATCH(state->Buffer[I915_DESTREG_DRAWRECT3]);
	 OUT_BATCH(state->Buffer[I915_DESTREG_DRAWRECT4]);
	 OUT_BATCH(state->Buffer[I915_DESTREG_DRAWRECT5]);
      }

      ADVANCE_BATCH();
   }

   if (dirty & I915_UPLOAD_STIPPLE) {
      if (INTEL_DEBUG & DEBUG_STATE)
         fprintf(stderr, "I915_UPLOAD_STIPPLE:\n");
      emit(intel, state->Stipple, sizeof(state->Stipple));
   }

   if (dirty & I915_UPLOAD_FOG) {
      if (INTEL_DEBUG & DEBUG_STATE)
         fprintf(stderr, "I915_UPLOAD_FOG:\n");
      emit(intel, state->Fog, sizeof(state->Fog));
   }

   /* Combine all the dirty texture state into a single command to
    * avoid lockups on I915 hardware. 
    */
   if (dirty & I915_UPLOAD_TEX_ALL) {
      int nr = 0;

      for (i = 0; i < I915_TEX_UNITS; i++)
         if (dirty & I915_UPLOAD_TEX(i))
            nr++;

      BEGIN_BATCH(2 + nr * 3, IGNORE_CLIPRECTS);
      OUT_BATCH(_3DSTATE_MAP_STATE | (3 * nr));
      OUT_BATCH((dirty & I915_UPLOAD_TEX_ALL) >> I915_UPLOAD_TEX_0_SHIFT);
      for (i = 0; i < I915_TEX_UNITS; i++)
         if (dirty & I915_UPLOAD_TEX(i)) {

            if (state->tex_buffer[i]) {
               OUT_RELOC(state->tex_buffer[i],
			 I915_GEM_DOMAIN_SAMPLER, 0,
                         state->tex_offset[i]);
            }
            else if (state == &i915->meta) {
               assert(i == 0);
               OUT_BATCH(0);
            }
            else {
               OUT_BATCH(state->tex_offset[i]);
            }

            OUT_BATCH(state->Tex[i][I915_TEXREG_MS3]);
            OUT_BATCH(state->Tex[i][I915_TEXREG_MS4]);
         }
      ADVANCE_BATCH();

      BEGIN_BATCH(2 + nr * 3, IGNORE_CLIPRECTS);
      OUT_BATCH(_3DSTATE_SAMPLER_STATE | (3 * nr));
      OUT_BATCH((dirty & I915_UPLOAD_TEX_ALL) >> I915_UPLOAD_TEX_0_SHIFT);
      for (i = 0; i < I915_TEX_UNITS; i++)
         if (dirty & I915_UPLOAD_TEX(i)) {
            OUT_BATCH(state->Tex[i][I915_TEXREG_SS2]);
            OUT_BATCH(state->Tex[i][I915_TEXREG_SS3]);
            OUT_BATCH(state->Tex[i][I915_TEXREG_SS4]);
         }
      ADVANCE_BATCH();
   }

   if (dirty & I915_UPLOAD_CONSTANTS) {
      if (INTEL_DEBUG & DEBUG_STATE)
         fprintf(stderr, "I915_UPLOAD_CONSTANTS:\n");
      emit(intel, state->Constant, state->ConstantSize * sizeof(GLuint));
   }

   if (dirty & I915_UPLOAD_PROGRAM) {
      if (state->ProgramSize) {
         if (INTEL_DEBUG & DEBUG_STATE)
            fprintf(stderr, "I915_UPLOAD_PROGRAM:\n");

         assert((state->Program[0] & 0x1ff) + 2 == state->ProgramSize);

         emit(intel, state->Program, state->ProgramSize * sizeof(GLuint));
         if (INTEL_DEBUG & DEBUG_STATE)
            i915_disassemble_program(state->Program, state->ProgramSize);
      }
   }

   intel->batch->dirty_state &= ~dirty;
   assert(get_dirty(state) == 0);
   assert((intel->batch->dirty_state & (1<<1)) == 0);
}

static void
i915_destroy_context(struct intel_context *intel)
{
   GLuint i;
   struct i915_context *i915 = i915_context(&intel->ctx);

   intel_region_release(&i915->state.draw_region);
   intel_region_release(&i915->state.depth_region);
   intel_region_release(&i915->meta.draw_region);
   intel_region_release(&i915->meta.depth_region);
   intel_region_release(&i915->initial.draw_region);
   intel_region_release(&i915->initial.depth_region);

   for (i = 0; i < I915_TEX_UNITS; i++) {
      if (i915->state.tex_buffer[i] != NULL) {
	 dri_bo_unreference(i915->state.tex_buffer[i]);
	 i915->state.tex_buffer[i] = NULL;
      }
   }

   _tnl_free_vertices(&intel->ctx);
}


/**
 * Set the drawing regions for the color and depth/stencil buffers.
 * This involves setting the pitch, cpp and buffer ID/location.
 * Also set pixel format for color and Z rendering
 * Used for setting both regular and meta state.
 */
void
i915_state_draw_region(struct intel_context *intel,
                       struct i915_hw_state *state,
                       struct intel_region *color_region,
                       struct intel_region *depth_region)
{
   struct i915_context *i915 = i915_context(&intel->ctx);
   GLcontext *ctx = &intel->ctx;
   GLuint value;

   ASSERT(state == &i915->state || state == &i915->meta);

   if (state->draw_region != color_region) {
      intel_region_release(&state->draw_region);
      intel_region_reference(&state->draw_region, color_region);
   }
   if (state->depth_region != depth_region) {
      intel_region_release(&state->depth_region);
      intel_region_reference(&state->depth_region, depth_region);
   }

   /*
    * Set stride/cpp values
    */
   if (color_region) {
      state->Buffer[I915_DESTREG_CBUFADDR0] = _3DSTATE_BUF_INFO_CMD;
      state->Buffer[I915_DESTREG_CBUFADDR1] =
         (BUF_3D_ID_COLOR_BACK |
          BUF_3D_PITCH(color_region->pitch * color_region->cpp) |
          BUF_3D_USE_FENCE);
   }

   if (depth_region) {
      state->Buffer[I915_DESTREG_DBUFADDR0] = _3DSTATE_BUF_INFO_CMD;
      state->Buffer[I915_DESTREG_DBUFADDR1] =
         (BUF_3D_ID_DEPTH |
          BUF_3D_PITCH(depth_region->pitch * depth_region->cpp) |
          BUF_3D_USE_FENCE);
   }

   /*
    * Compute/set I915_DESTREG_DV1 value
    */
   value = (DSTORG_HORT_BIAS(0x8) |     /* .5 */
            DSTORG_VERT_BIAS(0x8) |     /* .5 */
            LOD_PRECLAMP_OGL | TEX_DEFAULT_COLOR_OGL);
   if (color_region && color_region->cpp == 4) {
      value |= DV_PF_8888;
   }
   else {
      value |= (DITHER_FULL_ALWAYS | DV_PF_565);
   }
   if (depth_region && depth_region->cpp == 4) {
      value |= DEPTH_FRMT_24_FIXED_8_OTHER;
   }
   else {
      value |= DEPTH_FRMT_16_FIXED;
   }
   state->Buffer[I915_DESTREG_DV1] = value;

   if (intel->constant_cliprect) {
      state->Buffer[I915_DESTREG_DRAWRECT0] = _3DSTATE_DRAWRECT_INFO;
      state->Buffer[I915_DESTREG_DRAWRECT1] = 0;
      state->Buffer[I915_DESTREG_DRAWRECT2] = 0; /* xmin, ymin */
      state->Buffer[I915_DESTREG_DRAWRECT3] =
	 (ctx->DrawBuffer->Width & 0xffff) |
	 (ctx->DrawBuffer->Height << 16);
      state->Buffer[I915_DESTREG_DRAWRECT4] = 0; /* xoff, yoff */
      state->Buffer[I915_DESTREG_DRAWRECT5] = 0;
   } else {
      state->Buffer[I915_DESTREG_DRAWRECT0] = MI_NOOP;
      state->Buffer[I915_DESTREG_DRAWRECT1] = MI_NOOP;
      state->Buffer[I915_DESTREG_DRAWRECT2] = MI_NOOP;
      state->Buffer[I915_DESTREG_DRAWRECT3] = MI_NOOP;
      state->Buffer[I915_DESTREG_DRAWRECT4] = MI_NOOP;
      state->Buffer[I915_DESTREG_DRAWRECT5] = MI_NOOP;
   }

   I915_STATECHANGE(i915, I915_UPLOAD_BUFFERS);
}


static void
i915_set_draw_region(struct intel_context *intel,
                     struct intel_region *color_regions[],
                     struct intel_region *depth_region,
		     GLuint num_regions)
{
   struct i915_context *i915 = i915_context(&intel->ctx);
   i915_state_draw_region(intel, &i915->state, color_regions[0], depth_region);
}



static void
i915_new_batch(struct intel_context *intel)
{
   struct i915_context *i915 = i915_context(&intel->ctx);

   /* Mark all state as needing to be emitted when starting a new batchbuffer.
    * Using hardware contexts would be an alternative, but they have some
    * difficulties associated with them (physical address requirements).
    */
   i915->state.emitted = 0;

   /* Check that we didn't just wrap our batchbuffer at a bad time. */
   assert(!intel->no_batch_wrap);
}

static GLuint
i915_flush_cmd(void)
{
   return MI_FLUSH | FLUSH_MAP_CACHE;
}

static void 
i915_assert_not_dirty( struct intel_context *intel )
{
   struct i915_context *i915 = i915_context(&intel->ctx);
   struct i915_hw_state *state = i915->current;
   GLuint dirty = get_dirty(state);
   assert(!dirty);
}

static void
i915_note_unlock( struct intel_context *intel )
{
    /* nothing */
}


void
i915InitVtbl(struct i915_context *i915)
{
   i915->intel.vtbl.check_vertex_size = i915_check_vertex_size;
   i915->intel.vtbl.destroy = i915_destroy_context;
   i915->intel.vtbl.emit_state = i915_emit_state;
   i915->intel.vtbl.new_batch = i915_new_batch;
   i915->intel.vtbl.reduced_primitive_state = i915_reduced_primitive_state;
   i915->intel.vtbl.render_start = i915_render_start;
   i915->intel.vtbl.render_prevalidate = i915_render_prevalidate;
   i915->intel.vtbl.set_draw_region = i915_set_draw_region;
   i915->intel.vtbl.update_texture_state = i915UpdateTextureState;
   i915->intel.vtbl.flush_cmd = i915_flush_cmd;
   i915->intel.vtbl.assert_not_dirty = i915_assert_not_dirty;
   i915->intel.vtbl.note_unlock = i915_note_unlock; 
   i915->intel.vtbl.finish_batch = intel_finish_vb;
}
