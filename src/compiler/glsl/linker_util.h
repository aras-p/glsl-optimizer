/*
 * Copyright © 2018 Intel Corporation
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

#ifndef GLSL_LINKER_UTIL_H
#define GLSL_LINKER_UTIL_H

#include "util/bitset.h"
#include "util/glheader.h"
#include "compiler/glsl/ir_list.h"
#include "compiler/glsl_types.h"
#include "main/mtypes.h"
#include "main/shader_types.h"

struct gl_constants;
struct gl_shader_program;
struct gl_uniform_storage;
struct set;

/**
 * Built-in / reserved GL variables names start with "gl_"
 */
static inline bool
is_gl_identifier(const char *s)
{
   return s && s[0] == 'g' && s[1] == 'l' && s[2] == '_';
}

static inline GLenum
glsl_get_gl_type(const struct glsl_type *t)
{
   return t->gl_type;
}

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sometimes there are empty slots left over in UniformRemapTable after we
 * allocate slots to explicit locations. This struct represents a single
 * continouous block of empty slots in UniformRemapTable.
 */
struct empty_uniform_block {
   struct ir_exec_node link;
   /* The start location of the block */
   unsigned start;
   /* The number of slots in the block */
   unsigned slots;
};

/**
 * Describes an access of an array element or an access of the whole array
 */
struct array_deref_range {
   /**
    * Index that was accessed.
    *
    * All valid array indices are less than the size of the array.  If index
    * is equal to the size of the array, this means the entire array has been
    * accessed (e.g., due to use of a non-constant index).
    */
   unsigned index;

   /** Size of the array.  Used for offset calculations. */
   unsigned size;
};

void
link_shaders_init(struct gl_context *ctx, struct gl_shader_program *prog);

void
linker_error(struct gl_shader_program *prog, const char *fmt, ...);

void
linker_warning(struct gl_shader_program *prog, const char *fmt, ...);

long
link_util_parse_program_resource_name(const GLchar *name, const size_t len,
                                      const GLchar **out_base_name_end);

bool
link_util_should_add_buffer_variable(struct gl_shader_program *prog,
                                     struct gl_uniform_storage *uniform,
                                     int top_level_array_base_offset,
                                     int top_level_array_size_in_bytes,
                                     int second_element_offset,
                                     int block_index);

bool
link_util_add_program_resource(struct gl_shader_program *prog,
                               struct set *resource_set,
                               GLenum type, const void *data, uint8_t stages);

int
link_util_find_empty_block(struct gl_shader_program *prog,
                           struct gl_uniform_storage *uniform);

void
link_util_update_empty_uniform_locations(const struct gl_constants *consts,
                                         struct gl_shader_program *prog);

void
link_util_check_subroutine_resources(struct gl_shader_program *prog);

void
link_util_check_uniform_resources(const struct gl_constants *consts,
                                  struct gl_shader_program *prog);

void
link_util_calculate_subroutine_compat(struct gl_shader_program *prog);

void
resource_name_updated(struct gl_resource_name *name);

/**
 * Get the string value for an interpolation qualifier
 *
 * \return The string that would be used in a shader to specify \c
 * mode will be returned.
 *
 * This function is used to generate error messages of the form "shader
 * uses %s interpolation qualifier", so in the case where there is no
 * interpolation qualifier, it returns "no".
 *
 * This function should only be used on a shader input or output variable.
 */
const char *interpolation_string(unsigned interpolation);

/**
 * \brief Can \c from be implicitly converted to \c desired
 *
 * \return True if the types are identical or if \c from type can be converted
 *         to \c desired according to Section 4.1.10 of the GLSL spec.
 *
 * \verbatim
 * From page 25 (31 of the pdf) of the GLSL 1.50 spec, Section 4.1.10
 * Implicit Conversions:
 *
 *     In some situations, an expression and its type will be implicitly
 *     converted to a different type. The following table shows all allowed
 *     implicit conversions:
 *
 *     Type of expression | Can be implicitly converted to
 *     --------------------------------------------------
 *     int                  float
 *     uint
 *
 *     ivec2                vec2
 *     uvec2
 *
 *     ivec3                vec3
 *     uvec3
 *
 *     ivec4                vec4
 *     uvec4
 *
 *     There are no implicit array or structure conversions. For example,
 *     an array of int cannot be implicitly converted to an array of float.
 *     There are no implicit conversions between signed and unsigned
 *     integers.
 * \endverbatim
 */
extern bool _mesa_glsl_can_implicitly_convert(const glsl_type *from, const glsl_type *desired,
                                              bool has_implicit_conversions,
                                              bool has_implicit_int_to_uint_conversion);

#ifdef __cplusplus
}
#endif

#endif /* GLSL_LINKER_UTIL_H */
