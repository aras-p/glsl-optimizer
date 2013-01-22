/* -*- c++ -*- */
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

#pragma once
#ifndef GLSL_LINKER_H
#define GLSL_LINKER_H

extern bool
link_function_calls(gl_shader_program *prog, gl_shader *main,
		    gl_shader **shader_list, unsigned num_shaders);

extern void
link_invalidate_variable_locations(gl_shader *sh, enum ir_variable_mode mode,
				   int generic_base);

extern void
link_assign_uniform_locations(struct gl_shader_program *prog);

extern void
link_set_uniform_initializers(struct gl_shader_program *prog);

extern int
link_cross_validate_uniform_block(void *mem_ctx,
				  struct gl_uniform_block **linked_blocks,
				  unsigned int *num_linked_blocks,
				  struct gl_uniform_block *new_block);

void
link_assign_uniform_block_offsets(struct gl_shader *shader);

extern bool
link_uniform_blocks_are_compatible(const gl_uniform_block *a,
				   const gl_uniform_block *b);

extern int
link_uniform_blocks(void *mem_ctx,
                    struct gl_shader_program *prog,
                    struct gl_shader **shader_list,
                    unsigned num_shaders,
                    struct gl_uniform_block **blocks_ret);

/**
 * Class for processing all of the leaf fields of an uniform
 *
 * Leaves are, roughly speaking, the parts of the uniform that the application
 * could query with \c glGetUniformLocation (or that could be returned by
 * \c glGetActiveUniforms).
 *
 * Classes my derive from this class to implement specific functionality.
 * This class only provides the mechanism to iterate over the leaves.  Derived
 * classes must implement \c ::visit_field and may override \c ::process.
 */
class uniform_field_visitor {
public:
   /**
    * Begin processing a uniform
    *
    * Classes that overload this function should call \c ::process from the
    * base class to start the recursive processing of the uniform.
    *
    * \param var  The uniform variable that is to be processed
    *
    * Calls \c ::visit_field for each leaf of the uniform.
    *
    * \warning
    * This entry should only be used with uniform blocks in cases where the
    * row / column ordering of matrices in the block does not matter.  For
    * example, enumerating the names of members of the block, but not for
    * determining the offsets of members.
    */
   void process(ir_variable *var);

   /**
    * Begin processing a uniform of a structured type.
    *
    * This flavor of \c process should be used to handle structured types
    * (i.e., structures, interfaces, or arrays there of) that need special
    * name handling.  A common usage is to handle cases where the block name
    * (instead of the instance name) is used for an interface block.
    *
    * \param type  Type that is to be processed, associated with \c name
    * \param name  Base name of the structured uniform being processed
    *
    * \note
    * \c type must be \c GLSL_TYPE_RECORD, \c GLSL_TYPE_INTERFACE, or an array
    * there of.
    */
   void process(const glsl_type *type, const char *name);

protected:
   /**
    * Method invoked for each leaf of the uniform
    *
    * \param type  Type of the field.
    * \param name  Fully qualified name of the field.
    * \param row_major  For a matrix type, is it stored row-major.
    */
   virtual void visit_field(const glsl_type *type, const char *name,
                            bool row_major) = 0;

   /**
    * Visit a record before visiting its fields
    *
    * For structures-of-structures or interfaces-of-structures, this visits
    * the inner structure before visiting its fields.
    *
    * The default implementation does nothing.
    */
   virtual void visit_field(const glsl_struct_field *field);

private:
   /**
    * \param name_length  Length of the current name \b not including the
    *                     terminating \c NUL character.
    */
   void recursion(const glsl_type *t, char **name, size_t name_length,
                  bool row_major);
};

void
linker_error(gl_shader_program *prog, const char *fmt, ...);

void
linker_warning(gl_shader_program *prog, const char *fmt, ...);

unsigned
count_attribute_slots(const glsl_type *t);

#endif /* GLSL_LINKER_H */
