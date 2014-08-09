/*
 * Copyright © 2010 Intel Corporation
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

extern "C" {
#include "main/macros.h"
#include "brw_context.h"
}
#include "brw_vs.h"
#include "brw_vec4_gs.h"
#include "brw_fs.h"
#include "brw_cfg.h"
#include "glsl/ir_optimization.h"
#include "glsl/glsl_parser_extras.h"
#include "main/shaderapi.h"

struct gl_shader *
brw_new_shader(struct gl_context *ctx, GLuint name, GLuint type)
{
   struct brw_shader *shader;

   shader = rzalloc(NULL, struct brw_shader);
   if (shader) {
      shader->base.Type = type;
      shader->base.Stage = _mesa_shader_enum_to_shader_stage(type);
      shader->base.Name = name;
      _mesa_init_shader(ctx, &shader->base);
   }

   return &shader->base;
}

struct gl_shader_program *
brw_new_shader_program(struct gl_context *ctx, GLuint name)
{
   struct gl_shader_program *prog = rzalloc(NULL, struct gl_shader_program);
   if (prog) {
      prog->Name = name;
      _mesa_init_shader_program(ctx, prog);
   }
   return prog;
}

/**
 * Performs a compile of the shader stages even when we don't know
 * what non-orthogonal state will be set, in the hope that it reflects
 * the eventual NOS used, and thus allows us to produce link failures.
 */
static bool
brw_shader_precompile(struct gl_context *ctx, struct gl_shader_program *prog)
{
   struct brw_context *brw = brw_context(ctx);

   if (brw->precompile && !brw_fs_precompile(ctx, prog))
      return false;

   if (brw->precompile && !brw_gs_precompile(ctx, prog))
      return false;

   if (brw->precompile && !brw_vs_precompile(ctx, prog))
      return false;

   return true;
}

static void
brw_lower_packing_builtins(struct brw_context *brw,
                           gl_shader_stage shader_type,
                           exec_list *ir)
{
   int ops = LOWER_PACK_SNORM_2x16
           | LOWER_UNPACK_SNORM_2x16
           | LOWER_PACK_UNORM_2x16
           | LOWER_UNPACK_UNORM_2x16
           | LOWER_PACK_SNORM_4x8
           | LOWER_UNPACK_SNORM_4x8
           | LOWER_PACK_UNORM_4x8
           | LOWER_UNPACK_UNORM_4x8;

   if (brw->gen >= 7) {
      /* Gen7 introduced the f32to16 and f16to32 instructions, which can be
       * used to execute packHalf2x16 and unpackHalf2x16. For AOS code, no
       * lowering is needed. For SOA code, the Half2x16 ops must be
       * scalarized.
       */
      if (shader_type == MESA_SHADER_FRAGMENT) {
         ops |= LOWER_PACK_HALF_2x16_TO_SPLIT
             |  LOWER_UNPACK_HALF_2x16_TO_SPLIT;
      }
   } else {
      ops |= LOWER_PACK_HALF_2x16
          |  LOWER_UNPACK_HALF_2x16;
   }

   lower_packing_builtins(ir, ops);
}

