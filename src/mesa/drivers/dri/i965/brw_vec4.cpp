/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "brw_vec4.h"
extern "C" {
#include "main/macros.h"
#include "program/prog_parameter.h"
}

#define MAX_INSTRUCTION (1 << 30)

namespace brw {

/**
 * Common helper for constructing swizzles.  When only a subset of
 * channels of a vec4 are used, we don't want to reference the other
 * channels, as that will tell optimization passes that those other
 * channels are used.
 */
unsigned
swizzle_for_size(int size)
{
   static const unsigned size_swizzles[4] = {
      BRW_SWIZZLE4(SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
      BRW_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y),
      BRW_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_Z),
      BRW_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W),
   };

   assert((size >= 1) && (size <= 4));
   return size_swizzles[size - 1];
}

void
src_reg::init()
{
   memset(this, 0, sizeof(*this));

   this->file = BAD_FILE;
}

src_reg::src_reg(register_file file, int reg, const glsl_type *type)
{
   init();

   this->file = file;
   this->reg = reg;
   if (type && (type->is_scalar() || type->is_vector() || type->is_matrix()))
      this->swizzle = swizzle_for_size(type->vector_elements);
   else
      this->swizzle = SWIZZLE_XYZW;
}

/** Generic unset register constructor. */
src_reg::src_reg()
{
   init();
}

src_reg::src_reg(float f)
{
   init();

   this->file = IMM;
   this->type = BRW_REGISTER_TYPE_F;
   this->imm.f = f;
}

src_reg::src_reg(uint32_t u)
{
   init();

   this->file = IMM;
   this->type = BRW_REGISTER_TYPE_UD;
   this->imm.u = u;
}

src_reg::src_reg(int32_t i)
{
   init();

   this->file = IMM;
   this->type = BRW_REGISTER_TYPE_D;
   this->imm.i = i;
}

src_reg::src_reg(dst_reg reg)
{
   init();

   this->file = reg.file;
   this->reg = reg.reg;
   this->reg_offset = reg.reg_offset;
   this->type = reg.type;
   this->reladdr = reg.reladdr;
   this->fixed_hw_reg = reg.fixed_hw_reg;

   int swizzles[4];
   int next_chan = 0;
   int last = 0;

   for (int i = 0; i < 4; i++) {
      if (!(reg.writemask & (1 << i)))
         continue;

      swizzles[next_chan++] = last = i;
   }

   for (; next_chan < 4; next_chan++) {
      swizzles[next_chan] = last;
   }

   this->swizzle = BRW_SWIZZLE4(swizzles[0], swizzles[1],
                                swizzles[2], swizzles[3]);
}

bool
vec4_instruction::is_tex()
{
   return (opcode == SHADER_OPCODE_TEX ||
	   opcode == SHADER_OPCODE_TXD ||
	   opcode == SHADER_OPCODE_TXF ||
	   opcode == SHADER_OPCODE_TXL ||
	   opcode == SHADER_OPCODE_TXS);
}

void
dst_reg::init()
{
   memset(this, 0, sizeof(*this));
   this->file = BAD_FILE;
   this->writemask = WRITEMASK_XYZW;
}

dst_reg::dst_reg()
{
   init();
}

dst_reg::dst_reg(register_file file, int reg)
{
   init();

   this->file = file;
   this->reg = reg;
}

dst_reg::dst_reg(register_file file, int reg, const glsl_type *type,
                 int writemask)
{
   init();

   this->file = file;
   this->reg = reg;
   this->type = brw_type_for_base_type(type);
   this->writemask = writemask;
}

dst_reg::dst_reg(struct brw_reg reg)
{
   init();

   this->file = HW_REG;
   this->fixed_hw_reg = reg;
}

dst_reg::dst_reg(src_reg reg)
{
   init();

   this->file = reg.file;
   this->reg = reg.reg;
   this->reg_offset = reg.reg_offset;
   this->type = reg.type;
   this->writemask = WRITEMASK_XYZW;
   this->reladdr = reg.reladdr;
   this->fixed_hw_reg = reg.fixed_hw_reg;
}

