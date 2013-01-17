/*
 * Copyright © 2012 Intel Corporation
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
 * \file link_varyings.cpp
 *
 * Linker functions related specifically to linking varyings between shader
 * stages.
 */


#include "main/mtypes.h"
#include "glsl_symbol_table.h"
#include "ir_optimization.h"
#include "linker.h"
#include "link_varyings.h"
#include "main/macros.h"


/**
 * Validate that outputs from one stage match inputs of another
 */
bool
cross_validate_outputs_to_inputs(struct gl_shader_program *prog,
				 gl_shader *producer, gl_shader *consumer)
{
   glsl_symbol_table parameters;
   /* FINISHME: Figure these out dynamically. */
   const char *const producer_stage = "vertex";
   const char *const consumer_stage = "fragment";

   /* Find all shader outputs in the "producer" stage.
    */
   foreach_list(node, producer->ir) {
      ir_variable *const var = ((ir_instruction *) node)->as_variable();

      if ((var == NULL) || (var->mode != ir_var_shader_out))
	 continue;

      parameters.add_variable(var);
   }


   /* Find all shader inputs in the "consumer" stage.  Any variables that have
    * matching outputs already in the symbol table must have the same type and
    * qualifiers.
    */
   foreach_list(node, consumer->ir) {
      ir_variable *const input = ((ir_instruction *) node)->as_variable();

      if ((input == NULL) || (input->mode != ir_var_shader_in))
	 continue;

      ir_variable *const output = parameters.get_variable(input->name);
      if (output != NULL) {
	 /* Check that the types match between stages.
	  */
	 if (input->type != output->type) {
	    /* There is a bit of a special case for gl_TexCoord.  This
	     * built-in is unsized by default.  Applications that variable
	     * access it must redeclare it with a size.  There is some
	     * language in the GLSL spec that implies the fragment shader
	     * and vertex shader do not have to agree on this size.  Other
	     * driver behave this way, and one or two applications seem to
	     * rely on it.
	     *
	     * Neither declaration needs to be modified here because the array
	     * sizes are fixed later when update_array_sizes is called.
	     *
	     * From page 48 (page 54 of the PDF) of the GLSL 1.10 spec:
	     *
	     *     "Unlike user-defined varying variables, the built-in
	     *     varying variables don't have a strict one-to-one
	     *     correspondence between the vertex language and the
	     *     fragment language."
	     */
	    if (!output->type->is_array()
		|| (strncmp("gl_", output->name, 3) != 0)) {
	       linker_error(prog,
			    "%s shader output `%s' declared as type `%s', "
			    "but %s shader input declared as type `%s'\n",
			    producer_stage, output->name,
			    output->type->name,
			    consumer_stage, input->type->name);
	       return false;
	    }
	 }

	 /* Check that all of the qualifiers match between stages.
	  */
	 if (input->centroid != output->centroid) {
	    linker_error(prog,
			 "%s shader output `%s' %s centroid qualifier, "
			 "but %s shader input %s centroid qualifier\n",
			 producer_stage,
			 output->name,
			 (output->centroid) ? "has" : "lacks",
			 consumer_stage,
			 (input->centroid) ? "has" : "lacks");
	    return false;
	 }

	 if (input->invariant != output->invariant) {
	    linker_error(prog,
			 "%s shader output `%s' %s invariant qualifier, "
			 "but %s shader input %s invariant qualifier\n",
			 producer_stage,
			 output->name,
			 (output->invariant) ? "has" : "lacks",
			 consumer_stage,
			 (input->invariant) ? "has" : "lacks");
	    return false;
	 }

	 if (input->interpolation != output->interpolation) {
	    linker_error(prog,
			 "%s shader output `%s' specifies %s "
			 "interpolation qualifier, "
			 "but %s shader input specifies %s "
			 "interpolation qualifier\n",
			 producer_stage,
			 output->name,
			 output->interpolation_string(),
			 consumer_stage,
			 input->interpolation_string());
	    return false;
	 }
      }
   }

   return true;
}


/**
 * Initialize this object based on a string that was passed to
 * glTransformFeedbackVaryings.  If there is a parse error, the error is
 * reported using linker_error(), and false is returned.
 */
