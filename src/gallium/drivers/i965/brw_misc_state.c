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
 


#include "brw_debug.h"
#include "brw_batchbuffer.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_screen.h"
#include "brw_pipe_rast.h"





/***********************************************************************
 * Blend color
 */

static int upload_blend_constant_color(struct brw_context *brw)
{
   BRW_CACHED_BATCH_STRUCT(brw, &brw->curr.bcc);
   return 0;
}


const struct brw_tracked_state brw_blend_constant_color = {
   .dirty = {
      .mesa = PIPE_NEW_BLEND_COLOR,
      .brw = 0,
      .cache = 0
   },
   .emit = upload_blend_constant_color
};

/***********************************************************************
 * Drawing rectangle - framebuffer dimensions
 */
static int upload_drawing_rect(struct brw_context *brw)
{
   BEGIN_BATCH(4, NO_LOOP_CLIPRECTS);
   OUT_BATCH(_3DSTATE_DRAWRECT_INFO_I965);
   OUT_BATCH(0);
   OUT_BATCH(((brw->curr.fb.width - 1) & 0xffff) |
	    ((brw->curr.fb.height - 1) << 16));
   OUT_BATCH(0);
   ADVANCE_BATCH();
   return 0;
}

const struct brw_tracked_state brw_drawing_rect = {
   .dirty = {
      .mesa = PIPE_NEW_FRAMEBUFFER_DIMENSIONS,
      .brw = 0,
      .cache = 0
   },
   .emit = upload_drawing_rect
};


/***********************************************************************
 * Binding table pointers
 */

static int prepare_binding_table_pointers(struct brw_context *brw)
{
   brw_add_validated_bo(brw, brw->vs.bind_bo);
   brw_add_validated_bo(brw, brw->wm.bind_bo);
   return 0;
}

/**
 * Upload the binding table pointers, which point each stage's array of surface
 * state pointers.
 *
 * The binding table pointers are relative to the surface state base address,
 * which is 0.
 */