GLboolean
brw_link_shader(struct gl_context *ctx, struct gl_shader_program *shProg)
{
   struct brw_context *brw = brw_context(ctx);
   unsigned int stage;

   for (stage = 0; stage < ARRAY_SIZE(shProg->_LinkedShaders); stage++) {
      const struct gl_shader_compiler_options *options =
         &ctx->Const.ShaderCompilerOptions[stage];
      struct brw_shader *shader =
	 (struct brw_shader *)shProg->_LinkedShaders[stage];

      if (!shader)
	 continue;

      struct gl_program *prog =
	 ctx->Driver.NewProgram(ctx, _mesa_shader_stage_to_program(stage),
                                shader->base.Name);
      if (!prog)
	return false;
      prog->Parameters = _mesa_new_parameter_list();

      _mesa_copy_linked_program_data((gl_shader_stage) stage, shProg, prog);

      bool progress;

      /* lower_packing_builtins() inserts arithmetic instructions, so it
       * must precede lower_instructions().
       */
      brw_lower_packing_builtins(brw, (gl_shader_stage) stage, shader->base.ir);
      do_mat_op_to_vec(shader->base.ir);
      const int bitfield_insert = brw->gen >= 7
                                  ? BITFIELD_INSERT_TO_BFM_BFI
                                  : 0;
      lower_instructions(shader->base.ir,
			 MOD_TO_FRACT |
			 DIV_TO_MUL_RCP |
			 SUB_TO_ADD_NEG |
			 EXP_TO_EXP2 |
			 LOG_TO_LOG2 |
                         bitfield_insert |
                         LDEXP_TO_ARITH);

      /* Pre-gen6 HW can only nest if-statements 16 deep.  Beyond this,
       * if-statements need to be flattened.
       */
      if (brw->gen < 6)
	 lower_if_to_cond_assign(shader->base.ir, 16);

      do_lower_texture_projection(shader->base.ir);
      brw_lower_texture_gradients(brw, shader->base.ir);
      do_vec_index_to_cond_assign(shader->base.ir);
      lower_vector_insert(shader->base.ir, true);
      brw_do_cubemap_normalize(shader->base.ir);
      lower_offset_arrays(shader->base.ir);
      brw_do_lower_unnormalized_offset(shader->base.ir);
      lower_noise(shader->base.ir);
      lower_quadop_vector(shader->base.ir, false);

      bool lowered_variable_indexing =
         lower_variable_index_to_cond_assign(shader->base.ir,
                                             options->EmitNoIndirectInput,
                                             options->EmitNoIndirectOutput,
                                             options->EmitNoIndirectTemp,
                                             options->EmitNoIndirectUniform);

      if (unlikely(brw->perf_debug && lowered_variable_indexing)) {
         perf_debug("Unsupported form of variable indexing in FS; falling "
                    "back to very inefficient code generation\n");
      }

      lower_ubo_reference(&shader->base, shader->base.ir);

      do {
	 progress = false;

	 if (stage == MESA_SHADER_FRAGMENT) {
	    brw_do_channel_expressions(shader->base.ir);
	    brw_do_vector_splitting(shader->base.ir);
	 }

	 progress = do_lower_jumps(shader->base.ir, true, true,
				   true, /* main return */
				   false, /* continue */
				   false /* loops */
				   ) || progress;

	 progress = do_common_optimization(shader->base.ir, true, true,
                                           options, ctx->Const.NativeIntegers)
	   || progress;
      } while (progress);

      /* Make a pass over the IR to add state references for any built-in
       * uniforms that are used.  This has to be done now (during linking).
       * Code generation doesn't happen until the first time this shader is
       * used for rendering.  Waiting until then to generate the parameters is
       * too late.  At that point, the values for the built-in uniforms won't
       * get sent to the shader.
       */
      foreach_in_list(ir_instruction, node, shader->base.ir) {
	 ir_variable *var = node->as_variable();

	 if ((var == NULL) || (var->data.mode != ir_var_uniform)
	     || (strncmp(var->name, "gl_", 3) != 0))
	    continue;

	 const ir_state_slot *const slots = var->state_slots;
	 assert(var->state_slots != NULL);

	 for (unsigned int i = 0; i < var->num_state_slots; i++) {
	    _mesa_add_state_reference(prog->Parameters,
				      (gl_state_index *) slots[i].tokens);
	 }
      }

      validate_ir_tree(shader->base.ir);

      do_set_program_inouts(shader->base.ir, prog, shader->base.Stage);

      prog->SamplersUsed = shader->base.active_samplers;
      _mesa_update_shader_textures_used(shProg, prog);

      _mesa_reference_program(ctx, &shader->base.Program, prog);

      brw_add_texrect_params(prog);

      _mesa_reference_program(ctx, &prog, NULL);

      if (ctx->_Shader->Flags & GLSL_DUMP) {
         fprintf(stderr, "\n");
         fprintf(stderr, "GLSL IR for linked %s program %d:\n",
                 _mesa_shader_stage_to_string(shader->base.Stage),
                 shProg->Name);
         _mesa_print_ir(stderr, shader->base.ir, NULL);
         fprintf(stderr, "\n");
      }
   }

   if ((ctx->_Shader->Flags & GLSL_DUMP) && shProg->Name != 0) {
      for (unsigned i = 0; i < shProg->NumShaders; i++) {
         const struct gl_shader *sh = shProg->Shaders[i];
         if (!sh)
            continue;

         fprintf(stderr, "GLSL %s shader %d source for linked program %d:\n",
                 _mesa_shader_stage_to_string(sh->Stage),
                 i, shProg->Name);
         fprintf(stderr, "%s", sh->Source);
         fprintf(stderr, "\n");
      }
   }

   if (!brw_shader_precompile(ctx, shProg))
      return false;

   return true;
}


