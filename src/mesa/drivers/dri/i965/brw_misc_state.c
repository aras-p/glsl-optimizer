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





/***********************************************************************
 * Blend color
 */

static void upload_blend_constant_color(struct brw_context *brw)
{
   GLcontext *ctx = &brw->intel.ctx;
   struct brw_blend_constant_color bcc;

   memset(&bcc, 0, sizeof(bcc));      
   bcc.header.opcode = CMD_BLEND_CONSTANT_COLOR;
   bcc.header.length = sizeof(bcc)/4-2;
   bcc.blend_constant_color[0] = ctx->Color.BlendColor[0];
   bcc.blend_constant_color[1] = ctx->Color.BlendColor[1];
   bcc.blend_constant_color[2] = ctx->Color.BlendColor[2];
   bcc.blend_constant_color[3] = ctx->Color.BlendColor[3];

   BRW_CACHED_BATCH_STRUCT(brw, &bcc);
}


const struct brw_tracked_state brw_blend_constant_color = {
   .dirty = {
      .mesa = _NEW_COLOR,
      .brw = 0,
      .cache = 0
   },
   .emit = upload_blend_constant_color
};

/* Constant single cliprect for framebuffer object or DRI2 drawing */
static void upload_drawing_rect(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   GLcontext *ctx = &intel->ctx;

   if (!intel->constant_cliprect)
      return;

   BEGIN_BATCH(4, NO_LOOP_CLIPRECTS);
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
      .brw = 0,
      .cache = 0
   },
   .emit = upload_drawing_rect
};

static void prepare_binding_table_pointers(struct brw_context *brw)
{
   brw_add_validated_bo(brw, brw->wm.bind_bo);
}

/**
 * Upload the binding table pointers, which point each stage's array of surface
 * state pointers.
 *
 * The binding table pointers are relative to the surface state base address,
 * which is 0.
 */
