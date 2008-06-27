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

#include "brw_batch.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"





/***********************************************************************
 * Blend color
 */

static void upload_blend_constant_color(struct brw_context *brw)
{
   struct brw_blend_constant_color bcc;

   memset(&bcc, 0, sizeof(bcc));
   bcc.header.opcode = CMD_BLEND_CONSTANT_COLOR;
   bcc.header.length = sizeof(bcc)/4-2;
   bcc.blend_constant_color[0] = brw->attribs.BlendColor.color[0];
   bcc.blend_constant_color[1] = brw->attribs.BlendColor.color[1];
   bcc.blend_constant_color[2] = brw->attribs.BlendColor.color[2];
   bcc.blend_constant_color[3] = brw->attribs.BlendColor.color[3];

   BRW_CACHED_BATCH_STRUCT(brw, &bcc);
}


const struct brw_tracked_state brw_blend_constant_color = {
   .dirty = {
      .brw = BRW_NEW_BLEND,
      .cache = 0
   },
   .update = upload_blend_constant_color
};


/***********************************************************************
 * Drawing rectangle 
 */
static void upload_drawing_rect(struct brw_context *brw)
{
   struct brw_drawrect bdr;

   memset(&bdr, 0, sizeof(bdr));
   bdr.header.opcode = CMD_DRAW_RECT;
   bdr.header.length = sizeof(bdr)/4 - 2;
   bdr.xmin = 0;
   bdr.ymin = 0;
   bdr.xmax = brw->attribs.FrameBuffer.cbufs[0]->width;
   bdr.ymax = brw->attribs.FrameBuffer.cbufs[0]->height;
   bdr.xorg = 0;
   bdr.yorg = 0;

   /* Can't use BRW_CACHED_BATCH_STRUCT because this is also emitted
    * uncached in brw_draw.c:
    */
   BRW_BATCH_STRUCT(brw, &bdr);
}

const struct brw_tracked_state brw_drawing_rect = {
   .dirty = {
      .brw = BRW_NEW_SCENE,
      .cache = 0
   },
   .update = upload_drawing_rect
};

/**
 * Upload the binding table pointers, which point each stage's array of surface
 * state pointers.
 *
 * The binding table pointers are relative to the surface state base address,
 * which is the BRW_SS_POOL cache buffer.
 */
static void upload_binding_table_pointers(struct brw_context *brw)
{
   struct brw_binding_table_pointers btp;
   memset(&btp, 0, sizeof(btp));

   btp.header.opcode = CMD_BINDING_TABLE_PTRS;
   btp.header.length = sizeof(btp)/4 - 2;
   btp.vs = 0;
   btp.gs = 0;
   btp.clp = 0;
   btp.sf = 0;
   btp.wm = brw->wm.bind_ss_offset;

   BRW_CACHED_BATCH_STRUCT(brw, &btp);
}

const struct brw_tracked_state brw_binding_table_pointers = {
   .dirty = {
      .brw = 0,
      .cache = CACHE_NEW_SURF_BIND
   },
   .update = upload_binding_table_pointers,
};


/**
 * Upload pointers to the per-stage state.
 *
 * The state pointers in this packet are all relative to the general state
 * base address set by CMD_STATE_BASE_ADDRESS, which is the BRW_GS_POOL buffer.
 */
static void upload_pipelined_state_pointers(struct brw_context *brw )
{
   struct brw_pipelined_state_pointers psp;
   memset(&psp, 0, sizeof(psp));

   psp.header.opcode = CMD_PIPELINED_STATE_POINTERS;
   psp.header.length = sizeof(psp)/4 - 2;

   psp.vs.offset = brw->vs.state_gs_offset >> 5;
   psp.sf.offset = brw->sf.state_gs_offset >> 5;
   psp.wm.offset = brw->wm.state_gs_offset >> 5;
   psp.cc.offset = brw->cc.state_gs_offset >> 5;

   /* GS gets turned on and off regularly.  Need to re-emit URB fence
    * after this occurs.
    */
   if (brw->gs.prog_active) {
      psp.gs.offset = brw->gs.state_gs_offset >> 5;
      psp.gs.enable = 1;
   }

   if (0) {
      psp.clp.offset = brw->clip.state_gs_offset >> 5;
      psp.clp.enable = 1;
   }


   if (BRW_CACHED_BATCH_STRUCT(brw, &psp))
      brw->state.dirty.brw |= BRW_NEW_PSP;
}

