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

#include "util/u_memory.h"
#include "util/u_math.h"

#include "brw_debug.h"
#include "brw_wm.h"



/***********************************************************************
 */

static struct brw_wm_ref *get_ref( struct brw_wm_compile *c )
{
   assert(c->nr_refs < BRW_WM_MAX_REF);
   return &c->refs[c->nr_refs++];
}

static struct brw_wm_value *get_value( struct brw_wm_compile *c)
{
   assert(c->nr_refs < BRW_WM_MAX_VREG);
   return &c->vreg[c->nr_vreg++];
}

/** return pointer to a newly allocated instruction */
static struct brw_wm_instruction *get_instruction( struct brw_wm_compile *c )
{
   assert(c->nr_insns < BRW_WM_MAX_INSN);
   return &c->instruction[c->nr_insns++];
}

/***********************************************************************
 */

/** Init the "undef" register */
static void pass0_init_undef( struct brw_wm_compile *c)
{
   struct brw_wm_ref *ref = &c->undef_ref;
   ref->value = &c->undef_value;
   ref->hw_reg = brw_vec8_grf(0, 0);
   ref->insn = 0;
   ref->prevuse = NULL;
}

/** Set a FP register to a value */
static void pass0_set_fpreg_value( struct brw_wm_compile *c,
				   GLuint file,
				   GLuint idx,
				   GLuint component,
				   struct brw_wm_value *value )
{
   struct brw_wm_ref *ref = get_ref(c);
   ref->value = value;
   ref->hw_reg = brw_vec8_grf(0, 0);
   ref->insn = 0;
   ref->prevuse = NULL;
   c->pass0_fp_reg[file][idx][component] = ref;
}

/** Set a FP register to a ref */
static void pass0_set_fpreg_ref( struct brw_wm_compile *c,
				 GLuint file,
				 GLuint idx,
				 GLuint component,
				 const struct brw_wm_ref *src_ref )
{
   c->pass0_fp_reg[file][idx][component] = src_ref;
}

static const struct brw_wm_ref *get_param_ref( struct brw_wm_compile *c, 
					       unsigned idx,
                                               unsigned component)
{
   GLuint i = idx * 4 + component;
   
   if (i >= BRW_WM_MAX_PARAM) {
      debug_printf("%s: out of params\n", __FUNCTION__);
      c->prog_data.error = 1;
      return NULL;
   }
   else {
      struct brw_wm_ref *ref = get_ref(c);

      c->nr_creg = MAX2(c->nr_creg, (i+16)/16);

      /* Push the offsets into hw_reg.  These will be added to the
       * real register numbers once one is allocated in pass2.
       */
      ref->hw_reg = brw_vec1_grf((i&8)?1:0, i%8);
      ref->value = &c->creg[i/16];
      ref->insn = 0;
      ref->prevuse = NULL;

      return ref;
   }
}




/* Lookup our internal registers
 */
static const struct brw_wm_ref *pass0_get_reg( struct brw_wm_compile *c,
					       GLuint file,
					       GLuint idx,
					       GLuint component )
{
   const struct brw_wm_ref *ref = c->pass0_fp_reg[file][idx][component];

   if (!ref) {
      switch (file) {
      case TGSI_FILE_INPUT:
      case TGSI_FILE_TEMPORARY:
      case TGSI_FILE_OUTPUT:
      case BRW_FILE_PAYLOAD:
	 /* should already be done?? */
	 break;

      case TGSI_FILE_CONSTANT:
	 ref = get_param_ref(c, 
                             c->fp->info.immediate_count + idx,
                             component);
	 break;

      case TGSI_FILE_IMMEDIATE:
	 ref = get_param_ref(c, 
                             idx,
                             component);
	 break;

      default:
	 assert(0);
	 break;
      }

      c->pass0_fp_reg[file][idx][component] = ref;
   }

   if (!ref)
      ref = &c->undef_ref;

   return ref;
}



/***********************************************************************
 * Straight translation to internal instruction format
 */

static void pass0_set_dst( struct brw_wm_compile *c,
			   struct brw_wm_instruction *out,
			   const struct brw_fp_instruction *inst,
			   GLuint writemask )
{
   const struct brw_fp_dst dst = inst->dst;
   GLuint i;

   for (i = 0; i < 4; i++) {
      if (writemask & (1<<i)) {
	 out->dst[i] = get_value(c);
	 pass0_set_fpreg_value(c, dst.file, dst.index, i, out->dst[i]);
      }
   }

   out->writemask = writemask;
}


static const struct brw_wm_ref *get_fp_src_reg_ref( struct brw_wm_compile *c,
						    struct brw_fp_src src,
						    GLuint i )
{
   return pass0_get_reg(c, src.file, src.index, BRW_GET_SWZ(src.swizzle,i));
}