bool
vec4_instruction::is_math()
{
   return (opcode == SHADER_OPCODE_RCP ||
	   opcode == SHADER_OPCODE_RSQ ||
	   opcode == SHADER_OPCODE_SQRT ||
	   opcode == SHADER_OPCODE_EXP2 ||
	   opcode == SHADER_OPCODE_LOG2 ||
	   opcode == SHADER_OPCODE_SIN ||
	   opcode == SHADER_OPCODE_COS ||
	   opcode == SHADER_OPCODE_INT_QUOTIENT ||
	   opcode == SHADER_OPCODE_INT_REMAINDER ||
	   opcode == SHADER_OPCODE_POW);
}
/**
 * Returns how many MRFs an opcode will write over.
 *
 * Note that this is not the 0 or 1 implied writes in an actual gen
 * instruction -- the generate_* functions generate additional MOVs
 * for setup.
 */
int
vec4_visitor::implied_mrf_writes(vec4_instruction *inst)
{
   if (inst->mlen == 0)
      return 0;

   switch (inst->opcode) {
   case SHADER_OPCODE_RCP:
   case SHADER_OPCODE_RSQ:
   case SHADER_OPCODE_SQRT:
   case SHADER_OPCODE_EXP2:
   case SHADER_OPCODE_LOG2:
   case SHADER_OPCODE_SIN:
   case SHADER_OPCODE_COS:
      return 1;
   case SHADER_OPCODE_POW:
      return 2;
   case VS_OPCODE_URB_WRITE:
      return 1;
   case VS_OPCODE_PULL_CONSTANT_LOAD:
      return 2;
   case VS_OPCODE_SCRATCH_READ:
      return 2;
   case VS_OPCODE_SCRATCH_WRITE:
      return 3;
   default:
      assert(!"not reached");
      return inst->mlen;
   }
}

bool
src_reg::equals(src_reg *r)
{
   return (file == r->file &&
	   reg == r->reg &&
	   reg_offset == r->reg_offset &&
	   type == r->type &&
	   negate == r->negate &&
	   abs == r->abs &&
	   swizzle == r->swizzle &&
	   !reladdr && !r->reladdr &&
	   memcmp(&fixed_hw_reg, &r->fixed_hw_reg,
		  sizeof(fixed_hw_reg)) == 0 &&
	   imm.u == r->imm.u);
}

/**
 * Must be called after calculate_live_intervales() to remove unused
 * writes to registers -- register allocation will fail otherwise
 * because something deffed but not used won't be considered to
 * interfere with other regs.
 */
bool
vec4_visitor::dead_code_eliminate()
{
   bool progress = false;
   int pc = 0;

   calculate_live_intervals();

   foreach_list_safe(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      if (inst->dst.file == GRF && this->virtual_grf_use[inst->dst.reg] <= pc) {
	 inst->remove();
	 progress = true;
      }

      pc++;
   }

   if (progress)
      live_intervals_valid = false;

   return progress;
}

void
vec4_visitor::split_uniform_registers()
{
   /* Prior to this, uniforms have been in an array sized according to
    * the number of vector uniforms present, sparsely filled (so an
    * aggregate results in reg indices being skipped over).  Now we're
    * going to cut those aggregates up so each .reg index is one
    * vector.  The goal is to make elimination of unused uniform
    * components easier later.
    */
   foreach_list(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      for (int i = 0 ; i < 3; i++) {
	 if (inst->src[i].file != UNIFORM)
	    continue;

	 assert(!inst->src[i].reladdr);

	 inst->src[i].reg += inst->src[i].reg_offset;
	 inst->src[i].reg_offset = 0;
      }
   }

   /* Update that everything is now vector-sized. */
   for (int i = 0; i < this->uniforms; i++) {
      this->uniform_size[i] = 1;
   }
}

