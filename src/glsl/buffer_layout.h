// Uniform/storage block and atomic counter buffer layout, shared by reflection
// and the Metal printer so that offsets and Metal resource slots agree.
#pragma once

#include "compiler/nir/nir.h"
#include "compiler/glsl_types.h"
#include "util/u_math.h"
#include <algorithm>
#include <set>
#include <vector>
#include <string.h>

struct glslopt_buffer_block {
   const glsl_type* type; // interface type
   nir_variable* variable; // first variable of the block (instance, or a member of an unnamed block)
   bool named;
   unsigned array_size;
   unsigned metal_index;
   bool needs_size;
};

inline bool glslopt_block_is_storage(const glslopt_buffer_block& b)
{
   return b.variable->data.mode == nir_var_mem_ssbo;
}

inline bool glslopt_buffer_has_runtime_array(const glsl_type* block)
{
   unsigned n = glsl_get_length(block);
   return n && glsl_type_is_unsized_array(glsl_get_struct_field(block, n - 1));
}

inline std::vector<glslopt_buffer_block> glslopt_collect_buffer_blocks(nir_shader* nir)
{
   std::vector<glslopt_buffer_block> blocks;
   nir_foreach_variable_with_modes(v, nir, nir_var_mem_ubo | nir_var_mem_ssbo) {
      const glsl_type* type = v->interface_type;
      if (!type)
         continue;
      auto found = std::find_if(blocks.begin(), blocks.end(), [type](const glslopt_buffer_block& b) { return b.type == type; });
      if (found != blocks.end())
         continue;
      bool named = glsl_without_array(v->type) == type;
      blocks.push_back({type, v, named, named && glsl_type_is_array(v->type) ? glsl_get_aoa_size(v->type) : 1u, 0, false});
   }
   std::sort(blocks.begin(), blocks.end(), [](const glslopt_buffer_block& a, const glslopt_buffer_block& b) {
      return strcmp(glsl_get_type_name(a.type), glsl_get_type_name(b.type)) < 0;
   });
   unsigned index = 1; // Slot zero is reserved for ordinary uniforms.
   for (auto& b : blocks) { b.metal_index = index; index += b.array_size; }
   {
      std::set<const glsl_type*> sized;
      nir_foreach_function_impl(impl, nir) {
         nir_foreach_block(block, impl) {
            nir_foreach_instr(instr, block) {
               if (instr->type != nir_instr_type_intrinsic ||
                   nir_instr_as_intrinsic(instr)->intrinsic != nir_intrinsic_deref_buffer_array_length)
                  continue;
               nir_variable* v = nir_deref_instr_get_variable(nir_src_as_deref(nir_instr_as_intrinsic(instr)->src[0]));
               if (v && v->interface_type)
                  sized.insert(v->interface_type);
            }
         }
      }
      for (auto& b : blocks) b.needs_size = sized.count(b.type) != 0;
   }
   return blocks;
}

// The size table follows all data buffers; entry N describes Metal buffer N.
inline unsigned glslopt_buffer_size_table_index(const std::vector<glslopt_buffer_block>& blocks)
{
   for (const auto& b : blocks)
      if (b.needs_size) return blocks.back().metal_index + blocks.back().array_size;
   return 0;
}

struct glslopt_atomic_counter_buffer {
   unsigned binding;
   unsigned metal_index;
   unsigned size;
   std::vector<nir_variable*> variables;
};

// Counter buffers follow block buffers and their optional byte-size table.
inline std::vector<glslopt_atomic_counter_buffer>
glslopt_collect_atomic_counter_buffers(nir_shader* nir, const std::vector<glslopt_buffer_block>& blocks)
{
   std::vector<glslopt_atomic_counter_buffer> counters;
   nir_foreach_variable_with_modes(v, nir, nir_var_uniform) {
      if (!glsl_contains_atomic(v->type)) continue;
      auto found = std::find_if(counters.begin(), counters.end(), [v](const glslopt_atomic_counter_buffer& b) { return b.binding == v->data.binding; });
      if (found == counters.end()) {
         counters.push_back({v->data.binding, 0, 0, {}});
         found = counters.end() - 1;
      }
      found->size = MAX2(found->size, v->data.offset + glsl_atomic_size(v->type));
      found->variables.push_back(v);
   }
   if (counters.empty()) return counters;
   unsigned index = blocks.empty() ? 1 : blocks.back().metal_index + blocks.back().array_size;
   if (glslopt_buffer_size_table_index(blocks)) ++index;
   std::sort(counters.begin(), counters.end(), [](const glslopt_atomic_counter_buffer& a, const glslopt_atomic_counter_buffer& b) { return a.binding < b.binding; });
   for (auto& b : counters) b.metal_index = index++;
   return counters;
}