static int upload_binding_table_pointers(struct brw_context *brw)
{
   BEGIN_BATCH(6, IGNORE_CLIPRECTS);
   OUT_BATCH(CMD_BINDING_TABLE_PTRS << 16 | (6 - 2));
   if (brw->vs.bind_bo != NULL)
      OUT_RELOC(brw->vs.bind_bo, 
		BRW_USAGE_SAMPLER,
		0); /* vs */
   else
      OUT_BATCH(0);
   OUT_BATCH(0); /* gs */
   OUT_BATCH(0); /* clip */
   OUT_BATCH(0); /* sf */
   OUT_RELOC(brw->wm.bind_bo,
	     BRW_USAGE_SAMPLER,
	     0); /* wm/ps */
   ADVANCE_BATCH();
   return 0;
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


/**********************************************************************
 * Upload pointers to the per-stage state.
 *
 * The state pointers in this packet are all relative to the general state
 * base address set by CMD_STATE_BASE_ADDRESS, which is 0.
 */
static int upload_pipelined_state_pointers(struct brw_context *brw )
{
   BEGIN_BATCH(7, IGNORE_CLIPRECTS);
   OUT_BATCH(CMD_PIPELINED_STATE_POINTERS << 16 | (7 - 2));
   OUT_RELOC(brw->vs.state_bo, 
	     BRW_USAGE_STATE,
	     0);
   if (brw->gs.prog_active)
      OUT_RELOC(brw->gs.state_bo, 
		BRW_USAGE_STATE,
		1);
   else
      OUT_BATCH(0);
   OUT_RELOC(brw->clip.state_bo, 
	     BRW_USAGE_STATE,
	     1);
   OUT_RELOC(brw->sf.state_bo,
	     BRW_USAGE_STATE,
	     0);
   OUT_RELOC(brw->wm.state_bo,
	     BRW_USAGE_STATE,
	     0);
   OUT_RELOC(brw->cc.state_bo,
	     BRW_USAGE_STATE,
	     0);
   ADVANCE_BATCH();

   brw->state.dirty.brw |= BRW_NEW_PSP;
   return 0;
}


static int prepare_psp_urb_cbs(struct brw_context *brw)
{
   brw_add_validated_bo(brw, brw->vs.state_bo);
   brw_add_validated_bo(brw, brw->gs.state_bo);
   brw_add_validated_bo(brw, brw->clip.state_bo);
   brw_add_validated_bo(brw, brw->sf.state_bo);
   brw_add_validated_bo(brw, brw->wm.state_bo);
   brw_add_validated_bo(brw, brw->cc.state_bo);
   return 0;
}

static int upload_psp_urb_cbs(struct brw_context *brw )
{
   int ret;
   
   ret = upload_pipelined_state_pointers(brw);
   if (ret)
      return ret;

   ret = brw_upload_urb_fence(brw);
   if (ret)
      return ret;

   ret = brw_upload_cs_urb_state(brw);
   if (ret)
      return ret;

   return 0;
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


/***********************************************************************
 * Depth buffer 
 */

static int prepare_depthbuffer(struct brw_context *brw)
{
   struct pipe_surface *zsbuf = brw->curr.fb.zsbuf;

   if (zsbuf)
      brw_add_validated_bo(brw, brw_surface(zsbuf)->bo);

   return 0;
}

static int emit_depthbuffer(struct brw_context *brw)
{
   struct pipe_surface *surface = brw->curr.fb.zsbuf;
   unsigned int len = (BRW_IS_G4X(brw) || BRW_IS_IGDNG(brw)) ? 6 : 5;

   if (surface == NULL) {
      BEGIN_BATCH(len, IGNORE_CLIPRECTS);
      OUT_BATCH(CMD_DEPTH_BUFFER << 16 | (len - 2));
      OUT_BATCH((BRW_DEPTHFORMAT_D32_FLOAT << 18) |
		(BRW_SURFACE_NULL << 29));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);

      if (BRW_IS_G4X(brw) || BRW_IS_IGDNG(brw))
         OUT_BATCH(0);

      ADVANCE_BATCH();
   } else {
      struct brw_winsys_buffer *bo;
      unsigned int format;
      unsigned int pitch;
      unsigned int cpp;

      switch (surface->format) {
      case PIPE_FORMAT_Z16_UNORM:
	 format = BRW_DEPTHFORMAT_D16_UNORM;
	 cpp = 2;
	 break;
      case PIPE_FORMAT_Z24X8_UNORM:
      case PIPE_FORMAT_Z24S8_UNORM:
	 format = BRW_DEPTHFORMAT_D24_UNORM_S8_UINT;
	 cpp = 4;
	 break;
      case PIPE_FORMAT_Z32_FLOAT:
	 format = BRW_DEPTHFORMAT_D32_FLOAT;
	 cpp = 4;
	 break;
      default:
	 assert(0);
	 return PIPE_ERROR_BAD_INPUT;
      }

      bo = brw_surface(surface)->bo;
      pitch = brw_surface(surface)->pitch;

      BEGIN_BATCH(len, IGNORE_CLIPRECTS);
      OUT_BATCH(CMD_DEPTH_BUFFER << 16 | (len - 2));
      OUT_BATCH(((pitch * cpp) - 1) |
		(format << 18) |
		(BRW_TILEWALK_YMAJOR << 26) |
		((surface->layout != PIPE_SURFACE_LAYOUT_LINEAR) << 27) |
		(BRW_SURFACE_2D << 29));
      OUT_RELOC(bo,
		BRW_USAGE_DEPTH_BUFFER,
		surface->offset);
      OUT_BATCH((BRW_SURFACE_MIPMAPLAYOUT_BELOW << 1) |
		((pitch - 1) << 6) |
		((surface->height - 1) << 19));
      OUT_BATCH(0);

      if (BRW_IS_G4X(brw) || BRW_IS_IGDNG(brw))
         OUT_BATCH(0);

      ADVANCE_BATCH();
   }

   return 0;
}

const struct brw_tracked_state brw_depthbuffer = {
   .dirty = {
      .mesa = PIPE_NEW_DEPTH_BUFFER,
      .brw = BRW_NEW_BATCH,
      .cache = 0,
   },
   .prepare = prepare_depthbuffer,
   .emit = emit_depthbuffer,
};



/***********************************************************************
 * Polygon stipple packet
 */

static int upload_polygon_stipple(struct brw_context *brw)
{
   BRW_CACHED_BATCH_STRUCT(brw, &brw->curr.bps);
   return 0;
}

const struct brw_tracked_state brw_polygon_stipple = {
   .dirty = {
      .mesa = PIPE_NEW_POLYGON_STIPPLE,
      .brw = 0,
      .cache = 0
   },
   .emit = upload_polygon_stipple
};


/***********************************************************************
 * Line stipple packet
 */

static int upload_line_stipple(struct brw_context *brw)
{
   const struct brw_line_stipple *bls = &brw->curr.rast->bls;
   if (bls->header.opcode) {
      BRW_CACHED_BATCH_STRUCT(brw, bls);
   }
   return 0;
}

const struct brw_tracked_state brw_line_stipple = {
   .dirty = {
      .mesa = PIPE_NEW_RAST,
      .brw = 0,
      .cache = 0
   },
   .emit = upload_line_stipple
};


/***********************************************************************
 * Misc invarient state packets
 */

static int upload_invarient_state( struct brw_context *brw )
{
   {
      /* 0x61040000  Pipeline Select */
      /*     PipelineSelect            : 0 */
      struct brw_pipeline_select ps;

      memset(&ps, 0, sizeof(ps));
      if (BRW_IS_G4X(brw) || BRW_IS_IGDNG(brw))
	 ps.header.opcode = CMD_PIPELINE_SELECT_GM45;
      else
	 ps.header.opcode = CMD_PIPELINE_SELECT_965;
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

   /* VF Statistics */
   {
      struct brw_vf_statistics vfs;
      memset(&vfs, 0, sizeof(vfs));

      if (BRW_IS_G4X(brw) || BRW_IS_IGDNG(brw)) 
	 vfs.opcode = CMD_VF_STATISTICS_GM45;
      else 
	 vfs.opcode = CMD_VF_STATISTICS_965;

      if (BRW_DEBUG & DEBUG_STATS)
	 vfs.statistics_enable = 1; 

      BRW_BATCH_STRUCT(brw, &vfs);
   }
   
   if (!BRW_IS_965(brw))
   {
      struct brw_aa_line_parameters balp;

      /* use legacy aa line coverage computation */
      memset(&balp, 0, sizeof(balp));
      balp.header.opcode = CMD_AA_LINE_PARAMETERS;
      balp.header.length = sizeof(balp) / 4 - 2;
   
      BRW_BATCH_STRUCT(brw, &balp);
   }

   {
      struct brw_polygon_stipple_offset bpso;
      
      /* This is invarient state in gallium:
       */
      memset(&bpso, 0, sizeof(bpso));
      bpso.header.opcode = CMD_POLY_STIPPLE_OFFSET;
      bpso.header.length = sizeof(bpso)/4-2;
      bpso.bits0.y_offset = 0;
      bpso.bits0.x_offset = 0;

      BRW_BATCH_STRUCT(brw, &bpso);
   }
   
   return 0;
}

const struct brw_tracked_state brw_invarient_state = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_invarient_state
};


/***********************************************************************
 * State base address 
 */

/**
 * Define the base addresses which some state is referenced from.
 *
 * This allows us to avoid having to emit relocations in many places for
 * cached state, and instead emit pointers inside of large, mostly-static
 * state pools.  This comes at the expense of memory, and more expensive cache
 * misses.
 */
static int upload_state_base_address( struct brw_context *brw )
{
   /* Output the structure (brw_state_base_address) directly to the
    * batchbuffer, so we can emit relocations inline.
    */
   if (BRW_IS_IGDNG(brw)) {
       BEGIN_BATCH(8, IGNORE_CLIPRECTS);
       OUT_BATCH(CMD_STATE_BASE_ADDRESS << 16 | (8 - 2));
       OUT_BATCH(1); /* General state base address */
       OUT_BATCH(1); /* Surface state base address */
       OUT_BATCH(1); /* Indirect object base address */
       OUT_BATCH(1); /* Instruction base address */
       OUT_BATCH(1); /* General state upper bound */
       OUT_BATCH(1); /* Indirect object upper bound */
       OUT_BATCH(1); /* Instruction access upper bound */
       ADVANCE_BATCH();
   } else {
       BEGIN_BATCH(6, IGNORE_CLIPRECTS);
       OUT_BATCH(CMD_STATE_BASE_ADDRESS << 16 | (6 - 2));
       OUT_BATCH(1); /* General state base address */
       OUT_BATCH(1); /* Surface state base address */
       OUT_BATCH(1); /* Indirect object base address */
       OUT_BATCH(1); /* General state upper bound */
       OUT_BATCH(1); /* Indirect object upper bound */
       ADVANCE_BATCH();
   }
   return 0;
}

const struct brw_tracked_state brw_state_base_address = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0,
   },
   .emit = upload_state_base_address
};
