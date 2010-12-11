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
 


#include "intel_batchbuffer.h"
#include "intel_regions.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

/* Constant single cliprect for framebuffer object or DRI2 drawing */
static void upload_drawing_rect(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_DRAWRECT_INFO_I965);
   OUT_BATCH(0); /* xmin, ymin */
   OUT_BATCH(((ctx->DrawBuffer->Width - 1) & 0xffff) |
	    ((ctx->DrawBuffer->Height - 1) << 16));
   OUT_BATCH(0);
   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_drawing_rect = {
   .dirty = {
      .mesa = _NEW_BUFFERS,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_drawing_rect
};

/**
 * Upload the binding table pointers, which point each stage's array of surface
 * state pointers.
 *
 * The binding table pointers are relative to the surface state base address,
 * which points at the batchbuffer containing the streamed batch state.
 */
static void upload_binding_table_pointers(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(6);
   OUT_BATCH(CMD_BINDING_TABLE_PTRS << 16 | (6 - 2));
   OUT_BATCH(brw->vs.bind_bo_offset);
   OUT_BATCH(0); /* gs */
   OUT_BATCH(0); /* clip */
   OUT_BATCH(0); /* sf */
   OUT_BATCH(brw->wm.bind_bo_offset);
   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_binding_table_pointers = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH | BRW_NEW_BINDING_TABLE,
      .cache = 0,
   },
   .emit = upload_binding_table_pointers,
};

/**
 * Upload the binding table pointers, which point each stage's array of surface
 * state pointers.
 *
 * The binding table pointers are relative to the surface state base address,
 * which points at the batchbuffer containing the streamed batch state.
 */
static void upload_gen6_binding_table_pointers(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(4);
   OUT_BATCH(CMD_BINDING_TABLE_PTRS << 16 |
	     GEN6_BINDING_TABLE_MODIFY_VS |
	     GEN6_BINDING_TABLE_MODIFY_GS |
	     GEN6_BINDING_TABLE_MODIFY_PS |
	     (4 - 2));
   OUT_BATCH(brw->vs.bind_bo_offset); /* vs */
   OUT_BATCH(0); /* gs */
   OUT_BATCH(brw->wm.bind_bo_offset); /* wm/ps */
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen6_binding_table_pointers = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH | BRW_NEW_BINDING_TABLE,
      .cache = 0,
   },
   .emit = upload_gen6_binding_table_pointers,
};

/**
 * Upload pointers to the per-stage state.
 *
 * The state pointers in this packet are all relative to the general state
 * base address set by CMD_STATE_BASE_ADDRESS, which is 0.
 */
