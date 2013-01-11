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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file lower_varyings_to_packed.cpp
 *
 * This lowering pass generates GLSL code that manually packs varyings into
 * vec4 slots, for the benefit of back-ends that don't support packed varyings
 * natively.
 *
 * For example, the following shader:
 *
 *   out mat3x2 foo;  // location=4, location_frac=0
 *   out vec3 bar[2]; // location=5, location_frac=2
 *
 *   main()
 *   {
 *     ...
 *   }
 *
 * Is rewritten to:
 *
 *   mat3x2 foo;
 *   vec3 bar[2];
 *   out vec4 packed4; // location=4, location_frac=0
 *   out vec4 packed5; // location=5, location_frac=0
 *   out vec4 packed6; // location=6, location_frac=0
 *
 *   main()
 *   {
 *     ...
 *     packed4.xy = foo[0];
 *     packed4.zw = foo[1];
 *     packed5.xy = foo[2];
 *     packed5.zw = bar[0].xy;
 *     packed6.x = bar[0].z;
 *     packed6.yzw = bar[1];
 *   }
 *
 * This lowering pass properly handles "double parking" of a varying vector
 * across two varying slots.  For example, in the code above, two of the
 * components of bar[0] are stored in packed5, and the remaining component is
 * stored in packed6.
 *
 * Note that in theory, the extra instructions may cause some loss of
 * performance.  However, hopefully in most cases the performance loss will
 * either be absorbed by a later optimization pass, or it will be offset by
 * memory bandwidth savings (because fewer varyings are used).
 *
 * This lowering pass also packs flat floats, ints, and uints together, by
 * using ivec4 as the base type of flat "varyings", and using appropriate
 * casts to convert floats and uints into ints.
 *
 * This lowering pass also handles varyings whose type is a struct or an array
 * of struct.  Structs are packed in order and with no gaps, so there may be a
 * performance penalty due to structure elements being double-parked.
 */

#include "glsl_symbol_table.h"
#include "ir.h"
#include "ir_optimization.h"

/**
 * Visitor that performs varying packing.  For each varying declared in the
 * shader, this visitor determines whether it needs to be packed.  If so, it
 * demotes it to an ordinary global, creates new packed varyings, and
 * generates assignments to convert between the original varying and the
 * packed varying.
 */
class lower_packed_varyings_visitor
{
public:
   lower_packed_varyings_visitor(void *mem_ctx, unsigned location_base,
                                 unsigned locations_used,
                                 ir_variable_mode mode,
                                 exec_list *main_instructions);

   void run(exec_list *instructions);

private:
   ir_assignment *bitwise_assign_pack(ir_rvalue *lhs, ir_rvalue *rhs);
   ir_assignment *bitwise_assign_unpack(ir_rvalue *lhs, ir_rvalue *rhs);
   unsigned lower_rvalue(ir_rvalue *rvalue, unsigned fine_location,
                         ir_variable *unpacked_var, const char *name);
   unsigned lower_arraylike(ir_rvalue *rvalue, unsigned array_size,
                            unsigned fine_location,
                            ir_variable *unpacked_var, const char *name);
   ir_variable *get_packed_varying(unsigned location,
                                   ir_variable *unpacked_var,
                                   const char *name);
   bool needs_lowering(ir_variable *var);

   /**
    * Memory context used to allocate new instructions for the shader.
    */
   void * const mem_ctx;

   /**
    * Location representing the first generic varying slot for this shader
    * stage (e.g. VERT_RESULT_VAR0 if we are packing vertex shader outputs).
    * Varyings whose location is less than this value are assumed to
    * correspond to special fixed function hardware, so they are not lowered.
    */
   const unsigned location_base;

   /**
    * Number of generic varying slots which are used by this shader.  This is
    * used to allocate temporary intermediate data structures.  If any any
    * varying used by this shader has a location greater than or equal to
    * location_base + locations_used, an assertion will fire.
    */
   const unsigned locations_used;

   /**
    * Array of pointers to the packed varyings that have been created for each
    * generic varying slot.  NULL entries in this array indicate varying slots
    * for which a packed varying has not been created yet.
    */
   ir_variable **packed_varyings;