bool
tfeedback_decl::init(struct gl_context *ctx, struct gl_shader_program *prog,
                     const void *mem_ctx, const char *input)
{
   /* We don't have to be pedantic about what is a valid GLSL variable name,
    * because any variable with an invalid name can't exist in the IR anyway.
    */

   this->location = -1;
   this->orig_name = input;
   this->is_clip_distance_mesa = false;
   this->skip_components = 0;
   this->next_buffer_separator = false;

   if (ctx->Extensions.ARB_transform_feedback3) {
      /* Parse gl_NextBuffer. */
      if (strcmp(input, "gl_NextBuffer") == 0) {
         this->next_buffer_separator = true;
         return true;
      }

      /* Parse gl_SkipComponents. */
      if (strcmp(input, "gl_SkipComponents1") == 0)
         this->skip_components = 1;
      else if (strcmp(input, "gl_SkipComponents2") == 0)
         this->skip_components = 2;
      else if (strcmp(input, "gl_SkipComponents3") == 0)
         this->skip_components = 3;
      else if (strcmp(input, "gl_SkipComponents4") == 0)
         this->skip_components = 4;

      if (this->skip_components)
         return true;
   }

   /* Parse a declaration. */
   const char *bracket = strrchr(input, '[');

   if (bracket) {
      this->var_name = ralloc_strndup(mem_ctx, input, bracket - input);
      if (sscanf(bracket, "[%u]", &this->array_subscript) != 1) {
         linker_error(prog, "Cannot parse transform feedback varying %s", input);
         return false;
      }
      this->is_subscripted = true;
   } else {
      this->var_name = ralloc_strdup(mem_ctx, input);
      this->is_subscripted = false;
   }

   /* For drivers that lower gl_ClipDistance to gl_ClipDistanceMESA, this
    * class must behave specially to account for the fact that gl_ClipDistance
    * is converted from a float[8] to a vec4[2].
    */
   if (ctx->ShaderCompilerOptions[MESA_SHADER_VERTEX].LowerClipDistance &&
       strcmp(this->var_name, "gl_ClipDistance") == 0) {
      this->is_clip_distance_mesa = true;
   }

   return true;
}


/**
 * Determine whether two tfeedback_decl objects refer to the same variable and
 * array index (if applicable).
 */
bool
tfeedback_decl::is_same(const tfeedback_decl &x, const tfeedback_decl &y)
{
   assert(x.is_varying() && y.is_varying());

   if (strcmp(x.var_name, y.var_name) != 0)
      return false;
   if (x.is_subscripted != y.is_subscripted)
      return false;
   if (x.is_subscripted && x.array_subscript != y.array_subscript)
      return false;
   return true;
}


/**
 * Assign a location for this tfeedback_decl object based on the location
 * assignment in output_var.
 *
 * If an error occurs, the error is reported through linker_error() and false
 * is returned.
 */