const struct brw_tracked_state brw_pipelined_state_pointers = {
   .dirty = {
      .brw = 0,
      .cache = (CACHE_NEW_VS_UNIT |
		CACHE_NEW_GS_UNIT |
		CACHE_NEW_GS_PROG |
		CACHE_NEW_CLIP_UNIT |
		CACHE_NEW_SF_UNIT |
		CACHE_NEW_WM_UNIT |
		CACHE_NEW_CC_UNIT)
   },
   .update = upload_pipelined_state_pointers
};

static void upload_psp_urb_cbs(struct brw_context *brw )
{
   upload_pipelined_state_pointers(brw);
   brw_upload_urb_fence(brw);
   brw_upload_constant_buffer_state(brw);
}


const struct brw_tracked_state brw_psp_urb_cbs = {
   .dirty = {
      .brw = BRW_NEW_URB_FENCE,
      .cache = (CACHE_NEW_VS_UNIT |
		CACHE_NEW_GS_UNIT |
		CACHE_NEW_GS_PROG |
		CACHE_NEW_CLIP_UNIT |
		CACHE_NEW_SF_UNIT |
		CACHE_NEW_WM_UNIT |
		CACHE_NEW_CC_UNIT)
   },
   .update = upload_psp_urb_cbs
};

/**
 * Upload the depthbuffer offset and format.
 *
 * We have to do this per state validation as we need to emit the relocation
 * in the batch buffer.
 */
