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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file linker.cpp
 * GLSL linker implementation
 *
 * Given a set of shaders that are to be linked to generate a final program,
 * there are three distinct stages.
 *
 * In the first stage shaders are partitioned into groups based on the shader
 * type.  All shaders of a particular type (e.g., vertex shaders) are linked
 * together.
 *
 *   - Undefined references in each shader are resolve to definitions in
 *     another shader.
 *   - Types and qualifiers of uniforms, outputs, and global variables defined
 *     in multiple shaders with the same name are verified to be the same.
 *   - Initializers for uniforms and global variables defined
 *     in multiple shaders with the same name are verified to be the same.
 *
 * The result, in the terminology of the GLSL spec, is a set of shader
 * executables for each processing unit.
 *
 * After the first stage is complete, a series of semantic checks are performed
 * on each of the shader executables.
 *
 *   - Each shader executable must define a \c main function.
 *   - Each vertex shader executable must write to \c gl_Position.
 *   - Each fragment shader executable must write to either \c gl_FragData or
 *     \c gl_FragColor.
 *
 * In the final stage individual shader executables are linked to create a
 * complete exectuable.
 *
 *   - Types of uniforms defined in multiple shader stages with the same name
 *     are verified to be the same.
 *   - Initializers for uniforms defined in multiple shader stages with the
 *     same name are verified to be the same.
 *   - Types and qualifiers of outputs defined in one stage are verified to
 *     be the same as the types and qualifiers of inputs defined with the same
 *     name in a later stage.
 *
 * \author Ian Romanick <ian.d.romanick@intel.com>
 */
#include <cstdlib>
#include <cstdio>
#include <cstdarg>

extern "C" {
#include <talloc.h>
}

#include "main/mtypes.h"
#include "glsl_symbol_table.h"
#include "ir.h"
#include "program.h"
#include "hash_table.h"
#include "shader_api.h"

/**
 * Visitor that determines whether or not a variable is ever written.
 */
class find_assignment_visitor : public ir_hierarchical_visitor {
public:
   find_assignment_visitor(const char *name)
      : name(name), found(false)
   {
      /* empty */
   }

   virtual ir_visitor_status visit_enter(ir_assignment *ir)
   {
      ir_variable *const var = ir->lhs->variable_referenced();

      if (strcmp(name, var->name) == 0) {
	 found = true;
	 return visit_stop;
      }

      return visit_continue_with_parent;
   }

   bool variable_found()
   {
      return found;
   }

private:
   const char *name;       /**< Find writes to a variable with this name. */
   bool found;             /**< Was a write to the variable found? */
};


void
linker_error_printf(gl_shader_program *prog, const char *fmt, ...)
{
   va_list ap;

   prog->InfoLog = talloc_strdup_append(prog->InfoLog, "error: ");
   va_start(ap, fmt);
   prog->InfoLog = talloc_vasprintf_append(prog->InfoLog, fmt, ap);
   va_end(ap);
}


void
invalidate_variable_locations(gl_shader *sh, enum ir_variable_mode mode,
			      int generic_base)
{
   foreach_list(node, sh->ir) {
      ir_variable *const var = ((ir_instruction *) node)->as_variable();

      if ((var == NULL) || (var->mode != (unsigned) mode))
	 continue;

      /* Only assign locations for generic attributes / varyings / etc.
       */
      if (var->location >= generic_base)
	  var->location = -1;
   }
}


/**
 * Determine the number of attribute slots required for a particular type
 *
 * This code is here because it implements the language rules of a specific
 * GLSL version.  Since it's a property of the language and not a property of
 * types in general, it doesn't really belong in glsl_type.
 */
unsigned
count_attribute_slots(const glsl_type *t)
{
   /* From page 31 (page 37 of the PDF) of the GLSL 1.50 spec:
    *
    *     "A scalar input counts the same amount against this limit as a vec4,
    *     so applications may want to consider packing groups of four
    *     unrelated float inputs together into a vector to better utilize the
    *     capabilities of the underlying hardware. A matrix input will use up
    *     multiple locations.  The number of locations used will equal the
    *     number of columns in the matrix."
    *
    * The spec does not explicitly say how arrays are counted.  However, it
    * should be safe to assume the total number of slots consumed by an array
    * is the number of entries in the array multiplied by the number of slots
    * consumed by a single element of the array.
    */

   if (t->is_array())
      return t->array_size() * count_attribute_slots(t->element_type());

   if (t->is_matrix())
      return t->matrix_columns;

   return 1;
}