bool
tfeedback_decl::assign_location(struct gl_context *ctx,
                                struct gl_shader_program *prog,
                                ir_variable *output_var)
{
   assert(this->is_varying());

   if (output_var->type->is_array()) {
      /* Array variable */
      const unsigned matrix_cols =
         output_var->type->fields.array->matrix_columns;
      const unsigned vector_elements =
         output_var->type->fields.array->vector_elements;
      unsigned actual_array_size = this->is_clip_distance_mesa ?
         prog->Vert.ClipDistanceArraySize : output_var->type->array_size();

      if (this->is_subscripted) {
         /* Check array bounds. */
         if (this->array_subscript >= actual_array_size) {
            linker_error(prog, "Transform feedback varying %s has index "
                         "%i, but the array size is %u.",
                         this->orig_name, this->array_subscript,
                         actual_array_size);
            return false;
         }
         if (this->is_clip_distance_mesa) {
            this->location =
               output_var->location + this->array_subscript / 4;
            this->location_frac = this->array_subscript % 4;
         } else {
            unsigned fine_location
               = output_var->location * 4 + output_var->location_frac;
            unsigned array_elem_size = vector_elements * matrix_cols;
            fine_location += array_elem_size * this->array_subscript;
            this->location = fine_location / 4;
            this->location_frac = fine_location % 4;
         }
         this->size = 1;
      } else {
         this->location = output_var->location;
         this->location_frac = output_var->location_frac;
         this->size = actual_array_size;
      }
      this->vector_elements = vector_elements;
      this->matrix_columns = matrix_cols;
      if (this->is_clip_distance_mesa)
         this->type = GL_FLOAT;
      else
         this->type = output_var->type->fields.array->gl_type;
   } else {
      /* Regular variable (scalar, vector, or matrix) */
      if (this->is_subscripted) {
         linker_error(prog, "Transform feedback varying %s requested, "
                      "but %s is not an array.",
                      this->orig_name, this->var_name);
         return false;
      }
      this->location = output_var->location;
      this->location_frac = output_var->location_frac;
      this->size = 1;
      this->vector_elements = output_var->type->vector_elements;
      this->matrix_columns = output_var->type->matrix_columns;
      this->type = output_var->type->gl_type;
   }

   /* From GL_EXT_transform_feedback:
    *   A program will fail to link if:
    *
    *   * the total number of components to capture in any varying
    *     variable in <varyings> is greater than the constant
    *     MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS_EXT and the
    *     buffer mode is SEPARATE_ATTRIBS_EXT;
    */
   if (prog->TransformFeedback.BufferMode == GL_SEPARATE_ATTRIBS &&
       this->num_components() >
       ctx->Const.MaxTransformFeedbackSeparateComponents) {
      linker_error(prog, "Transform feedback varying %s exceeds "
                   "MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS.",
                   this->orig_name);
      return false;
   }

   return true;
}


unsigned
tfeedback_decl::get_num_outputs() const
{
   if (!this->is_varying()) {
      return 0;
   }

   return (this->num_components() + this->location_frac + 3)/4;
}


/**
 * Update gl_transform_feedback_info to reflect this tfeedback_decl.
 *
 * If an error occurs, the error is reported through linker_error() and false
 * is returned.
 */
bool
tfeedback_decl::store(struct gl_context *ctx, struct gl_shader_program *prog,
                      struct gl_transform_feedback_info *info,
                      unsigned buffer, const unsigned max_outputs) const
{
   assert(!this->next_buffer_separator);

   /* Handle gl_SkipComponents. */
   if (this->skip_components) {
      info->BufferStride[buffer] += this->skip_components;
      return true;
   }

   /* From GL_EXT_transform_feedback:
    *   A program will fail to link if:
    *
    *     * the total number of components to capture is greater than
    *       the constant MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS_EXT
    *       and the buffer mode is INTERLEAVED_ATTRIBS_EXT.
    */
   if (prog->TransformFeedback.BufferMode == GL_INTERLEAVED_ATTRIBS &&
       info->BufferStride[buffer] + this->num_components() >
       ctx->Const.MaxTransformFeedbackInterleavedComponents) {
      linker_error(prog, "The MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS "
                   "limit has been exceeded.");
      return false;
   }

   unsigned location = this->location;
   unsigned location_frac = this->location_frac;
   unsigned num_components = this->num_components();
   while (num_components > 0) {
      unsigned output_size = MIN2(num_components, 4 - location_frac);
      assert(info->NumOutputs < max_outputs);
      info->Outputs[info->NumOutputs].ComponentOffset = location_frac;
      info->Outputs[info->NumOutputs].OutputRegister = location;
      info->Outputs[info->NumOutputs].NumComponents = output_size;
      info->Outputs[info->NumOutputs].OutputBuffer = buffer;
      info->Outputs[info->NumOutputs].DstOffset = info->BufferStride[buffer];
      ++info->NumOutputs;
      info->BufferStride[buffer] += output_size;
      num_components -= output_size;
      location++;
      location_frac = 0;
   }

   info->Varyings[info->NumVarying].Name = ralloc_strdup(prog, this->orig_name);
   info->Varyings[info->NumVarying].Type = this->type;
   info->Varyings[info->NumVarying].Size = this->size;
   info->NumVarying++;

   return true;
}