static void upload_binding_table_pointers(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(6, IGNORE_CLIPRECTS);
   OUT_BATCH(CMD_BINDING_TABLE_PTRS << 16 | (6 - 2));
   OUT_BATCH(0); /* vs */
   OUT_BATCH(0); /* gs */
   OUT_BATCH(0); /* clip */
   OUT_BATCH(0); /* sf */
   OUT_RELOC(brw->wm.bind_bo,
	     I915_GEM_DOMAIN_SAMPLER, 0,
	     0);
   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_binding_table_pointers = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH,
      .cache = CACHE_NEW_SURF_BIND,
   },
   .prepare = prepare_binding_table_pointers,
   .emit = upload_binding_table_pointers,
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

   BEGIN_BATCH(7, IGNORE_CLIPRECTS);
   OUT_BATCH(CMD_PIPELINED_STATE_POINTERS << 16 | (7 - 2));
   OUT_RELOC(brw->vs.state_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
   if (brw->gs.prog_active)
      OUT_RELOC(brw->gs.state_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 1);
   else
      OUT_BATCH(0);
   OUT_RELOC(brw->clip.state_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 1);
   OUT_RELOC(brw->sf.state_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
   OUT_RELOC(brw->wm.state_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
   OUT_RELOC(brw->cc.state_bo, I915_GEM_DOMAIN_INSTRUCTION, 0, 0);
   ADVANCE_BATCH();

   brw->state.dirty.brw |= BRW_NEW_PSP;
}


static void prepare_psp_urb_cbs(struct brw_context *brw)
{
   brw_add_validated_bo(brw, brw->vs.state_bo);
   brw_add_validated_bo(brw, brw->gs.state_bo);
   brw_add_validated_bo(brw, brw->clip.state_bo);
   brw_add_validated_bo(brw, brw->wm.state_bo);
   brw_add_validated_bo(brw, brw->cc.state_bo);
}

static void upload_psp_urb_cbs(struct brw_context *brw )
{
   upload_pipelined_state_pointers(brw);
   brw_upload_urb_fence(brw);
   brw_upload_constant_buffer_state(brw);
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
   unsigned int len = BRW_IS_G4X(brw) ? 6 : 5;

   if (region == NULL) {
      BEGIN_BATCH(len, IGNORE_CLIPRECTS);
      OUT_BATCH(CMD_DEPTH_BUFFER << 16 | (len - 2));
      OUT_BATCH((BRW_DEPTHFORMAT_D32_FLOAT << 18) |
		(BRW_SURFACE_NULL << 29));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);

      if (BRW_IS_G4X(brw))
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

      BEGIN_BATCH(len, IGNORE_CLIPRECTS);
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
		((region->pitch - 1) << 6) |
		((region->height - 1) << 19));
      OUT_BATCH(0);

      if (BRW_IS_G4X(brw))
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
   GLcontext *ctx = &brw->intel.ctx;
   struct brw_polygon_stipple bps;
   GLuint i;

   memset(&bps, 0, sizeof(bps));
   bps.header.opcode = CMD_POLY_STIPPLE_PATTERN;
   bps.header.length = sizeof(bps)/4-2;

   for (i = 0; i < 32; i++)
      bps.stipple[i] = ctx->PolygonStipple[31 - i]; /* invert */

   BRW_CACHED_BATCH_STRUCT(brw, &bps);
}

const struct brw_tracked_state brw_polygon_stipple = {
   .dirty = {
      .mesa = _NEW_POLYGONSTIPPLE,
      .brw = 0,
      .cache = 0
   },
   .emit = upload_polygon_stipple
};


/***********************************************************************
 * Polygon stipple offset packet
 */

static void upload_polygon_stipple_offset(struct brw_context *brw)
{
   __DRIdrawablePrivate *dPriv = brw->intel.driDrawable;
   struct brw_polygon_stipple_offset bpso;

   memset(&bpso, 0, sizeof(bpso));
   bpso.header.opcode = CMD_POLY_STIPPLE_OFFSET;
   bpso.header.length = sizeof(bpso)/4-2;

   bpso.bits0.x_offset = (32 - (dPriv->x & 31)) & 31;
   bpso.bits0.y_offset = (32 - ((dPriv->y + dPriv->h) & 31)) & 31;

   BRW_CACHED_BATCH_STRUCT(brw, &bpso);
}

#define _NEW_WINDOW_POS 0x40000000

const struct brw_tracked_state brw_polygon_stipple_offset = {
   .dirty = {
      .mesa = _NEW_WINDOW_POS,
      .brw = 0,
      .cache = 0
   },
   .emit = upload_polygon_stipple_offset
};

/**********************************************************************
 * AA Line parameters
 */
static void upload_aa_line_parameters(struct brw_context *brw)
{
   struct brw_aa_line_parameters balp;
   
   if (!BRW_IS_G4X(brw))
      return;

   /* use legacy aa line coverage computation */
   memset(&balp, 0, sizeof(balp));
   balp.header.opcode = CMD_AA_LINE_PARAMETERS;
   balp.header.length = sizeof(balp) / 4 - 2;
   
   BRW_CACHED_BATCH_STRUCT(brw, &balp);
}

const struct brw_tracked_state brw_aa_line_parameters = {
   .dirty = {
      .mesa = 0,
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
   GLcontext *ctx = &brw->intel.ctx;
   struct brw_line_stipple bls;
   GLfloat tmp;
   GLint tmpi;

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
      .brw = 0,
      .cache = 0
   },
   .emit = upload_line_stipple
};


/***********************************************************************
 * Misc invarient state packets
 */

static void upload_invarient_state( struct brw_context *brw )
{
   {
      /* 0x61040000  Pipeline Select */
      /*     PipelineSelect            : 0 */
      struct brw_pipeline_select ps;

      memset(&ps, 0, sizeof(ps));
      ps.header.opcode = CMD_PIPELINE_SELECT(brw);
      ps.header.pipeline_select = 0;
      BRW_BATCH_STRUCT(brw, &ps);
   }

   {
      struct brw_global_depth_offset_clamp gdo;
      memset(&gdo, 0, sizeof(gdo));

      /* Disable depth offset clamping. 
       */
      gdo.header.opcode = CMD_GLOBAL_DEPTH_OFFSET_CLAMP;
      gdo.header.length = sizeof(gdo)/4 - 2;
      gdo.depth_offset_clamp = 0.0;

      BRW_BATCH_STRUCT(brw, &gdo);
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

      vfs.opcode = CMD_VF_STATISTICS(brw);
      if (INTEL_DEBUG & DEBUG_STATS)
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
 * This allows us to avoid having to emit relocations in many places for
 * cached state, and instead emit pointers inside of large, mostly-static
 * state pools.  This comes at the expense of memory, and more expensive cache
 * misses.
 */
static void upload_state_base_address( struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;

   /* Output the structure (brw_state_base_address) directly to the
    * batchbuffer, so we can emit relocations inline.
    */
   BEGIN_BATCH(6, IGNORE_CLIPRECTS);
   OUT_BATCH(CMD_STATE_BASE_ADDRESS << 16 | (6 - 2));
   OUT_BATCH(1); /* General state base address */
   OUT_BATCH(1); /* Surface state base address */
   OUT_BATCH(1); /* Indirect object base address */
   OUT_BATCH(1); /* General state upper bound */
   OUT_BATCH(1); /* Indirect object upper bound */
   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_state_base_address = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0,
   },
   .emit = upload_state_base_address
};