void
vec4_visitor::pack_uniform_registers()
{
   bool uniform_used[this->uniforms];
   int new_loc[this->uniforms];
   int new_chan[this->uniforms];

   memset(uniform_used, 0, sizeof(uniform_used));
   memset(new_loc, 0, sizeof(new_loc));
   memset(new_chan, 0, sizeof(new_chan));

   /* Find which uniform vectors are actually used by the program.  We
    * expect unused vector elements when we've moved array access out
    * to pull constants, and from some GLSL code generators like wine.
    */
   foreach_list(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      for (int i = 0 ; i < 3; i++) {
	 if (inst->src[i].file != UNIFORM)
	    continue;

	 uniform_used[inst->src[i].reg] = true;
      }
   }

   int new_uniform_count = 0;

   /* Now, figure out a packing of the live uniform vectors into our
    * push constants.
    */
   for (int src = 0; src < uniforms; src++) {
      int size = this->uniform_vector_size[src];

      if (!uniform_used[src]) {
	 this->uniform_vector_size[src] = 0;
	 continue;
      }

      int dst;
      /* Find the lowest place we can slot this uniform in. */
      for (dst = 0; dst < src; dst++) {
	 if (this->uniform_vector_size[dst] + size <= 4)
	    break;
      }

      if (src == dst) {
	 new_loc[src] = dst;
	 new_chan[src] = 0;
      } else {
	 new_loc[src] = dst;
	 new_chan[src] = this->uniform_vector_size[dst];

	 /* Move the references to the data */
	 for (int j = 0; j < size; j++) {
	    c->prog_data.param[dst * 4 + new_chan[src] + j] =
	       c->prog_data.param[src * 4 + j];
	 }

	 this->uniform_vector_size[dst] += size;
	 this->uniform_vector_size[src] = 0;
      }

      new_uniform_count = MAX2(new_uniform_count, dst + 1);
   }

   this->uniforms = new_uniform_count;

   /* Now, update the instructions for our repacked uniforms. */
   foreach_list(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      for (int i = 0 ; i < 3; i++) {
	 int src = inst->src[i].reg;

	 if (inst->src[i].file != UNIFORM)
	    continue;

	 inst->src[i].reg = new_loc[src];

	 int sx = BRW_GET_SWZ(inst->src[i].swizzle, 0) + new_chan[src];
	 int sy = BRW_GET_SWZ(inst->src[i].swizzle, 1) + new_chan[src];
	 int sz = BRW_GET_SWZ(inst->src[i].swizzle, 2) + new_chan[src];
	 int sw = BRW_GET_SWZ(inst->src[i].swizzle, 3) + new_chan[src];
	 inst->src[i].swizzle = BRW_SWIZZLE4(sx, sy, sz, sw);
      }
   }
}

bool
src_reg::is_zero() const
{
   if (file != IMM)
      return false;

   if (type == BRW_REGISTER_TYPE_F) {
      return imm.f == 0.0;
   } else {
      return imm.i == 0;
   }
}

bool
src_reg::is_one() const
{
   if (file != IMM)
      return false;

   if (type == BRW_REGISTER_TYPE_F) {
      return imm.f == 1.0;
   } else {
      return imm.i == 1;
   }
}

/**
 * Does algebraic optimizations (0 * a = 0, 1 * a = a, a + 0 = a).
 *
 * While GLSL IR also performs this optimization, we end up with it in
 * our instruction stream for a couple of reasons.  One is that we
 * sometimes generate silly instructions, for example in array access
 * where we'll generate "ADD offset, index, base" even if base is 0.
 * The other is that GLSL IR's constant propagation doesn't track the
 * components of aggregates, so some VS patterns (initialize matrix to
 * 0, accumulate in vertex blending factors) end up breaking down to
 * instructions involving 0.
 */
bool
vec4_visitor::opt_algebraic()
{
   bool progress = false;

   foreach_list(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      switch (inst->opcode) {
      case BRW_OPCODE_ADD:
	 if (inst->src[1].is_zero()) {
	    inst->opcode = BRW_OPCODE_MOV;
	    inst->src[1] = src_reg();
	    progress = true;
	 }
	 break;

      case BRW_OPCODE_MUL:
	 if (inst->src[1].is_zero()) {
	    inst->opcode = BRW_OPCODE_MOV;
	    switch (inst->src[0].type) {
	    case BRW_REGISTER_TYPE_F:
	       inst->src[0] = src_reg(0.0f);
	       break;
	    case BRW_REGISTER_TYPE_D:
	       inst->src[0] = src_reg(0);
	       break;
	    case BRW_REGISTER_TYPE_UD:
	       inst->src[0] = src_reg(0u);
	       break;
	    default:
	       assert(!"not reached");
	       inst->src[0] = src_reg(0.0f);
	       break;
	    }
	    inst->src[1] = src_reg();
	    progress = true;
	 } else if (inst->src[1].is_one()) {
	    inst->opcode = BRW_OPCODE_MOV;
	    inst->src[1] = src_reg();
	    progress = true;
	 }
	 break;
      default:
	 break;
      }
   }

   if (progress)
      this->live_intervals_valid = false;

   return progress;
}