ir_variable *
tfeedback_decl::find_output_var(gl_shader_program *prog,
                                gl_shader *producer) const
{
   const char *name = this->is_clip_distance_mesa
      ? "gl_ClipDistanceMESA" : this->var_name;
   ir_variable *var = producer->symbols->get_variable(name);
   if (var && var->mode == ir_var_shader_out) {
      const glsl_type *type = var->type;
      while (type->base_type == GLSL_TYPE_ARRAY)
         type = type->fields.array;
      if (type->base_type == GLSL_TYPE_STRUCT) {
         linker_error(prog, "Transform feedback of varying structs not "
                      "implemented yet.");
         return NULL;
      }
      return var;
   }

   /* From GL_EXT_transform_feedback:
    *   A program will fail to link if:
    *
    *   * any variable name specified in the <varyings> array is not
    *     declared as an output in the geometry shader (if present) or
    *     the vertex shader (if no geometry shader is present);
    */
   linker_error(prog, "Transform feedback varying %s undeclared.",
                this->orig_name);
   return NULL;
}


/**
 * Parse all the transform feedback declarations that were passed to
 * glTransformFeedbackVaryings() and store them in tfeedback_decl objects.
 *
 * If an error occurs, the error is reported through linker_error() and false
 * is returned.
 */
bool
parse_tfeedback_decls(struct gl_context *ctx, struct gl_shader_program *prog,
                      const void *mem_ctx, unsigned num_names,
                      char **varying_names, tfeedback_decl *decls)
{
   for (unsigned i = 0; i < num_names; ++i) {
      if (!decls[i].init(ctx, prog, mem_ctx, varying_names[i]))
         return false;

      if (!decls[i].is_varying())
         continue;

      /* From GL_EXT_transform_feedback:
       *   A program will fail to link if:
       *
       *   * any two entries in the <varyings> array specify the same varying
       *     variable;
       *
       * We interpret this to mean "any two entries in the <varyings> array
       * specify the same varying variable and array index", since transform
       * feedback of arrays would be useless otherwise.
       */
      for (unsigned j = 0; j < i; ++j) {
         if (!decls[j].is_varying())
            continue;

         if (tfeedback_decl::is_same(decls[i], decls[j])) {
            linker_error(prog, "Transform feedback varying %s specified "
                         "more than once.", varying_names[i]);
            return false;
         }
      }
   }
   return true;
}


/**
 * Store transform feedback location assignments into
 * prog->LinkedTransformFeedback based on the data stored in tfeedback_decls.
 *
 * If an error occurs, the error is reported through linker_error() and false
 * is returned.
 */
bool
store_tfeedback_info(struct gl_context *ctx, struct gl_shader_program *prog,
                     unsigned num_tfeedback_decls,
                     tfeedback_decl *tfeedback_decls)
{
   bool separate_attribs_mode =
      prog->TransformFeedback.BufferMode == GL_SEPARATE_ATTRIBS;

   ralloc_free(prog->LinkedTransformFeedback.Varyings);
   ralloc_free(prog->LinkedTransformFeedback.Outputs);

   memset(&prog->LinkedTransformFeedback, 0,
          sizeof(prog->LinkedTransformFeedback));

   prog->LinkedTransformFeedback.Varyings =
      rzalloc_array(prog,
		    struct gl_transform_feedback_varying_info,
		    num_tfeedback_decls);

   unsigned num_outputs = 0;
   for (unsigned i = 0; i < num_tfeedback_decls; ++i)
      num_outputs += tfeedback_decls[i].get_num_outputs();

   prog->LinkedTransformFeedback.Outputs =
      rzalloc_array(prog,
                    struct gl_transform_feedback_output,
                    num_outputs);

   unsigned num_buffers = 0;

   if (separate_attribs_mode) {
      /* GL_SEPARATE_ATTRIBS */
      for (unsigned i = 0; i < num_tfeedback_decls; ++i) {
         if (!tfeedback_decls[i].store(ctx, prog, &prog->LinkedTransformFeedback,
                                       num_buffers, num_outputs))
            return false;

         num_buffers++;
      }
   }
   else {
      /* GL_INVERLEAVED_ATTRIBS */
      for (unsigned i = 0; i < num_tfeedback_decls; ++i) {
         if (tfeedback_decls[i].is_next_buffer_separator()) {
            num_buffers++;
            continue;
         }

         if (!tfeedback_decls[i].store(ctx, prog,
                                       &prog->LinkedTransformFeedback,
                                       num_buffers, num_outputs))
            return false;
      }
      num_buffers++;
   }

   assert(prog->LinkedTransformFeedback.NumOutputs == num_outputs);

   prog->LinkedTransformFeedback.NumBuffers = num_buffers;
   return true;
}


