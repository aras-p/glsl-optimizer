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
#include "program.h"

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
      malloc(sizeof(struct glsl_shader *) * 2 * prog->NumShaders);
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

   prog->LinkStatus = true;

done:
   free(vert_shader_list);
}