/**
 * Only a limited number of hardware registers may be used for push
 * constants, so this turns access to the overflowed constants into
 * pull constants.
 */
void
vec4_visitor::move_push_constants_to_pull_constants()
{
   int pull_constant_loc[this->uniforms];

   /* Only allow 32 registers (256 uniform components) as push constants,
    * which is the limit on gen6.
    */
   int max_uniform_components = 32 * 8;
   if (this->uniforms * 4 <= max_uniform_components)
      return;

   /* Make some sort of choice as to which uniforms get sent to pull
    * constants.  We could potentially do something clever here like
    * look for the most infrequently used uniform vec4s, but leave
    * that for later.
    */
   for (int i = 0; i < this->uniforms * 4; i += 4) {
      pull_constant_loc[i / 4] = -1;

      if (i >= max_uniform_components) {
	 const float **values = &prog_data->param[i];

	 /* Try to find an existing copy of this uniform in the pull
	  * constants if it was part of an array access already.
	  */
	 for (unsigned int j = 0; j < prog_data->nr_pull_params; j += 4) {
	    int matches;

	    for (matches = 0; matches < 4; matches++) {
	       if (prog_data->pull_param[j + matches] != values[matches])
		  break;
	    }

	    if (matches == 4) {
	       pull_constant_loc[i / 4] = j / 4;
	       break;
	    }
	 }

	 if (pull_constant_loc[i / 4] == -1) {
	    assert(prog_data->nr_pull_params % 4 == 0);
	    pull_constant_loc[i / 4] = prog_data->nr_pull_params / 4;

	    for (int j = 0; j < 4; j++) {
	       prog_data->pull_param[prog_data->nr_pull_params++] = values[j];
	    }
	 }
      }
   }

   /* Now actually rewrite usage of the things we've moved to pull
    * constants.
    */
   foreach_list_safe(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      for (int i = 0 ; i < 3; i++) {
	 if (inst->src[i].file != UNIFORM ||
	     pull_constant_loc[inst->src[i].reg] == -1)
	    continue;

	 int uniform = inst->src[i].reg;

	 dst_reg temp = dst_reg(this, glsl_type::vec4_type);

	 emit_pull_constant_load(inst, temp, inst->src[i],
				 pull_constant_loc[uniform]);

	 inst->src[i].file = temp.file;
	 inst->src[i].reg = temp.reg;
	 inst->src[i].reg_offset = temp.reg_offset;
	 inst->src[i].reladdr = NULL;
      }
   }

   /* Repack push constants to remove the now-unused ones. */
   pack_uniform_registers();
}

/*
 * Tries to reduce extra MOV instructions by taking GRFs that get just
 * written and then MOVed into an MRF and making the original write of
 * the GRF write directly to the MRF instead.
 */