/**
 * Data structure recording the relationship between outputs of one shader
 * stage (the "producer") and inputs of another (the "consumer").
 */
class varying_matches
{
public:
   varying_matches(bool disable_varying_packing);
   ~varying_matches();
   void record(ir_variable *producer_var, ir_variable *consumer_var);
   unsigned assign_locations();
   void store_locations(unsigned producer_base, unsigned consumer_base) const;

private:
   /**
    * If true, this driver disables varying packing, so all varyings need to
    * be aligned on slot boundaries, and take up a number of slots equal to
    * their number of matrix columns times their array size.
    */
   const bool disable_varying_packing;

   /**
    * Enum representing the order in which varyings are packed within a
    * packing class.
    *
    * Currently we pack vec4's first, then vec2's, then scalar values, then
    * vec3's.  This order ensures that the only vectors that are at risk of
    * having to be "double parked" (split between two adjacent varying slots)
    * are the vec3's.
    */
   enum packing_order_enum {
      PACKING_ORDER_VEC4,
      PACKING_ORDER_VEC2,
      PACKING_ORDER_SCALAR,
      PACKING_ORDER_VEC3,
   };

   static unsigned compute_packing_class(ir_variable *var);
   static packing_order_enum compute_packing_order(ir_variable *var);
   static int match_comparator(const void *x_generic, const void *y_generic);

   /**
    * Structure recording the relationship between a single producer output
    * and a single consumer input.
    */
   struct match {
      /**
       * Packing class for this varying, computed by compute_packing_class().
       */
      unsigned packing_class;

      /**
       * Packing order for this varying, computed by compute_packing_order().
       */
      packing_order_enum packing_order;
      unsigned num_components;

      /**
       * The output variable in the producer stage.
       */
      ir_variable *producer_var;

      /**
       * The input variable in the consumer stage.
       */
      ir_variable *consumer_var;

      /**
       * The location which has been assigned for this varying.  This is
       * expressed in multiples of a float, with the first generic varying
       * (i.e. the one referred to by VERT_RESULT_VAR0 or FRAG_ATTRIB_VAR0)
       * represented by the value 0.
       */
      unsigned generic_location;
   } *matches;

   /**
    * The number of elements in the \c matches array that are currently in
    * use.
    */
   unsigned num_matches;

   /**
    * The number of elements that were set aside for the \c matches array when
    * it was allocated.
    */
   unsigned matches_capacity;
};


varying_matches::varying_matches(bool disable_varying_packing)
   : disable_varying_packing(disable_varying_packing)
{
   /* Note: this initial capacity is rather arbitrarily chosen to be large
    * enough for many cases without wasting an unreasonable amount of space.
    * varying_matches::record() will resize the array if there are more than
    * this number of varyings.
    */
   this->matches_capacity = 8;
   this->matches = (match *)
      malloc(sizeof(*this->matches) * this->matches_capacity);
   this->num_matches = 0;
}


varying_matches::~varying_matches()
{
   free(this->matches);
}


/**
 * Record the given producer/consumer variable pair in the list of variables
 * that should later be assigned locations.
 *
 * It is permissible for \c consumer_var to be NULL (this happens if a
 * variable is output by the producer and consumed by transform feedback, but
 * not consumed by the consumer).
 *
 * If \c producer_var has already been paired up with a consumer_var, or
 * producer_var is part of fixed pipeline functionality (and hence already has
 * a location assigned), this function has no effect.
 */
