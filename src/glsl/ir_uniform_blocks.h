#pragma once

#include "ir.h"
#include <algorithm>
#include <vector>
#include <cstring>

// Shared by reflection and the Metal printer so resource indices agree.
struct uniform_block {
   const glsl_type* type;
   ir_variable* variable;
   unsigned array_size;
   unsigned metal_index;
};

inline std::vector<uniform_block> collect_uniform_blocks(exec_list* ir)
{
   std::vector<uniform_block> blocks;
   foreach_in_list(ir_instruction, instruction, ir) {
      ir_variable* v = instruction->as_variable();
      if (!v || v->data.mode != ir_var_uniform || !v->is_in_buffer_block()) continue;
      const glsl_type* type = v->get_interface_type();
      auto found = std::find_if(blocks.begin(), blocks.end(), [type](const uniform_block& b) { return b.type == type; });
      if (found != blocks.end()) continue;
      blocks.push_back({type, v, v->is_interface_instance() && v->type->is_array() ? v->type->arrays_of_arrays_size() : 1, 0});
   }
   std::sort(blocks.begin(), blocks.end(), [](const uniform_block& a, const uniform_block& b) { return strcmp(a.type->name, b.type->name) < 0; });
   unsigned index = 1; // Slot zero is reserved for ordinary uniforms.
   for (auto& b : blocks) { b.metal_index = index; index += b.array_size; }
   return blocks;
}

inline bool uniform_field_row_major(const glsl_struct_field& field, bool inherited)
{
   return field.matrix_layout == GLSL_MATRIX_LAYOUT_ROW_MAJOR ||
      (field.matrix_layout == GLSL_MATRIX_LAYOUT_INHERITED && inherited);
}

inline unsigned uniform_align(unsigned value, unsigned alignment)
{
   return (value + alignment - 1) & ~(alignment - 1);
}

inline unsigned uniform_field_offset(const glsl_type* block, unsigned field, unsigned cursor, bool row_major)
{
   const auto& f = block->fields.structure[field];
   if (block->is_interface() && f.offset >= 0) cursor = f.offset;
   return uniform_align(cursor, f.type->std140_base_alignment(uniform_field_row_major(f, row_major)));
}

inline unsigned uniform_block_size(const glsl_type* block)
{
   unsigned size = 0, alignment = 16;
   bool row = block->get_interface_row_major();
   for (unsigned i = 0; i < block->length; ++i) {
      const auto& f = block->fields.structure[i];
      bool fieldRow = uniform_field_row_major(f, row);
      alignment = MAX2(alignment, f.type->std140_base_alignment(fieldRow));
      size = uniform_field_offset(block, i, size, row) + f.type->std140_size(fieldRow);
   }
   return uniform_align(size, alignment);
}