/**
 * Verify that a vertex shader executable meets all semantic requirements
 *
 * \param shader  Vertex shader executable to be verified
 */
bool
validate_vertex_shader_executable(struct gl_shader_program *prog,
				  struct gl_shader *shader)
{
   if (shader == NULL)
      return true;

   if (!shader->symbols->get_function("main")) {
      linker_error_printf(prog, "vertex shader lacks `main'\n");
      return false;
   }

   find_assignment_visitor find("gl_Position");
   find.run(shader->ir);
   if (!find.variable_found()) {
      linker_error_printf(prog,
			  "vertex shader does not write to `gl_Position'\n");
      return false;
   }

   return true;
}


/**
 * Verify that a fragment shader executable meets all semantic requirements
 *
 * \param shader  Fragment shader executable to be verified
 */
bool
validate_fragment_shader_executable(struct gl_shader_program *prog,
				    struct gl_shader *shader)
{
   if (shader == NULL)
      return true;

   if (!shader->symbols->get_function("main")) {
      linker_error_printf(prog, "fragment shader lacks `main'\n");
      return false;
   }

   find_assignment_visitor frag_color("gl_FragColor");
   find_assignment_visitor frag_data("gl_FragData");

   frag_color.run(shader->ir);
   frag_data.run(shader->ir);

   if (frag_color.variable_found() && frag_data.variable_found()) {
      linker_error_printf(prog,  "fragment shader writes to both "
			  "`gl_FragColor' and `gl_FragData'\n");
      return false;
   }

   return true;
}


/**
 * Generate a string describing the mode of a variable
 */
static const char *
mode_string(const ir_variable *var)
{
   switch (var->mode) {
   case ir_var_auto:
      return (var->read_only) ? "global constant" : "global variable";

   case ir_var_uniform: return "uniform";
   case ir_var_in:      return "shader input";
   case ir_var_out:     return "shader output";
   case ir_var_inout:   return "shader inout";
   default:
      assert(!"Should not get here.");
      return "invalid variable";
   }
}


/**
 * Perform validation of global variables used across multiple shaders
 */
bool
cross_validate_globals(struct gl_shader_program *prog,
		       struct gl_shader **shader_list,
		       unsigned num_shaders,
		       bool uniforms_only)
{
   /* Examine all of the uniforms in all of the shaders and cross validate
    * them.
    */
   glsl_symbol_table variables;
   for (unsigned i = 0; i < num_shaders; i++) {
      foreach_list(node, shader_list[i]->ir) {
	 ir_variable *const var = ((ir_instruction *) node)->as_variable();

	 if (var == NULL)
	    continue;

	 if (uniforms_only && (var->mode != ir_var_uniform))
	    continue;

	 /* If a global with this name has already been seen, verify that the
	  * new instance has the same type.  In addition, if the globals have
	  * initializers, the values of the initializers must be the same.
	  */
	 ir_variable *const existing = variables.get_variable(var->name);
	 if (existing != NULL) {
	    if (var->type != existing->type) {
	       linker_error_printf(prog, "%s `%s' declared as type "
				   "`%s' and type `%s'\n",
				   mode_string(var),
				   var->name, var->type->name,
				   existing->type->name);
	       return false;
	    }

	    /* FINISHME: Handle non-constant initializers.
	     */
	    if (var->constant_value != NULL) {
	       if (existing->constant_value != NULL) {
		  if (!var->constant_value->has_value(existing->constant_value)) {
		     linker_error_printf(prog, "initializers for %s "
					 "`%s' have differing values\n",
					 mode_string(var), var->name);
		     return false;
		  }
	       } else
		  /* If the first-seen instance of a particular uniform did not
		   * have an initializer but a later instance does, copy the
		   * initializer to the version stored in the symbol table.
		   */
		  existing->constant_value = var->constant_value->clone(NULL);
	    }
	 } else
	    variables.add_variable(var->name, var);
      }
   }

   return true;
}