void
varying_matches::record(ir_variable *producer_var, ir_variable *consumer_var)
{
   if (!producer_var->is_unmatched_generic_inout) {
      /* Either a location already exists for this variable (since it is part
       * of fixed functionality), or it has already been recorded as part of a
       * previous match.
       */
      return;
   }

   if (this->num_matches == this->matches_capacity) {
      this->matches_capacity *= 2;
      this->matches = (match *)
         realloc(this->matches,
                 sizeof(*this->matches) * this->matches_capacity);
   }
   this->matches[this->num_matches].packing_class
      = this->compute_packing_class(producer_var);
   this->matches[this->num_matches].packing_order
      = this->compute_packing_order(producer_var);
   if (this->disable_varying_packing) {
      unsigned slots = producer_var->type->is_array()
         ? (producer_var->type->length
            * producer_var->type->fields.array->matrix_columns)
         : producer_var->type->matrix_columns;
      this->matches[this->num_matches].num_components = 4 * slots;
   } else {
      this->matches[this->num_matches].num_components
         = producer_var->type->component_slots();
   }
   this->matches[this->num_matches].producer_var = producer_var;
   this->matches[this->num_matches].consumer_var = consumer_var;
   this->num_matches++;
   producer_var->is_unmatched_generic_inout = 0;
   if (consumer_var)
      consumer_var->is_unmatched_generic_inout = 0;
}


/**
 * Choose locations for all of the variable matches that were previously
 * passed to varying_matches::record().
 */
unsigned
varying_matches::assign_locations()
{
   /* Sort varying matches into an order that makes them easy to pack. */
   qsort(this->matches, this->num_matches, sizeof(*this->matches),
         &varying_matches::match_comparator);

   unsigned generic_location = 0;

   for (unsigned i = 0; i < this->num_matches; i++) {
      /* Advance to the next slot if this varying has a different packing
       * class than the previous one, and we're not already on a slot
       * boundary.
       */
      if (i > 0 &&
          this->matches[i - 1].packing_class
          != this->matches[i].packing_class) {
         generic_location = ALIGN(generic_location, 4);
      }

      this->matches[i].generic_location = generic_location;

      generic_location += this->matches[i].num_components;
   }

   return (generic_location + 3) / 4;
}


/**
 * Update the producer and consumer shaders to reflect the locations
 * assignments that were made by varying_matches::assign_locations().
 */
void
varying_matches::store_locations(unsigned producer_base,
                                 unsigned consumer_base) const
{
   for (unsigned i = 0; i < this->num_matches; i++) {
      ir_variable *producer_var = this->matches[i].producer_var;
      ir_variable *consumer_var = this->matches[i].consumer_var;
      unsigned generic_location = this->matches[i].generic_location;
      unsigned slot = generic_location / 4;
      unsigned offset = generic_location % 4;

      producer_var->location = producer_base + slot;
      producer_var->location_frac = offset;
      if (consumer_var) {
         assert(consumer_var->location == -1);
         consumer_var->location = consumer_base + slot;
         consumer_var->location_frac = offset;
      }
   }
}


/**
 * Compute the "packing class" of the given varying.  This is an unsigned
 * integer with the property that two variables in the same packing class can
 * be safely backed into the same vec4.
 */
unsigned
varying_matches::compute_packing_class(ir_variable *var)
{
   /* Without help from the back-end, there is no way to pack together
    * variables with different interpolation types, because
    * lower_packed_varyings must choose exactly one interpolation type for
    * each packed varying it creates.
    *
    * However, we can safely pack together floats, ints, and uints, because:
    *
    * - varyings of base type "int" and "uint" must use the "flat"
    *   interpolation type, which can only occur in GLSL 1.30 and above.
    *
    * - On platforms that support GLSL 1.30 and above, lower_packed_varyings
    *   can store flat floats as ints without losing any information (using
    *   the ir_unop_bitcast_* opcodes).
    *
    * Therefore, the packing class depends only on the interpolation type.
    */
   unsigned packing_class = var->centroid ? 1 : 0;
   packing_class *= 4;
   packing_class += var->interpolation;
   return packing_class;
}