   /**
    * Type of varying which is being lowered in this pass (either
    * ir_var_shader_in or ir_var_shader_out).
    */
   const ir_variable_mode mode;

   /**
    * List of instructions corresponding to the main() function.  This is
    * where we add instructions to pack or unpack the varyings.
    */
   exec_list *main_instructions;
};

lower_packed_varyings_visitor::lower_packed_varyings_visitor(
      void *mem_ctx, unsigned location_base, unsigned locations_used,
      ir_variable_mode mode, exec_list *main_instructions)
   : mem_ctx(mem_ctx),
     location_base(location_base),
     locations_used(locations_used),
     packed_varyings((ir_variable **)
                     rzalloc_array_size(mem_ctx, sizeof(*packed_varyings),
                                        locations_used)),
     mode(mode),
     main_instructions(main_instructions)
{
}

void
lower_packed_varyings_visitor::run(exec_list *instructions)
{
   foreach_list (node, instructions) {
      ir_variable *var = ((ir_instruction *) node)->as_variable();
      if (var == NULL)
         continue;

      if (var->mode != this->mode ||
          var->location < (int) this->location_base ||
          !this->needs_lowering(var))
         continue;

      /* Change the old varying into an ordinary global. */
      var->mode = ir_var_auto;

      /* Create a reference to the old varying. */
      ir_dereference_variable *deref
         = new(this->mem_ctx) ir_dereference_variable(var);

      /* Recursively pack or unpack it. */
      this->lower_rvalue(deref, var->location * 4 + var->location_frac, var,
                         var->name);
   }
}


/**
 * Make an ir_assignment from \c rhs to \c lhs, performing appropriate
 * bitcasts if necessary to match up types.
 *
 * This function is called when packing varyings.
 */
ir_assignment *
lower_packed_varyings_visitor::bitwise_assign_pack(ir_rvalue *lhs,
                                                   ir_rvalue *rhs)
{
   if (lhs->type->base_type != rhs->type->base_type) {
      /* Since we only mix types in flat varyings, and we always store flat
       * varyings as type ivec4, we need only produce conversions from (uint
       * or float) to int.
       */
      assert(lhs->type->base_type == GLSL_TYPE_INT);
      switch (rhs->type->base_type) {
      case GLSL_TYPE_UINT:
         rhs = new(this->mem_ctx)
            ir_expression(ir_unop_u2i, lhs->type, rhs);
         break;
      case GLSL_TYPE_FLOAT:
         rhs = new(this->mem_ctx)
            ir_expression(ir_unop_bitcast_f2i, lhs->type, rhs);
         break;
      default:
         assert(!"Unexpected type conversion while lowering varyings");
         break;
      }
   }
   return new(this->mem_ctx) ir_assignment(lhs, rhs);
}


/**
 * Make an ir_assignment from \c rhs to \c lhs, performing appropriate
 * bitcasts if necessary to match up types.
 *
 * This function is called when unpacking varyings.
 */
ir_assignment *
lower_packed_varyings_visitor::bitwise_assign_unpack(ir_rvalue *lhs,
                                                     ir_rvalue *rhs)
{
   if (lhs->type->base_type != rhs->type->base_type) {
      /* Since we only mix types in flat varyings, and we always store flat
       * varyings as type ivec4, we need only produce conversions from int to
       * (uint or float).
       */
      assert(rhs->type->base_type == GLSL_TYPE_INT);
      switch (lhs->type->base_type) {
      case GLSL_TYPE_UINT:
         rhs = new(this->mem_ctx)
            ir_expression(ir_unop_i2u, lhs->type, rhs);
         break;
      case GLSL_TYPE_FLOAT:
         rhs = new(this->mem_ctx)
            ir_expression(ir_unop_bitcast_i2f, lhs->type, rhs);
         break;
      default:
         assert(!"Unexpected type conversion while lowering varyings");
         break;
      }
   }
   return new(this->mem_ctx) ir_assignment(lhs, rhs);
}


