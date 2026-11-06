/*
 * Copyright © 2014 Intel Corporation
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
 *
 * Authors:
 *    Connor Abbott (cwabbott0@gmail.com)
 *
 */

#include <inttypes.h> /* for PRIx64 macro */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "compiler/shader_enums.h"
#include "util/half_float.h"
#include "util/memstream.h"
/* glsl-optimizer change: removed include of util/mesa-blake3.h */
#include "util/ralloc.h"
#include "nir.h"
#include "nir_builder.h"

simple_mtx_t nir_print_lock = SIMPLE_MTX_INITIALIZER;

static void
print_indentation(unsigned levels, FILE *fp)
{
   for (unsigned i = 0; i < levels; i++)
      fprintf(fp, "    ");
}

typedef struct {
   FILE *fp;
   nir_shader *shader;

   const char *def_prefix;

   /** map from nir_variable -> printable name */
   struct hash_table *ht;

   /** sorted list of block's predecessors  */
   nir_block **preds;

   /** set of names used so far for nir_variables */
   struct set *syms;

   /* set of struct types that were already printed */
   struct set *struct_types;

   /* an index used to make new non-conflicting names */
   unsigned index;

   /* Used with nir_gather_types() to identify best representation
    * to print terse inline constant values together with SSA sources.
    * Updated per nir_function_impl being printed.
    */
   BITSET_WORD *float_types;
   BITSET_WORD *int_types;

   /**
    * Optional table of annotations mapping nir object
    * (such as instr or var) to message to print.
    */
   struct hash_table *annotations;

   /* Maximum length for SSA or Reg index in the current impl */
   unsigned max_dest_index;

   /* Padding for instructions without destination to make
    * them align with the `=` for instructions with destination.
    */
   unsigned padding_for_no_dest;

   /* Whether divergence metadata is valid. */
   bool divergence_valid;

   bool gather_debug_info;

   nir_instr_debug_info last_debug_info;
} print_state;

static void
print_annotation(print_state *state, void *obj)
{
   FILE *fp = state->fp;

   if (!state->annotations)
      return;

   struct hash_entry *entry = _mesa_hash_table_search(state->annotations, obj);
   if (!entry)
      return;

   const char *note = entry->data;
   _mesa_hash_table_remove(state->annotations, entry);

   fprintf(fp, " %s", note);
}

/* For 1 element, the size is intentionally omitted. */
static const char *sizes[] = { "x??", "   ", "x2 ", "x3 ", "x4 ",
                               "x5 ", "x??", "x??", "x8 ",
                               "x??", "x??", "x??", "x??",
                               "x??", "x??", "x??", "x16" };

static const char *
divergence_status(print_state *state, bool divergent)
{
   if (state->divergence_valid)
      return divergent ? "div " : "con ";

   return "";
}

static unsigned
count_digits(unsigned n)
{
   return n ? (unsigned)floor(log10(n)) + 1u : 1u;
}

static void
print_def(nir_def *def, print_state *state)
{
   FILE *fp = state->fp;

   const unsigned ssa_padding = state->max_dest_index ? count_digits(state->max_dest_index) - count_digits(def->index) : 0;

   const unsigned padding = (def->bit_size <= 8) + 1 + ssa_padding;

   fprintf(fp, "%s%u%s%*s%s%u",
           divergence_status(state, def->divergent),
           def->bit_size, sizes[def->num_components],
           padding, "", state->def_prefix, def->index);

   if (nir_def_instr(def)->has_debug_info) {
      nir_instr_debug_info *debug_info = nir_instr_get_debug_info(nir_def_instr(def));
      if (debug_info->variable_name)
         fprintf(fp, ".%s", debug_info->variable_name);
   }
}

static unsigned
calculate_padding_for_no_dest(print_state *state)
{
   const unsigned div = state->divergence_valid ? 4 : 0;
   const unsigned ssa_size = 5;
   const unsigned percent = 1;
   const unsigned ssa_index = count_digits(state->max_dest_index);
   const unsigned equals = 1;
   return ssa_size + 1 + div + percent + ssa_index + 1 + equals + 1;
}

static void
print_no_dest_padding(print_state *state)
{
   FILE *fp = state->fp;

   if (state->padding_for_no_dest)
      fprintf(fp, "%*s", state->padding_for_no_dest, "");
}

static void
print_hex_padded_const_value(const nir_const_value *value, unsigned bit_size, FILE *fp)
{
   switch (bit_size) {
   case 64:
      fprintf(fp, "0x%016" PRIx64, value->u64);
      break;
   case 32:
      fprintf(fp, "0x%08x", value->u32);
      break;
   case 16:
      fprintf(fp, "0x%04x", value->u16);
      break;
   case 8:
      fprintf(fp, "0x%02x", value->u8);
      break;
   default:
      UNREACHABLE("unhandled bit size");
   }
}

static void
print_hex_terse_const_value(const nir_const_value *value, unsigned bit_size, FILE *fp)
{
   switch (bit_size) {
   case 64:
      fprintf(fp, "0x%" PRIx64, value->u64);
      break;
   case 32:
      fprintf(fp, "0x%x", value->u32);
      break;
   case 16:
      fprintf(fp, "0x%x", value->u16);
      break;
   case 8:
      fprintf(fp, "0x%x", value->u8);
      break;
   default:
      UNREACHABLE("unhandled bit size");
   }
}

static void
print_float_const_value(const nir_const_value *value, unsigned bit_size, FILE *fp)
{
   double dval = nir_const_value_as_float(*value, bit_size);

   if (fabs(dval) >= 1000000.0)
      fprintf(fp, "%e", dval);
   else
      fprintf(fp, "%f", dval);
}

static void
print_int_const_value(const nir_const_value *value, unsigned bit_size, FILE *fp)
{
   switch (bit_size) {
   case 64:
      fprintf(fp, "%+" PRIi64, value->i64);
      break;
   case 32:
      fprintf(fp, "%+d", value->i32);
      break;
   case 16:
      fprintf(fp, "%+d", value->i16);
      break;
   case 8:
      fprintf(fp, "%+d", value->i8);
      break;
   default:
      UNREACHABLE("unhandled bit size");
   }
}

static void
print_uint_const_value(const nir_const_value *value, unsigned bit_size, FILE *fp)
{
   switch (bit_size) {
   case 64:
      fprintf(fp, "%" PRIu64, value->u64);
      break;
   case 32:
      fprintf(fp, "%u", value->u32);
      break;
   case 16:
      fprintf(fp, "%u", value->u16);
      break;
   case 8:
      fprintf(fp, "%u", value->u8);
      break;
   default:
      UNREACHABLE("unhandled bit size");
   }
}

static void
print_const_from_load(nir_load_const_instr *instr, print_state *state, nir_alu_type type)
{
   FILE *fp = state->fp;

   const unsigned bit_size = instr->def.bit_size;
   const unsigned num_components = instr->def.num_components;

   type = nir_alu_type_get_base_type(type);

   /* There's only one way to print booleans. */
   if (bit_size == 1 || type == nir_type_bool) {
      fprintf(fp, "(");
      for (unsigned i = 0; i < num_components; i++) {
         if (i != 0)
            fprintf(fp, ", ");
         fprintf(fp, "%s", instr->value[i].b ? "true" : "false");
      }
      fprintf(fp, ")");
      return;
   }

   fprintf(fp, "(");

   if (type != nir_type_invalid) {
      for (unsigned i = 0; i < num_components; i++) {
         const nir_const_value *v = &instr->value[i];
         if (i != 0)
            fprintf(fp, ", ");
         switch (type) {
         case nir_type_float:
            print_float_const_value(v, bit_size, fp);
            break;
         case nir_type_int:
         case nir_type_uint:
            print_hex_terse_const_value(v, bit_size, fp);
            break;

         default:
            UNREACHABLE("invalid nir alu base type");
         }
      }
   } else {
#define PRINT_VALUES(F)                               \
   do {                                               \
      for (unsigned i = 0; i < num_components; i++) { \
         if (i != 0)                                  \
            fprintf(fp, ", ");                        \
         F(&instr->value[i], bit_size, fp);           \
      }                                               \
   } while (0)

#define SEPARATOR()         \
   if (num_components > 1)  \
      fprintf(fp, ") = ("); \
   else                     \
      fprintf(fp, " = ")

      bool needs_float = bit_size > 8;
      bool needs_signed = false;
      bool needs_decimal = false;
      for (unsigned i = 0; i < num_components; i++) {
         const nir_const_value *v = &instr->value[i];
         switch (bit_size) {
         case 64:
            needs_signed |= v->i64 < 0;
            needs_decimal |= v->u64 >= 10;
            break;
         case 32:
            needs_signed |= v->i32 < 0;
            needs_decimal |= v->u32 >= 10;
            break;
         case 16:
            needs_signed |= v->i16 < 0;
            needs_decimal |= v->u16 >= 10;
            break;
         case 8:
            needs_signed |= v->i8 < 0;
            needs_decimal |= v->u8 >= 10;
            break;
         default:
            UNREACHABLE("invalid bit size");
         }
      }

      if (state->int_types) {
         const unsigned index = instr->def.index;
         const bool inferred_int = BITSET_TEST(state->int_types, index);
         const bool inferred_float = BITSET_TEST(state->float_types, index);

         if (inferred_int && !inferred_float) {
            needs_float = false;
         } else if (inferred_float && !inferred_int) {
            needs_signed = false;
            needs_decimal = false;
         }
      }

      PRINT_VALUES(print_hex_padded_const_value);

      if (needs_float) {
         SEPARATOR();
         PRINT_VALUES(print_float_const_value);
      }

      if (needs_signed) {
         SEPARATOR();
         PRINT_VALUES(print_int_const_value);
      }

      if (needs_decimal) {
         SEPARATOR();
         PRINT_VALUES(print_uint_const_value);
      }
   }

   fprintf(fp, ")");
}

static void
print_load_const_instr(nir_load_const_instr *instr, print_state *state)
{
   FILE *fp = state->fp;

   print_def(&instr->def, state);

   fprintf(fp, " = load_const ");

   /* In the definition, print all interpretations of the value. */
   print_const_from_load(instr, state, nir_type_invalid);
}

static void
print_src(const nir_src *src, print_state *state, nir_alu_type src_type)
{
   FILE *fp = state->fp;
   fprintf(fp, "%s%u", state->def_prefix, src->ssa->index);
   nir_instr *instr = nir_def_instr(src->ssa);

   if (instr->has_debug_info) {
      nir_instr_debug_info *debug_info = nir_instr_get_debug_info(instr);
      if (debug_info->variable_name)
         fprintf(fp, ".%s", debug_info->variable_name);
   }

   if (instr->type == nir_instr_type_load_const && !NIR_DEBUG(PRINT_NO_INLINE_CONSTS)) {
      nir_load_const_instr *load_const = nir_instr_as_load_const(instr);
      fprintf(fp, " ");

      nir_alu_type type = nir_alu_type_get_base_type(src_type);

      if (type == nir_type_invalid && state->int_types) {
         const unsigned index = load_const->def.index;
         const bool inferred_int = BITSET_TEST(state->int_types, index);
         const bool inferred_float = BITSET_TEST(state->float_types, index);

         if (inferred_float && !inferred_int)
            type = nir_type_float;
      }

      if (type == nir_type_invalid)
         type = nir_type_uint;

      /* For a constant in a source, always pick one interpretation. */
      assert(type != nir_type_invalid);
      print_const_from_load(load_const, state, type);
   }
}

static const char *
comp_mask_string(unsigned num_components)
{
   return (num_components > 4) ? "abcdefghijklmnop" : "xyzw";
}

static void
print_alu_src(nir_alu_instr *instr, unsigned src, print_state *state)
{
   FILE *fp = state->fp;

   const nir_op_info *info = &nir_op_infos[instr->op];
   print_src(&instr->src[src].src, state, info->input_types[src]);

   bool print_swizzle = false;
   nir_component_mask_t used_channels = 0;

   for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++) {
      if (!nir_alu_instr_channel_used(instr, src, i))
         continue;

      used_channels++;

      if (instr->src[src].swizzle[i] != i) {
         print_swizzle = true;
         break;
      }
   }

   unsigned live_channels = nir_src_num_components(instr->src[src].src);

   if (print_swizzle || used_channels != live_channels) {
      fprintf(fp, ".");
      for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++) {
         if (!nir_alu_instr_channel_used(instr, src, i))
            continue;

         fprintf(fp, "%c", comp_mask_string(live_channels)[instr->src[src].swizzle[i]]);
      }
   }
}

static void
print_fp_math_ctrl(unsigned fp_math_ctrl, print_state *state)
{
   FILE *fp = state->fp;

   if (fp_math_ctrl & nir_fp_exact) {
      if ((fp_math_ctrl & nir_fp_exact) == nir_fp_exact) {
         fprintf(fp, "exact");
      } else if (fp_math_ctrl & nir_fp_no_contract) {
         fprintf(fp, "no-contract");
      } else if (fp_math_ctrl & nir_fp_no_reassoc) {
         fprintf(fp, "no-reassoc");
      } else if (fp_math_ctrl & nir_fp_no_transform) {
         fprintf(fp, "no-transform");
      }

      if (fp_math_ctrl & ~nir_fp_exact)
         fprintf(fp, ", ");
   }

   if (fp_math_ctrl & nir_fp_preserve_sz_inf_nan) {
      fprintf(fp, "preserve:");

      static const struct {
         nir_fp_math_control bit;
         const char *name;
      } preserve_bits[] = {
         { nir_fp_preserve_signed_zero, "sz" },
         { nir_fp_preserve_inf, "inf" },
         { nir_fp_preserve_nan, "nan" },
      };

      bool first = true;
      for (unsigned i = 0; i < ARRAY_SIZE(preserve_bits); i++) {
         if (fp_math_ctrl & preserve_bits[i].bit) {
            if (!first)
               fprintf(fp, ",");
            first = false;
            fprintf(fp, "%s", preserve_bits[i].name);
         }
      }
   }
}

