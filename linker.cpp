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

#include "glsl_symbol_table.h"
#include "glsl_parser_extras.h"
#include "ir.h"
#include "ir_optimization.h"
#include "program.h"
#include "hash_table.h"

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


/**
 * Verify that a vertex shader executable meets all semantic requirements
 *
 * \param shader  Vertex shader executable to be verified
 */
bool
validate_vertex_shader_executable(struct glsl_shader *shader)
{
   if (shader == NULL)
      return true;

   if (!shader->symbols->get_function("main")) {
      printf("error: vertex shader lacks `main'\n");
      return false;
   }

   find_assignment_visitor find("gl_Position");
   find.run(&shader->ir);
   if (!find.variable_found()) {
      printf("error: vertex shader does not write to `gl_Position'\n");
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
validate_fragment_shader_executable(struct glsl_shader *shader)
{
   if (shader == NULL)
      return true;

   if (!shader->symbols->get_function("main")) {
      printf("error: fragment shader lacks `main'\n");
      return false;
   }

   find_assignment_visitor frag_color("gl_FragColor");
   find_assignment_visitor frag_data("gl_FragData");

   frag_color.run(&shader->ir);
   frag_data.run(&shader->ir);

   if (!frag_color.variable_found() && !frag_data.variable_found()) {
      printf("error: fragment shader does not write to `gl_FragColor' or "
	     "`gl_FragData'\n");
      return false;
   }

   if (frag_color.variable_found() && frag_data.variable_found()) {
      printf("error: fragment shader write to both `gl_FragColor' and "
	     "`gl_FragData'\n");
      return false;
   }

   return true;
}


/**
 * Perform validation of uniforms used across multiple shader stages
 */
bool
cross_validate_uniforms(struct glsl_shader **shaders, unsigned num_shaders)
{
   /* Examine all of the uniforms in all of the shaders and cross validate
    * them.
    */
   glsl_symbol_table uniforms;
   for (unsigned i = 0; i < num_shaders; i++) {
      foreach_list(node, &shaders[i]->ir) {
	 ir_variable *const var = ((ir_instruction *) node)->as_variable();

	 if ((var == NULL) || (var->mode != ir_var_uniform))
	    continue;

	 /* If a uniform with this name has already been seen, verify that the
	  * new instance has the same type.  In addition, if the uniforms have
	  * initializers, the values of the initializers must be the same.
	  */
	 ir_variable *const existing = uniforms.get_variable(var->name);
	 if (existing != NULL) {
	    if (var->type != existing->type) {
	       printf("error: uniform `%s' declared as type `%s' and "
		      "type `%s'\n",
		      var->name, var->type->name, existing->type->name);
	       return false;
	    }

	    if (var->constant_value != NULL) {
	       if (existing->constant_value != NULL) {
		  if (!var->constant_value->has_value(existing->constant_value)) {
		     printf("error: initializers for uniform `%s' have "
			    "differing values\n",
			    var->name);
		     return false;
		  }
	       } else
		  /* If the first-seen instance of a particular uniform did not
		   * have an initializer but a later instance does, copy the
		   * initializer to the version stored in the symbol table.
		   */
		  existing->constant_value = var->constant_value->clone();
	    }
	 } else
	    uniforms.add_variable(var->name, var);
      }
   }

   return true;
}


/**
 * Validate that outputs from one stage match inputs of another
 */
bool
cross_validate_outputs_to_inputs(glsl_shader *producer, glsl_shader *consumer)
{
   glsl_symbol_table parameters;
   /* FINISHME: Figure these out dynamically. */
   const char *const producer_stage = "vertex";
   const char *const consumer_stage = "fragment";

   /* Find all shader outputs in the "producer" stage.
    */
   foreach_list(node, &producer->ir) {
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
   foreach_list(node, &consumer->ir) {
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
	    printf("error: %s shader output `%s' delcared as type `%s', but "
		   "%s shader input declared as type `%s'\n",
		   producer_stage, output->name, output->type->name,
		   consumer_stage, input->type->name);
	    return false;
	 }

	 /* Check that all of the qualifiers match between stages.
	  */
	 if (input->centroid != output->centroid) {
	    printf("error: %s shader output `%s' %s centroid qualifier, but "
		   "%s shader input %s centroid qualifier\n",
		   producer_stage,
		   output->name,
		   (output->centroid) ? "has" : "lacks",
		   consumer_stage,
		   (input->centroid) ? "has" : "lacks");
	    return false;
	 }

	 if (input->invariant != output->invariant) {
	    printf("error: %s shader output `%s' %s invariant qualifier, but "
		   "%s shader input %s invariant qualifier\n",
		   producer_stage,
		   output->name,
		   (output->invariant) ? "has" : "lacks",
		   consumer_stage,
		   (input->invariant) ? "has" : "lacks");
	    return false;
	 }

	 if (input->interpolation != output->interpolation) {
	    printf("error: %s shader output `%s' specifies %s interpolation "
		   "qualifier, "
		   "but %s shader input specifies %s interpolation "
		   "qualifier\n",
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


struct uniform_node {
   exec_node link;
   struct gl_uniform *u;
   unsigned slots;
};

void
assign_uniform_locations(struct glsl_program *prog)
{
   /* */
   exec_list uniforms;
   unsigned total_uniforms = 0;
   hash_table *ht = hash_table_ctor(32, hash_table_string_hash,
				    hash_table_string_compare);

   for (unsigned i = 0; i < prog->_NumLinkedShaders; i++) {
      unsigned next_position = 0;

      foreach_list(node, &prog->_LinkedShaders[i]->ir) {
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


void
link_shaders(struct glsl_program *prog)
{
   prog->LinkStatus = false;
   prog->Validated = false;
   prog->_Used = false;

   /* Separate the shaders into groups based on their type.
    */
   struct glsl_shader **vert_shader_list;
   unsigned num_vert_shaders = 0;
   struct glsl_shader **frag_shader_list;
   unsigned num_frag_shaders = 0;

   vert_shader_list = (struct glsl_shader **)
      calloc(2 * prog->NumShaders, sizeof(struct glsl_shader *));
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
   assert(num_vert_shaders <= 1);
   assert(num_frag_shaders <= 1);

   /* Verify that each of the per-target executables is valid.
    */
   if (!validate_vertex_shader_executable(vert_shader_list[0])
       || !validate_fragment_shader_executable(frag_shader_list[0]))
      goto done;


   /* FINISHME: Perform inter-stage linking. */
   prog->_LinkedShaders = (struct glsl_shader **)
      calloc(2, sizeof(struct glsl_shader *));
   prog->_NumLinkedShaders = 0;

   if (num_vert_shaders > 0) {
      prog->_LinkedShaders[prog->_NumLinkedShaders] = vert_shader_list[0];
      prog->_NumLinkedShaders++;
   }

   if (num_frag_shaders > 0) {
      prog->_LinkedShaders[prog->_NumLinkedShaders] = frag_shader_list[0];
      prog->_NumLinkedShaders++;
   }

   if (cross_validate_uniforms(prog->_LinkedShaders, prog->_NumLinkedShaders)) {
      /* Validate the inputs of each stage with the output of the preceeding
       * stage.
       */
      for (unsigned i = 1; i < prog->_NumLinkedShaders; i++) {
	 if (!cross_validate_outputs_to_inputs(prog->_LinkedShaders[i - 1],
					       prog->_LinkedShaders[i]))
	    goto done;
      }

      prog->LinkStatus = true;
   }

   /* FINISHME: Perform whole-program optimization here. */

   assign_uniform_locations(prog);

   /* FINISHME: Assign vertex shader input locations. */

   /* FINISHME: Assign vertex shader output / fragment shader input
    * FINISHME: locations.
    */

   /* FINISHME: Assign fragment shader output locations. */

   /* FINISHME: Generate code here. */

done:
   free(vert_shader_list);
}
