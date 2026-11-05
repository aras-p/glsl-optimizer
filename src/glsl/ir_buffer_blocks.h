#pragma once

#include "ir.h"
#include "ir_hierarchical_visitor.h"
#include <unordered_set>
#include <algorithm>
#include <vector>
#include <cstring>

struct buffer_atomic_function {
   ir_intrinsic_id intrinsic;
   const char* glsl;
   const char* metal;
};

inline const buffer_atomic_function* find_buffer_atomic(const ir_function_signature* signature)
{
   if (!signature->is_builtin()) return nullptr;
   bool intrinsic = signature->is_intrinsic();
   if (intrinsic) {
      if (signature->intrinsic_id < ir_intrinsic_generic_atomic_add ||
          signature->intrinsic_id > ir_intrinsic_generic_atomic_comp_swap) return nullptr;
   } else if (strncmp(signature->function_name(), "atomic", 6) != 0) {
      return nullptr;
   }
   static const buffer_atomic_function functions[] = {
      {ir_intrinsic_generic_atomic_add, "atomicAdd", "atomic_fetch_add_explicit"},
      {ir_intrinsic_generic_atomic_min, "atomicMin", "atomic_fetch_min_explicit"},
      {ir_intrinsic_generic_atomic_max, "atomicMax", "atomic_fetch_max_explicit"},
      {ir_intrinsic_generic_atomic_and, "atomicAnd", "atomic_fetch_and_explicit"},
      {ir_intrinsic_generic_atomic_or, "atomicOr", "atomic_fetch_or_explicit"},
      {ir_intrinsic_generic_atomic_xor, "atomicXor", "atomic_fetch_xor_explicit"},
      {ir_intrinsic_generic_atomic_exchange, "atomicExchange", "atomic_exchange_explicit"},
      {ir_intrinsic_generic_atomic_comp_swap, "atomicCompSwap", "_xlat_atomic_comp_swap"},
   };
   for (const auto& f : functions)
      if (intrinsic ? signature->intrinsic_id == f.intrinsic : strcmp(signature->function_name(), f.glsl) == 0) return &f;
   return nullptr;
}

// Shared by reflection and the Metal printer so resource indices agree.
struct buffer_block {
   const glsl_type* type;
   ir_variable* variable;
   unsigned array_size;
   unsigned metal_index;
   bool needs_size;
};

inline bool buffer_has_runtime_array(const glsl_type* block)
{
   return block->fields.structure[block->length - 1].type->is_unsized_array();
}

class buffer_length_visitor : public ir_hierarchical_visitor {
public:
   std::unordered_set<const glsl_type*> blocks;
   ir_visitor_status visit_enter(ir_expression* expression) override {
      if (expression->operation == ir_unop_ssbo_unsized_array_length)
         blocks.insert(expression->operands[0]->variable_referenced()->get_interface_type());
      return visit_continue;
   }
};

inline std::vector<buffer_block> collect_buffer_blocks(exec_list* ir, bool find_lengths = false)
{
   std::vector<buffer_block> blocks;
   foreach_in_list(ir_instruction, instruction, ir) {
      ir_variable* v = instruction->as_variable();
      if (!v || (v->data.mode != ir_var_uniform && v->data.mode != ir_var_shader_storage) || !v->is_in_buffer_block()) continue;
      const glsl_type* type = v->get_interface_type();
      auto found = std::find_if(blocks.begin(), blocks.end(), [type](const buffer_block& b) { return b.type == type; });
      if (found != blocks.end()) continue;
      blocks.push_back({type, v, v->is_interface_instance() && v->type->is_array() ? v->type->arrays_of_arrays_size() : 1, 0, false});
   }
   std::sort(blocks.begin(), blocks.end(), [](const buffer_block& a, const buffer_block& b) { return strcmp(a.type->name, b.type->name) < 0; });
   unsigned index = 1; // Slot zero is reserved for ordinary uniforms.
   for (auto& b : blocks) { b.metal_index = index; index += b.array_size; }
   bool runtime = false;
   if (find_lengths)
      for (const auto& b : blocks) runtime |= buffer_has_runtime_array(b.type);
   if (runtime) {
      buffer_length_visitor lengths;
      visit_list_elements(&lengths, ir);
      for (auto& b : blocks) b.needs_size = lengths.blocks.count(b.type) != 0;
   }
   return blocks;
}

// The size table follows all data buffers; entry N describes Metal buffer N.
inline unsigned buffer_size_table_index(const std::vector<buffer_block>& blocks)
{
   for (const auto& b : blocks)
      if (b.needs_size) return blocks.back().metal_index + blocks.back().array_size;
   return 0;
}

// Shared/packed SSBOs use compact storage on Metal; explicit layouts are preserved.
inline bool buffer_uses_std430(const buffer_block& block, bool metal)
{
   auto packing = block.type->get_interface_packing();
   return packing == GLSL_INTERFACE_PACKING_STD430 ||
      (metal && block.variable->data.mode == ir_var_shader_storage &&
       packing != GLSL_INTERFACE_PACKING_STD140);
}

inline bool buffer_field_row_major(const glsl_struct_field& field, bool inherited)
{
   return field.matrix_layout == GLSL_MATRIX_LAYOUT_ROW_MAJOR ||
      (field.matrix_layout == GLSL_MATRIX_LAYOUT_INHERITED && inherited);
}

inline unsigned buffer_align(unsigned value, unsigned alignment)
{
   return (value + alignment - 1) & ~(alignment - 1);
}

inline unsigned buffer_alignment(const glsl_type* type, bool row, bool std430)
{
   if (type->is_interface()) {
      unsigned alignment = std430 ? 1 : 16;
      for (unsigned i = 0; i < type->length; ++i) {
         const auto& f = type->fields.structure[i];
         alignment = MAX2(alignment, buffer_alignment(f.type, buffer_field_row_major(f, row), std430));
      }
      return alignment;
   }
   return std430 ? type->std430_base_alignment(row) : type->std140_base_alignment(row);
}

inline unsigned buffer_size(const glsl_type* type, bool row, bool std430)
{
   return std430 ? type->std430_size(row) : type->std140_size(row);
}

inline unsigned buffer_array_stride(const glsl_type* element, bool row, bool std430)
{
   return buffer_align(buffer_size(element, row, std430),
      MAX2(std430 ? 1 : 16, buffer_alignment(element, row, std430)));
}

inline unsigned buffer_field_offset(const glsl_type* block, unsigned field, unsigned cursor, bool row_major, bool std430 = false)
{
   const auto& f = block->fields.structure[field];
   if (block->is_interface() && f.offset >= 0) cursor = f.offset;
   return buffer_align(cursor, buffer_alignment(f.type, buffer_field_row_major(f, row_major), std430));
}

inline unsigned buffer_block_size(const glsl_type* block, bool std430)
{
   unsigned size = 0, alignment = std430 ? 1 : 16;
   bool row = block->get_interface_row_major();
   for (unsigned i = 0; i < block->length; ++i) {
      const auto& f = block->fields.structure[i];
      bool fieldRow = buffer_field_row_major(f, row);
      alignment = MAX2(alignment, buffer_alignment(f.type, fieldRow, std430));
      size = buffer_field_offset(block, i, size, row, std430) + buffer_size(f.type, fieldRow, std430);
   }
   return buffer_align(size, alignment);
}