static void
print_alu_instr(nir_alu_instr *instr, print_state *state)
{
   FILE *fp = state->fp;

   print_def(&instr->def, state);

   fprintf(fp, " = %s", nir_op_infos[instr->op].name);
   if (instr->no_signed_wrap)
      fprintf(fp, ".nsw");
   if (instr->no_unsigned_wrap)
      fprintf(fp, ".nuw");
   fprintf(fp, " ");

   for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++) {
      if (i != 0)
         fprintf(fp, ", ");

      print_alu_src(instr, i, state);
   }

   if (instr->fp_math_ctrl) {
      fprintf(fp, " // ");
      print_fp_math_ctrl(instr->fp_math_ctrl, state);
   }
}

static const char *
get_name(const void *ctx, const char *identifier, const char *default_name,
         print_state *state)
{
   if (state->ht == NULL)
      return identifier ? identifier : "unnamed";

   assert(state->syms);

   struct hash_entry *entry = _mesa_hash_table_search(state->ht, ctx);
   if (entry)
      return entry->data;

   char *name;
   if (identifier == NULL || strlen(identifier) == 0) {
      name = ralloc_asprintf(state->syms, "%s#%u", default_name, state->index++);
   } else {
      struct set_entry *set_entry = _mesa_set_search(state->syms, identifier);
      if (set_entry != NULL) {
         /* we have a collision with another name, append an # + a unique
          * index */
         name = ralloc_asprintf(state->syms, "%s#%u", identifier,
                                state->index++);
      } else {
         /* Mark this one as seen */
         _mesa_set_add(state->syms, identifier);
         name = (char *)identifier;
      }
   }

   _mesa_hash_table_insert(state->ht, ctx, name);

   return name;
}

static const char *
get_var_name(nir_variable *var, print_state *state)
{
   return get_name(var, var->name, "", state);
}

static const char *
get_type_name(const struct glsl_type *type, print_state *state)
{
   return get_name(type, glsl_get_type_name(type), "type", state);
}

static const char *
get_constant_sampler_addressing_mode(enum cl_sampler_addressing_mode mode)
{
   switch (mode) {
   case SAMPLER_ADDRESSING_MODE_NONE:
      return "none";
   case SAMPLER_ADDRESSING_MODE_CLAMP_TO_EDGE:
      return "clamp_to_edge";
   case SAMPLER_ADDRESSING_MODE_CLAMP:
      return "clamp";
   case SAMPLER_ADDRESSING_MODE_REPEAT:
      return "repeat";
   case SAMPLER_ADDRESSING_MODE_REPEAT_MIRRORED:
      return "repeat_mirrored";
   default:
      UNREACHABLE("Invalid addressing mode");
   }
}

static const char *
get_constant_sampler_filter_mode(enum cl_sampler_filter_mode mode)
{
   switch (mode) {
   case SAMPLER_FILTER_MODE_NEAREST:
      return "nearest";
   case SAMPLER_FILTER_MODE_LINEAR:
      return "linear";
   default:
      UNREACHABLE("Invalid filter mode");
   }
}

static void
print_constant(nir_constant *c, const struct glsl_type *type, print_state *state)
{
   FILE *fp = state->fp;
   const unsigned rows = glsl_get_vector_elements(type);
   const unsigned cols = glsl_get_matrix_columns(type);
   unsigned i;

   switch (glsl_get_base_type(type)) {
   case GLSL_TYPE_BOOL:
      /* Only float base types can be matrices. */
      assert(cols == 1);

      for (i = 0; i < rows; i++) {
         if (i > 0)
            fprintf(fp, ", ");
         fprintf(fp, "%s", c->values[i].b ? "true" : "false");
      }
      break;

   case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
      /* Only float base types can be matrices. */
      assert(cols == 1);

      for (i = 0; i < rows; i++) {
         if (i > 0)
            fprintf(fp, ", ");
         fprintf(fp, "0x%02x", c->values[i].u8);
      }
      break;

   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
      /* Only float base types can be matrices. */
      assert(cols == 1);

      for (i = 0; i < rows; i++) {
         if (i > 0)
            fprintf(fp, ", ");
         fprintf(fp, "0x%04x", c->values[i].u16);
      }
      break;

   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
      /* Only float base types can be matrices. */
      assert(cols == 1);

      for (i = 0; i < rows; i++) {
         if (i > 0)
            fprintf(fp, ", ");
         fprintf(fp, "0x%08x", c->values[i].u32);
      }
      break;

   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_DOUBLE:
      if (cols > 1) {
         for (i = 0; i < cols; i++) {
            if (i > 0)
               fprintf(fp, ", ");
            print_constant(c->elements[i], glsl_get_column_type(type), state);
         }
      } else {
         switch (glsl_get_base_type(type)) {
         case GLSL_TYPE_FLOAT16:
            for (i = 0; i < rows; i++) {
               if (i > 0)
                  fprintf(fp, ", ");
               fprintf(fp, "%f", _mesa_half_to_float(c->values[i].u16));
            }
            break;

         case GLSL_TYPE_FLOAT:
            for (i = 0; i < rows; i++) {
               if (i > 0)
                  fprintf(fp, ", ");
               fprintf(fp, "%f", c->values[i].f32);
            }
            break;

         case GLSL_TYPE_DOUBLE:
            for (i = 0; i < rows; i++) {
               if (i > 0)
                  fprintf(fp, ", ");
               fprintf(fp, "%f", c->values[i].f64);
            }
            break;

         default:
            UNREACHABLE("Cannot get here from the first level switch");
         }
      }
      break;

   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64:
      /* Only float base types can be matrices. */
      assert(cols == 1);

      for (i = 0; i < cols; i++) {
         if (i > 0)
            fprintf(fp, ", ");
         fprintf(fp, "0x%08" PRIx64, c->values[i].u64);
      }
      break;

   case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_INTERFACE:
      for (i = 0; i < c->num_elements; i++) {
         if (i > 0)
            fprintf(fp, ", ");
         fprintf(fp, "{ ");
         print_constant(c->elements[i], glsl_get_struct_field(type, i), state);
         fprintf(fp, " }");
      }
      break;

   case GLSL_TYPE_ARRAY:
      for (i = 0; i < c->num_elements; i++) {
         if (i > 0)
            fprintf(fp, ", ");
         fprintf(fp, "{ ");
         print_constant(c->elements[i], glsl_get_array_element(type), state);
         fprintf(fp, " }");
      }
      break;

   case GLSL_TYPE_COOPERATIVE_MATRIX:
      // This occurs as the constant initializer for a cmat variable.
      // In this case it's a scalar constant, and its word value is
      // c->values[0], but we have to interpet it via the component type.
      fprintf(fp, "%s(", glsl_get_type_name(type));
      print_constant(c, glsl_get_cmat_element(type), state);
      fprintf(fp, ")");
      break;

   default:
      UNREACHABLE("not reached");
   }
}

static const char *
get_variable_mode_str(nir_variable_mode mode, bool want_local_global_mode)
{
   switch (mode) {
   case nir_var_shader_in:
      return "shader_in";
   case nir_var_shader_out:
      return "shader_out";
   case nir_var_uniform:
      return "uniform";
   case nir_var_mem_ubo:
      return "ubo";
   case nir_var_system_value:
      return "system";
   case nir_var_mem_ssbo:
      return "ssbo";
   case nir_var_mem_shared:
      return "shared";
   case nir_var_mem_global:
      return "global";
   case nir_var_mem_push_const:
      return "push_const";
   case nir_var_mem_constant:
      return "constant";
   case nir_var_mem_pixel_local_in:
      return "pixel_local_in";
   case nir_var_mem_pixel_local_out:
      return "pixel_local_out";
   case nir_var_mem_pixel_local_inout:
      return "pixel_local";
   case nir_var_image:
      return "image";
   case nir_var_shader_temp:
      return want_local_global_mode ? "shader_temp" : "";
   case nir_var_function_temp:
      return want_local_global_mode ? "function_temp" : "";
   case nir_var_shader_call_data:
      return "shader_call_data";
   case nir_var_ray_hit_attrib:
      return "ray_hit_attrib";
   case nir_var_mem_task_payload:
      return "task_payload";
   case nir_var_mem_node_payload:
      return "node_payload";
   case nir_var_mem_node_payload_in:
      return "node_payload_in";
   case nir_var_resource_heap:
      return "resource_heap";
   case nir_var_sampler_heap:
      return "sampler_heap";
   default:
      if (mode && (mode & nir_var_mem_generic) == mode)
         return "generic";
      return "";
   }
}

static const char *
get_location_str(unsigned location, mesa_shader_stage stage,
                 nir_variable_mode mode, char *buf)
{
   switch (stage) {
   case MESA_SHADER_VERTEX:
      if (mode == nir_var_shader_in)
         return gl_vert_attrib_name(location);
      else if (mode == nir_var_shader_out)
         return gl_varying_slot_name_for_stage(location, stage);

      break;
   case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_TESS_EVAL:
   case MESA_SHADER_TASK:
   case MESA_SHADER_MESH:
   case MESA_SHADER_GEOMETRY:
      if (mode == nir_var_shader_in || mode == nir_var_shader_out)
         return gl_varying_slot_name_for_stage(location, stage);

      break;
   case MESA_SHADER_FRAGMENT:
      if (mode == nir_var_shader_in)
         return gl_varying_slot_name_for_stage(location, stage);
      else if (mode == nir_var_shader_out)
         return gl_frag_result_name(location);

      break;
   case MESA_SHADER_COMPUTE:
   case MESA_SHADER_KERNEL:
   default:
      /* TODO */
      break;
   }

   if (mode == nir_var_system_value)
      return gl_system_value_name(location);

   if (location == ~0) {
      return "~0";
   } else {
      snprintf(buf, 4, "%u", location);
      return buf;
   }
}

static void
print_access(enum gl_access_qualifier access, print_state *state, const char *separator)
{
   if (!access) {
      fputs("none", state->fp);
      return;
   }

   static const struct {
      enum gl_access_qualifier bit;
      const char *name;
   } modes[] = {
      { ACCESS_COHERENT, "coherent" },
      { ACCESS_RESTRICT, "restrict" },
      { ACCESS_VOLATILE, "volatile" },
      { ACCESS_NON_WRITEABLE, "readonly" },
      { ACCESS_NON_READABLE, "writeonly" },
      { ACCESS_NON_UNIFORM, "non-uniform" },
      { ACCESS_CAN_REORDER, "reorderable" },
      { ACCESS_NON_TEMPORAL, "non-temporal" },
      { ACCESS_INCLUDE_HELPERS, "include-helpers" },
      { ACCESS_IS_SWIZZLED_AMD, "is-swizzled-amd" },
      { ACCESS_USES_FORMAT_AMD, "uses-format-amd" },
      { ACCESS_FMASK_LOWERED_AMD, "fmask-lowered-amd" },
      { ACCESS_CAN_SPECULATE, "speculatable" },
      { ACCESS_CP_GE_COHERENT_AMD, "cp-ge-coherent-amd" },
      { ACCESS_IN_BOUNDS, "in-bounds" },
      { ACCESS_KEEP_SCALAR, "keep-scalar" },
      { ACCESS_SMEM_AMD, "smem-amd" },
      { ACCESS_SKIP_HELPERS, "skip-helpers" },
      { ACCESS_ATOMIC, "atomic" },
      { ACCESS_FUSED_EU_DISABLE_INTEL, "fused-eu-disable-intel" },
      { ACCESS_SPARSE, "sparse" },
      { ACCESS_ISTREAM_PAN, "istream-pan" },
      { ACCESS_ESTREAM_PAN, "estream-pan" },
   };

   bool first = true;
   for (unsigned i = 0; i < ARRAY_SIZE(modes); ++i) {
      if (access & modes[i].bit) {
         fprintf(state->fp, "%s%s", first ? "" : separator, modes[i].name);
         first = false;
      }
   }
}

static void
print_struct_decl(const struct glsl_type *type, print_state *state)
{
   if (_mesa_set_search(state->struct_types, type))
      return;

   _mesa_set_add(state->struct_types, type);

   FILE *fp = state->fp;

   for (uint32_t i = 0; i < type->length; i++) {
      const struct glsl_type *field_type =
         glsl_without_array(glsl_get_struct_field(type, i));

      if (glsl_type_is_struct_or_ifc(field_type))
         print_struct_decl(field_type, state);
   }

   fprintf(fp, "struct %s {\n", get_type_name(type, state));

   for (uint32_t i = 0; i < type->length; i++) {
      fprintf(fp, "   %s %s;\n", get_type_name(glsl_get_struct_field(type, i), state),
              glsl_get_struct_elem_name(type, i));
   }

   fprintf(fp, "}\n");
}

