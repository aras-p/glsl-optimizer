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

#include "tgsi/tgsi_info.h"

#include "brw_context.h"
#include "brw_wm.h"

static void print_writemask( unsigned writemask )
{
   if (writemask != BRW_WRITEMASK_XYZW)
      debug_printf(".%s%s%s%s", 
		   (writemask & BRW_WRITEMASK_X) ? "x" : "",
		   (writemask & BRW_WRITEMASK_Y) ? "y" : "",
		   (writemask & BRW_WRITEMASK_Z) ? "z" : "",
		   (writemask & BRW_WRITEMASK_W) ? "w" : "");
}

static void print_swizzle( unsigned swizzle )
{
   char *swz = "xyzw";
   if (swizzle != BRW_SWIZZLE_XYZW)
      debug_printf(".%c%c%c%c", 
		   swz[BRW_GET_SWZ(swizzle, X)],
		   swz[BRW_GET_SWZ(swizzle, Y)],
		   swz[BRW_GET_SWZ(swizzle, Z)],
		   swz[BRW_GET_SWZ(swizzle, W)]);
}

static void print_opcode( unsigned opcode )
{
   switch (opcode) {
   case WM_PIXELXY:
      debug_printf("PIXELXY");
      break;
   case WM_DELTAXY:
      debug_printf("DELTAXY");
      break;
   case WM_PIXELW:
      debug_printf("PIXELW");
      break;
   case WM_WPOSXY:
      debug_printf("WPOSXY");
      break;
   case WM_PINTERP:
      debug_printf("PINTERP");
      break;
   case WM_LINTERP:
      debug_printf("LINTERP");
      break;
   case WM_CINTERP:
      debug_printf("CINTERP");
      break;
   case WM_FB_WRITE:
      debug_printf("FB_WRITE");
      break;
   case WM_FRONTFACING:
      debug_printf("FRONTFACING");
      break;
   default:
      debug_printf("%s", tgsi_get_opcode_info(opcode)->mnemonic);
      break;
   }
}

void brw_wm_print_value( struct brw_wm_compile *c,
		       struct brw_wm_value *value )
{
   assert(value);
   if (c->state >= PASS2_DONE) 
      brw_print_reg(value->hw_reg);
   else if( value == &c->undef_value )
      debug_printf("undef");
   else if( value - c->vreg >= 0 &&
	    value - c->vreg < BRW_WM_MAX_VREG)
      debug_printf("r%d", value - c->vreg);
   else if (value - c->creg >= 0 &&
	    value - c->creg < BRW_WM_MAX_PARAM)
      debug_printf("c%d", value - c->creg);
   else if (value - c->payload.input_interp >= 0 &&
	    value - c->payload.input_interp < PIPE_MAX_SHADER_INPUTS)
      debug_printf("i%d", value - c->payload.input_interp);
   else if (value - c->payload.depth >= 0 &&
	    value - c->payload.depth < PIPE_MAX_SHADER_INPUTS)
      debug_printf("d%d", value - c->payload.depth);
   else 
      debug_printf("?");
}

void brw_wm_print_ref( struct brw_wm_compile *c,
		       struct brw_wm_ref *ref )
{
   struct brw_reg hw_reg = ref->hw_reg;

   if (ref->unspill_reg)
      debug_printf("UNSPILL(%x)/", ref->value->spill_slot);

   if (c->state >= PASS2_DONE)
      brw_print_reg(ref->hw_reg);
   else {
      debug_printf("%s", hw_reg.negate ? "-" : "");
      debug_printf("%s", hw_reg.abs ? "abs/" : "");
      brw_wm_print_value(c, ref->value);
      if ((hw_reg.nr&1) || hw_reg.subnr) {
	 debug_printf("->%d.%d", (hw_reg.nr&1), hw_reg.subnr);
      }
   }
}

void brw_wm_print_insn( struct brw_wm_compile *c,
			struct brw_wm_instruction *inst )
{
   GLuint i, arg;
   GLuint nr_args = brw_wm_nr_args(inst->opcode);

   debug_printf("[");
   for (i = 0; i < 4; i++) {
      if (inst->dst[i]) {
	 brw_wm_print_value(c, inst->dst[i]);
	 if (inst->dst[i]->spill_slot)
	    debug_printf("/SPILL(%x)",inst->dst[i]->spill_slot);
      }
      else
	 debug_printf("#");
      if (i < 3)      
	 debug_printf(",");
   }
   debug_printf("]");
   print_writemask(inst->writemask);
   
   debug_printf(" = ");
   print_opcode(inst->opcode);
  
   if (inst->saturate)
      debug_printf("_SAT");

   for (arg = 0; arg < nr_args; arg++) {

      debug_printf(" [");

      for (i = 0; i < 4; i++) {
	 if (inst->src[arg][i]) {
	    brw_wm_print_ref(c, inst->src[arg][i]);
	 }
	 else
	    debug_printf("%%");

	 if (i < 3) 
	    debug_printf(",");
	 else
	    debug_printf("]");
      }
   }
   debug_printf("\n");
}

void brw_wm_print_program( struct brw_wm_compile *c,
			   const char *stage )
{
   GLuint insn;

   debug_printf("%s:\n", stage);
   for (insn = 0; insn < c->nr_insns; insn++)
      brw_wm_print_insn(c, &c->instruction[insn]);
   debug_printf("\n");
}

static const char *file_strings[TGSI_FILE_COUNT+1] = {
   "NULL",
   "CONST",
   "IN",
   "OUT",
   "TEMP",
   "SAMPLER",
   "ADDR",
   "IMM",
   "LOOP",
   "PAYLOAD"
};

static void brw_wm_print_fp_insn( struct brw_wm_compile *c,
                                  struct brw_fp_instruction *inst )
{
   GLuint i;
   GLuint nr_args = brw_wm_nr_args(inst->opcode);

   print_opcode(inst->opcode);
   if (inst->dst.saturate)
      debug_printf("_SAT");
   debug_printf(" ");

   if (inst->dst.indirect)
      debug_printf("[");

   debug_printf("%s[%d]",
                file_strings[inst->dst.file],
                inst->dst.index );
   print_writemask(inst->dst.writemask);

   if (inst->dst.indirect)
      debug_printf("]");

   debug_printf(nr_args ? ", " : "\n");
   
   for (i = 0; i < nr_args; i++) {
      debug_printf("%s%s%s[%d]%s",
                   inst->src[i].negate ? "-" : "",
                   inst->src[i].abs ? "ABS(" : "",
                   file_strings[inst->src[i].file],
                   inst->src[i].index,
                   inst->src[i].abs ? ")" : "");
      print_swizzle(inst->src[i].swizzle);
      debug_printf("%s", i == nr_args - 1 ? "\n" : ", ");
   }
}


void brw_wm_print_fp_program( struct brw_wm_compile *c,
                              const char *stage )
{
   GLuint insn;

   debug_printf("%s:\n", stage);
   for (insn = 0; insn < c->nr_fp_insns; insn++)
      brw_wm_print_fp_insn(c, &c->fp_instructions[insn]);
   debug_printf("\n");
}