static struct brw_wm_ref *get_new_ref( struct brw_wm_compile *c,
				       struct brw_fp_src src,
				       GLuint i,
				       struct brw_wm_instruction *insn)
{
   const struct brw_wm_ref *ref = get_fp_src_reg_ref(c, src, i);
   struct brw_wm_ref *newref = get_ref(c);

   newref->value = ref->value;
   newref->hw_reg = ref->hw_reg;

   if (insn) {
      newref->insn = insn - c->instruction;
      newref->prevuse = newref->value->lastuse;
      newref->value->lastuse = newref;
   }

   if (src.negate)
      newref->hw_reg.negate ^= 1;

   if (src.abs) {
      newref->hw_reg.negate = 0;
      newref->hw_reg.abs = 1;
   }

   return newref;
}


static void
translate_insn(struct brw_wm_compile *c,
               const struct brw_fp_instruction *inst)
{
   struct brw_wm_instruction *out = get_instruction(c);
   GLuint writemask = inst->dst.writemask;
   GLuint nr_args = brw_wm_nr_args(inst->opcode);
   GLuint i, j;

   /* Copy some data out of the instruction
    */
   out->opcode = inst->opcode;
   out->saturate = inst->dst.saturate;
   out->tex_unit = inst->tex_unit;
   out->target = inst->target;

   /* Nasty hack:
    */
   out->eot = (inst->opcode == WM_FB_WRITE &&
               inst->tex_unit != 0);


   /* Args:
    */
   for (i = 0; i < nr_args; i++) {
      for (j = 0; j < 4; j++) {
	 out->src[i][j] = get_new_ref(c, inst->src[i], j, out);
      }
   }

   /* Dst:
    */
   pass0_set_dst(c, out, inst, writemask);
}



/***********************************************************************
 * Optimize moves and swizzles away:
 */ 
static void pass0_precalc_mov( struct brw_wm_compile *c,
			       const struct brw_fp_instruction *inst )
{
   const struct brw_fp_dst dst = inst->dst;
   GLuint writemask = dst.writemask;
   struct brw_wm_ref *refs[4];
   GLuint i;

   /* Get the effect of a MOV by manipulating our register table:
    * First get all refs, then assign refs.  This ensures that "in-place"
    * swizzles such as:
    *   MOV t, t.xxyx
    * are handled correctly.  Previously, these two steps were done in
    * one loop and the above case was incorrectly handled.
    */
   for (i = 0; i < 4; i++) {
      refs[i] = get_new_ref(c, inst->src[0], i, NULL);
   }
   for (i = 0; i < 4; i++) {
      if (writemask & (1 << i)) {	    
         pass0_set_fpreg_ref( c, dst.file, dst.index, i, refs[i]);
      }
   }
}


/* Initialize payload "registers".
 */
static void pass0_init_payload( struct brw_wm_compile *c )
{
   GLuint i;

   for (i = 0; i < 4; i++) {
      GLuint j = i >= c->key.nr_depth_regs ? 0 : i;
      pass0_set_fpreg_value( c, BRW_FILE_PAYLOAD, PAYLOAD_DEPTH, i, 
			     &c->payload.depth[j] );
   }

   for (i = 0; i < c->key.nr_inputs; i++)
      pass0_set_fpreg_value( c, BRW_FILE_PAYLOAD, i, 0, 
			     &c->payload.input_interp[i] );      
}


/***********************************************************************
 * PASS 0
 *
 * Work forwards to give each calculated value a unique number.  Where
 * an instruction produces duplicate values (eg DP3), all are given
 * the same number.
 *
 * Translate away swizzling and eliminate non-saturating moves.
 *
 * Translate instructions from our fp_instruction structs to our
 * internal brw_wm_instruction representation.
 */
void brw_wm_pass0( struct brw_wm_compile *c )
{
   GLuint insn;

   c->nr_vreg = 0;
   c->nr_insns = 0;

   pass0_init_undef(c);
   pass0_init_payload(c);

   for (insn = 0; insn < c->nr_fp_insns; insn++) {
      const struct brw_fp_instruction *inst = &c->fp_instructions[insn];

      /* Optimize away moves, otherwise emit translated instruction:
       */      
      switch (inst->opcode) {
      case TGSI_OPCODE_MOV: 
	 if (!inst->dst.saturate) {
	    pass0_precalc_mov(c, inst);
	 }
	 else {
	    translate_insn(c, inst);
	 }
	 break;
      default:
	 translate_insn(c, inst);
	 break;
      }
   }
 
   if (BRW_DEBUG & DEBUG_WM) {
      brw_wm_print_program(c, "pass0");
   }
}
