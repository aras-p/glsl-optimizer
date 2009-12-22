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
                  

#include "brw_wm.h"
#include "brw_debug.h"


static GLuint get_tracked_mask(struct brw_wm_compile *c,
			       struct brw_wm_instruction *inst)
{
   GLuint i;
   for (i = 0; i < 4; i++) {
      if (inst->writemask & (1<<i)) {
	 if (!inst->dst[i]->contributes_to_output) {
	    inst->writemask &= ~(1<<i);
	    inst->dst[i] = 0;
	 }
      }
   }

   return inst->writemask;
}

/* Remove a reference from a value's usage chain.
 */
static void unlink_ref(struct brw_wm_ref *ref)
{
   struct brw_wm_value *value = ref->value;

   if (ref == value->lastuse) {
      value->lastuse = ref->prevuse;
   }
   else {
      struct brw_wm_ref *i = value->lastuse;
      while (i->prevuse != ref) i = i->prevuse;
      i->prevuse = ref->prevuse;
   }
}

static void track_arg(struct brw_wm_compile *c,
		      struct brw_wm_instruction *inst,
		      GLuint arg,
		      GLuint readmask)
{
   GLuint i;

   for (i = 0; i < 4; i++) {
      struct brw_wm_ref *ref = inst->src[arg][i];
      if (ref) {
	 if (readmask & (1<<i)) {
	    ref->value->contributes_to_output = 1;
         }
	 else {
	    unlink_ref(ref);
	    inst->src[arg][i] = NULL;
	 }
      }
   }
}

static GLuint get_texcoord_mask( GLuint tex_idx )
{
   switch (tex_idx) {
   case TGSI_TEXTURE_1D:
      return BRW_WRITEMASK_X;
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
      return BRW_WRITEMASK_XY;
   case TGSI_TEXTURE_3D:
      return BRW_WRITEMASK_XYZ;
   case TGSI_TEXTURE_CUBE:
      return BRW_WRITEMASK_XYZ;

   case TGSI_TEXTURE_SHADOW1D:
      return BRW_WRITEMASK_XZ;
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
      return BRW_WRITEMASK_XYZ;
   default: 
      assert(0);
      return 0;
   }
}


/* Step two: Basically this is dead code elimination.  
 *
 * Iterate backwards over instructions, noting which values
 * contribute to the final result.  Adjust writemasks to only
 * calculate these values.
 */