enum brw_reg_type
brw_type_for_base_type(const struct glsl_type *type)
{
   switch (type->base_type) {
   case GLSL_TYPE_FLOAT:
      return BRW_REGISTER_TYPE_F;
   case GLSL_TYPE_INT:
   case GLSL_TYPE_BOOL:
      return BRW_REGISTER_TYPE_D;
   case GLSL_TYPE_UINT:
      return BRW_REGISTER_TYPE_UD;
   case GLSL_TYPE_ARRAY:
      return brw_type_for_base_type(type->fields.array);
   case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_ATOMIC_UINT:
      /* These should be overridden with the type of the member when
       * dereferenced into.  BRW_REGISTER_TYPE_UD seems like a likely
       * way to trip up if we don't.
       */
      return BRW_REGISTER_TYPE_UD;
   case GLSL_TYPE_IMAGE:
      return BRW_REGISTER_TYPE_UD;
   case GLSL_TYPE_VOID:
   case GLSL_TYPE_ERROR:
   case GLSL_TYPE_INTERFACE:
      unreachable("not reached");
   }

   return BRW_REGISTER_TYPE_F;
}

enum brw_conditional_mod
brw_conditional_for_comparison(unsigned int op)
{
   switch (op) {
   case ir_binop_less:
      return BRW_CONDITIONAL_L;
   case ir_binop_greater:
      return BRW_CONDITIONAL_G;
   case ir_binop_lequal:
      return BRW_CONDITIONAL_LE;
   case ir_binop_gequal:
      return BRW_CONDITIONAL_GE;
   case ir_binop_equal:
   case ir_binop_all_equal: /* same as equal for scalars */
      return BRW_CONDITIONAL_Z;
   case ir_binop_nequal:
   case ir_binop_any_nequal: /* same as nequal for scalars */
      return BRW_CONDITIONAL_NZ;
   default:
      unreachable("not reached: bad operation for comparison");
   }
}

uint32_t
brw_math_function(enum opcode op)
{
   switch (op) {
   case SHADER_OPCODE_RCP:
      return BRW_MATH_FUNCTION_INV;
   case SHADER_OPCODE_RSQ:
      return BRW_MATH_FUNCTION_RSQ;
   case SHADER_OPCODE_SQRT:
      return BRW_MATH_FUNCTION_SQRT;
   case SHADER_OPCODE_EXP2:
      return BRW_MATH_FUNCTION_EXP;
   case SHADER_OPCODE_LOG2:
      return BRW_MATH_FUNCTION_LOG;
   case SHADER_OPCODE_POW:
      return BRW_MATH_FUNCTION_POW;
   case SHADER_OPCODE_SIN:
      return BRW_MATH_FUNCTION_SIN;
   case SHADER_OPCODE_COS:
      return BRW_MATH_FUNCTION_COS;
   case SHADER_OPCODE_INT_QUOTIENT:
      return BRW_MATH_FUNCTION_INT_DIV_QUOTIENT;
   case SHADER_OPCODE_INT_REMAINDER:
      return BRW_MATH_FUNCTION_INT_DIV_REMAINDER;
   default:
      unreachable("not reached: unknown math function");
   }
}