/**
 * Perform validation of uniforms used across multiple shader stages
 */
bool
cross_validate_uniforms(struct gl_shader_program *prog)
{
   return cross_validate_globals(prog, prog->_LinkedShaders,
				 prog->_NumLinkedShaders, true);
}


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

      /* FINISHME: For geometry shaders, this should also look for inout
       * FINISHME: variables.
       */
      if ((var == NULL) || (var->mode != ir_var_out))
	 continue;

      parameters.add_variable(var->name, var);
   }


   /* Find all shader inputs in the "consumer" stage.  Any variables that have
    * matching outputs already in the symbol table must have the same type and
    * qualifiers.
    */
   foreach_list(node, consumer->ir) {
      ir_variable *const input = ((ir_instruction *) node)->as_variable();

      /* FINISHME: For geometry shaders, this should also look for inout
       * FINISHME: variables.
       */
      if ((input == NULL) || (input->mode != ir_var_in))
	 continue;

      ir_variable *const output = parameters.get_variable(input->name);
      if (output != NULL) {
	 /* Check that the types match between stages.
	  */
	 if (input->type != output->type) {
	    linker_error_printf(prog,
				"%s shader output `%s' delcared as "
				"type `%s', but %s shader input declared "
				"as type `%s'\n",
				producer_stage, output->name,
				output->type->name,
				consumer_stage, input->type->name);
	    return false;
	 }

	 /* Check that all of the qualifiers match between stages.
	  */
	 if (input->centroid != output->centroid) {
	    linker_error_printf(prog,
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
	    linker_error_printf(prog,
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
	    linker_error_printf(prog,
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
 * Populates a shaders symbol table with all global declarations
 */
static void
populate_symbol_table(gl_shader *sh)
{
   sh->symbols = new(sh) glsl_symbol_table;

   foreach_list(node, sh->ir) {
      ir_instruction *const inst = (ir_instruction *) node;
      ir_variable *var;
      ir_function *func;

      if ((func = inst->as_function()) != NULL) {
	 sh->symbols->add_function(func->name, func);
      } else if ((var = inst->as_variable()) != NULL) {
	 sh->symbols->add_variable(var->name, var);
      }
   }
}


/**
 * Remap variables referenced in an instruction tree
 *
 * This is used when instruction trees are cloned from one shader and placed in
 * another.  These trees will contain references to \c ir_variable nodes that
 * do not exist in the target shader.  This function finds these \c ir_variable
 * references and replaces the references with matching variables in the target
 * shader.
 *
 * If there is no matching variable in the target shader, a clone of the
 * \c ir_variable is made and added to the target shader.  The new variable is
 * added to \b both the instruction stream and the symbol table.
 *
 * \param inst         IR tree that is to be processed.
 * \param symbols      Symbol table containing global scope symbols in the
 *                     linked shader.
 * \param instructions Instruction stream where new variable declarations
 *                     should be added.
 */
void
remap_variables(ir_instruction *inst, glsl_symbol_table *symbols,
		exec_list *instructions)
{
   class remap_visitor : public ir_hierarchical_visitor {
   public:
      remap_visitor(glsl_symbol_table *symbols, exec_list *instructions)
      {
	 this->symbols = symbols;
	 this->instructions = instructions;
      }

      virtual ir_visitor_status visit(ir_dereference_variable *ir)
      {
	 ir_variable *const existing =
	    this->symbols->get_variable(ir->var->name);
	 if (existing != NULL)
	    ir->var = existing;
	 else {
	    ir_variable *copy = ir->var->clone(NULL);

	    this->symbols->add_variable(copy->name, copy);
	    this->instructions->push_head(copy);
	 }

	 return visit_continue;
      }

   private:
      glsl_symbol_table *symbols;
      exec_list *instructions;
   };

   remap_visitor v(symbols, instructions);

   inst->accept(&v);
}


/**
 * Move non-declarations from one instruction stream to another
 *
 * The intended usage pattern of this function is to pass the pointer to the
 * head sentinal of a list (i.e., a pointer to the list cast to an \c exec_node
 * pointer) for \c last and \c false for \c make_copies on the first
 * call.  Successive calls pass the return value of the previous call for
 * \c last and \c true for \c make_copies.
 *
 * \param instructions Source instruction stream
 * \param last         Instruction after which new instructions should be
 *                     inserted in the target instruction stream
 * \param make_copies  Flag selecting whether instructions in \c instructions
 *                     should be copied (via \c ir_instruction::clone) into the
 *                     target list or moved.
 *
 * \return
 * The new "last" instruction in the target instruction stream.  This pointer
 * is suitable for use as the \c last parameter of a later call to this
 * function.
 */
exec_node *
move_non_declarations(exec_list *instructions, exec_node *last,
		      bool make_copies, gl_shader *target)
{
   foreach_list_safe(node, instructions) {
      ir_instruction *inst = (ir_instruction *) node;

      if (inst->as_variable() || inst->as_function())
	 continue;

      assert(inst->as_assignment());

      if (make_copies) {
	 inst = inst->clone(NULL);
	 remap_variables(inst, target->symbols, target->ir);
      } else {
	 inst->remove();
      }

      last->insert_after(inst);
      last = inst;
   }

   return last;
}

/**
 * Get the function signature for main from a shader
 */
static ir_function_signature *
get_main_function_signature(gl_shader *sh)
{
   ir_function *const f = sh->symbols->get_function("main");
   if (f != NULL) {
      exec_list void_parameters;

      /* Look for the 'void main()' signature and ensure that it's defined.
       * This keeps the linker from accidentally pick a shader that just
       * contains a prototype for main.
       *
       * We don't have to check for multiple definitions of main (in multiple
       * shaders) because that would have already been caught above.
       */
      ir_function_signature *sig = f->matching_signature(&void_parameters);
      if ((sig != NULL) && sig->is_defined) {
	 return sig;
      }
   }

   return NULL;
}


/**
 * Combine a group of shaders for a single stage to generate a linked shader
 *
 * \note
 * If this function is supplied a single shader, it is cloned, and the new
 * shader is returned.
 */
static struct gl_shader *
link_intrastage_shaders(struct gl_shader_program *prog,
			struct gl_shader **shader_list,
			unsigned num_shaders)
{
   /* Check that global variables defined in multiple shaders are consistent.
    */
   if (!cross_validate_globals(prog, shader_list, num_shaders, false))
      return NULL;

   /* Check that there is only a single definition of each function signature
    * across all shaders.
    */
   for (unsigned i = 0; i < (num_shaders - 1); i++) {
      foreach_list(node, shader_list[i]->ir) {
	 ir_function *const f = ((ir_instruction *) node)->as_function();

	 if (f == NULL)
	    continue;

	 for (unsigned j = i + 1; j < num_shaders; j++) {
	    ir_function *const other =
	       shader_list[j]->symbols->get_function(f->name);

	    /* If the other shader has no function (and therefore no function
	     * signatures) with the same name, skip to the next shader.
	     */
	    if (other == NULL)
	       continue;

	    foreach_iter (exec_list_iterator, iter, *f) {
	       ir_function_signature *sig =
		  (ir_function_signature *) iter.get();

	       if (!sig->is_defined || sig->is_built_in)
		  continue;

	       ir_function_signature *other_sig =
		  other->exact_matching_signature(& sig->parameters);

	       if ((other_sig != NULL) && other_sig->is_defined
		   && !other_sig->is_built_in) {
		  linker_error_printf(prog,
				      "function `%s' is multiply defined",
				      f->name);
		  return NULL;
	       }
	    }
	 }
      }
   }

   /* Find the shader that defines main, and make a clone of it.
    *
    * Starting with the clone, search for undefined references.  If one is
    * found, find the shader that defines it.  Clone the reference and add
    * it to the shader.  Repeat until there are no undefined references or
    * until a reference cannot be resolved.
    */
   gl_shader *main = NULL;
   for (unsigned i = 0; i < num_shaders; i++) {
      if (get_main_function_signature(shader_list[i]) != NULL) {
	 main = shader_list[i];
	 break;
      }
   }

   if (main == NULL) {
      linker_error_printf(prog, "%s shader lacks `main'\n",
			  (shader_list[0]->Type == GL_VERTEX_SHADER)
			  ? "vertex" : "fragment");
      return NULL;
   }

   gl_shader *const linked = _mesa_new_shader(NULL, 0, main->Type);
   linked->ir = new(linked) exec_list;
   clone_ir_list(linked->ir, main->ir);

   populate_symbol_table(linked);

   /* The a pointer to the main function in the final linked shader (i.e., the
    * copy of the original shader that contained the main function).
    */
   ir_function_signature *const main_sig = get_main_function_signature(linked);

   /* Move any instructions other than variable declarations or function
    * declarations into main.
    */
   exec_node *insertion_point =
      move_non_declarations(linked->ir, (exec_node *) &main_sig->body, false,
			    linked);

   for (unsigned i = 0; i < num_shaders; i++) {
      if (shader_list[i] == main)
	 continue;

      insertion_point = move_non_declarations(shader_list[i]->ir,
					      insertion_point, true, linked);
   }

   /* Resolve initializers for global variables in the linked shader.
    */

   return linked;
}


struct uniform_node {
   exec_node link;
   struct gl_uniform *u;
   unsigned slots;
};

void
assign_uniform_locations(struct gl_shader_program *prog)
{
   /* */
   exec_list uniforms;
   unsigned total_uniforms = 0;
   hash_table *ht = hash_table_ctor(32, hash_table_string_hash,
				    hash_table_string_compare);

   for (unsigned i = 0; i < prog->_NumLinkedShaders; i++) {
      unsigned next_position = 0;

      foreach_list(node, prog->_LinkedShaders[i]->ir) {
	 ir_variable *const var = ((ir_instruction *) node)->as_variable();

	 if ((var == NULL) || (var->mode != ir_var_uniform))
	    continue;

	 const unsigned vec4_slots = (var->component_slots() + 3) / 4;
	 assert(vec4_slots != 0);

	 uniform_node *n = (uniform_node *) hash_table_find(ht, var->name);
	 if (n == NULL) {
	    n = (uniform_node *) calloc(1, sizeof(struct uniform_node));
	    n->u = (gl_uniform *) calloc(vec4_slots, sizeof(struct gl_uniform));
	    n->slots = vec4_slots;

	    n->u[0].Name = strdup(var->name);
	    for (unsigned j = 1; j < vec4_slots; j++)
	       n->u[j].Name = n->u[0].Name;

	    hash_table_insert(ht, n, n->u[0].Name);
	    uniforms.push_tail(& n->link);
	    total_uniforms += vec4_slots;
	 }

	 if (var->constant_value != NULL)
	    for (unsigned j = 0; j < vec4_slots; j++)
	       n->u[j].Initialized = true;

	 var->location = next_position;

	 for (unsigned j = 0; j < vec4_slots; j++) {
	    switch (prog->_LinkedShaders[i]->Type) {
	    case GL_VERTEX_SHADER:
	       n->u[j].VertPos = next_position;
	       break;
	    case GL_FRAGMENT_SHADER:
	       n->u[j].FragPos = next_position;
	       break;
	    case GL_GEOMETRY_SHADER:
	       /* FINISHME: Support geometry shaders. */
	       assert(prog->_LinkedShaders[i]->Type != GL_GEOMETRY_SHADER);
	       break;
	    }

	    next_position++;
	 }
      }
   }

   gl_uniform_list *ul = (gl_uniform_list *)
      calloc(1, sizeof(gl_uniform_list));

   ul->Size = total_uniforms;
   ul->NumUniforms = total_uniforms;
   ul->Uniforms = (gl_uniform *) calloc(total_uniforms, sizeof(gl_uniform));

   unsigned idx = 0;
   uniform_node *next;
   for (uniform_node *node = (uniform_node *) uniforms.head
	   ; node->link.next != NULL
	   ; node = next) {
      next = (uniform_node *) node->link.next;

      node->link.remove();
      memcpy(&ul->Uniforms[idx], node->u, sizeof(gl_uniform) * node->slots);
      idx += node->slots;

      free(node->u);
      free(node);
   }

   hash_table_dtor(ht);

   prog->Uniforms = ul;
}


/**
 * Find a contiguous set of available bits in a bitmask
 *
 * \param used_mask     Bits representing used (1) and unused (0) locations
 * \param needed_count  Number of contiguous bits needed.
 *
 * \return
 * Base location of the available bits on success or -1 on failure.
 */
int
find_available_slots(unsigned used_mask, unsigned needed_count)
{
   unsigned needed_mask = (1 << needed_count) - 1;
   const int max_bit_to_test = (8 * sizeof(used_mask)) - needed_count;

   /* The comparison to 32 is redundant, but without it GCC emits "warning:
    * cannot optimize possibly infinite loops" for the loop below.
    */
   if ((needed_count == 0) || (max_bit_to_test < 0) || (max_bit_to_test > 32))
      return -1;

   for (int i = 0; i <= max_bit_to_test; i++) {
      if ((needed_mask & ~used_mask) == needed_mask)
	 return i;

      needed_mask <<= 1;
   }

   return -1;
}


bool
assign_attribute_locations(gl_shader_program *prog, unsigned max_attribute_index)
{
   /* Mark invalid attribute locations as being used.
    */
   unsigned used_locations = (max_attribute_index >= 32)
      ? ~0 : ~((1 << max_attribute_index) - 1);

   gl_shader *const sh = prog->_LinkedShaders[0];
   assert(sh->Type == GL_VERTEX_SHADER);

   /* Operate in a total of four passes.
    *
    * 1. Invalidate the location assignments for all vertex shader inputs.
    *
    * 2. Assign locations for inputs that have user-defined (via
    *    glBindVertexAttribLocation) locatoins.
    *
    * 3. Sort the attributes without assigned locations by number of slots
    *    required in decreasing order.  Fragmentation caused by attribute
    *    locations assigned by the application may prevent large attributes
    *    from having enough contiguous space.
    *
    * 4. Assign locations to any inputs without assigned locations.
    */

   invalidate_variable_locations(sh, ir_var_in, VERT_ATTRIB_GENERIC0);

   if (prog->Attributes != NULL) {
      for (unsigned i = 0; i < prog->Attributes->NumParameters; i++) {
	 ir_variable *const var =
	    sh->symbols->get_variable(prog->Attributes->Parameters[i].Name);

	 /* Note: attributes that occupy multiple slots, such as arrays or
	  * matrices, may appear in the attrib array multiple times.
	  */
	 if ((var == NULL) || (var->location != -1))
	    continue;

	 /* From page 61 of the OpenGL 4.0 spec:
	  *
	  *     "LinkProgram will fail if the attribute bindings assigned by
	  *     BindAttribLocation do not leave not enough space to assign a
	  *     location for an active matrix attribute or an active attribute
	  *     array, both of which require multiple contiguous generic
	  *     attributes."
	  *
	  * Previous versions of the spec contain similar language but omit the
	  * bit about attribute arrays.
	  *
	  * Page 61 of the OpenGL 4.0 spec also says:
	  *
	  *     "It is possible for an application to bind more than one
	  *     attribute name to the same location. This is referred to as
	  *     aliasing. This will only work if only one of the aliased
	  *     attributes is active in the executable program, or if no path
	  *     through the shader consumes more than one attribute of a set
	  *     of attributes aliased to the same location. A link error can
	  *     occur if the linker determines that every path through the
	  *     shader consumes multiple aliased attributes, but
	  *     implementations are not required to generate an error in this
	  *     case."
	  *
	  * These two paragraphs are either somewhat contradictory, or I don't
	  * fully understand one or both of them.
	  */
	 /* FINISHME: The code as currently written does not support attribute
	  * FINISHME: location aliasing (see comment above).
	  */
	 const int attr = prog->Attributes->Parameters[i].StateIndexes[0];
	 const unsigned slots = count_attribute_slots(var->type);

	 /* Mask representing the contiguous slots that will be used by this
	  * attribute.
	  */
	 const unsigned use_mask = (1 << slots) - 1;

	 /* Generate a link error if the set of bits requested for this
	  * attribute overlaps any previously allocated bits.
	  */
	 if ((~(use_mask << attr) & used_locations) != used_locations) {
	    linker_error_printf(prog,
				"insufficient contiguous attribute locations "
				"available for vertex shader input `%s'",
				var->name);
	    return false;
	 }

	 var->location = VERT_ATTRIB_GENERIC0 + attr;
	 used_locations |= (use_mask << attr);
      }
   }

   /* Temporary storage for the set of attributes that need locations assigned.
    */
   struct temp_attr {
      unsigned slots;
      ir_variable *var;

      /* Used below in the call to qsort. */
      static int compare(const void *a, const void *b)
      {
	 const temp_attr *const l = (const temp_attr *) a;
	 const temp_attr *const r = (const temp_attr *) b;

	 /* Reversed because we want a descending order sort below. */
	 return r->slots - l->slots;
      }
   } to_assign[16];

   unsigned num_attr = 0;

   foreach_list(node, sh->ir) {
      ir_variable *const var = ((ir_instruction *) node)->as_variable();

      if ((var == NULL) || (var->mode != ir_var_in))
	 continue;

      /* The location was explicitly assigned, nothing to do here.
       */
      if (var->location != -1)
	 continue;

      to_assign[num_attr].slots = count_attribute_slots(var->type);
      to_assign[num_attr].var = var;
      num_attr++;
   }

   /* If all of the attributes were assigned locations by the application (or
    * are built-in attributes with fixed locations), return early.  This should
    * be the common case.
    */
   if (num_attr == 0)
      return true;

   qsort(to_assign, num_attr, sizeof(to_assign[0]), temp_attr::compare);

   /* VERT_ATTRIB_GENERIC0 is a psdueo-alias for VERT_ATTRIB_POS.  It can only
    * be explicitly assigned by via glBindAttribLocation.  Mark it as reserved
    * to prevent it from being automatically allocated below.
    */
   used_locations |= (1 << 0);

   for (unsigned i = 0; i < num_attr; i++) {
      /* Mask representing the contiguous slots that will be used by this
       * attribute.
       */
      const unsigned use_mask = (1 << to_assign[i].slots) - 1;

      int location = find_available_slots(used_locations, to_assign[i].slots);

      if (location < 0) {
	 linker_error_printf(prog,
			     "insufficient contiguous attribute locations "
			     "available for vertex shader input `%s'",
			     to_assign[i].var->name);
	 return false;
      }

      to_assign[i].var->location = VERT_ATTRIB_GENERIC0 + location;
      used_locations |= (use_mask << location);
   }

   return true;
}


void
assign_varying_locations(gl_shader *producer, gl_shader *consumer)
{
   /* FINISHME: Set dynamically when geometry shader support is added. */
   unsigned output_index = VERT_RESULT_VAR0;
   unsigned input_index = FRAG_ATTRIB_VAR0;

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

   invalidate_variable_locations(producer, ir_var_out, VERT_RESULT_VAR0);
   invalidate_variable_locations(consumer, ir_var_in, FRAG_ATTRIB_VAR0);

   foreach_list(node, producer->ir) {
      ir_variable *const output_var = ((ir_instruction *) node)->as_variable();

      if ((output_var == NULL) || (output_var->mode != ir_var_out)
	  || (output_var->location != -1))
	 continue;

      ir_variable *const input_var =
	 consumer->symbols->get_variable(output_var->name);

      if ((input_var == NULL) || (input_var->mode != ir_var_in))
	 continue;

      assert(input_var->location == -1);

      /* FINISHME: Location assignment will need some changes when arrays,
       * FINISHME: matrices, and structures are allowed as shader inputs /
       * FINISHME: outputs.
       */
      output_var->location = output_index;
      input_var->location = input_index;

      output_index++;
      input_index++;
   }

   foreach_list(node, producer->ir) {
      ir_variable *const var = ((ir_instruction *) node)->as_variable();

      if ((var == NULL) || (var->mode != ir_var_out))
	 continue;

      /* An 'out' variable is only really a shader output if its value is read
       * by the following stage.
       */
      if (var->location == -1) {
	 var->shader_out = false;
	 var->mode = ir_var_auto;
      }
   }

   foreach_list(node, consumer->ir) {
      ir_variable *const var = ((ir_instruction *) node)->as_variable();

      if ((var == NULL) || (var->mode != ir_var_in))
	 continue;

      /* An 'in' variable is only really a shader input if its value is written
       * by the previous stage.
       */
      var->shader_in = (var->location != -1);
   }
}


void
link_shaders(struct gl_shader_program *prog)
{
   prog->LinkStatus = false;
   prog->Validated = false;
   prog->_Used = false;

   if (prog->InfoLog != NULL)
      talloc_free(prog->InfoLog);

   prog->InfoLog = talloc_strdup(NULL, "");

   /* Separate the shaders into groups based on their type.
    */
   struct gl_shader **vert_shader_list;
   unsigned num_vert_shaders = 0;
   struct gl_shader **frag_shader_list;
   unsigned num_frag_shaders = 0;

   vert_shader_list = (struct gl_shader **)
      calloc(2 * prog->NumShaders, sizeof(struct gl_shader *));
   frag_shader_list =  &vert_shader_list[prog->NumShaders];

   for (unsigned i = 0; i < prog->NumShaders; i++) {
      switch (prog->Shaders[i]->Type) {
      case GL_VERTEX_SHADER:
	 vert_shader_list[num_vert_shaders] = prog->Shaders[i];
	 num_vert_shaders++;
	 break;
      case GL_FRAGMENT_SHADER:
	 frag_shader_list[num_frag_shaders] = prog->Shaders[i];
	 num_frag_shaders++;
	 break;
      case GL_GEOMETRY_SHADER:
	 /* FINISHME: Support geometry shaders. */
	 assert(prog->Shaders[i]->Type != GL_GEOMETRY_SHADER);
	 break;
      }
   }

   /* FINISHME: Implement intra-stage linking. */
   prog->_NumLinkedShaders = 0;
   if (num_vert_shaders > 0) {
      gl_shader *const sh =
	 link_intrastage_shaders(prog, vert_shader_list, num_vert_shaders);

      if (sh == NULL)
	 goto done;

      if (!validate_vertex_shader_executable(prog, sh))
	  goto done;

      prog->_LinkedShaders[prog->_NumLinkedShaders] = sh;
      prog->_NumLinkedShaders++;
   }

   if (num_frag_shaders > 0) {
      gl_shader *const sh =
	 link_intrastage_shaders(prog, frag_shader_list, num_frag_shaders);

      if (sh == NULL)
	 goto done;

      if (!validate_fragment_shader_executable(prog, sh))
	  goto done;

      prog->_LinkedShaders[prog->_NumLinkedShaders] = sh;
      prog->_NumLinkedShaders++;
   }

   /* Here begins the inter-stage linking phase.  Some initial validation is
    * performed, then locations are assigned for uniforms, attributes, and
    * varyings.
    */
   if (cross_validate_uniforms(prog)) {
      /* Validate the inputs of each stage with the output of the preceeding
       * stage.
       */
      for (unsigned i = 1; i < prog->_NumLinkedShaders; i++) {
	 if (!cross_validate_outputs_to_inputs(prog,
					       prog->_LinkedShaders[i - 1],
					       prog->_LinkedShaders[i]))
	    goto done;
      }

      prog->LinkStatus = true;
   }

   /* FINISHME: Perform whole-program optimization here. */

   assign_uniform_locations(prog);

   if (prog->_LinkedShaders[0]->Type == GL_VERTEX_SHADER)
      /* FINISHME: The value of the max_attribute_index parameter is
       * FINISHME: implementation dependent based on the value of
       * FINISHME: GL_MAX_VERTEX_ATTRIBS.  GL_MAX_VERTEX_ATTRIBS must be
       * FINISHME: at least 16, so hardcode 16 for now.
       */
      if (!assign_attribute_locations(prog, 16))
	 goto done;

   for (unsigned i = 1; i < prog->_NumLinkedShaders; i++)
      assign_varying_locations(prog->_LinkedShaders[i - 1],
			       prog->_LinkedShaders[i]);

   /* FINISHME: Assign fragment shader output locations. */

done:
   free(vert_shader_list);
}