/**
 * Compute the "packing order" of the given varying.  This is a sort key we
 * use to determine when to attempt to pack the given varying relative to
 * other varyings in the same packing class.
 */
varying_matches::packing_order_enum
varying_matches::compute_packing_order(ir_variable *var)
{
   const glsl_type *element_type = var->type;

   while (element_type->base_type == GLSL_TYPE_ARRAY) {
      element_type = element_type->fields.array;
   }

   switch (element_type->component_slots() % 4) {
   case 1: return PACKING_ORDER_SCALAR;
   case 2: return PACKING_ORDER_VEC2;
   case 3: return PACKING_ORDER_VEC3;
   case 0: return PACKING_ORDER_VEC4;
   default:
      assert(!"Unexpected value of vector_elements");
      return PACKING_ORDER_VEC4;
   }
}


/**
 * Comparison function passed to qsort() to sort varyings by packing_class and
 * then by packing_order.
 */
int
varying_matches::match_comparator(const void *x_generic, const void *y_generic)
{
   const match *x = (const match *) x_generic;
   const match *y = (const match *) y_generic;

   if (x->packing_class != y->packing_class)
      return x->packing_class - y->packing_class;
   return x->packing_order - y->packing_order;
}


/**
 * Is the given variable a varying variable to be counted against the
 * limit in ctx->Const.MaxVarying?
 * This includes variables such as texcoords, colors and generic
 * varyings, but excludes variables such as gl_FrontFacing and gl_FragCoord.
 */
static bool
is_varying_var(GLenum shaderType, const ir_variable *var)
{
   /* Only fragment shaders will take a varying variable as an input */
   if (shaderType == GL_FRAGMENT_SHADER &&
       var->mode == ir_var_shader_in) {
      switch (var->location) {
      case FRAG_ATTRIB_WPOS:
      case FRAG_ATTRIB_FACE:
      case FRAG_ATTRIB_PNTC:
         return false;
      default:
         return true;
      }
   }
   return false;
}


/**
 * Assign locations for all variables that are produced in one pipeline stage
 * (the "producer") and consumed in the next stage (the "consumer").
 *
 * Variables produced by the producer may also be consumed by transform
 * feedback.
 *
 * \param num_tfeedback_decls is the number of declarations indicating
 *        variables that may be consumed by transform feedback.
 *
 * \param tfeedback_decls is a pointer to an array of tfeedback_decl objects
 *        representing the result of parsing the strings passed to
 *        glTransformFeedbackVaryings().  assign_location() will be called for
 *        each of these objects that matches one of the outputs of the
 *        producer.
 *
 * When num_tfeedback_decls is nonzero, it is permissible for the consumer to
 * be NULL.  In this case, varying locations are assigned solely based on the
 * requirements of transform feedback.
 */