static void
print_var_decl(nir_variable *var, print_state *state)
{
   FILE *fp = state->fp;

   fprintf(fp, "decl_var ");

   const char *const bindless = (var->data.bindless) ? "bindless " : "";
   const char *const cent = (var->data.centroid) ? "centroid " : "";
   const char *const samp = (var->data.sample) ? "sample " : "";
   const char *const patch = (var->data.patch) ? "patch " : "";
   const char *const inv = (var->data.invariant) ? "invariant " : "";
   const char *const per_view = (var->data.per_view) ? "per_view " : "";
   const char *const per_primitive = (var->data.per_primitive) ? "per_primitive " : "";
   const char *const per_vertex = (var->data.per_vertex) ? "per_vertex " : "";
   const char *const ray_query = (var->data.ray_query) ? "ray_query " : "";
   const char *const fb_fetch = var->data.fb_fetch_output ? "fb_fetch_output " : "";
   fprintf(fp, "%s%s%s%s%s%s%s%s%s%s%s %s ",
           bindless, cent, samp, patch, inv, per_view, per_primitive, per_vertex,
           ray_query, fb_fetch,
           get_variable_mode_str(var->data.mode, false),
           glsl_interp_mode_name(var->data.interpolation));

   print_access(var->data.access, state, " ");
   fprintf(fp, " ");

   if (glsl_get_base_type(glsl_without_array(var->type)) == GLSL_TYPE_IMAGE) {
      fprintf(fp, "%s ", util_format_short_name(var->data.image.format));
   }

   if (var->data.precision) {
      const char *precisions[] = {
         "",
         "highp",
         "mediump",
         "lowp",
      };
      fprintf(fp, "%s ", precisions[var->data.precision]);
   }

   fprintf(fp, "%s %s", get_type_name(var->type, state),
           get_var_name(var, state));

   if (var->data.mode & (nir_var_shader_in |
                         nir_var_shader_out |
                         nir_var_uniform |
                         nir_var_system_value |
                         nir_var_mem_ubo |
                         nir_var_mem_ssbo |
                         nir_var_image)) {
      char buf[4];
      const char *loc = get_location_str(var->data.location,
                                         state->shader->info.stage,
                                         var->data.mode, buf);

      /* For shader I/O vars that have been split to components or packed,
       * print the fractional location within the input/output.
       */
      unsigned int num_components =
         glsl_get_components(glsl_without_array(var->type));
      const char *components = "";
      char components_local[18] = { '.' /* the rest is 0-filled */ };
      switch (var->data.mode) {
      case nir_var_shader_in:
      case nir_var_shader_out:
         if (num_components < 16 && num_components != 0) {
            const char *xyzw = comp_mask_string(num_components);
            for (int i = 0; i < num_components; i++)
               components_local[i + 1] = xyzw[i + var->data.location_frac];

            components = components_local;
         }
         break;
      default:
         break;
      }

      if (var->data.mode & nir_var_system_value) {
         fprintf(fp, " (%s%s)", loc, components);
      } else {
         fprintf(fp, " (%s%s, %u, %u)%s", loc,
                 components,
                 var->data.driver_location, var->data.binding,
                 var->data.compact ? " compact" : "");
      }
   }

   if (var->constant_initializer) {
      if (var->constant_initializer->is_null_constant) {
         fprintf(fp, " = null");
      } else {
         fprintf(fp, " = { ");
         print_constant(var->constant_initializer, var->type, state);
         fprintf(fp, " }");
      }
   }
   if (glsl_type_is_sampler(var->type) && var->data.sampler.is_inline_sampler) {
      fprintf(fp, " = { %s, %s, %s }",
              get_constant_sampler_addressing_mode(var->data.sampler.addressing_mode),
              var->data.sampler.normalized_coordinates ? "true" : "false",
              get_constant_sampler_filter_mode(var->data.sampler.filter_mode));
   }
   if (var->pointer_initializer)
      fprintf(fp, " = &%s", get_var_name(var->pointer_initializer, state));

   print_annotation(state, var);

   fprintf(fp, "\n");
}

static void
print_deref_link(const nir_deref_instr *instr, bool whole_chain, print_state *state)
{
   FILE *fp = state->fp;

   if (instr->deref_type == nir_deref_type_var) {
      fprintf(fp, "%s", get_var_name(instr->var, state));
      return;
   } else if (instr->deref_type == nir_deref_type_cast) {
      fprintf(fp, "(%s *)", get_type_name(instr->type, state));
      print_src(&instr->parent, state, nir_type_invalid);
      return;
   }

   nir_deref_instr *parent =
      nir_def_as_deref(instr->parent.ssa);

   /* Is the parent we're going to print a bare cast? */
   const bool is_parent_cast =
      whole_chain && parent->deref_type == nir_deref_type_cast;

   /* If we're not printing the whole chain, the parent we print will be a SSA
    * value that represents a pointer.  The only deref type that naturally
    * gives a pointer is a cast.
    */
   const bool is_parent_pointer =
      !whole_chain || parent->deref_type == nir_deref_type_cast;

   /* Struct derefs have a nice syntax that works on pointers, arrays derefs
    * do not.
    */
   const bool need_deref =
      is_parent_pointer && instr->deref_type != nir_deref_type_struct;

   /* Cast need extra parens and so * dereferences */
   if (is_parent_cast || need_deref)
      fprintf(fp, "(");

   if (need_deref)
      fprintf(fp, "*");

   if (whole_chain) {
      print_deref_link(parent, whole_chain, state);
   } else {
      print_src(&instr->parent, state, nir_type_invalid);
   }

   if (is_parent_cast || need_deref)
      fprintf(fp, ")");

   switch (instr->deref_type) {
   case nir_deref_type_struct:
      fprintf(fp, "%s%s", is_parent_pointer ? "->" : ".",
              glsl_get_struct_elem_name(parent->type, instr->strct.index));
      if (whole_chain &&
          parent->type->fields.structure[instr->strct.index].pixel_local_storage) {
         fprintf(fp, " (%s)",
                 util_format_short_name(parent->type->fields.structure[instr->strct.index].image_format));
      }
      break;

   case nir_deref_type_array:
   case nir_deref_type_ptr_as_array: {
      if (nir_src_is_const(instr->arr.index)) {
         fprintf(fp, "[%" PRId64 "]", nir_src_as_int(instr->arr.index));
      } else {
         fprintf(fp, "[");
         print_src(&instr->arr.index, state, nir_type_invalid);
         fprintf(fp, "]");
      }
      break;
   }

   case nir_deref_type_array_wildcard:
      fprintf(fp, "[*]");
      break;

   default:
      UNREACHABLE("Invalid deref instruction type");
   }
}

static void
print_deref_instr(nir_deref_instr *instr, print_state *state)
{
   FILE *fp = state->fp;

   print_def(&instr->def, state);

   switch (instr->deref_type) {
   case nir_deref_type_var:
      fprintf(fp, " = deref_var ");
      break;
   case nir_deref_type_array:
   case nir_deref_type_array_wildcard:
      fprintf(fp, " = deref_array ");
      break;
   case nir_deref_type_struct:
      fprintf(fp, " = deref_struct ");
      break;
   case nir_deref_type_cast:
      fprintf(fp, " = deref_cast ");
      break;
   case nir_deref_type_ptr_as_array:
      fprintf(fp, " = deref_ptr_as_array ");
      break;
   default:
      UNREACHABLE("Invalid deref instruction type");
   }

   /* Only casts naturally return a pointer type */
   if (instr->deref_type != nir_deref_type_cast)
      fprintf(fp, "&");

   print_deref_link(instr, false, state);

   fprintf(fp, " (");
   unsigned modes = instr->modes;
   while (modes) {
      int m = u_bit_scan(&modes);
      fprintf(fp, "%s%s", get_variable_mode_str(1 << m, true),
              modes ? "|" : "");
   }

   nir_variable *var = nir_deref_instr_get_variable(instr);
   if (var) {
      static const char *precision_str[] = {"", " highp", " mediump", " lowp"};
      fprintf(fp, "%s", precision_str[var->data.precision]);
   }

   fprintf(fp, " %s)", get_type_name(instr->type, state));

   if (instr->deref_type == nir_deref_type_cast) {
      fprintf(fp, "  (ptr_stride=%u, align_mul=%u, align_offset=%u)",
              instr->cast.ptr_stride,
              instr->cast.align_mul, instr->cast.align_offset);
   }

   if (nir_deref_instr_is_arr(instr)) {
      if (instr->arr.in_bounds)
         fprintf(fp, "  (in bounds)");
   }

   if (instr->deref_type != nir_deref_type_var &&
       instr->deref_type != nir_deref_type_cast) {
      /* Print the entire chain as a comment */
      fprintf(fp, "  // &");
      print_deref_link(instr, true, state);
   }
}

static const char *
nir_descriptor_type_name(nir_descriptor_type type)
{
   switch (type) {
   case nir_descriptor_type_uniform_buffer:
      return "UBO";
   case nir_descriptor_type_storage_buffer:
      return "SSBO";
   case nir_descriptor_type_acceleration_structure:
      return "accel-struct";
   default:
      return "unknown";
   }
}

static const char *
nir_resource_type_name(nir_resource_type type)
{
   switch (type) {
   case nir_resource_type_sampler:
      return "sampler";
   case nir_resource_type_sampled_image:
      return "texture";
   case nir_resource_type_read_only_image:
      return "RO-image";
   case nir_resource_type_read_write_image:
      return "RW-image";
   case nir_resource_type_combined_sampled_image:
      return "texture+sampler";
   case nir_resource_type_uniform_buffer:
      return "UBO";
   case nir_resource_type_read_only_storage_buffer:
      return "RO-SSBO";
   case nir_resource_type_read_write_storage_buffer:
      return "RW-SSBO";
   case nir_resource_type_acceleration_structure:
      return "accel-struct";
   default:
      return "unknown";
   }
}

static void
print_alu_type(nir_alu_type type, print_state *state)
{
   FILE *fp = state->fp;
   unsigned size = nir_alu_type_get_type_size(type);
   const char *name;

   switch (nir_alu_type_get_base_type(type)) {
   case nir_type_int:
      name = "int";
      break;
   case nir_type_uint:
      name = "uint";
      break;
   case nir_type_bool:
      name = "bool";
      break;
   case nir_type_float:
      name = "float";
      break;
   default:
      name = "invalid";
   }
   if (size)
      fprintf(fp, "%s%u", name, size);
   else
      fprintf(fp, "%s", name);
}

static const char *sampler_dim_name[] = {
   [GLSL_SAMPLER_DIM_1D] = "1D",
   [GLSL_SAMPLER_DIM_2D] = "2D",
   [GLSL_SAMPLER_DIM_3D] = "3D",
   [GLSL_SAMPLER_DIM_CUBE] = "Cube",
   [GLSL_SAMPLER_DIM_RECT] = "Rect",
   [GLSL_SAMPLER_DIM_BUF] = "Buf",
   [GLSL_SAMPLER_DIM_EXTERNAL] = "External",
   [GLSL_SAMPLER_DIM_MS] = "2D-MSAA",
   [GLSL_SAMPLER_DIM_SUBPASS] = "Subpass",
   [GLSL_SAMPLER_DIM_SUBPASS_MS] = "Subpass-MSAA",
};

static void
print_cmat_description(struct glsl_cmat_description desc, print_state *state)
{
   FILE *fp = state->fp;
   const struct glsl_type *t = glsl_cmat_type(&desc);
   fprintf(fp, "%s", get_type_name(t, state));
}