static void upload_depthbuffer(struct brw_context *brw)
{
   struct pipe_surface *depth_surface = brw->attribs.FrameBuffer.zsbuf;

   BEGIN_BATCH(5, INTEL_BATCH_NO_CLIPRECTS);
   OUT_BATCH(CMD_DEPTH_BUFFER << 16 | (5 - 2));
   if (depth_surface == NULL) {
      OUT_BATCH((BRW_DEPTHFORMAT_D32_FLOAT << 18) |
		(BRW_SURFACE_NULL << 29));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
   } else {
      unsigned int format;

      assert(depth_surface->block.width == 1);
      assert(depth_surface->block.height == 1);
      switch (depth_surface->block.size) {
      case 2:
	 format = BRW_DEPTHFORMAT_D16_UNORM;
	 break;
      case 4:
	 if (depth_surface->format == PIPE_FORMAT_Z32_FLOAT)
	    format = BRW_DEPTHFORMAT_D32_FLOAT;
	 else
	    format = BRW_DEPTHFORMAT_D24_UNORM_S8_UINT;
	 break;
      default:
	 assert(0);
	 return;
      }

      OUT_BATCH((depth_surface->stride - 1) |
		(format << 18) |
		(BRW_TILEWALK_YMAJOR << 26) |
//		(depth_surface->region->tiled << 27) |
		(BRW_SURFACE_2D << 29));
      OUT_RELOC(depth_surface->buffer,
		PIPE_BUFFER_USAGE_GPU_READ | PIPE_BUFFER_USAGE_GPU_WRITE, 0);
      OUT_BATCH((BRW_SURFACE_MIPMAPLAYOUT_BELOW << 1) |
		((depth_surface->stride/depth_surface->block.size - 1) << 6) |
		((depth_surface->height - 1) << 19));
      OUT_BATCH(0);
   }
   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_depthbuffer = {
   .dirty = {
      .brw = BRW_NEW_SCENE,
      .cache = 0
   },
   .update = upload_depthbuffer,
};




/***********************************************************************
 * Polygon stipple packet
 */

static void upload_polygon_stipple(struct brw_context *brw)
{
   struct brw_polygon_stipple bps;
   unsigned i;

   memset(&bps, 0, sizeof(bps));
   bps.header.opcode = CMD_POLY_STIPPLE_PATTERN;
   bps.header.length = sizeof(bps)/4-2;

   /* XXX: state tracker should send *all* state down initially!
    */
   if (brw->attribs.PolygonStipple)
      for (i = 0; i < 32; i++)
	 bps.stipple[i] = brw->attribs.PolygonStipple->stipple[31 - i]; /* invert */

   BRW_CACHED_BATCH_STRUCT(brw, &bps);
}

const struct brw_tracked_state brw_polygon_stipple = {
   .dirty = {
      .brw = BRW_NEW_STIPPLE,
      .cache = 0
   },
   .update = upload_polygon_stipple
};


/***********************************************************************
 * Line stipple packet
 */

static void upload_line_stipple(struct brw_context *brw)
{
   struct brw_line_stipple bls;
   float tmp;
   int tmpi;

   memset(&bls, 0, sizeof(bls));
   bls.header.opcode = CMD_LINE_STIPPLE_PATTERN;
   bls.header.length = sizeof(bls)/4 - 2;

   bls.bits0.pattern = brw->attribs.Raster->line_stipple_pattern;
   bls.bits1.repeat_count = brw->attribs.Raster->line_stipple_factor;

   tmp = 1.0 / (float) brw->attribs.Raster->line_stipple_factor;
   tmpi = tmp * (1<<13);


   bls.bits1.inverse_repeat_count = tmpi;

   BRW_CACHED_BATCH_STRUCT(brw, &bls);
}

const struct brw_tracked_state brw_line_stipple = {
   .dirty = {
      .brw = BRW_NEW_STIPPLE,
      .cache = 0
   },
   .update = upload_line_stipple
};


/***********************************************************************
 * Misc constant state packets
 */

static void upload_pipe_control(struct brw_context *brw)
{
   struct brw_pipe_control pc;

   return;

   memset(&pc, 0, sizeof(pc));

   pc.header.opcode = CMD_PIPE_CONTROL;
   pc.header.length = sizeof(pc)/4 - 2;
   pc.header.post_sync_operation = PIPE_CONTROL_NOWRITE;

   pc.header.instruction_state_cache_flush_enable = 1;

   pc.bits1.dest_addr_type = PIPE_CONTROL_GTTWRITE_GLOBAL;

   BRW_BATCH_STRUCT(brw, &pc);
}

const struct brw_tracked_state brw_pipe_control = {
   .dirty = {
      .brw = BRW_NEW_SCENE,
      .cache = 0
   },
   .update = upload_pipe_control
};


/***********************************************************************
 * Misc invarient state packets
 */

static void upload_invarient_state( struct brw_context *brw )
{
   {
      struct brw_mi_flush flush;

      memset(&flush, 0, sizeof(flush));      
      flush.opcode = CMD_MI_FLUSH;
      flush.flags = BRW_FLUSH_STATE_CACHE | BRW_FLUSH_READ_CACHE;
      BRW_BATCH_STRUCT(brw, &flush);
   }

   {
      /* 0x61040000  Pipeline Select */
      /*     PipelineSelect            : 0 */
      struct brw_pipeline_select ps;

      memset(&ps, 0, sizeof(ps));
      ps.header.opcode = CMD_PIPELINE_SELECT;
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

      vfs.opcode = CMD_VF_STATISTICS;
      if (BRW_DEBUG & DEBUG_STATS)
	 vfs.statistics_enable = 1;

      BRW_BATCH_STRUCT(brw, &vfs);
   }

   
   {
      struct brw_polygon_stipple_offset bpso;
      
      memset(&bpso, 0, sizeof(bpso));
      bpso.header.opcode = CMD_POLY_STIPPLE_OFFSET;
      bpso.header.length = sizeof(bpso)/4-2;      
      bpso.bits0.x_offset = 0;
      bpso.bits0.y_offset = 0;

      BRW_BATCH_STRUCT(brw, &bpso);
   }
}

const struct brw_tracked_state brw_invarient_state = {
   .dirty = {
      .brw = BRW_NEW_SCENE,
      .cache = 0
   },
   .update = upload_invarient_state
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
   /* Output the structure (brw_state_base_address) directly to the
    * batchbuffer, so we can emit relocations inline.
    */
   BEGIN_BATCH(6, INTEL_BATCH_NO_CLIPRECTS);
   OUT_BATCH(CMD_STATE_BASE_ADDRESS << 16 | (6 - 2));
   OUT_RELOC(brw->pool[BRW_GS_POOL].buffer,
	     PIPE_BUFFER_USAGE_GPU_READ,
	     1); /* General state base address */
   OUT_RELOC(brw->pool[BRW_SS_POOL].buffer,
	     PIPE_BUFFER_USAGE_GPU_READ,
	     1); /* Surface state base address */
   OUT_BATCH(1); /* Indirect object base address */
   OUT_BATCH(1); /* General state upper bound */
   OUT_BATCH(1); /* Indirect object upper bound */
   ADVANCE_BATCH();
}


const struct brw_tracked_state brw_state_base_address = {
   .dirty = {
      .brw = BRW_NEW_SCENE,
      .cache = 0
   },
   .update = upload_state_base_address
};