uint32_t
brw_texture_offset(struct gl_context *ctx, ir_constant *offset)
{
   /* If the driver does not support GL_ARB_gpu_shader5, the offset
    * must be constant.
    */
   assert(offset != NULL || ctx->Extensions.ARB_gpu_shader5);

   if (!offset) return 0;  /* nonconstant offset; caller will handle it. */

   signed char offsets[3];
   for (unsigned i = 0; i < offset->type->vector_elements; i++)
      offsets[i] = (signed char) offset->value.i[i];

   /* Combine all three offsets into a single unsigned dword:
    *
    *    bits 11:8 - U Offset (X component)
    *    bits  7:4 - V Offset (Y component)
    *    bits  3:0 - R Offset (Z component)
    */
   unsigned offset_bits = 0;
   for (unsigned i = 0; i < offset->type->vector_elements; i++) {
      const unsigned shift = 4 * (2 - i);
      offset_bits |= (offsets[i] << shift) & (0xF << shift);
   }
   return offset_bits;
}

const char *
brw_instruction_name(enum opcode op)
{
   char *fallback;

   if (op < ARRAY_SIZE(opcode_descs) && opcode_descs[op].name)
      return opcode_descs[op].name;

   switch (op) {
   case FS_OPCODE_FB_WRITE:
      return "fb_write";
   case FS_OPCODE_BLORP_FB_WRITE:
      return "blorp_fb_write";

   case SHADER_OPCODE_RCP:
      return "rcp";
   case SHADER_OPCODE_RSQ:
      return "rsq";
   case SHADER_OPCODE_SQRT:
      return "sqrt";
   case SHADER_OPCODE_EXP2:
      return "exp2";
   case SHADER_OPCODE_LOG2:
      return "log2";
   case SHADER_OPCODE_POW:
      return "pow";
   case SHADER_OPCODE_INT_QUOTIENT:
      return "int_quot";
   case SHADER_OPCODE_INT_REMAINDER:
      return "int_rem";
   case SHADER_OPCODE_SIN:
      return "sin";
   case SHADER_OPCODE_COS:
      return "cos";

   case SHADER_OPCODE_TEX:
      return "tex";
   case SHADER_OPCODE_TXD:
      return "txd";
   case SHADER_OPCODE_TXF:
      return "txf";
   case SHADER_OPCODE_TXL:
      return "txl";
   case SHADER_OPCODE_TXS:
      return "txs";
   case FS_OPCODE_TXB:
      return "txb";
   case SHADER_OPCODE_TXF_CMS:
      return "txf_cms";
   case SHADER_OPCODE_TXF_UMS:
      return "txf_ums";
   case SHADER_OPCODE_TXF_MCS:
      return "txf_mcs";
   case SHADER_OPCODE_TG4:
      return "tg4";
   case SHADER_OPCODE_TG4_OFFSET:
      return "tg4_offset";
   case SHADER_OPCODE_SHADER_TIME_ADD:
      return "shader_time_add";

   case SHADER_OPCODE_LOAD_PAYLOAD:
      return "load_payload";

   case SHADER_OPCODE_GEN4_SCRATCH_READ:
      return "gen4_scratch_read";
   case SHADER_OPCODE_GEN4_SCRATCH_WRITE:
      return "gen4_scratch_write";
   case SHADER_OPCODE_GEN7_SCRATCH_READ:
      return "gen7_scratch_read";

   case FS_OPCODE_DDX:
      return "ddx";
   case FS_OPCODE_DDY:
      return "ddy";

   case FS_OPCODE_PIXEL_X:
      return "pixel_x";
   case FS_OPCODE_PIXEL_Y:
      return "pixel_y";

   case FS_OPCODE_CINTERP:
      return "cinterp";
   case FS_OPCODE_LINTERP:
      return "linterp";

   case FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD:
      return "uniform_pull_const";
   case FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD_GEN7:
      return "uniform_pull_const_gen7";
   case FS_OPCODE_VARYING_PULL_CONSTANT_LOAD:
      return "varying_pull_const";
   case FS_OPCODE_VARYING_PULL_CONSTANT_LOAD_GEN7:
      return "varying_pull_const_gen7";

   case FS_OPCODE_MOV_DISPATCH_TO_FLAGS:
      return "mov_dispatch_to_flags";
   case FS_OPCODE_DISCARD_JUMP:
      return "discard_jump";

   case FS_OPCODE_SET_SIMD4X2_OFFSET:
      return "set_simd4x2_offset";

   case FS_OPCODE_PACK_HALF_2x16_SPLIT:
      return "pack_half_2x16_split";
   case FS_OPCODE_UNPACK_HALF_2x16_SPLIT_X:
      return "unpack_half_2x16_split_x";
   case FS_OPCODE_UNPACK_HALF_2x16_SPLIT_Y:
      return "unpack_half_2x16_split_y";

   case FS_OPCODE_PLACEHOLDER_HALT:
      return "placeholder_halt";

   case VS_OPCODE_URB_WRITE:
      return "vs_urb_write";
   case VS_OPCODE_PULL_CONSTANT_LOAD:
      return "pull_constant_load";
   case VS_OPCODE_PULL_CONSTANT_LOAD_GEN7:
      return "pull_constant_load_gen7";
   case VS_OPCODE_UNPACK_FLAGS_SIMD4X2:
      return "unpack_flags_simd4x2";

   case GS_OPCODE_URB_WRITE:
      return "gs_urb_write";
   case GS_OPCODE_THREAD_END:
      return "gs_thread_end";
   case GS_OPCODE_SET_WRITE_OFFSET:
      return "set_write_offset";
   case GS_OPCODE_SET_VERTEX_COUNT:
      return "set_vertex_count";
   case GS_OPCODE_SET_DWORD_2_IMMED:
      return "set_dword_2_immed";
   case GS_OPCODE_PREPARE_CHANNEL_MASKS:
      return "prepare_channel_masks";
   case GS_OPCODE_SET_CHANNEL_MASKS:
      return "set_channel_masks";
   case GS_OPCODE_GET_INSTANCE_ID:
      return "get_instance_id";

   default:
      /* Yes, this leaks.  It's in debug code, it should never occur, and if
       * it does, you should just add the case to the list above.
       */
      asprintf(&fallback, "op%d", op);
      return fallback;
   }
}