static void
print_intrinsic_instr(nir_intrinsic_instr *instr, print_state *state)
{
   const nir_intrinsic_info *info = &nir_intrinsic_infos[instr->intrinsic];
   unsigned num_srcs = info->num_srcs;
   FILE *fp = state->fp;

   if (info->has_dest) {
      print_def(&instr->def, state);
      fprintf(fp, " = ");
   } else {
      print_no_dest_padding(state);
   }

   fprintf(fp, "@%s", info->name);

   for (unsigned i = 0; i < num_srcs; i++) {
      if (i == 0)
         fprintf(fp, " (");
      else
         fprintf(fp, ", ");

      print_src(&instr->src[i], state, nir_intrinsic_instr_src_type(instr, i));
   }

   if (num_srcs)
      fprintf(fp, ")");

   for (unsigned i = 0; i < info->num_indices; i++) {
      unsigned idx = info->indices[i];

      /* Skip "general" to denoise since it is the unremarkable default case */
      if (idx == NIR_INTRINSIC_PREAMBLE_CLASS &&
          nir_intrinsic_preamble_class(instr) == nir_preamble_class_general)
         continue;

      if (i == 0)
         fprintf(fp, " (");
      else
         fprintf(fp, ", ");
      switch (idx) {
      case NIR_INTRINSIC_WRITE_MASK: {
         /* special case wrmask to show it as a writemask.. */
         unsigned wrmask = nir_intrinsic_write_mask(instr);
         fprintf(fp, "wrmask=");
         for (unsigned i = 0; i < instr->num_components; i++)
            if ((wrmask >> i) & 1)
               fprintf(fp, "%c", comp_mask_string(instr->num_components)[i]);
         break;
      }

      case NIR_INTRINSIC_REDUCTION_OP: {
         nir_op reduction_op = nir_intrinsic_reduction_op(instr);
         fprintf(fp, "reduction_op=%s", nir_op_infos[reduction_op].name);
         break;
      }

      case NIR_INTRINSIC_ATOMIC_OP: {
         nir_atomic_op atomic_op = nir_intrinsic_atomic_op(instr);
         fprintf(fp, "atomic_op=");

         switch (atomic_op) {
         case nir_atomic_op_iadd:
            fprintf(fp, "iadd");
            break;
         case nir_atomic_op_isub:
            fprintf(fp, "isub");
            break;
         case nir_atomic_op_imin:
            fprintf(fp, "imin");
            break;
         case nir_atomic_op_umin:
            fprintf(fp, "umin");
            break;
         case nir_atomic_op_imax:
            fprintf(fp, "imax");
            break;
         case nir_atomic_op_umax:
            fprintf(fp, "umax");
            break;
         case nir_atomic_op_iand:
            fprintf(fp, "iand");
            break;
         case nir_atomic_op_ior:
            fprintf(fp, "ior");
            break;
         case nir_atomic_op_ixor:
            fprintf(fp, "ixor");
            break;
         case nir_atomic_op_xchg:
            fprintf(fp, "xchg");
            break;
         case nir_atomic_op_fadd:
            fprintf(fp, "fadd");
            break;
         case nir_atomic_op_fmin:
            fprintf(fp, "fmin");
            break;
         case nir_atomic_op_fmax:
            fprintf(fp, "fmax");
            break;
         case nir_atomic_op_cmpxchg:
            fprintf(fp, "cmpxchg");
            break;
         case nir_atomic_op_fcmpxchg:
            fprintf(fp, "fcmpxchg");
            break;
         case nir_atomic_op_inc_wrap:
            fprintf(fp, "inc_wrap");
            break;
         case nir_atomic_op_dec_wrap:
            fprintf(fp, "dec_wrap");
            break;
         case nir_atomic_op_ordered_add_gfx12_amd:
            fprintf(fp, "ordered_add");
            break;
         }
         break;
      }

      case NIR_INTRINSIC_IMAGE_DIM: {
         enum glsl_sampler_dim dim = nir_intrinsic_image_dim(instr);
         assert(dim < ARRAY_SIZE(sampler_dim_name) && sampler_dim_name[dim]);
         fprintf(fp, "image_dim=%s", sampler_dim_name[dim]);
         break;
      }

      case NIR_INTRINSIC_IMAGE_ARRAY: {
         bool array = nir_intrinsic_image_array(instr);
         fprintf(fp, "image_array=%s", array ? "true" : "false");
         break;
      }

      case NIR_INTRINSIC_FORMAT: {
         enum pipe_format format = nir_intrinsic_format(instr);
         fprintf(fp, "format=%s", util_format_short_name(format));
         break;
      }

      case NIR_INTRINSIC_DESC_TYPE: {
         nir_descriptor_type desc_type = nir_intrinsic_desc_type(instr);
         fprintf(fp, "desc_type=%s", nir_descriptor_type_name(desc_type));
         break;
      }

      case NIR_INTRINSIC_RESOURCE_TYPE: {
         nir_resource_type res_type = nir_intrinsic_resource_type(instr);
         fprintf(fp, "resource_type=%s", nir_resource_type_name(res_type));
         break;
      }

      case NIR_INTRINSIC_SRC_TYPE: {
         fprintf(fp, "src_type=");
         print_alu_type(nir_intrinsic_src_type(instr), state);
         break;
      }

      case NIR_INTRINSIC_DEST_TYPE: {
         fprintf(fp, "dest_type=");
         print_alu_type(nir_intrinsic_dest_type(instr), state);
         break;
      }

      case NIR_INTRINSIC_SWIZZLE_MASK: {
         fprintf(fp, "swizzle_mask=");
         unsigned mask = nir_intrinsic_swizzle_mask(instr);
         if (instr->intrinsic == nir_intrinsic_quad_swizzle_amd) {
            for (unsigned i = 0; i < 4; i++)
               fprintf(fp, "%d", (mask >> (i * 2) & 3));
         } else if (instr->intrinsic == nir_intrinsic_masked_swizzle_amd) {
            fprintf(fp, "((id & %d) | %d) ^ %d", mask & 0x1F,
                    (mask >> 5) & 0x1F,
                    (mask >> 10) & 0x1F);
         } else {
            fprintf(fp, "%d", mask);
         }
         break;
      }

      case NIR_INTRINSIC_MEMORY_SEMANTICS: {
         nir_memory_semantics semantics = nir_intrinsic_memory_semantics(instr);
         fprintf(fp, "mem_semantics=");
         switch (semantics & (NIR_MEMORY_ACQUIRE | NIR_MEMORY_RELEASE)) {
         case 0:
            fprintf(fp, "NONE");
            break;
         case NIR_MEMORY_ACQUIRE:
            fprintf(fp, "ACQ");
            break;
         case NIR_MEMORY_RELEASE:
            fprintf(fp, "REL");
            break;
         default:
            fprintf(fp, "ACQ|REL");
            break;
         }
         if (semantics & (NIR_MEMORY_MAKE_AVAILABLE))
            fprintf(fp, "|AVAILABLE");
         if (semantics & (NIR_MEMORY_MAKE_VISIBLE))
            fprintf(fp, "|VISIBLE");
         if (semantics & (NIR_MEMORY_CONTROL_ARRIVE))
            fprintf(fp, "|ARRIVE");
         if (semantics & (NIR_MEMORY_CONTROL_WAIT))
            fprintf(fp, "|WAIT");
         break;
      }

      case NIR_INTRINSIC_MEMORY_MODES: {
         fprintf(fp, "mem_modes=");
         unsigned int modes = nir_intrinsic_memory_modes(instr);
         if (modes == 0)
            fputc('0', fp);
         while (modes) {
            nir_variable_mode m = u_bit_scan(&modes);
            fprintf(fp, "%s%s", get_variable_mode_str(1 << m, true), modes ? "|" : "");
         }
         break;
      }

      case NIR_INTRINSIC_EXECUTION_SCOPE:
      case NIR_INTRINSIC_MEMORY_SCOPE: {
         mesa_scope scope =
            idx == NIR_INTRINSIC_MEMORY_SCOPE ? nir_intrinsic_memory_scope(instr)
                                              : nir_intrinsic_execution_scope(instr);
         const char *name = mesa_scope_name(scope);
         static const char prefix[] = "SCOPE_";
         if (strncmp(name, prefix, sizeof(prefix) - 1) == 0)
            name += sizeof(prefix) - 1;
         fprintf(fp, "%s=%s", nir_intrinsic_index_names[idx], name);
         break;
      }

      case NIR_INTRINSIC_IO_SEMANTICS: {
         struct nir_io_semantics io = nir_intrinsic_io_semantics(instr);

         /* Try to figure out the mode so we can interpret the location */
         nir_variable_mode mode = nir_var_mem_generic;
         switch (instr->intrinsic) {
         case nir_intrinsic_load_input:
         case nir_intrinsic_load_per_primitive_input:
         case nir_intrinsic_load_interpolated_input:
         case nir_intrinsic_load_per_vertex_input:
         case nir_intrinsic_load_input_vertex:
         case nir_intrinsic_load_coefficients_agx:
         case nir_intrinsic_load_attribute_pan:
         case nir_intrinsic_load_fs_coeffs_pco:
            mode = nir_var_shader_in;
            break;

         case nir_intrinsic_load_output:
         case nir_intrinsic_load_per_vertex_output:
         case nir_intrinsic_load_per_primitive_output:
         case nir_intrinsic_store_output:
         case nir_intrinsic_store_per_primitive_output:
         case nir_intrinsic_store_per_vertex_output:
         case nir_intrinsic_store_per_view_output:
         case nir_intrinsic_blend_pan:
         case nir_intrinsic_blend2_pan:
         case nir_intrinsic_load_blend_input_pan:
         case nir_intrinsic_load_clear_value_pan:
         case nir_intrinsic_load_tile_pan:
         case nir_intrinsic_load_tile_res_pan:
         case nir_intrinsic_store_tile_pan:
            mode = nir_var_shader_out;
            break;

         case nir_intrinsic_load_pixel_local:
         case nir_intrinsic_store_pixel_local:
            mode = nir_var_mem_pixel_local_inout;
            break;

         default:
            break;
         }

         /* Using that mode, we should be able to name the location */
         char buf[4];
         const char *loc = get_location_str(io.location,
                                            state->shader->info.stage, mode,
                                            buf);

         fprintf(fp, "io location=%s slots=%u", loc, io.num_slots);

         if (io.interp_explicit_strict)
            fprintf(fp, " explicit_strict");

         if (io.dual_source_blend_index)
            fprintf(fp, " dualsrc");

         if (io.fb_fetch_output)
            fprintf(fp, " fbfetch");

         if (io.fb_fetch_output_coherent)
            fprintf(fp, " coherent");

         if (io.per_view)
            fprintf(fp, " perview");

         if (io.medium_precision)
            fprintf(fp, " mediump");

         if (io.high_16bits)
            fprintf(fp, " high_16bits");

         if (io.high_dvec2)
            fprintf(fp, " high_dvec2");

         if (io.no_varying)
            fprintf(fp, " no_varying");

         if (io.no_sysval_output)
            fprintf(fp, " no_sysval_output");

         if (state->shader &&
             state->shader->info.stage == MESA_SHADER_GEOMETRY &&
             (instr->intrinsic == nir_intrinsic_store_output ||
              instr->intrinsic == nir_intrinsic_store_per_primitive_output ||
              instr->intrinsic == nir_intrinsic_store_per_vertex_output ||
              instr->intrinsic == nir_intrinsic_store_per_view_output)) {
            unsigned gs_streams = io.gs_streams;
            fprintf(fp, " gs_streams(");
            for (unsigned i = 0; i < 4; i++) {
               fprintf(fp, "%s%c=%u", i ? " " : "", "xyzw"[i],
                       (gs_streams >> (i * 2)) & 0x3);
            }
            fprintf(fp, ")");
         }

         if (io.no_validate)
            fprintf(fp, " no_validate");

         if (io.no_signed_zero)
            fprintf(fp, " no_signed_zero");

         break;
      }

      case NIR_INTRINSIC_IO_XFB: {
         fprintf(fp, "xfb(");
         bool first = true;
         for (unsigned i = 0; i < 4; i++) {
            nir_io_xfb xfb = nir_intrinsic_io_xfb(instr);

            if (!xfb.out[i].num_components)
               continue;

            if (!first)
               fprintf(fp, ", ");
            first = false;

            if (xfb.out[i].num_components > 1) {
               fprintf(fp, "components=%u..%u",
                       i, i + xfb.out[i].num_components - 1);
            } else {
               fprintf(fp, "component=%u", i);
            }
            fprintf(fp, " buffer=%u offset=%u",
                    xfb.out[i].buffer, (uint32_t)xfb.out[i].offset * 4);
         }
         fprintf(fp, ")");
         break;
      }

      case NIR_INTRINSIC_ROUNDING_MODE: {
         fprintf(fp, "rounding_mode=");
         switch (nir_intrinsic_rounding_mode(instr)) {
         case nir_rounding_mode_undef:
            fprintf(fp, "undef");
            break;
         case nir_rounding_mode_rtne:
            fprintf(fp, "rtne");
            break;
         case nir_rounding_mode_ru:
            fprintf(fp, "ru");
            break;
         case nir_rounding_mode_rd:
            fprintf(fp, "rd");
            break;
         case nir_rounding_mode_rtz:
            fprintf(fp, "rtz");
            break;
         default:
            fprintf(fp, "unknown");
            break;
         }
         break;
      }

      case NIR_INTRINSIC_RAY_QUERY_VALUE: {
         fprintf(fp, "ray_query_value=");
         switch (nir_intrinsic_ray_query_value(instr)) {
#define VAL(_name)                   \
   case nir_ray_query_value_##_name: \
      fprintf(fp, #_name);           \
      break
            VAL(intersection_type);
            VAL(intersection_t);
            VAL(intersection_instance_custom_index);
            VAL(intersection_instance_id);
            VAL(intersection_instance_sbt_index);
            VAL(intersection_geometry_index);
            VAL(intersection_primitive_index);
            VAL(intersection_barycentrics);
            VAL(intersection_front_face);
            VAL(intersection_object_ray_direction);
            VAL(intersection_object_ray_origin);
            VAL(intersection_object_to_world);
            VAL(intersection_world_to_object);
            VAL(intersection_candidate_aabb_opaque);
            VAL(tmin);
            VAL(flags);
            VAL(world_ray_direction);
            VAL(world_ray_origin);
#undef VAL
         default:
            fprintf(fp, "unknown");
            break;
         }
         break;
      }

      case NIR_INTRINSIC_RESOURCE_ACCESS_INTEL: {
         fprintf(fp, "resource_intel=");
         unsigned int modes = nir_intrinsic_resource_access_intel(instr);
         if (modes == 0)
            fputc('0', fp);
         while (modes) {
            nir_resource_data_intel i = 1u << u_bit_scan(&modes);
            switch (i) {
            case nir_resource_intel_bindless:
               fprintf(fp, "bindless");
               break;
            case nir_resource_intel_pushable:
               fprintf(fp, "pushable");
               break;
            case nir_resource_intel_sampler:
               fprintf(fp, "sampler");
               break;
            case nir_resource_intel_non_uniform:
               fprintf(fp, "non-uniform");
               break;
            case nir_resource_intel_sampler_embedded:
               fprintf(fp, "sampler-embedded");
               break;
            case nir_resource_intel_internal:
               fprintf(fp, "internal");
               break;
            default:
               fprintf(fp, "unknown");
               break;
            }
            fprintf(fp, "%s", modes ? "|" : "");
         }
         break;
      }

      case NIR_INTRINSIC_ACCESS: {
         fprintf(fp, "access=");
         print_access(nir_intrinsic_access(instr), state, "|");
         break;
      }

      case NIR_INTRINSIC_MATRIX_LAYOUT: {
         fprintf(fp, "matrix_layout=");
         switch (nir_intrinsic_matrix_layout(instr)) {
         case GLSL_MATRIX_LAYOUT_ROW_MAJOR:
            fprintf(fp, "row_major");
            break;
         case GLSL_MATRIX_LAYOUT_COLUMN_MAJOR:
            fprintf(fp, "col_major");
            break;
         default:
            fprintf(fp, "unknown");
            break;
         }
         break;
      }

      case NIR_INTRINSIC_CMAT_DESC:
         print_cmat_description(nir_intrinsic_cmat_desc(instr), state);
         break;

      case NIR_INTRINSIC_DST_CMAT_DESC:
         print_cmat_description(nir_intrinsic_dst_cmat_desc(instr), state);
         break;

      case NIR_INTRINSIC_SRC_CMAT_DESC:
         print_cmat_description(nir_intrinsic_src_cmat_desc(instr), state);
         break;

      case NIR_INTRINSIC_CMAT_SIGNED_MASK: {
         fprintf(fp, "cmat_signed=");
         unsigned int mask = nir_intrinsic_cmat_signed_mask(instr);
         if (mask == 0)
            fputc('0', fp);
         while (mask) {
            nir_cmat_signed i = 1u << u_bit_scan(&mask);
            switch (i) {
            case NIR_CMAT_A_SIGNED:
               fputc('A', fp);
               break;
            case NIR_CMAT_B_SIGNED:
               fputc('B', fp);
               break;
            case NIR_CMAT_C_SIGNED:
               fputc('C', fp);
               break;
            case NIR_CMAT_RESULT_SIGNED:
               fprintf(fp, "Result");
               break;
            default:
               fprintf(fp, "unknown");
               break;
            }
            fprintf(fp, "%s", mask ? "|" : "");
         }
         break;
      }

      case NIR_INTRINSIC_ALU_OP: {
         nir_op alu_op = nir_intrinsic_alu_op(instr);
         fprintf(fp, "alu_op=%s", nir_op_infos[alu_op].name);
         break;
      }

      case NIR_INTRINSIC_INTERP_MODE:
         fprintf(fp, "interp_mode=%s",
                 glsl_interp_mode_name(nir_intrinsic_interp_mode(instr)));
         break;

      case NIR_INTRINSIC_PREAMBLE_CLASS: {
         /* "General" handled above */
         nir_preamble_class cls = nir_intrinsic_preamble_class(instr);
         if (cls == nir_preamble_class_image)
            fprintf(fp, "class=image");
         else if (cls == nir_preamble_class_sampler)
            fprintf(fp, "class=sampler");
         else
            UNREACHABLE("invalid class");

         break;
      }

      case NIR_INTRINSIC_FP_MATH_CTRL: {
         unsigned fp_math_ctrl = nir_intrinsic_fp_math_ctrl(instr);
         if (fp_math_ctrl)
            print_fp_math_ctrl(fp_math_ctrl, state);
         else
            fprintf(fp, "fp-fast-math");
         break;
      }

      default: {
         unsigned off = info->index_map[idx] - 1;
         fprintf(fp, "%s=%d", nir_intrinsic_index_names[idx], instr->const_index[off]);
         break;
      }
      }
   }
   if (info->num_indices)
      fprintf(fp, ")");

   if (!state->shader)
      return;

   nir_variable_mode var_mode;
   switch (instr->intrinsic) {
   case nir_intrinsic_load_uniform:
      var_mode = nir_var_uniform;
      break;
   case nir_intrinsic_load_input:
   case nir_intrinsic_load_per_primitive_input:
   case nir_intrinsic_load_interpolated_input:
   case nir_intrinsic_load_per_vertex_input:
      var_mode = nir_var_shader_in;
      break;
   case nir_intrinsic_load_output:
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
   case nir_intrinsic_store_per_view_output:
      var_mode = nir_var_shader_out;
      break;
   default:
      return;
   }

   if (instr->name) {
      fprintf(fp, "  // %s", instr->name);
      return;
   }

   nir_foreach_variable_with_modes(var, state->shader, var_mode) {
      if (!var->name)
         continue;

      bool match;
      if (instr->intrinsic == nir_intrinsic_load_uniform) {
         match = var->data.driver_location == nir_intrinsic_base(instr);
      } else {
         match = var->data.location == nir_intrinsic_io_semantics(instr).location &&
                 nir_intrinsic_component(instr) >= var->data.location_frac &&
                 nir_intrinsic_component(instr) <
                    (var->data.location_frac + glsl_get_components(var->type));
      }

      if (match) {
         fprintf(fp, "  // %s", var->name);
         break;
      }
   }
}

static void
print_tex_instr(nir_tex_instr *instr, print_state *state)
{
   FILE *fp = state->fp;

   print_def(&instr->def, state);

   fprintf(fp, " = (");
   print_alu_type(instr->dest_type, state);
   fprintf(fp, ")");

   switch (instr->op) {
   case nir_texop_tex:
      fprintf(fp, "tex ");
      break;
   case nir_texop_txb:
      fprintf(fp, "txb ");
      break;
   case nir_texop_txl:
      fprintf(fp, "txl ");
      break;
   case nir_texop_txd:
      fprintf(fp, "txd ");
      break;
   case nir_texop_txf:
      fprintf(fp, "txf ");
      break;
   case nir_texop_txf_ms:
      fprintf(fp, "txf_ms ");
      break;
   case nir_texop_txf_ms_fb:
      fprintf(fp, "txf_ms_fb ");
      break;
   case nir_texop_txf_ms_mcs_intel:
      fprintf(fp, "txf_ms_mcs_intel ");
      break;
   case nir_texop_txs:
      fprintf(fp, "txs ");
      break;
   case nir_texop_lod:
      fprintf(fp, "lod ");
      break;
   case nir_texop_tg4:
      fprintf(fp, "tg4 ");
      break;
   case nir_texop_query_levels:
      fprintf(fp, "query_levels ");
      break;
   case nir_texop_texture_samples:
      fprintf(fp, "texture_samples ");
      break;
   case nir_texop_samples_identical:
      fprintf(fp, "samples_identical ");
      break;
   case nir_texop_tex_prefetch:
      fprintf(fp, "tex (pre-dispatchable) ");
      break;
   case nir_texop_lod_bias:
      fprintf(fp, "lod_bias ");
      break;
   case nir_texop_fragment_fetch_amd:
      fprintf(fp, "fragment_fetch_amd ");
      break;
   case nir_texop_fragment_mask_fetch_amd:
      fprintf(fp, "fragment_mask_fetch_amd ");
      break;
   case nir_texop_descriptor_amd:
      fprintf(fp, "descriptor_amd ");
      break;
   case nir_texop_sampler_descriptor_amd:
      fprintf(fp, "sampler_descriptor_amd ");
      break;
   case nir_texop_image_min_lod_agx:
      fprintf(fp, "image_min_lod_agx ");
      break;
   case nir_texop_has_custom_border_color_agx:
      fprintf(fp, "has_custom_border_color_agx ");
      break;
   case nir_texop_custom_border_color_agx:
      fprintf(fp, "custom_border_color_agx ");
      break;
   case nir_texop_hdr_dim_nv:
      fprintf(fp, "hdr_dim_nv ");
      break;
   case nir_texop_tex_type_nv:
      fprintf(fp, "tex_type_nv ");
      break;
   case nir_texop_sample_pos_nv:
      fprintf(fp, "sample_pos_nv ");
      break;
   case nir_texop_gradient_pan:
      fprintf(fp, "gradient_pan ");
      break;
   case nir_texop_sample_weighted_qcom:
      fprintf(fp, "sample_weighted_qcom ");
      break;
   case nir_texop_box_filter_qcom:
      fprintf(fp, "box_filter_qcom ");
      break;
   case nir_texop_block_match_sad_qcom:
      fprintf(fp, "block_match_sad_qcom ");
      break;
   case nir_texop_block_match_ssd_qcom:
      fprintf(fp, "block_match_ssd_qcom ");
      break;
   case nir_texop_resinfo_intel:
      fprintf(fp, "resinfo_intel ");
      break;
   case nir_texop_sparse_residency_intel:
      fprintf(fp, "sparse_residency_intel ");
      break;
   case nir_texop_sparse_residency_txf_intel:
      fprintf(fp, "sparse_residency_txf_intel ");
      break;
   default:
      UNREACHABLE("Invalid texture operation");
      break;
   }

   bool has_texture_deref = false, has_sampler_deref = false;
   for (unsigned i = 0; i < instr->num_srcs; i++) {
      if (i > 0) {
         fprintf(fp, ", ");
      }

      print_src(&instr->src[i].src, state, nir_tex_instr_src_type(instr, i));
      fprintf(fp, " ");

      switch (instr->src[i].src_type) {
      case nir_tex_src_backend1:
         fprintf(fp, "(backend1)");
         break;
      case nir_tex_src_backend2:
         fprintf(fp, "(backend2)");
         break;
      case nir_tex_src_coord:
         fprintf(fp, "(coord)");
         break;
      case nir_tex_src_ref_coord:
         fprintf(fp, "(ref_coord)");
         break;
      case nir_tex_src_projector:
         fprintf(fp, "(projector)");
         break;
      case nir_tex_src_comparator:
         fprintf(fp, "(comparator)");
         break;
      case nir_tex_src_offset:
         fprintf(fp, "(offset)");
         break;
      case nir_tex_src_bias:
         fprintf(fp, "(bias)");
         break;
      case nir_tex_src_lod:
         fprintf(fp, "(lod)");
         break;
      case nir_tex_src_min_lod:
         fprintf(fp, "(min_lod)");
         break;
      case nir_tex_src_max_lod_kk:
         fprintf(fp, "(max_lod_kk)");
         break;
      case nir_tex_src_ms_index:
         fprintf(fp, "(ms_index)");
         break;
      case nir_tex_src_ms_mcs_intel:
         fprintf(fp, "(ms_mcs_intel)");
         break;
      case nir_tex_src_lod_bias_min_agx:
         fprintf(fp, "(lod_bias_min_agx)");
         break;
      case nir_tex_src_ddx:
         fprintf(fp, "(ddx)");
         break;
      case nir_tex_src_ddy:
         fprintf(fp, "(ddy)");
         break;
      case nir_tex_src_sampler_deref_intrinsic:
         has_texture_deref = true;
         fprintf(fp, "(sampler_deref_intrinsic)");
         break;
      case nir_tex_src_texture_deref_intrinsic:
         has_texture_deref = true;
         fprintf(fp, "(texture_deref_intrinsic)");
         break;
      case nir_tex_src_texture_deref:
         has_texture_deref = true;
         fprintf(fp, "(texture_deref)");
         break;
      case nir_tex_src_texture_2_deref:
         fprintf(fp, "(texture_2_deref)");
         break;
      case nir_tex_src_sampler_deref:
         has_sampler_deref = true;
         fprintf(fp, "(sampler_deref)");
         break;
      case nir_tex_src_sampler_2_deref:
         fprintf(fp, "(sampler_2_deref)");
         break;
      case nir_tex_src_texture_offset:
         fprintf(fp, "(texture_offset)");
         break;
      case nir_tex_src_sampler_offset:
         fprintf(fp, "(sampler_offset)");
         break;
      case nir_tex_src_texture_handle:
         fprintf(fp, "(texture_handle)");
         break;
      case nir_tex_src_texture_2_handle:
         fprintf(fp, "(texture_2_handle)");
         break;
      case nir_tex_src_sampler_handle:
         fprintf(fp, "(sampler_handle)");
         break;
      case nir_tex_src_sampler_2_handle:
         fprintf(fp, "(ref_sampler_handle)");
         break;
      case nir_tex_src_block_size:
         fprintf(fp, "(block_size)");
         break;
      case nir_tex_src_box_size:
         fprintf(fp, "(box_size)");
         break;
      case nir_tex_src_texture_heap_offset:
         fprintf(fp, "(texture_heap_offset)");
         break;
      case nir_tex_src_sampler_heap_offset:
         fprintf(fp, "(sampler_heap_offset)");
         break;
      case nir_tex_src_plane:
         fprintf(fp, "(plane)");
         break;

      default:
         UNREACHABLE("Invalid texture source type");
         break;
      }
   }

   if (instr->is_gather_implicit_lod)
      fprintf(fp, ", implicit lod");

   if (instr->op == nir_texop_tg4) {
      fprintf(fp, ", %u (gather_component)", instr->component);
   }

   if (nir_tex_instr_has_explicit_tg4_offsets(instr)) {
      fprintf(fp, ", { (%i, %i)", instr->tg4_offsets[0][0], instr->tg4_offsets[0][1]);
      for (unsigned i = 1; i < 4; ++i)
         fprintf(fp, ", (%i, %i)", instr->tg4_offsets[i][0],
                 instr->tg4_offsets[i][1]);
      fprintf(fp, " } (offsets)");
   }

   if (instr->op != nir_texop_txf_ms_fb && !has_texture_deref) {
      fprintf(fp, ", %u (texture)", instr->texture_index);
   }

   if (nir_tex_instr_need_sampler(instr) && !has_sampler_deref) {
      fprintf(fp, ", %u (sampler)", instr->sampler_index);
   }

   if (instr->texture_non_uniform) {
      fprintf(fp, ", texture non-uniform");
   }

   if (instr->sampler_non_uniform) {
      fprintf(fp, ", sampler non-uniform");
   }

   if (instr->is_sparse) {
      fprintf(fp, ", sparse");
   }

   if (instr->skip_helpers) {
      fprintf(fp, ", skip_helpers");
   }

   if (instr->backend_flags) {
      fprintf(fp, ", backend_flags=0x%X", instr->backend_flags);
   }

   fprintf(fp, ", %s", sampler_dim_name[instr->sampler_dim]);
}

static void
print_call_instr(nir_call_instr *instr, print_state *state)
{
   FILE *fp = state->fp;

   print_no_dest_padding(state);

   bool indirect = instr->indirect_callee.ssa;

   fprintf(fp, "call %s ", instr->callee->name);
   if (indirect) {
      fprintf(fp, "(indirect ");
      print_src(&instr->indirect_callee, state, nir_type_invalid);
      fprintf(fp, ") ");
   }

   for (unsigned i = 0; i < instr->num_params; i++) {
      if (i != 0)
         fprintf(fp, ", ");

      if (instr->callee->params[i].name)
         fprintf(fp, "%s ", instr->callee->params[i].name);

      print_src(&instr->params[i], state, nir_type_invalid);
   }
}

static const char *
get_cmat_call_op_str(nir_cmat_call_op op)
{
   switch (op) {
   case nir_cmat_call_op_reduce:
      return "cmat_call_reduce";
   case nir_cmat_call_op_reduce_finish:
      return "cmat_call_reduce_finish";
   case nir_cmat_call_op_reduce_2x2:
      return "cmat_call_reduce_2x2";
   case nir_cmat_call_op_per_element_op:
      return "cmat_call_per_element";
   }
   UNREACHABLE("Unknown cmat call op");
}

static void
print_cmat_call_instr(nir_cmat_call_instr *instr, print_state *state)
{
   FILE *fp = state->fp;

   print_no_dest_padding(state);

   fprintf(fp, "%s %s ", get_cmat_call_op_str(instr->op), instr->callee->name);

   for (unsigned i = 0; i < instr->num_params; i++) {
      if (i != 0)
         fprintf(fp, ", ");

      if (instr->callee->params[i].name)
         fprintf(fp, "%s ", instr->callee->params[i].name);

      print_src(&instr->params[i], state, nir_type_invalid);
   }
}

static void
print_jump_instr(nir_jump_instr *instr, print_state *state)
{
   FILE *fp = state->fp;

   print_no_dest_padding(state);

   switch (instr->type) {
   case nir_jump_break:
      fprintf(fp, "break");
      break;

   case nir_jump_continue:
      fprintf(fp, "continue");
      break;

   case nir_jump_return:
      fprintf(fp, "return");
      break;

   case nir_jump_halt:
      fprintf(fp, "halt");
      break;

   case nir_jump_abort:
      fprintf(fp, "abort");
      break;

   case nir_jump_goto:
      fprintf(fp, "goto b%u",
              instr->target ? instr->target->index : -1);
      break;

   case nir_jump_goto_if:
      fprintf(fp, "goto b%u if ",
              instr->target ? instr->target->index : -1);
      print_src(&instr->condition, state, nir_type_invalid);
      fprintf(fp, " else b%u",
              instr->else_target ? instr->else_target->index : -1);
      break;
   default:
      UNREACHABLE("Unknown jump instruction");
   }
}

static void
print_ssa_undef_instr(nir_undef_instr *instr, print_state *state)
{
   FILE *fp = state->fp;
   print_def(&instr->def, state);
   fprintf(fp, " = undefined");
}

static void
print_phi_instr(nir_phi_instr *instr, print_state *state)
{
   FILE *fp = state->fp;
   print_def(&instr->def, state);
   fprintf(fp, " = phi ");
   nir_block **preds =
      state->preds ? state->preds : nir_block_get_predecessors_sorted(instr->instr.block, NULL);

   for (unsigned i = 0; i < nir_block_num_preds(instr->instr.block); i++) {
      nir_phi_src *src = nir_phi_get_src_from_block(instr, preds[i]);
      if (i != 0)
         fprintf(fp, ", ");

      fprintf(fp, "b%u: ", preds[i]->index);
      print_src(&src->src, state, nir_type_invalid);
   }

   if (!state->preds)
      ralloc_free(preds);
}

static void
print_instr(const nir_instr *instr, print_state *state, unsigned tabs)
{
   FILE *fp = state->fp;

   if (state->gather_debug_info) {
      nir_instr_debug_info *debug_info = nir_instr_get_debug_info((nir_instr *)instr);
      debug_info->nir_line = (uint32_t)ftell(fp);
   }

   if (instr->has_debug_info && !state->gather_debug_info) {
      nir_instr_debug_info *debug_info = nir_instr_get_debug_info((nir_instr *)instr);

      bool changed = state->last_debug_info.spirv_offset != debug_info->spirv_offset;
      changed |= state->last_debug_info.nir_line != debug_info->nir_line;

      if (changed && debug_info->spirv_offset) {
         print_indentation(tabs, fp);

         fprintf(fp, "// 0x%x", debug_info->spirv_offset);

         if (debug_info->filename)
            fprintf(fp, " %s:%u:%u", debug_info->filename, debug_info->line, debug_info->column);

         if (debug_info->nir_line)
            fprintf(fp, " nir:%u", debug_info->nir_line);

         fprintf(fp, "\n");
      }

      state->last_debug_info = *debug_info;
   }

   print_indentation(tabs, fp);

   switch (instr->type) {
   case nir_instr_type_alu:
      print_alu_instr(nir_instr_as_alu(instr), state);
      break;

   case nir_instr_type_deref:
      print_deref_instr(nir_instr_as_deref(instr), state);
      break;

   case nir_instr_type_call:
      print_call_instr(nir_instr_as_call(instr), state);
      break;

   case nir_instr_type_cmat_call:
      print_cmat_call_instr(nir_instr_as_cmat_call(instr), state);
      break;

   case nir_instr_type_intrinsic:
      print_intrinsic_instr(nir_instr_as_intrinsic(instr), state);
      break;

   case nir_instr_type_tex:
      print_tex_instr(nir_instr_as_tex(instr), state);
      break;

   case nir_instr_type_load_const:
      print_load_const_instr(nir_instr_as_load_const(instr), state);
      break;

   case nir_instr_type_jump:
      print_jump_instr(nir_instr_as_jump(instr), state);
      break;

   case nir_instr_type_undef:
      print_ssa_undef_instr(nir_instr_as_undef(instr), state);
      break;

   case nir_instr_type_phi:
      print_phi_instr(nir_instr_as_phi(instr), state);
      break;

   default:
      UNREACHABLE("Invalid instruction type");
      break;
   }

   if (NIR_DEBUG(PRINT_PASS_FLAGS) && instr->pass_flags)
      fprintf(fp, " (pass_flags: 0x%x)", instr->pass_flags);
}

static bool
block_has_instruction_with_dest(nir_block *block)
{
   nir_foreach_instr(instr, block) {
      switch (instr->type) {
      case nir_instr_type_load_const:
      case nir_instr_type_deref:
      case nir_instr_type_alu:
      case nir_instr_type_tex:
      case nir_instr_type_undef:
      case nir_instr_type_phi:
         return true;

      case nir_instr_type_intrinsic: {
         nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
         const nir_intrinsic_info *info = &nir_intrinsic_infos[intrin->intrinsic];
         if (info->has_dest)
            return true;

         /* Doesn't define a new value. */
         break;
      }

      case nir_instr_type_jump:
      case nir_instr_type_call:
      case nir_instr_type_cmat_call:
         /* Doesn't define a new value. */
         break;
      }
   }

   return false;
}

static void print_cf_node(nir_cf_node *node, print_state *state,
                          unsigned tabs);

static void
print_block_preds(nir_block *block, print_state *state)
{
   FILE *fp = state->fp;
   for (unsigned i = 0; i < nir_block_num_preds(block); i++) {
      fprintf(fp, " b%u", state->preds[i]->index);
   }
}

static void
print_block_succs(nir_block *block, print_state *state)
{
   FILE *fp = state->fp;
   for (unsigned i = 0; i < 2; i++) {
      if (block->successors[i]) {
         fprintf(fp, " b%u", block->successors[i]->index);
      }
   }
}

static void
print_block(nir_block *block, print_state *state, unsigned tabs)
{
   FILE *fp = state->fp;

   if (block_has_instruction_with_dest(block))
      state->padding_for_no_dest = calculate_padding_for_no_dest(state);
   else
      state->padding_for_no_dest = 0;

   print_indentation(tabs, fp);
   fprintf(fp, "%sblock b%u:",
           divergence_status(state, block->divergent),
           block->index);

   const bool empty_block = exec_list_is_empty(&block->instr_list);
   state->preds = nir_block_get_predecessors_sorted(block, NULL);
   if (empty_block) {
      fprintf(fp, "  // preds:");
      print_block_preds(block, state);
      fprintf(fp, ", succs:");
      print_block_succs(block, state);
      fprintf(fp, "\n");
      ralloc_free(state->preds);
      return;
   }

   const unsigned block_length = 7 + count_digits(block->index) + 1;
   const unsigned pred_padding = block_length < state->padding_for_no_dest ? state->padding_for_no_dest - block_length : 0;

   fprintf(fp, "%*s// preds:", pred_padding, "");
   print_block_preds(block, state);
   fprintf(fp, "\n");

   nir_foreach_instr(instr, block) {
      print_instr(instr, state, tabs);
      print_annotation(state, instr);
      fprintf(fp, "\n");
   }

   print_indentation(tabs, fp);
   fprintf(fp, "%*s// succs:", state->padding_for_no_dest, "");
   print_block_succs(block, state);
   fprintf(fp, "\n");
   ralloc_free(state->preds);
}

static void
print_if(nir_if *if_stmt, print_state *state, unsigned tabs)
{
   FILE *fp = state->fp;

   print_indentation(tabs, fp);
   fprintf(fp, "if ");
   print_src(&if_stmt->condition, state, nir_type_invalid);
   switch (if_stmt->control) {
   case nir_selection_control_flatten:
      fprintf(fp, "  // flatten");
      break;
   case nir_selection_control_dont_flatten:
      fprintf(fp, "  // don't flatten");
      break;
   case nir_selection_control_divergent_always_taken:
      fprintf(fp, "  // divergent always taken");
      break;
   case nir_selection_control_none:
   default:
      break;
   }
   fprintf(fp, " {\n");
   foreach_list_typed(nir_cf_node, node, node, &if_stmt->then_list) {
      print_cf_node(node, state, tabs + 1);
   }
   print_indentation(tabs, fp);
   fprintf(fp, "} else {\n");
   foreach_list_typed(nir_cf_node, node, node, &if_stmt->else_list) {
      print_cf_node(node, state, tabs + 1);
   }
   print_indentation(tabs, fp);
   fprintf(fp, "}\n");
}

static void
print_loop(nir_loop *loop, print_state *state, unsigned tabs)
{
   FILE *fp = state->fp;

   print_indentation(tabs, fp);
   fprintf(fp, "%sloop", divergence_status(state, loop->divergent_break));
   switch (loop->control) {
   case nir_loop_control_unroll:
      fprintf(fp, "  // unroll");
      break;
   case nir_loop_control_dont_unroll:
      fprintf(fp, "  // don't unroll");
      break;
   case nir_loop_control_none:
   default:
      break;
   }
   fprintf(fp, " {\n");
   foreach_list_typed(nir_cf_node, node, node, &loop->body) {
      print_cf_node(node, state, tabs + 1);
   }
   print_indentation(tabs, fp);

   if (nir_loop_has_continue_construct(loop)) {
      fprintf(fp, "} continue {\n");
      foreach_list_typed(nir_cf_node, node, node, &loop->continue_list) {
         print_cf_node(node, state, tabs + 1);
      }
      print_indentation(tabs, fp);
   }

   fprintf(fp, "}\n");
}

static void
print_cf_node(nir_cf_node *node, print_state *state, unsigned int tabs)
{
   switch (node->type) {
   case nir_cf_node_block:
      print_block(nir_cf_node_as_block(node), state, tabs);
      break;

   case nir_cf_node_if:
      print_if(nir_cf_node_as_if(node), state, tabs);
      break;

   case nir_cf_node_loop:
      print_loop(nir_cf_node_as_loop(node), state, tabs);
      break;

   default:
      UNREACHABLE("Invalid CFG node type");
   }
}

static void
print_function_impl(nir_function_impl *impl, print_state *state, bool print_name)
{
   FILE *fp = state->fp;

   state->max_dest_index = impl->ssa_alloc;
   state->divergence_valid = impl->valid_metadata & nir_metadata_divergence;

   if (print_name) {
      fprintf(fp, "\nimpl %s ", impl->function->name);

      fprintf(fp, "{\n");
   }

   if (impl->preamble) {
      print_indentation(1, fp);
      fprintf(fp, "preamble %s\n", impl->preamble->name);
   }

   if (!NIR_DEBUG(PRINT_NO_INLINE_CONSTS)) {
      /* Don't reindex the SSA as suggested by nir_gather_types() because
       * nir_print don't modify the shader.  If needed, a limit for ssa_alloc
       * can be added.
       */
      state->float_types = BITSET_CALLOC(impl->ssa_alloc);
      state->int_types = BITSET_CALLOC(impl->ssa_alloc);
      nir_gather_types(impl, state->float_types, state->int_types);
   }

   nir_foreach_function_temp_variable(var, impl) {
      print_indentation(1, fp);
      print_var_decl(var, state);
   }

   nir_index_blocks(impl);

   foreach_list_typed(nir_cf_node, node, node, &impl->body) {
      print_cf_node(node, state, 1);
   }

   print_indentation(1, fp);
   fprintf(fp, "block b%u:\n", impl->end_block->index);

   if (print_name) {
      fprintf(fp, "}\n\n");
   }

   free(state->float_types);
   free(state->int_types);
   state->max_dest_index = 0;
}

static void
print_function(nir_function *function, print_state *state)
{
   FILE *fp = state->fp;

   fprintf(fp, "decl_function %s (", function->name);

   for (unsigned i = 0; i < function->num_params; ++i) {
      if (i != 0) {
         fprintf(fp, ", ");
      }

      nir_parameter param = function->params[i];

      fprintf(fp, "%u", param.bit_size);
      if (param.num_components != 1) {
         fprintf(fp, "x%u", param.num_components);
      }

      if (param.name) {
         fprintf(fp, " %s", param.name);
      } else if (param.is_return) {
         fprintf(fp, " return");
      }
   }

   fprintf(fp, ")");

   /* clang-format off */
   fprintf(fp, "%s%s%s", function->dont_inline ? " (noinline)" :
                       function->should_inline ? " (inline)" : "",
                       function->is_exported ? " (exported)" : "",
                       function->is_entrypoint ? " (entrypoint)" : "");
   /* clang-format on */

   if (function->workgroup_size[0]) {
      fprintf(fp, " (%ux%ux%u)",
              function->workgroup_size[0],
              function->workgroup_size[1],
              function->workgroup_size[2]);
   }

   fprintf(fp, "\n");

   if (function->impl != NULL) {
      print_function_impl(function->impl, state, true);
      return;
   }
}

static void
init_print_state(print_state *state, nir_shader *shader, FILE *fp)
{
   state->fp = fp;
   state->shader = shader;
   state->ht = _mesa_pointer_hash_table_create(NULL);
   state->syms = _mesa_set_create(NULL, _mesa_hash_string,
                                  _mesa_key_string_equal);
   state->index = 0;
   state->int_types = NULL;
   state->float_types = NULL;
   state->max_dest_index = 0;
   state->padding_for_no_dest = 0;

   if (NIR_DEBUG(PRINT_STRUCT_DECLS))
      state->struct_types = _mesa_pointer_set_create(NULL);
}

static void
destroy_print_state(print_state *state)
{
   _mesa_hash_table_destroy(state->ht, NULL);
   _mesa_set_destroy(state->syms, NULL);

   if (NIR_DEBUG(PRINT_STRUCT_DECLS))
      _mesa_set_destroy(state->struct_types, NULL);
}

static const char *
primitive_name(unsigned primitive)
{
#define PRIM(X)        \
   case MESA_PRIM_##X: \
      return #X
   switch (primitive) {
      PRIM(POINTS);
      PRIM(LINES);
      PRIM(LINE_LOOP);
      PRIM(LINE_STRIP);
      PRIM(TRIANGLES);
      PRIM(TRIANGLE_STRIP);
      PRIM(TRIANGLE_FAN);
      PRIM(QUADS);
      PRIM(QUAD_STRIP);
      PRIM(POLYGON);
      PRIM(LINES_ADJACENCY);
      PRIM(TRIANGLES_ADJACENCY);
   default:
      return "UNKNOWN";
   }
}

static void
print_bitset(FILE *fp, const char *label, const unsigned *words, int size)
{
   fprintf(fp, "%s: ", label);
   /* Iterate back-to-front to get proper digit order (most significant first). */
   for (int i = size - 1; i >= 0; --i) {
      fprintf(fp, (i == size - 1) ? "0x%08x" : "'%08x", words[i]);
   }
   fprintf(fp, "\n");
}

/* Print bitset, only if some bits are set */
static void
print_nz_bitset(FILE *fp, const char *label, const unsigned *words, int size)
{
   bool is_all_zero = true;
   for (int i = 0; i < size; ++i) {
      if (words[i]) {
         is_all_zero = false;
         break;
      }
   }

   if (!is_all_zero)
      print_bitset(fp, label, words, size);
}

/* Print uint64_t value, only if non-zero.
 * The value is printed by enumerating the ranges of bits that are set.
 * E.g. inputs_read: 0,15-17
 */
static void
print_nz_x64(FILE *fp, const char *label, uint64_t value)
{
   if (value) {
      char acc[256] = { 0 };
      char buf[32];
      int start = 0;
      int count = 0;
      while (value) {
         u_bit_scan_consecutive_range64(&value, &start, &count);
         assert(count > 0);
         bool is_first = !acc[0];
         if (count > 1) {
            snprintf(buf, sizeof(buf), is_first ? "%d-%d" : ",%d-%d", start, start + count - 1);
         } else {
            snprintf(buf, sizeof(buf), is_first ? "%d" : ",%d", start);
         }
         assert(strlen(acc) + strlen(buf) + 1 < sizeof(acc));
         strcat(acc, buf);
      }
      fprintf(fp, "%s: %s\n", label, acc);
   }
}

/* Print uint32_t value in hex, only if non-zero */
static void
print_nz_x32(FILE *fp, const char *label, uint32_t value)
{
   if (value)
      fprintf(fp, "%s: 0x%08" PRIx32 "\n", label, value);
}

/* Print uint16_t value in hex, only if non-zero */
static void
print_nz_x16(FILE *fp, const char *label, uint16_t value)
{
   if (value)
      fprintf(fp, "%s: 0x%04x\n", label, value);
}

/* Print uint8_t value in hex, only if non-zero */
static void
print_nz_x8(FILE *fp, const char *label, uint8_t value)
{
   if (value)
      fprintf(fp, "%s: 0x%02x\n", label, value);
}

/* Print unsigned value in decimal, only if non-zero */
static void
print_nz_unsigned(FILE *fp, const char *label, unsigned value)
{
   if (value)
      fprintf(fp, "%s: %u\n", label, value);
}

/* Print bool only if set */
static void
print_nz_bool(FILE *fp, const char *label, bool value)
{
   if (value)
      fprintf(fp, "%s: true\n", label);
}

static void
print_shader_info(const struct shader_info *info, FILE *fp)
{
   fprintf(fp, "shader: %s\n", mesa_shader_stage_name(info->stage));

   /* glsl-optimizer change: no source hashes (blake3) */

   if (info->name)
      fprintf(fp, "name: %s\n", info->name);

   if (info->label)
      fprintf(fp, "label: %s\n", info->label);

   if (info->spec)
      fprintf(fp, "%s", info->spec);

   print_nz_bool(fp, "internal", info->internal);

   if (mesa_shader_stage_uses_workgroup(info->stage)) {
      fprintf(fp, "workgroup_size: %u, %u, %u%s\n",
              info->workgroup_size[0],
              info->workgroup_size[1],
              info->workgroup_size[2],
              info->workgroup_size_variable ? " (variable)" : "");
   }

   if (info->prev_stage != MESA_SHADER_NONE)
      fprintf(fp, "prev_stage: %s\n", mesa_shader_stage_name(info->prev_stage));
   if (info->next_stage != MESA_SHADER_NONE)
      fprintf(fp, "next_stage: %s\n", mesa_shader_stage_name(info->next_stage));

   print_nz_unsigned(fp, "num_textures", info->num_textures);
   print_nz_unsigned(fp, "num_ubos", info->num_ubos);
   print_nz_unsigned(fp, "num_abos", info->num_abos);
   print_nz_unsigned(fp, "num_ssbos", info->num_ssbos);
   print_nz_unsigned(fp, "num_images", info->num_images);

   print_nz_x64(fp, "inputs_read", info->inputs_read);
   print_nz_x64(fp, "dual_slot_inputs", info->dual_slot_inputs);
   print_nz_x64(fp, "outputs_written", info->outputs_written);
   print_nz_x64(fp, "outputs_read", info->outputs_read);
   print_nz_x64(fp, "perspective_varyings", info->perspective_varyings);
   print_nz_x64(fp, "linear_varyings", info->linear_varyings);

   print_nz_bitset(fp, "system_values_read", info->system_values_read, ARRAY_SIZE(info->system_values_read));

   print_nz_x64(fp, "per_primitive_inputs", info->per_primitive_inputs);
   print_nz_x64(fp, "per_primitive_outputs", info->per_primitive_outputs);
   print_nz_x64(fp, "per_view_outputs", info->per_view_outputs);

   print_nz_x16(fp, "inputs_read_16bit", info->inputs_read_16bit);
   print_nz_x16(fp, "outputs_written_16bit", info->outputs_written_16bit);
   print_nz_x16(fp, "outputs_read_16bit", info->outputs_read_16bit);
   print_nz_x16(fp, "inputs_read_indirectly_16bit", info->inputs_read_indirectly_16bit);
   print_nz_x16(fp, "outputs_read_indirectly_16bit", info->outputs_read_indirectly_16bit);
   print_nz_x16(fp, "outputs_written_indirectly_16bit", info->outputs_written_indirectly_16bit);

   print_nz_x32(fp, "patch_inputs_read", info->patch_inputs_read);
   print_nz_x32(fp, "patch_outputs_written", info->patch_outputs_written);
   print_nz_x32(fp, "patch_outputs_read", info->patch_outputs_read);

   print_nz_x64(fp, "inputs_read_indirectly", info->inputs_read_indirectly);
   print_nz_x64(fp, "outputs_read_indirectly", info->outputs_read_indirectly);
   print_nz_x64(fp, "outputs_written_indirectly", info->outputs_written_indirectly);
   print_nz_x32(fp, "patch_inputs_read_indirectly", info->patch_inputs_read_indirectly);
   print_nz_x32(fp, "patch_outputs_read_indirectly", info->patch_outputs_read_indirectly);
   print_nz_x32(fp, "patch_outputs_written_indirectly", info->patch_outputs_written_indirectly);

   print_nz_bitset(fp, "textures_used", info->textures_used, ARRAY_SIZE(info->textures_used));
   print_nz_bitset(fp, "textures_used_by_txf", info->textures_used_by_txf, ARRAY_SIZE(info->textures_used_by_txf));
   print_nz_bitset(fp, "samplers_used", info->samplers_used, ARRAY_SIZE(info->samplers_used));
   print_nz_bitset(fp, "images_used", info->images_used, ARRAY_SIZE(info->images_used));
   print_nz_bitset(fp, "image_buffers", info->image_buffers, ARRAY_SIZE(info->image_buffers));
   print_nz_bitset(fp, "msaa_images", info->msaa_images, ARRAY_SIZE(info->msaa_images));

   print_nz_x32(fp, "float_controls_execution_mode", info->float_controls_execution_mode);

   print_nz_unsigned(fp, "shared_size", info->shared_size);

   if (info->stage == MESA_SHADER_MESH || info->stage == MESA_SHADER_TASK) {
      fprintf(fp, "task_payload_size: %u\n", info->task_payload_size);
   }

   print_nz_unsigned(fp, "ray queries", info->ray_queries);

   print_nz_unsigned(fp, "api_subgroup_size", info->api_subgroup_size);
   fprintf(fp, "max_subgroup_size: %u\n", info->max_subgroup_size);
   fprintf(fp, "min_subgroup_size: %u\n", info->min_subgroup_size);

   print_nz_bool(fp, "api_subgroup_size_draw_uniform", info->api_subgroup_size_draw_uniform);
   print_nz_bool(fp, "uses_wide_subgroup_intrinsics", info->uses_wide_subgroup_intrinsics);

   bool has_xfb_stride = info->xfb_stride[0] || info->xfb_stride[1] || info->xfb_stride[2] || info->xfb_stride[3];
   if (has_xfb_stride)
      fprintf(fp, "xfb_stride: {%u, %u, %u, %u}\n",
              info->xfb_stride[0],
              info->xfb_stride[1],
              info->xfb_stride[2],
              info->xfb_stride[3]);

   bool has_inlinable_uniform_dw_offsets = info->inlinable_uniform_dw_offsets[0] || info->inlinable_uniform_dw_offsets[1] || info->inlinable_uniform_dw_offsets[2] || info->inlinable_uniform_dw_offsets[3];
   if (has_inlinable_uniform_dw_offsets)
      fprintf(fp, "inlinable_uniform_dw_offsets: {%u, %u, %u, %u}\n",
              info->inlinable_uniform_dw_offsets[0],
              info->inlinable_uniform_dw_offsets[1],
              info->inlinable_uniform_dw_offsets[2],
              info->inlinable_uniform_dw_offsets[3]);

   print_nz_unsigned(fp, "num_inlinable_uniforms", info->num_inlinable_uniforms);
   print_nz_unsigned(fp, "clip_distance_array_size", info->clip_distance_array_size);
   print_nz_unsigned(fp, "cull_distance_array_size", info->cull_distance_array_size);

   print_nz_bool(fp, "uses_texture_gather", info->uses_texture_gather);
   print_nz_bool(fp, "uses_resource_info_query", info->uses_resource_info_query);

   print_nz_x8(fp, "bit_sizes_float", info->bit_sizes_float);
   print_nz_x8(fp, "bit_sizes_int", info->bit_sizes_int);

   print_nz_bool(fp, "first_ubo_is_default_ubo", info->first_ubo_is_default_ubo);
   print_nz_bool(fp, "separate_shader", info->separate_shader);
   print_nz_bool(fp, "known_interpolation_qualifiers", info->known_interpolation_qualifiers);
   print_nz_bool(fp, "has_transform_feedback_varyings", info->has_transform_feedback_varyings);
   print_nz_bool(fp, "flrp_lowered", info->flrp_lowered);
   print_nz_bool(fp, "io_lowered", info->io_lowered);
   print_nz_bool(fp, "writes_memory", info->writes_memory);
   print_nz_unsigned(fp, "derivative_group", info->derivative_group);

   switch (info->stage) {
   case MESA_SHADER_VERTEX:
      print_nz_x64(fp, "double_inputs", info->vs.double_inputs);
      print_nz_unsigned(fp, "blit_sgprs_amd", info->vs.blit_sgprs_amd);
      print_nz_bool(fp, "window_space_position", info->vs.window_space_position);
      print_nz_bool(fp, "needs_edge_flag", info->vs.needs_edge_flag);
      break;

   case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_TESS_EVAL:
      fprintf(fp, "primitive_mode: %u\n", info->tess._primitive_mode);
      fprintf(fp, "tcs_vertices_out: %u\n", info->tess.tcs_vertices_out);
      fprintf(fp, "spacing: %u\n", info->tess.spacing);

      print_nz_bool(fp, "ccw", info->tess.ccw);
      print_nz_bool(fp, "point_mode", info->tess.point_mode);
      print_nz_x64(fp, "tcs_same_invocation_inputs_read",
                   info->tess.tcs_same_invocation_inputs_read);
      print_nz_x64(fp, "tcs_cross_invocation_inputs_read", info->tess.tcs_cross_invocation_inputs_read);
      print_nz_x64(fp, "tcs_cross_invocation_outputs_read", info->tess.tcs_cross_invocation_outputs_read);
      break;

   case MESA_SHADER_GEOMETRY:
      fprintf(fp, "output_primitive: %s\n", primitive_name(info->gs.output_primitive));
      fprintf(fp, "input_primitive: %s\n", primitive_name(info->gs.input_primitive));
      fprintf(fp, "vertices_out: %u\n", info->gs.vertices_out);
      fprintf(fp, "invocations: %u\n", info->gs.invocations);
      fprintf(fp, "vertices_in: %u\n", info->gs.vertices_in);
      print_nz_bool(fp, "uses_end_primitive", info->gs.uses_end_primitive);
      fprintf(fp, "active_stream_mask: 0x%02x\n", info->gs.active_stream_mask);
      break;

   case MESA_SHADER_FRAGMENT:
      print_nz_bool(fp, "uses_discard", info->fs.uses_discard);
      print_nz_bool(fp, "uses_fbfetch_output", info->fs.uses_fbfetch_output);
      print_nz_bool(fp, "color_is_dual_source", info->fs.color_is_dual_source);

      print_nz_bool(fp, "require_full_quads", info->fs.require_full_quads);
      print_nz_bool(fp, "needs_coarse_quad_helper_invocations", info->fs.needs_coarse_quad_helper_invocations);
      print_nz_bool(fp, "needs_full_quad_helper_invocations", info->fs.needs_full_quad_helper_invocations);
      print_nz_bool(fp, "uses_sample_qualifier", info->fs.uses_sample_qualifier);
      print_nz_bool(fp, "uses_sample_shading", info->fs.uses_sample_shading);
      print_nz_bool(fp, "early_fragment_tests", info->fs.early_fragment_tests);
      print_nz_bool(fp, "inner_coverage", info->fs.inner_coverage);
      print_nz_bool(fp, "post_depth_coverage", info->fs.post_depth_coverage);

      print_nz_bool(fp, "pixel_center_integer", info->fs.pixel_center_integer);
      print_nz_bool(fp, "origin_upper_left", info->fs.origin_upper_left);
      print_nz_bool(fp, "pixel_interlock_ordered", info->fs.pixel_interlock_ordered);
      print_nz_bool(fp, "pixel_interlock_unordered", info->fs.pixel_interlock_unordered);
      print_nz_bool(fp, "sample_interlock_ordered", info->fs.sample_interlock_ordered);
      print_nz_bool(fp, "sample_interlock_unordered", info->fs.sample_interlock_unordered);
      print_nz_bool(fp, "untyped_color_outputs", info->fs.untyped_color_outputs);

      print_nz_unsigned(fp, "depth_layout", info->fs.depth_layout);
      print_nz_x32(fp, "advanced_blend_modes", info->fs.advanced_blend_modes);
      break;

   case MESA_SHADER_COMPUTE:
   case MESA_SHADER_KERNEL:
      if (info->cs.workgroup_size_hint[0] || info->cs.workgroup_size_hint[1] || info->cs.workgroup_size_hint[2])
         fprintf(fp, "workgroup_size_hint: {%u, %u, %u}\n",
                 info->cs.workgroup_size_hint[0],
                 info->cs.workgroup_size_hint[1],
                 info->cs.workgroup_size_hint[2]);
      print_nz_unsigned(fp, "user_data_components_amd", info->cs.user_data_components_amd);
      print_nz_unsigned(fp, "ptr_size", info->cs.ptr_size);
      break;

   case MESA_SHADER_MESH:
      print_nz_x64(fp, "ms_cross_invocation_output_access", info->mesh.ms_cross_invocation_output_access);
      fprintf(fp, "max_vertices_out: %u\n", info->mesh.max_vertices_out);
      fprintf(fp, "max_primitives_out: %u\n", info->mesh.max_primitives_out);
      fprintf(fp, "primitive_type: %s\n", primitive_name(info->mesh.primitive_type));
      print_nz_bool(fp, "nv", info->mesh.nv);
      break;

   default:
      fprintf(fp, "Unhandled stage %d\n", info->stage);
   }
}

void
nir_print_function_body(nir_function_impl *impl, FILE *fp)
{
   print_state state = { 0 };
   init_print_state(&state, impl->function->shader, fp);
   state.def_prefix = "%";
   print_function_impl(impl, &state, false);
   destroy_print_state(&state);
}

static void
_nir_print_shader_annotated(nir_shader *shader, FILE *fp,
                            struct hash_table *annotations,
                            bool gather_debug_info)
{
   print_state state;
   init_print_state(&state, shader, fp);
   state.def_prefix = gather_debug_info ? "ssa_" : "%";
   state.annotations = annotations;
   state.gather_debug_info = gather_debug_info;

   print_shader_info(&shader->info, fp);

   print_nz_unsigned(fp, "inputs", shader->num_inputs);
   print_nz_unsigned(fp, "outputs", shader->num_outputs);
   print_nz_unsigned(fp, "uniforms", shader->num_uniforms);
   if (shader->scratch_size)
      fprintf(fp, "scratch: %u\n", shader->scratch_size);
   if (shader->constant_data_size)
      fprintf(fp, "constants: %u\n", shader->constant_data_size);
   if (shader->printf_info_count)
      fprintf(fp, "printfs: %u\n", shader->printf_info_count);

   if (NIR_DEBUG(PRINT_STRUCT_DECLS)) {
      nir_foreach_variable_in_shader(var, shader) {
         const struct glsl_type *type = glsl_without_array(var->type);
         if (glsl_type_is_struct_or_ifc(type))
            print_struct_decl(type, &state);
      }

      nir_foreach_function_impl(impl, shader) {
         nir_foreach_function_temp_variable(var, impl) {
            const struct glsl_type *type = glsl_without_array(var->type);
            if (glsl_type_is_struct_or_ifc(type))
               print_struct_decl(type, &state);
         }
      }
   }

   for (unsigned i = 0; i < nir_num_variable_modes; i++) {
      nir_variable_mode mode = BITFIELD_BIT(i);
      if (mode == nir_var_function_temp)
         continue;

      if (mode == nir_var_shader_in || mode == nir_var_shader_out) {
         for (unsigned j = 0; j < 128; j++) {
            nir_variable *vars[NIR_MAX_VEC_COMPONENTS] = { 0 };
            nir_foreach_variable_with_modes(var, shader, mode) {
               if (var->data.location == j)
                  vars[var->data.location_frac] = var;
            }
            for (unsigned j = 0; j < ARRAY_SIZE(vars); j++)
               if (vars[j]) {
                  print_var_decl(vars[j], &state);
               }
         }
      } else {
         nir_foreach_variable_with_modes(var, shader, mode)
            print_var_decl(var, &state);
      }
   }

   foreach_list_typed(nir_function, func, node, &shader->functions) {
      print_function(func, &state);
   }

   destroy_print_state(&state);
}

void
nir_print_shader_annotated(nir_shader *shader, FILE *fp,
                           struct hash_table *annotations)
{
   _nir_print_shader_annotated(shader, fp, annotations, false);
}

void
nir_print_shader(nir_shader *shader, FILE *fp)
{
   nir_foreach_function_impl(impl, shader) {
      nir_index_ssa_defs(impl);
   }
   nir_print_shader_annotated(shader, fp, NULL);
   fflush(fp);
}

static char *
_nir_shader_as_str_annotated(nir_shader *nir, struct hash_table *annotations, void *mem_ctx,
                             bool gather_debug_info)
{
   char *stream_data = NULL;
   size_t stream_size = 0;
   struct u_memstream mem;
   if (u_memstream_open(&mem, &stream_data, &stream_size)) {
      FILE *const stream = u_memstream_get(&mem);
      _nir_print_shader_annotated(nir, stream, annotations, gather_debug_info);
      u_memstream_close(&mem);
   }

   char *str = ralloc_size(mem_ctx, stream_size + 1);
   memcpy(str, stream_data, stream_size);
   str[stream_size] = '\0';

   free(stream_data);

   return str;
}

char *
nir_shader_as_str_annotated(nir_shader *nir, struct hash_table *annotations, void *mem_ctx)
{
   return _nir_shader_as_str_annotated(nir, annotations, mem_ctx, false);
}

char *
nir_shader_as_str(nir_shader *nir, void *mem_ctx)
{
   return nir_shader_as_str_annotated(nir, NULL, mem_ctx);
}

void
nir_print_instr(const nir_instr *instr, FILE *fp)
{
   print_state state = {
      .fp = fp,
      .def_prefix = "%",
   };
   if (instr->block) {
      nir_function_impl *impl = instr->block->impl;
      state.shader = impl->function->shader;
      state.divergence_valid = impl->valid_metadata & nir_metadata_divergence;
   }

   print_instr(instr, &state, 0);
}

char *
nir_instr_as_str(const nir_instr *instr, void *mem_ctx)
{
   char *stream_data = NULL;
   size_t stream_size = 0;
   struct u_memstream mem;
   if (u_memstream_open(&mem, &stream_data, &stream_size)) {
      FILE *const stream = u_memstream_get(&mem);
      nir_print_instr(instr, stream);
      u_memstream_close(&mem);
   }

   char *str = ralloc_size(mem_ctx, stream_size + 1);
   memcpy(str, stream_data, stream_size);
   str[stream_size] = '\0';

   free(stream_data);

   return str;
}

void
nir_print_deref(const nir_deref_instr *deref, FILE *fp)
{
   print_state state = {
      .fp = fp,
      .def_prefix = "%",
   };
   print_deref_link(deref, true, &state);
}

void
nir_log_shader_annotated_tagged(enum mesa_log_level level, const char *tag,
                                nir_shader *shader, struct hash_table *annotations)
{
   char *str = nir_shader_as_str_annotated(shader, annotations, NULL);
   _mesa_log_multiline(level, tag, str);
   ralloc_free(str);
}

char *
nir_shader_gather_debug_info(nir_shader *shader, const char *filename, uint32_t first_line)
{
   char *str = _nir_shader_as_str_annotated(shader, NULL, NULL, shader->has_debug_info);

   if (!shader->has_debug_info)
      return str;

   uint32_t line = first_line;
   uint32_t character_index = 0;

   nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
         nir_foreach_instr_safe(instr, block) {
            nir_instr_debug_info *di = nir_instr_get_debug_info(instr);
            while (character_index < di->nir_line) {
               if (str[character_index] == '\n')
                  line++;
               character_index++;
            }

            di->nir_line = line;
         }
      }
   }

   return str;
}