void brw_wm_pass1( struct brw_wm_compile *c )
{
   GLint insn;

   for (insn = c->nr_insns-1; insn >= 0; insn--) {
      struct brw_wm_instruction *inst = &c->instruction[insn];
      GLuint writemask;
      GLuint read0, read1, read2;

      if (inst->opcode == TGSI_OPCODE_KIL) {
	 track_arg(c, inst, 0, BRW_WRITEMASK_XYZW); /* All args contribute to final */
	 continue;
      }

      if (inst->opcode == WM_FB_WRITE) {
	 track_arg(c, inst, 0, BRW_WRITEMASK_XYZW); 
	 track_arg(c, inst, 1, BRW_WRITEMASK_XYZW); 
	 if (c->key.source_depth_to_render_target &&
	     c->key.computes_depth)
	    track_arg(c, inst, 2, BRW_WRITEMASK_Z); 
	 else
	    track_arg(c, inst, 2, 0); 
	 continue;
      }

      /* Lookup all the registers which were written by this
       * instruction and get a mask of those that contribute to the output:
       */
      writemask = get_tracked_mask(c, inst);
      if (!writemask) {
	 GLuint arg;
	 for (arg = 0; arg < 3; arg++)
	    track_arg(c, inst, arg, 0);
	 continue;
      }

      read0 = 0;
      read1 = 0;
      read2 = 0;

      /* Mark all inputs which contribute to the marked outputs:
       */
      switch (inst->opcode) {
      case TGSI_OPCODE_ABS:
      case TGSI_OPCODE_FLR:
      case TGSI_OPCODE_FRC:
      case TGSI_OPCODE_MOV:
      case TGSI_OPCODE_TRUNC:
	 read0 = writemask;
	 break;

      case TGSI_OPCODE_SUB:
      case TGSI_OPCODE_SLT:
      case TGSI_OPCODE_SLE:
      case TGSI_OPCODE_SGE:
      case TGSI_OPCODE_SGT:
      case TGSI_OPCODE_SEQ:
      case TGSI_OPCODE_SNE:
      case TGSI_OPCODE_ADD:
      case TGSI_OPCODE_MAX:
      case TGSI_OPCODE_MIN:
      case TGSI_OPCODE_MUL:
	 read0 = writemask;
	 read1 = writemask;
	 break;

      case TGSI_OPCODE_DDX:
      case TGSI_OPCODE_DDY:
	 read0 = writemask;
	 break;

      case TGSI_OPCODE_MAD:	
      case TGSI_OPCODE_CMP:
      case TGSI_OPCODE_LRP:
	 read0 = writemask;
	 read1 = writemask;	
	 read2 = writemask;	
	 break;

      case TGSI_OPCODE_XPD: 
	 if (writemask & BRW_WRITEMASK_X) read0 |= BRW_WRITEMASK_YZ;	 
	 if (writemask & BRW_WRITEMASK_Y) read0 |= BRW_WRITEMASK_XZ;	 
	 if (writemask & BRW_WRITEMASK_Z) read0 |= BRW_WRITEMASK_XY;
	 read1 = read0;
	 break;

      case TGSI_OPCODE_COS:
      case TGSI_OPCODE_EX2:
      case TGSI_OPCODE_LG2:
      case TGSI_OPCODE_RCP:
      case TGSI_OPCODE_RSQ:
      case TGSI_OPCODE_SIN:
      case TGSI_OPCODE_SCS:
      case WM_CINTERP:
      case WM_PIXELXY:
	 read0 = BRW_WRITEMASK_X;
	 break;

      case TGSI_OPCODE_POW:
	 read0 = BRW_WRITEMASK_X;
	 read1 = BRW_WRITEMASK_X;
	 break;

      case TGSI_OPCODE_TEX:
      case TGSI_OPCODE_TXP:
	 read0 = get_texcoord_mask(inst->target);
	 break;

      case TGSI_OPCODE_TXB:
	 read0 = get_texcoord_mask(inst->target) | BRW_WRITEMASK_W;
	 break;

      case WM_WPOSXY:
	 read0 = writemask & BRW_WRITEMASK_XY;
	 break;

      case WM_DELTAXY:
	 read0 = writemask & BRW_WRITEMASK_XY;
	 read1 = BRW_WRITEMASK_X;
	 break;

      case WM_PIXELW:
	 read0 = BRW_WRITEMASK_X;
	 read1 = BRW_WRITEMASK_XY;
	 break;

      case WM_LINTERP:
	 read0 = BRW_WRITEMASK_X;
	 read1 = BRW_WRITEMASK_XY;
	 break;

      case WM_PINTERP:
	 read0 = BRW_WRITEMASK_X; /* interpolant */
	 read1 = BRW_WRITEMASK_XY; /* deltas */
	 read2 = BRW_WRITEMASK_W; /* pixel w */
	 break;

      case TGSI_OPCODE_DP3:	
	 read0 = BRW_WRITEMASK_XYZ;
	 read1 = BRW_WRITEMASK_XYZ;
	 break;

      case TGSI_OPCODE_DPH:
	 read0 = BRW_WRITEMASK_XYZ;
	 read1 = BRW_WRITEMASK_XYZW;
	 break;

      case TGSI_OPCODE_DP4:
	 read0 = BRW_WRITEMASK_XYZW;
	 read1 = BRW_WRITEMASK_XYZW;
	 break;

      case TGSI_OPCODE_LIT: 
	 read0 = BRW_WRITEMASK_XYW;
	 break;

      case TGSI_OPCODE_DST:
      case WM_FRONTFACING:
      case TGSI_OPCODE_KILP:
      default:
	 break;
      }

      track_arg(c, inst, 0, read0);
      track_arg(c, inst, 1, read1);
      track_arg(c, inst, 2, read2);
   }

   if (BRW_DEBUG & DEBUG_WM) {
      brw_wm_print_program(c, "pass1");
   }
}