static void upload_pipelined_state_pointers(struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;

   if (intel->gen == 5) {
      /* Need to flush before changing clip max threads for errata. */
      BEGIN_BATCH(1);
      OUT_BATCH(MI_FLUSH);
      ADVANCE_BATCH();
   }

   BEGIN_BATCH(7);
   OUT_BATCH(CMD_PIPELINED_STATE_POINTERS << 16 | (7 - 2));
   OUT_RELOC(brw->vs.state_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
   if (brw->gs.prog_active)
      OUT_RELOC(brw->gs.state_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 1);
   else
      OUT_BATCH(0);
   OUT_RELOC(brw->clip.state_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 1);
   OUT_RELOC(brw->sf.state_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
   OUT_RELOC(brw->wm.state_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
   OUT_RELOC(brw->cc.state_bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
	     brw->cc.state_offset);
   ADVANCE_BATCH();

   brw->state.dirty.brw |= BRW_NEW_PSP;
}


static void prepare_psp_urb_cbs(struct brw_context *brw)
{
   brw_add_validated_bo(brw, brw->vs.state_bo);
   brw_add_validated_bo(brw, brw->gs.state_bo);
   brw_add_validated_bo(brw, brw->clip.state_bo);
   brw_add_validated_bo(brw, brw->sf.state_bo);
   brw_add_validated_bo(brw, brw->wm.state_bo);
}

static void upload_psp_urb_cbs(struct brw_context *brw )
{
   upload_pipelined_state_pointers(brw);
   brw_upload_urb_fence(brw);
   brw_upload_cs_urb_state(brw);
}

const struct brw_tracked_state brw_psp_urb_cbs = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_URB_FENCE | BRW_NEW_BATCH,
      .cache = (CACHE_NEW_VS_UNIT | 
		CACHE_NEW_GS_UNIT | 
		CACHE_NEW_GS_PROG | 
		CACHE_NEW_CLIP_UNIT | 
		CACHE_NEW_SF_UNIT | 
		CACHE_NEW_WM_UNIT | 
		CACHE_NEW_CC_UNIT)
   },
   .prepare = prepare_psp_urb_cbs,
   .emit = upload_psp_urb_cbs,
};

static void prepare_depthbuffer(struct brw_context *brw)
{
   struct intel_region *region = brw->state.depth_region;

   if (region != NULL)
      brw_add_validated_bo(brw, region->buffer);
}

static void emit_depthbuffer(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct intel_region *region = brw->state.depth_region;
   unsigned int len;

   if (intel->gen >= 6)
      len = 7;
   else if (intel->is_g4x || intel->gen == 5)
      len = 6;
   else
      len = 5;

   if (region == NULL) {
      BEGIN_BATCH(len);
      OUT_BATCH(CMD_DEPTH_BUFFER << 16 | (len - 2));
      OUT_BATCH((BRW_DEPTHFORMAT_D32_FLOAT << 18) |
		(BRW_SURFACE_NULL << 29));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);

      if (intel->is_g4x || intel->gen >= 5)
         OUT_BATCH(0);

      if (intel->gen >= 6)
	 OUT_BATCH(0);

      ADVANCE_BATCH();
   } else {
      unsigned int format;

      switch (region->cpp) {
      case 2:
	 format = BRW_DEPTHFORMAT_D16_UNORM;
	 break;
      case 4:
	 if (intel->depth_buffer_is_float)
	    format = BRW_DEPTHFORMAT_D32_FLOAT;
	 else
	    format = BRW_DEPTHFORMAT_D24_UNORM_S8_UINT;
	 break;
      default:
	 assert(0);
	 return;
      }

      assert(region->tiling != I915_TILING_X);
      if (intel->gen >= 6)
	 assert(region->tiling != I915_TILING_NONE);

      BEGIN_BATCH(len);
      OUT_BATCH(CMD_DEPTH_BUFFER << 16 | (len - 2));
      OUT_BATCH(((region->pitch * region->cpp) - 1) |
		(format << 18) |
		(BRW_TILEWALK_YMAJOR << 26) |
		((region->tiling != I915_TILING_NONE) << 27) |
		(BRW_SURFACE_2D << 29));
      OUT_RELOC(region->buffer,
		I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		0);
      OUT_BATCH((BRW_SURFACE_MIPMAPLAYOUT_BELOW << 1) |
		((region->width - 1) << 6) |
		((region->height - 1) << 19));
      OUT_BATCH(0);

      if (intel->is_g4x || intel->gen >= 5)
         OUT_BATCH(0);

      if (intel->gen >= 6)
	 OUT_BATCH(0);

      ADVANCE_BATCH();
   }

   /* Initialize it for safety. */
   if (intel->gen >= 6) {
      BEGIN_BATCH(2);
      OUT_BATCH(CMD_3D_CLEAR_PARAMS << 16 | (2 - 2));
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }
}

const struct brw_tracked_state brw_depthbuffer = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_DEPTH_BUFFER | BRW_NEW_BATCH,
      .cache = 0,
   },
   .prepare = prepare_depthbuffer,
   .emit = emit_depthbuffer,
};



/***********************************************************************
 * Polygon stipple packet
 */

static void upload_polygon_stipple(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct brw_polygon_stipple bps;
   GLuint i;

   if (!ctx->Polygon.StippleFlag)
      return;

   memset(&bps, 0, sizeof(bps));
   bps.header.opcode = CMD_POLY_STIPPLE_PATTERN;
   bps.header.length = sizeof(bps)/4-2;

   /* Polygon stipple is provided in OpenGL order, i.e. bottom
    * row first.  If we're rendering to a window (i.e. the
    * default frame buffer object, 0), then we need to invert
    * it to match our pixel layout.  But if we're rendering
    * to a FBO (i.e. any named frame buffer object), we *don't*
    * need to invert - we already match the layout.
    */
   if (ctx->DrawBuffer->Name == 0) {
      for (i = 0; i < 32; i++)
         bps.stipple[i] = ctx->PolygonStipple[31 - i]; /* invert */
   }
   else {
      for (i = 0; i < 32; i++)
         bps.stipple[i] = ctx->PolygonStipple[i]; /* don't invert */
   }

   BRW_CACHED_BATCH_STRUCT(brw, &bps);
}