backend_visitor::backend_visitor(struct brw_context *brw,
                                 struct gl_shader_program *shader_prog,
                                 struct gl_program *prog,
                                 struct brw_stage_prog_data *stage_prog_data,
                                 gl_shader_stage stage)
   : brw(brw),
     ctx(&brw->ctx),
     shader(shader_prog ?
        (struct brw_shader *)shader_prog->_LinkedShaders[stage] : NULL),
     shader_prog(shader_prog),
     prog(prog),
     stage_prog_data(stage_prog_data),
     cfg(NULL),
     stage(stage)
{
}

bool
backend_reg::is_zero() const
{
   if (file != IMM)
      return false;

   return fixed_hw_reg.dw1.d == 0;
}

bool
backend_reg::is_one() const
{
   if (file != IMM)
      return false;

   return type == BRW_REGISTER_TYPE_F
          ? fixed_hw_reg.dw1.f == 1.0
          : fixed_hw_reg.dw1.d == 1;
}

bool
backend_reg::is_null() const
{
   return file == HW_REG &&
          fixed_hw_reg.file == BRW_ARCHITECTURE_REGISTER_FILE &&
          fixed_hw_reg.nr == BRW_ARF_NULL;
}


bool
backend_reg::is_accumulator() const
{
   return file == HW_REG &&
          fixed_hw_reg.file == BRW_ARCHITECTURE_REGISTER_FILE &&
          fixed_hw_reg.nr == BRW_ARF_ACCUMULATOR;
}