/**
 * Recursively pack or unpack the given varying (or portion of a varying) by
 * traversing all of its constituent vectors.
 *
 * \param fine_location is the location where the first constituent vector
 * should be packed--the word "fine" indicates that this location is expressed
 * in multiples of a float, rather than multiples of a vec4 as is used
 * elsewhere in Mesa.
 *
 * \return the location where the next constituent vector (after this one)
 * should be packed.
 */
unsigned
lower_packed_varyings_visitor::lower_rvalue(ir_rvalue *rvalue,
                                            unsigned fine_location,
                                            ir_variable *unpacked_var,
                                            const char *name)
{
   if (rvalue->type->is_record()) {
      for (unsigned i = 0; i < rvalue->type->length; i++) {
         if (i != 0)
            rvalue = rvalue->clone(this->mem_ctx, NULL);
         const char *field_name = rvalue->type->fields.structure[i].name;
         ir_dereference_record *dereference_record = new(this->mem_ctx)
            ir_dereference_record(rvalue, field_name);
         char *deref_name
            = ralloc_asprintf(this->mem_ctx, "%s.%s", name, field_name);
         fine_location = this->lower_rvalue(dereference_record, fine_location,
                                            unpacked_var, deref_name);
      }
      return fine_location;
   } else if (rvalue->type->is_array()) {
      /* Arrays are packed/unpacked by considering each array element in
       * sequence.
       */
      return this->lower_arraylike(rvalue, rvalue->type->array_size(),
                                   fine_location, unpacked_var, name);
   } else if (rvalue->type->is_matrix()) {
      /* Matrices are packed/unpacked by considering each column vector in
       * sequence.
       */
      return this->lower_arraylike(rvalue, rvalue->type->matrix_columns,
                                   fine_location, unpacked_var, name);
   } else if (rvalue->type->vector_elements + fine_location % 4 > 4) {
      /* This vector is going to be "double parked" across two varying slots,
       * so handle it as two separate assignments.
       */
      unsigned left_components = 4 - fine_location % 4;
      unsigned right_components
         = rvalue->type->vector_elements - left_components;
      unsigned left_swizzle_values[4] = { 0, 0, 0, 0 };
      unsigned right_swizzle_values[4] = { 0, 0, 0, 0 };
      char left_swizzle_name[4] = { 0, 0, 0, 0 };
      char right_swizzle_name[4] = { 0, 0, 0, 0 };
      for (unsigned i = 0; i < left_components; i++) {
         left_swizzle_values[i] = i;
         left_swizzle_name[i] = "xyzw"[i];
      }
      for (unsigned i = 0; i < right_components; i++) {
         right_swizzle_values[i] = i + left_components;
         right_swizzle_name[i] = "xyzw"[i + left_components];
      }
      ir_swizzle *left_swizzle = new(this->mem_ctx)
         ir_swizzle(rvalue, left_swizzle_values, left_components);
      ir_swizzle *right_swizzle = new(this->mem_ctx)
         ir_swizzle(rvalue->clone(this->mem_ctx, NULL), right_swizzle_values,
                    right_components);
      char *left_name
         = ralloc_asprintf(this->mem_ctx, "%s.%s", name, left_swizzle_name);
      char *right_name
         = ralloc_asprintf(this->mem_ctx, "%s.%s", name, right_swizzle_name);
      fine_location = this->lower_rvalue(left_swizzle, fine_location,
                                         unpacked_var, left_name);
      return this->lower_rvalue(right_swizzle, fine_location, unpacked_var,
                                right_name);
   } else {
      /* No special handling is necessary; pack the rvalue into the
       * varying.
       */
      unsigned swizzle_values[4] = { 0, 0, 0, 0 };
      unsigned components = rvalue->type->vector_elements;
      unsigned location = fine_location / 4;
      unsigned location_frac = fine_location % 4;
      for (unsigned i = 0; i < components; ++i)
         swizzle_values[i] = i + location_frac;
      ir_dereference_variable *packed_deref = new(this->mem_ctx)
         ir_dereference_variable(this->get_packed_varying(location,
                                                          unpacked_var, name));
      ir_swizzle *swizzle = new(this->mem_ctx)
         ir_swizzle(packed_deref, swizzle_values, components);
      if (this->mode == ir_var_shader_out) {
         ir_assignment *assignment
            = this->bitwise_assign_pack(swizzle, rvalue);
         this->main_instructions->push_tail(assignment);
      } else {
         ir_assignment *assignment
            = this->bitwise_assign_unpack(rvalue, swizzle);
         this->main_instructions->push_head(assignment);
      }
      return fine_location + components;
   }
}