const struct brw_tracked_state brw_polygon_stipple = {
   .dirty = {
      .mesa = _NEW_POLYGONSTIPPLE,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_polygon_stipple
};


/***********************************************************************
 * Polygon stipple offset packet
 */

static void upload_polygon_stipple_offset(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct brw_polygon_stipple_offset bpso;

   if (!ctx->Polygon.StippleFlag)
      return;

   memset(&bpso, 0, sizeof(bpso));
   bpso.header.opcode = CMD_POLY_STIPPLE_OFFSET;
   bpso.header.length = sizeof(bpso)/4-2;

   /* If we're drawing to a system window (ctx->DrawBuffer->Name == 0),
    * we have to invert the Y axis in order to match the OpenGL
    * pixel coordinate system, and our offset must be matched
    * to the window position.  If we're drawing to a FBO
    * (ctx->DrawBuffer->Name != 0), then our native pixel coordinate
    * system works just fine, and there's no window system to
    * worry about.
    */
   if (brw->intel.ctx.DrawBuffer->Name == 0) {
      bpso.bits0.x_offset = 0;
      bpso.bits0.y_offset = (32 - (ctx->DrawBuffer->Height & 31)) & 31;
   }
   else {
      bpso.bits0.y_offset = 0;
      bpso.bits0.x_offset = 0;
   }

   BRW_CACHED_BATCH_STRUCT(brw, &bpso);
}

#define _NEW_WINDOW_POS 0x40000000

const struct brw_tracked_state brw_polygon_stipple_offset = {
   .dirty = {
      .mesa = _NEW_WINDOW_POS | _NEW_POLYGONSTIPPLE,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_polygon_stipple_offset
};

/**********************************************************************
 * AA Line parameters
 */
static void upload_aa_line_parameters(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct brw_aa_line_parameters balp;

   if (!ctx->Line.SmoothFlag || !brw->has_aa_line_parameters)
      return;

   /* use legacy aa line coverage computation */
   memset(&balp, 0, sizeof(balp));
   balp.header.opcode = CMD_AA_LINE_PARAMETERS;
   balp.header.length = sizeof(balp) / 4 - 2;
   
   BRW_CACHED_BATCH_STRUCT(brw, &balp);
}

const struct brw_tracked_state brw_aa_line_parameters = {
   .dirty = {
      .mesa = _NEW_LINE,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_aa_line_parameters
};

/***********************************************************************
 * Line stipple packet
 */

static void upload_line_stipple(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct brw_line_stipple bls;
   GLfloat tmp;
   GLint tmpi;

   if (!ctx->Line.StippleFlag)
      return;

   memset(&bls, 0, sizeof(bls));
   bls.header.opcode = CMD_LINE_STIPPLE_PATTERN;
   bls.header.length = sizeof(bls)/4 - 2;

   bls.bits0.pattern = ctx->Line.StipplePattern;
   bls.bits1.repeat_count = ctx->Line.StippleFactor;

   tmp = 1.0 / (GLfloat) ctx->Line.StippleFactor;
   tmpi = tmp * (1<<13);


   bls.bits1.inverse_repeat_count = tmpi;

   BRW_CACHED_BATCH_STRUCT(brw, &bls);
}

const struct brw_tracked_state brw_line_stipple = {
   .dirty = {
      .mesa = _NEW_LINE,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_line_stipple
};


/***********************************************************************
 * Misc invarient state packets
 */

static void upload_invarient_state( struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;

   {
      /* 0x61040000  Pipeline Select */
      /*     PipelineSelect            : 0 */
      struct brw_pipeline_select ps;

      memset(&ps, 0, sizeof(ps));
      ps.header.opcode = brw->CMD_PIPELINE_SELECT;
      ps.header.pipeline_select = 0;
      BRW_BATCH_STRUCT(brw, &ps);
   }

   if (intel->gen < 6) {
      struct brw_global_depth_offset_clamp gdo;
      memset(&gdo, 0, sizeof(gdo));

      /* Disable depth offset clamping. 
       */
      gdo.header.opcode = CMD_GLOBAL_DEPTH_OFFSET_CLAMP;
      gdo.header.length = sizeof(gdo)/4 - 2;
      gdo.depth_offset_clamp = 0.0;

      BRW_BATCH_STRUCT(brw, &gdo);
   }

   if (intel->gen >= 6) {
      int i;

      BEGIN_BATCH(3);
      OUT_BATCH(CMD_3D_MULTISAMPLE << 16 | (3 - 2));
      OUT_BATCH(MS_PIXEL_LOCATION_CENTER |
		MS_NUMSAMPLES_1);
      OUT_BATCH(0); /* positions for 4/8-sample */
      ADVANCE_BATCH();

      BEGIN_BATCH(2);
      OUT_BATCH(CMD_3D_SAMPLE_MASK << 16 | (2 - 2));
      OUT_BATCH(1);
      ADVANCE_BATCH();

      for (i = 0; i < 4; i++) {
	 BEGIN_BATCH(4);
	 OUT_BATCH(CMD_GS_SVB_INDEX << 16 | (4 - 2));
	 OUT_BATCH(i << SVB_INDEX_SHIFT);
	 OUT_BATCH(0);
	 OUT_BATCH(0xffffffff);
	 ADVANCE_BATCH();
      }
   }

   /* 0x61020000  State Instruction Pointer */
   {
      struct brw_system_instruction_pointer sip;
      memset(&sip, 0, sizeof(sip));

      sip.header.opcode = CMD_STATE_INSN_POINTER;
      sip.header.length = 0;
      sip.bits0.pad = 0;
      sip.bits0.system_instruction_pointer = 0;
      BRW_BATCH_STRUCT(brw, &sip);
   }


   {
      struct brw_vf_statistics vfs;
      memset(&vfs, 0, sizeof(vfs));

      vfs.opcode = brw->CMD_VF_STATISTICS;
      if (unlikely(INTEL_DEBUG & DEBUG_STATS))
	 vfs.statistics_enable = 1; 

      BRW_BATCH_STRUCT(brw, &vfs);
   }
}

const struct brw_tracked_state brw_invarient_state = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_invarient_state
};

/**
 * Define the base addresses which some state is referenced from.
 *
 * This allows us to avoid having to emit relocations for the objects,
 * and is actually required for binding table pointers on gen6.
 *
 * Surface state base address covers binding table pointers and
 * surface state objects, but not the surfaces that the surface state
 * objects point to.
 */
static void upload_state_base_address( struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;

   if (intel->gen >= 6) {
       BEGIN_BATCH(10);
       OUT_BATCH(CMD_STATE_BASE_ADDRESS << 16 | (10 - 2));
       OUT_BATCH(1); /* General state base address */
       OUT_RELOC(intel->batch->buf, I915_GEM_DOMAIN_SAMPLER, 0,
		 1); /* Surface state base address */
       OUT_BATCH(1); /* Dynamic state base address */
       OUT_BATCH(1); /* Indirect object base address */
       OUT_BATCH(1); /* Instruction base address */
       OUT_BATCH(1); /* General state upper bound */
       OUT_BATCH(1); /* Dynamic state upper bound */
       OUT_BATCH(1); /* Indirect object upper bound */
       OUT_BATCH(1); /* Instruction access upper bound */
       ADVANCE_BATCH();
   } else if (intel->gen == 5) {
       BEGIN_BATCH(8);
       OUT_BATCH(CMD_STATE_BASE_ADDRESS << 16 | (8 - 2));
       OUT_BATCH(1); /* General state base address */
       OUT_RELOC(intel->batch->buf, I915_GEM_DOMAIN_SAMPLER, 0,
		 1); /* Surface state base address */
       OUT_BATCH(1); /* Indirect object base address */
       OUT_BATCH(1); /* Instruction base address */
       OUT_BATCH(1); /* General state upper bound */
       OUT_BATCH(1); /* Indirect object upper bound */
       OUT_BATCH(1); /* Instruction access upper bound */
       ADVANCE_BATCH();
   } else {
       BEGIN_BATCH(6);
       OUT_BATCH(CMD_STATE_BASE_ADDRESS << 16 | (6 - 2));
       OUT_BATCH(1); /* General state base address */
       OUT_RELOC(intel->batch->buf, I915_GEM_DOMAIN_SAMPLER, 0,
		 1); /* Surface state base address */
       OUT_BATCH(1); /* Indirect object base address */
       OUT_BATCH(1); /* General state upper bound */
       OUT_BATCH(1); /* Indirect object upper bound */
       ADVANCE_BATCH();
   }
}

const struct brw_tracked_state brw_state_base_address = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH,
      .cache = 0,
   },
   .emit = upload_state_base_address
};