bool
backend_instruction::is_tex() const
{
   return (opcode == SHADER_OPCODE_TEX ||
           opcode == FS_OPCODE_TXB ||
           opcode == SHADER_OPCODE_TXD ||
           opcode == SHADER_OPCODE_TXF ||
           opcode == SHADER_OPCODE_TXF_CMS ||
           opcode == SHADER_OPCODE_TXF_UMS ||
           opcode == SHADER_OPCODE_TXF_MCS ||
           opcode == SHADER_OPCODE_TXL ||
           opcode == SHADER_OPCODE_TXS ||
           opcode == SHADER_OPCODE_LOD ||
           opcode == SHADER_OPCODE_TG4 ||
           opcode == SHADER_OPCODE_TG4_OFFSET);
}

bool
backend_instruction::is_math() const
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

bool
backend_instruction::is_control_flow() const
{
   switch (opcode) {
   case BRW_OPCODE_DO:
   case BRW_OPCODE_WHILE:
   case BRW_OPCODE_IF:
   case BRW_OPCODE_ELSE:
   case BRW_OPCODE_ENDIF:
   case BRW_OPCODE_BREAK:
   case BRW_OPCODE_CONTINUE:
      return true;
   default:
      return false;
   }
}

bool
backend_instruction::can_do_source_mods() const
{
   switch (opcode) {
   case BRW_OPCODE_ADDC:
   case BRW_OPCODE_BFE:
   case BRW_OPCODE_BFI1:
   case BRW_OPCODE_BFI2:
   case BRW_OPCODE_BFREV:
   case BRW_OPCODE_CBIT:
   case BRW_OPCODE_FBH:
   case BRW_OPCODE_FBL:
   case BRW_OPCODE_SUBB:
      return false;
   default:
      return true;
   }
}

bool
backend_instruction::can_do_saturate() const
{
   switch (opcode) {
   case BRW_OPCODE_ADD:
   case BRW_OPCODE_ASR:
   case BRW_OPCODE_AVG:
   case BRW_OPCODE_DP2:
   case BRW_OPCODE_DP3:
   case BRW_OPCODE_DP4:
   case BRW_OPCODE_DPH:
   case BRW_OPCODE_F16TO32:
   case BRW_OPCODE_F32TO16:
   case BRW_OPCODE_LINE:
   case BRW_OPCODE_LRP:
   case BRW_OPCODE_MAC:
   case BRW_OPCODE_MACH:
   case BRW_OPCODE_MAD:
   case BRW_OPCODE_MATH:
   case BRW_OPCODE_MOV:
   case BRW_OPCODE_MUL:
   case BRW_OPCODE_PLN:
   case BRW_OPCODE_RNDD:
   case BRW_OPCODE_RNDE:
   case BRW_OPCODE_RNDU:
   case BRW_OPCODE_RNDZ:
   case BRW_OPCODE_SEL:
   case BRW_OPCODE_SHL:
   case BRW_OPCODE_SHR:
   case FS_OPCODE_LINTERP:
   case SHADER_OPCODE_COS:
   case SHADER_OPCODE_EXP2:
   case SHADER_OPCODE_LOG2:
   case SHADER_OPCODE_POW:
   case SHADER_OPCODE_RCP:
   case SHADER_OPCODE_RSQ:
   case SHADER_OPCODE_SIN:
   case SHADER_OPCODE_SQRT:
      return true;
   default:
      return false;
   }
}