bool
assign_varying_locations(struct gl_context *ctx,
			 void *mem_ctx,
			 struct gl_shader_program *prog,
			 gl_shader *producer, gl_shader *consumer,
                         unsigned num_tfeedback_decls,
                         tfeedback_decl *tfeedback_decls)
{
   /* FINISHME: Set dynamically when geometry shader support is added. */
   const unsigned producer_base = VERT_RESULT_VAR0;
   const unsigned consumer_base = FRAG_ATTRIB_VAR0;
   varying_matches matches(ctx->Const.DisableVaryingPacking);

   /* Operate in a total of three passes.
    *
    * 1. Assign locations for any matching inputs and outputs.
    *
    * 2. Mark output variables in the producer that do not have locations as
    *    not being outputs.  This lets the optimizer eliminate them.
    *
    * 3. Mark input variables in the consumer that do not have locations as
    *    not being inputs.  This lets the optimizer eliminate them.
    */

   foreach_list(node, producer->ir) {
      ir_variable *const output_var = ((ir_instruction *) node)->as_variable();

      if ((output_var == NULL) || (output_var->mode != ir_var_shader_out))
	 continue;

      ir_variable *input_var =
	 consumer ? consumer->symbols->get_variable(output_var->name) : NULL;

      if (input_var && input_var->mode != ir_var_shader_in)
         input_var = NULL;

      if (input_var) {
         matches.record(output_var, input_var);
      }
   }

   for (unsigned i = 0; i < num_tfeedback_decls; ++i) {
      if (!tfeedback_decls[i].is_varying())
         continue;

      ir_variable *output_var
         = tfeedback_decls[i].find_output_var(prog, producer);

      if (output_var == NULL)
         return false;

      if (output_var->is_unmatched_generic_inout) {
         matches.record(output_var, NULL);
      }
   }

   const unsigned slots_used = matches.assign_locations();
   matches.store_locations(producer_base, consumer_base);

   for (unsigned i = 0; i < num_tfeedback_decls; ++i) {
      if (!tfeedback_decls[i].is_varying())
         continue;

      ir_variable *output_var
         = tfeedback_decls[i].find_output_var(prog, producer);

      if (!tfeedback_decls[i].assign_location(ctx, prog, output_var))
         return false;
   }

   if (ctx->Const.DisableVaryingPacking) {
      /* Transform feedback code assumes varyings are packed, so if the driver
       * has disabled varying packing, make sure it does not support transform
       * feedback.
       */
      assert(!ctx->Extensions.EXT_transform_feedback);
   } else {
      lower_packed_varyings(mem_ctx, producer_base, slots_used,
                            ir_var_shader_out, producer);
      if (consumer) {
         lower_packed_varyings(mem_ctx, consumer_base, slots_used,
                               ir_var_shader_in, consumer);
      }
   }

   unsigned varying_vectors = 0;

   if (consumer) {
      foreach_list(node, consumer->ir) {
         ir_variable *const var = ((ir_instruction *) node)->as_variable();

         if ((var == NULL) || (var->mode != ir_var_shader_in))
            continue;

         if (var->is_unmatched_generic_inout) {
            if (prog->Version <= 120) {
               /* On page 25 (page 31 of the PDF) of the GLSL 1.20 spec:
                *
                *     Only those varying variables used (i.e. read) in
                *     the fragment shader executable must be written to
                *     by the vertex shader executable; declaring
                *     superfluous varying variables in a vertex shader is
                *     permissible.
                *
                * We interpret this text as meaning that the VS must
                * write the variable for the FS to read it.  See
                * "glsl1-varying read but not written" in piglit.
                */

               linker_error(prog, "fragment shader varying %s not written "
                            "by vertex shader\n.", var->name);
            }

            /* An 'in' variable is only really a shader input if its
             * value is written by the previous stage.
             */
            var->mode = ir_var_auto;
         } else if (is_varying_var(consumer->Type, var)) {
            /* The packing rules are used for vertex shader inputs are also
             * used for fragment shader inputs.
             */
            varying_vectors += count_attribute_slots(var->type);
         }
      }
   }

   if (ctx->API == API_OPENGLES2 || prog->IsES) {
      if (varying_vectors > ctx->Const.MaxVarying) {
         if (ctx->Const.GLSLSkipStrictMaxVaryingLimitCheck) {
            linker_warning(prog, "shader uses too many varying vectors "
                           "(%u > %u), but the driver will try to optimize "
                           "them out; this is non-portable out-of-spec "
                           "behavior\n",
                           varying_vectors, ctx->Const.MaxVarying);
         } else {
            linker_error(prog, "shader uses too many varying vectors "
                         "(%u > %u)\n",
                         varying_vectors, ctx->Const.MaxVarying);
            return false;
         }
      }
   } else {
      const unsigned float_components = varying_vectors * 4;
      if (float_components > ctx->Const.MaxVarying * 4) {
         if (ctx->Const.GLSLSkipStrictMaxVaryingLimitCheck) {
            linker_warning(prog, "shader uses too many varying components "
                           "(%u > %u), but the driver will try to optimize "
                           "them out; this is non-portable out-of-spec "
                           "behavior\n",
                           float_components, ctx->Const.MaxVarying * 4);
         } else {
            linker_error(prog, "shader uses too many varying components "
                         "(%u > %u)\n",
                         float_components, ctx->Const.MaxVarying * 4);
            return false;
         }
      }
   }

   return true;
}