bool
vec4_visitor::opt_compute_to_mrf()
{
   bool progress = false;
   int next_ip = 0;

   calculate_live_intervals();

   foreach_list_safe(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      int ip = next_ip;
      next_ip++;

      if (inst->opcode != BRW_OPCODE_MOV ||
	  inst->predicate ||
	  inst->dst.file != MRF || inst->src[0].file != GRF ||
	  inst->dst.type != inst->src[0].type ||
	  inst->src[0].abs || inst->src[0].negate || inst->src[0].reladdr)
	 continue;

      int mrf = inst->dst.reg;

      /* Can't compute-to-MRF this GRF if someone else was going to
       * read it later.
       */
      if (this->virtual_grf_use[inst->src[0].reg] > ip)
	 continue;

      /* We need to check interference with the MRF between this
       * instruction and the earliest instruction involved in writing
       * the GRF we're eliminating.  To do that, keep track of which
       * of our source channels we've seen initialized.
       */
      bool chans_needed[4] = {false, false, false, false};
      int chans_remaining = 0;
      for (int i = 0; i < 4; i++) {
	 int chan = BRW_GET_SWZ(inst->src[0].swizzle, i);

	 if (!(inst->dst.writemask & (1 << i)))
	    continue;

	 /* We don't handle compute-to-MRF across a swizzle.  We would
	  * need to be able to rewrite instructions above to output
	  * results to different channels.
	  */
	 if (chan != i)
	    chans_remaining = 5;

	 if (!chans_needed[chan]) {
	    chans_needed[chan] = true;
	    chans_remaining++;
	 }
      }
      if (chans_remaining > 4)
	 continue;

      /* Now walk up the instruction stream trying to see if we can
       * rewrite everything writing to the GRF into the MRF instead.
       */
      vec4_instruction *scan_inst;
      for (scan_inst = (vec4_instruction *)inst->prev;
	   scan_inst->prev != NULL;
	   scan_inst = (vec4_instruction *)scan_inst->prev) {
	 if (scan_inst->dst.file == GRF &&
	     scan_inst->dst.reg == inst->src[0].reg &&
	     scan_inst->dst.reg_offset == inst->src[0].reg_offset) {
	    /* Found something writing to the reg we want to turn into
	     * a compute-to-MRF.
	     */

	    /* SEND instructions can't have MRF as a destination. */
	    if (scan_inst->mlen)
	       break;

	    if (intel->gen >= 6) {
	       /* gen6 math instructions must have the destination be
		* GRF, so no compute-to-MRF for them.
		*/
	       if (scan_inst->is_math()) {
		  break;
	       }
	    }

	    /* Mark which channels we found unconditional writes for. */
	    if (!scan_inst->predicate) {
	       for (int i = 0; i < 4; i++) {
		  if (scan_inst->dst.writemask & (1 << i) &&
		      chans_needed[i]) {
		     chans_needed[i] = false;
		     chans_remaining--;
		  }
	       }
	    }

	    if (chans_remaining == 0)
	       break;
	 }

	 /* We don't handle flow control here.  Most computation of
	  * values that end up in MRFs are shortly before the MRF
	  * write anyway.
	  */
	 if (scan_inst->opcode == BRW_OPCODE_DO ||
	     scan_inst->opcode == BRW_OPCODE_WHILE ||
	     scan_inst->opcode == BRW_OPCODE_ELSE ||
	     scan_inst->opcode == BRW_OPCODE_ENDIF) {
	    break;
	 }

	 /* You can't read from an MRF, so if someone else reads our
	  * MRF's source GRF that we wanted to rewrite, that stops us.
	  */
	 bool interfered = false;
	 for (int i = 0; i < 3; i++) {
	    if (scan_inst->src[i].file == GRF &&
		scan_inst->src[i].reg == inst->src[0].reg &&
		scan_inst->src[i].reg_offset == inst->src[0].reg_offset) {
	       interfered = true;
	    }
	 }
	 if (interfered)
	    break;

	 /* If somebody else writes our MRF here, we can't
	  * compute-to-MRF before that.
	  */
	 if (scan_inst->dst.file == MRF && mrf == scan_inst->dst.reg)
	    break;

	 if (scan_inst->mlen > 0) {
	    /* Found a SEND instruction, which means that there are
	     * live values in MRFs from base_mrf to base_mrf +
	     * scan_inst->mlen - 1.  Don't go pushing our MRF write up
	     * above it.
	     */
	    if (mrf >= scan_inst->base_mrf &&
		mrf < scan_inst->base_mrf + scan_inst->mlen) {
	       break;
	    }
	 }
      }

      if (chans_remaining == 0) {
	 /* If we've made it here, we have an inst we want to
	  * compute-to-MRF, and a scan_inst pointing to the earliest
	  * instruction involved in computing the value.  Now go
	  * rewrite the instruction stream between the two.
	  */

	 while (scan_inst != inst) {
	    if (scan_inst->dst.file == GRF &&
		scan_inst->dst.reg == inst->src[0].reg &&
		scan_inst->dst.reg_offset == inst->src[0].reg_offset) {
	       scan_inst->dst.file = MRF;
	       scan_inst->dst.reg = mrf;
	       scan_inst->dst.reg_offset = 0;
	       scan_inst->saturate |= inst->saturate;
	    }
	    scan_inst = (vec4_instruction *)scan_inst->next;
	 }
	 inst->remove();
	 progress = true;
      }
   }

   if (progress)
      live_intervals_valid = false;

   return progress;
}