// Shared/packed SSBOs use compact storage on Metal; explicit layouts are preserved.
inline bool glslopt_buffer_uses_std430(const glslopt_buffer_block& block, bool metal)
{
   auto packing = glsl_get_ifc_packing(block.type);
   return packing == GLSL_INTERFACE_PACKING_STD430 ||
      (metal && glslopt_block_is_storage(block) && packing != GLSL_INTERFACE_PACKING_STD140);
}

inline bool glslopt_buffer_layout_known(const glslopt_buffer_block& block, bool metal)
{
   auto packing = glsl_get_ifc_packing(block.type);
   return metal || packing == GLSL_INTERFACE_PACKING_STD140 || packing == GLSL_INTERFACE_PACKING_STD430;
}

inline bool glslopt_field_row_major(const glsl_struct_field& field, bool inherited)
{
   return field.matrix_layout == GLSL_MATRIX_LAYOUT_ROW_MAJOR ||
      (field.matrix_layout == GLSL_MATRIX_LAYOUT_INHERITED && inherited);
}

inline unsigned glslopt_buffer_alignment(const glsl_type* type, bool row, bool std430)
{
   if (glsl_type_is_interface(type)) {
      unsigned alignment = std430 ? 1 : 16;
      for (unsigned i = 0; i < glsl_get_length(type); ++i) {
         const glsl_struct_field& f = *glsl_get_struct_field_data(type, i);
         alignment = MAX2(alignment, glslopt_buffer_alignment(f.type, glslopt_field_row_major(f, row), std430));
      }
      return alignment;
   }
   return std430 ? glsl_get_std430_base_alignment(type, row) : glsl_get_std140_base_alignment(type, row);
}

inline unsigned glslopt_buffer_size(const glsl_type* type, bool row, bool std430)
{
   return std430 ? glsl_get_std430_size(type, row) : glsl_get_std140_size(type, row);
}

inline unsigned glslopt_buffer_array_stride(const glsl_type* element, bool row, bool std430)
{
   return align(glslopt_buffer_size(element, row, std430),
      MAX2(std430 ? 1u : 16u, glslopt_buffer_alignment(element, row, std430)));
}

// byte offsets of the fields of a struct or block
inline std::vector<unsigned> glslopt_buffer_field_offsets(const glsl_type* t, bool row, bool std430)
{
   std::vector<unsigned> offsets;
   unsigned cursor = 0;
   for (unsigned i = 0; i < glsl_get_length(t); ++i) {
      const glsl_struct_field& f = *glsl_get_struct_field_data(t, i);
      bool frow = glslopt_field_row_major(f, row);
      if (glsl_type_is_interface(t) && f.offset >= 0)
         cursor = f.offset;
      cursor = align(cursor, glslopt_buffer_alignment(f.type, frow, std430));
      offsets.push_back(cursor);
      cursor += glslopt_buffer_size(f.type, frow, std430);
   }
   return offsets;
}

inline unsigned glslopt_buffer_block_size(const glsl_type* block, bool std430)
{
   bool row = glsl_matrix_type_is_row_major(block);
   unsigned n = glsl_get_length(block);
   if (!n)
      return 0;
   const glsl_struct_field& last = *glsl_get_struct_field_data(block, n - 1);
   unsigned size = glslopt_buffer_field_offsets(block, row, std430).back() +
                   glslopt_buffer_size(last.type, glslopt_field_row_major(last, row), std430);
   return align(size, glslopt_buffer_alignment(block, row, std430));
}