/**
 * Recursively pack or unpack a varying for which we need to iterate over its
 * constituent elements, accessing each one using an ir_dereference_array.
 * This takes care of both arrays and matrices, since ir_dereference_array
 * treats a matrix like an array of its column vectors.
 */
unsigned
lower_packed_varyings_visitor::lower_arraylike(ir_rvalue *rvalue,
                                               unsigned array_size,
                                               unsigned fine_location,
                                               ir_variable *unpacked_var,
                                               const char *name)
{
   for (unsigned i = 0; i < array_size; i++) {
      if (i != 0)
         rvalue = rvalue->clone(this->mem_ctx, NULL);
      ir_constant *constant = new(this->mem_ctx) ir_constant(i);
      ir_dereference_array *dereference_array = new(this->mem_ctx)
         ir_dereference_array(rvalue, constant);
      char *subscripted_name
         = ralloc_asprintf(this->mem_ctx, "%s[%d]", name, i);
      fine_location = this->lower_rvalue(dereference_array, fine_location,
                                         unpacked_var, subscripted_name);
   }
   return fine_location;
}

/**
 * Retrieve the packed varying corresponding to the given varying location.
 * If no packed varying has been created for the given varying location yet,
 * create it and add it to the shader before returning it.
 *
 * The newly created varying inherits its interpolation parameters from \c
 * unpacked_var.  Its base type is ivec4 if we are lowering a flat varying,
 * vec4 otherwise.
 */
ir_variable *
lower_packed_varyings_visitor::get_packed_varying(unsigned location,
                                                  ir_variable *unpacked_var,
                                                  const char *name)
{
   unsigned slot = location - this->location_base;
   assert(slot < locations_used);
   if (this->packed_varyings[slot] == NULL) {
      char *packed_name = ralloc_asprintf(this->mem_ctx, "packed:%s", name);
      const glsl_type *packed_type;
      if (unpacked_var->interpolation == INTERP_QUALIFIER_FLAT)
         packed_type = glsl_type::ivec4_type;
      else
         packed_type = glsl_type::vec4_type;
      ir_variable *packed_var = new(this->mem_ctx)
         ir_variable(packed_type, packed_name, this->mode);
      packed_var->centroid = unpacked_var->centroid;
      packed_var->interpolation = unpacked_var->interpolation;
      packed_var->location = location;
      unpacked_var->insert_before(packed_var);
      this->packed_varyings[slot] = packed_var;
   } else {
      ralloc_asprintf_append((char **) &this->packed_varyings[slot]->name,
                             ",%s", name);
   }
   return this->packed_varyings[slot];
}

bool
lower_packed_varyings_visitor::needs_lowering(ir_variable *var)
{
   /* Things composed of vec4's don't need lowering.  Everything else does. */
   const glsl_type *type = var->type;
   if (type->is_array())
      type = type->fields.array;
   if (type->vector_elements == 4)
      return false;
   return true;
}

void
lower_packed_varyings(void *mem_ctx, unsigned location_base,
                      unsigned locations_used, ir_variable_mode mode,
                      gl_shader *shader)
{
   exec_list *instructions = shader->ir;
   ir_function *main_func = shader->symbols->get_function("main");
   exec_list void_parameters;
   ir_function_signature *main_func_sig
      = main_func->matching_signature(&void_parameters);
   exec_list *main_instructions = &main_func_sig->body;
   lower_packed_varyings_visitor visitor(mem_ctx, location_base,
                                         locations_used, mode,
                                         main_instructions);
   visitor.run(instructions);
}