bool
backend_instruction::reads_accumulator_implicitly() const
{
   switch (opcode) {
   case BRW_OPCODE_MAC:
   case BRW_OPCODE_MACH:
   case BRW_OPCODE_SADA2:
      return true;
   default:
      return false;
   }
}

bool
backend_instruction::writes_accumulator_implicitly(struct brw_context *brw) const
{
   return writes_accumulator ||
          (brw->gen < 6 &&
           ((opcode >= BRW_OPCODE_ADD && opcode < BRW_OPCODE_NOP) ||
            (opcode >= FS_OPCODE_DDX && opcode <= FS_OPCODE_LINTERP &&
             opcode != FS_OPCODE_CINTERP)));
}

bool
backend_instruction::has_side_effects() const
{
   switch (opcode) {
   case SHADER_OPCODE_UNTYPED_ATOMIC:
      return true;
   default:
      return false;
   }
}

void
backend_visitor::dump_instructions()
{
   dump_instructions(NULL);
}

void
backend_visitor::dump_instructions(const char *name)
{
   FILE *file = stderr;
   if (name && geteuid() != 0) {
      file = fopen(name, "w");
      if (!file)
         file = stderr;
   }

   int ip = 0;
   foreach_in_list(backend_instruction, inst, &instructions) {
      if (!name)
         fprintf(stderr, "%d: ", ip++);
      dump_instruction(inst, file);
   }

   if (file != stderr) {
      fclose(file);
   }
}

void
backend_visitor::calculate_cfg()
{
   if (this->cfg)
      return;
   cfg = new(mem_ctx) cfg_t(&this->instructions);
}

void
backend_visitor::invalidate_cfg()
{
   ralloc_free(this->cfg);
   this->cfg = NULL;
}

/**
 * Sets up the starting offsets for the groups of binding table entries
 * commong to all pipeline stages.
 *
 * Unused groups are initialized to 0xd0d0d0d0 to make it obvious that they're
 * unused but also make sure that addition of small offsets to them will
 * trigger some of our asserts that surface indices are < BRW_MAX_SURFACES.
 */
void
backend_visitor::assign_common_binding_table_offsets(uint32_t next_binding_table_offset)
{
   int num_textures = _mesa_fls(prog->SamplersUsed);

   stage_prog_data->binding_table.texture_start = next_binding_table_offset;
   next_binding_table_offset += num_textures;

   if (shader) {
      stage_prog_data->binding_table.ubo_start = next_binding_table_offset;
      next_binding_table_offset += shader->base.NumUniformBlocks;
   } else {
      stage_prog_data->binding_table.ubo_start = 0xd0d0d0d0;
   }

   if (INTEL_DEBUG & DEBUG_SHADER_TIME) {
      stage_prog_data->binding_table.shader_time_start = next_binding_table_offset;
      next_binding_table_offset++;
   } else {
      stage_prog_data->binding_table.shader_time_start = 0xd0d0d0d0;
   }

   if (prog->UsesGather) {
      if (brw->gen >= 8) {
         stage_prog_data->binding_table.gather_texture_start =
            stage_prog_data->binding_table.texture_start;
      } else {
         stage_prog_data->binding_table.gather_texture_start = next_binding_table_offset;
         next_binding_table_offset += num_textures;
      }
   } else {
      stage_prog_data->binding_table.gather_texture_start = 0xd0d0d0d0;
   }

   if (shader_prog && shader_prog->NumAtomicBuffers) {
      stage_prog_data->binding_table.abo_start = next_binding_table_offset;
      next_binding_table_offset += shader_prog->NumAtomicBuffers;
   } else {
      stage_prog_data->binding_table.abo_start = 0xd0d0d0d0;
   }

   /* This may or may not be used depending on how the compile goes. */
   stage_prog_data->binding_table.pull_constants_start = next_binding_table_offset;
   next_binding_table_offset++;

   assert(next_binding_table_offset <= BRW_MAX_SURFACES);

   /* prog_data->base.binding_table.size will be set by brw_mark_surface_used. */
}