/**
 * Splits virtual GRFs requesting more than one contiguous physical register.
 *
 * We initially create large virtual GRFs for temporary structures, arrays,
 * and matrices, so that the dereference visitor functions can add reg_offsets
 * to work their way down to the actual member being accessed.
 *
 * Unlike in the FS visitor, though, we have no SEND messages that return more
 * than 1 register.  We also don't do any array access in register space,
 * which would have required contiguous physical registers.  Thus, all those
 * large virtual GRFs can be split up into independent single-register virtual
 * GRFs, making allocation and optimization easier.
 */
void
vec4_visitor::split_virtual_grfs()
{
   int num_vars = this->virtual_grf_count;
   int new_virtual_grf[num_vars];

   memset(new_virtual_grf, 0, sizeof(new_virtual_grf));

   /* Allocate new space for split regs.  Note that the virtual
    * numbers will be contiguous.
    */
   for (int i = 0; i < num_vars; i++) {
      if (this->virtual_grf_sizes[i] == 1)
         continue;

      new_virtual_grf[i] = virtual_grf_alloc(1);
      for (int j = 2; j < this->virtual_grf_sizes[i]; j++) {
         int reg = virtual_grf_alloc(1);
         assert(reg == new_virtual_grf[i] + j - 1);
         (void) reg;
      }
      this->virtual_grf_sizes[i] = 1;
   }

   foreach_list(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      if (inst->dst.file == GRF &&
	  new_virtual_grf[inst->dst.reg] &&
	  inst->dst.reg_offset != 0) {
	 inst->dst.reg = (new_virtual_grf[inst->dst.reg] +
			  inst->dst.reg_offset - 1);
	 inst->dst.reg_offset = 0;
      }
      for (int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF &&
	     new_virtual_grf[inst->src[i].reg] &&
	     inst->src[i].reg_offset != 0) {
	    inst->src[i].reg = (new_virtual_grf[inst->src[i].reg] +
				inst->src[i].reg_offset - 1);
	    inst->src[i].reg_offset = 0;
	 }
      }
   }
   this->live_intervals_valid = false;
}

void
vec4_visitor::dump_instruction(vec4_instruction *inst)
{
   if (inst->opcode < ARRAY_SIZE(opcode_descs) &&
       opcode_descs[inst->opcode].name) {
      printf("%s ", opcode_descs[inst->opcode].name);
   } else {
      printf("op%d ", inst->opcode);
   }

   switch (inst->dst.file) {
   case GRF:
      printf("vgrf%d.%d", inst->dst.reg, inst->dst.reg_offset);
      break;
   case MRF:
      printf("m%d", inst->dst.reg);
      break;
   case BAD_FILE:
      printf("(null)");
      break;
   default:
      printf("???");
      break;
   }
   if (inst->dst.writemask != WRITEMASK_XYZW) {
      printf(".");
      if (inst->dst.writemask & 1)
         printf("x");
      if (inst->dst.writemask & 2)
         printf("y");
      if (inst->dst.writemask & 4)
         printf("z");
      if (inst->dst.writemask & 8)
         printf("w");
   }
   printf(", ");

   for (int i = 0; i < 3; i++) {
      switch (inst->src[i].file) {
      case GRF:
         printf("vgrf%d", inst->src[i].reg);
         break;
      case ATTR:
         printf("attr%d", inst->src[i].reg);
         break;
      case UNIFORM:
         printf("u%d", inst->src[i].reg);
         break;
      case BAD_FILE:
         printf("(null)");
         break;
      default:
         printf("???");
         break;
      }

      if (inst->src[i].reg_offset)
         printf(".%d", inst->src[i].reg_offset);

      static const char *chans[4] = {"x", "y", "z", "w"};
      printf(".");
      for (int c = 0; c < 4; c++) {
         printf(chans[BRW_GET_SWZ(inst->src[i].swizzle, c)]);
      }

      if (i < 3)
         printf(", ");
   }

   printf("\n");
}

void
vec4_visitor::dump_instructions()
{
   int ip = 0;
   foreach_list_safe(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;
      printf("%d: ", ip++);
      dump_instruction(inst);
   }
}

} /* namespace brw */
