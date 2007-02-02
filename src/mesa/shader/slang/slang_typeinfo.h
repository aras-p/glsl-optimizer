/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2005-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SLANG_ASSEMBLE_TYPEINFO_H
#define SLANG_ASSEMBLE_TYPEINFO_H 1

#include "imports.h"
#include "mtypes.h"
#include "slang_utility.h"
#include "slang_vartable.h"


struct slang_operation_;


/**
 * Holds complete information about vector swizzle - the <swizzle>
 * array contains vector component source indices, where 0 is "x", 1
 * is "y", 2 is "z" and 3 is "w".
 * Example: "xwz" --> { 3, { 0, 3, 2, not used } }.
 */
typedef struct slang_swizzle_
{
   GLuint num_components;
   GLuint swizzle[4];
} slang_swizzle;

typedef struct slang_name_space_
{
   struct slang_function_scope_ *funcs;
   struct slang_struct_scope_ *structs;
   struct slang_variable_scope_ *vars;
} slang_name_space;


typedef struct slang_assemble_ctx_
{
   slang_atom_pool *atoms;
   slang_name_space space;
   slang_swizzle swz;
   struct gl_program *program;
   slang_var_table *vartable;

   struct slang_function_ *CurFunction;
   slang_atom CurLoopBreak;
   slang_atom CurLoopCont;
} slang_assemble_ctx;

extern struct slang_function_ *
_slang_locate_function(const struct slang_function_scope_ *funcs,
                       slang_atom name, const struct slang_operation_ *params,
                       GLuint num_params,
                       const slang_name_space *space,
                       slang_atom_pool *);


extern GLboolean
_slang_is_swizzle(const char *field, GLuint rows, slang_swizzle *swz);

extern GLboolean
_slang_is_swizzle_mask(const slang_swizzle *swz, GLuint rows);

extern GLvoid
_slang_multiply_swizzles(slang_swizzle *, const slang_swizzle *,
                         const slang_swizzle *);


/**
 * The basic shading language types (float, vec4, mat3, etc)
 */
typedef enum slang_type_specifier_type_
{
   slang_spec_void,
   slang_spec_bool,
   slang_spec_bvec2,
   slang_spec_bvec3,
   slang_spec_bvec4,
   slang_spec_int,
   slang_spec_ivec2,
   slang_spec_ivec3,
   slang_spec_ivec4,
   slang_spec_float,
   slang_spec_vec2,
   slang_spec_vec3,
   slang_spec_vec4,
   slang_spec_mat2,
   slang_spec_mat3,
   slang_spec_mat4,
   slang_spec_sampler1D,
   slang_spec_sampler2D,
   slang_spec_sampler3D,
   slang_spec_samplerCube,
   slang_spec_sampler1DShadow,
   slang_spec_sampler2DShadow,
   slang_spec_struct,
   slang_spec_array
} slang_type_specifier_type;


/**
 * Describes more sophisticated types, like structs and arrays.
 */
typedef struct slang_type_specifier_
{
   slang_type_specifier_type type;
   struct slang_struct_ *_struct;         /**< used if type == spec_struct */
   struct slang_type_specifier_ *_array;  /**< used if type == spec_array */
} slang_type_specifier;


extern GLvoid
slang_type_specifier_ctr(slang_type_specifier *);

extern GLvoid
slang_type_specifier_dtr(slang_type_specifier *);

extern GLboolean
slang_type_specifier_copy(slang_type_specifier *, const slang_type_specifier *);

extern GLboolean
slang_type_specifier_equal(const slang_type_specifier *,
                           const slang_type_specifier *);


typedef struct slang_assembly_typeinfo_
{
   GLboolean can_be_referenced;
   GLboolean is_swizzled;
   slang_swizzle swz;
   slang_type_specifier spec;
   GLuint array_len;
} slang_assembly_typeinfo;

extern GLboolean
slang_assembly_typeinfo_construct(slang_assembly_typeinfo *);

extern GLvoid
slang_assembly_typeinfo_destruct(slang_assembly_typeinfo *);


/**
 * Retrieves type information about an operation.
 * Returns GL_TRUE on success.
 * Returns GL_FALSE otherwise.
 */
extern GLboolean
_slang_typeof_operation(const slang_assemble_ctx *,
                        const struct slang_operation_ *,
                        slang_assembly_typeinfo *);

extern GLboolean
_slang_typeof_operation_(const struct slang_operation_ *,
                         const slang_name_space *,
                         slang_assembly_typeinfo *, slang_atom_pool *);

/**
 * Retrieves type of a function prototype, if one exists.
 * Returns GL_TRUE on success, even if the function was not found.
 * Returns GL_FALSE otherwise.
 */
extern GLboolean
_slang_typeof_function(slang_atom a_name,
                       const struct slang_operation_ *params,
                       GLuint num_params, const slang_name_space *,
                       slang_type_specifier *spec, GLboolean *exists,
                       slang_atom_pool *);

extern GLboolean
_slang_type_is_matrix(slang_type_specifier_type);

extern GLboolean
_slang_type_is_vector(slang_type_specifier_type);

extern slang_type_specifier_type
_slang_type_base(slang_type_specifier_type);

extern GLuint
_slang_type_dim(slang_type_specifier_type);



#endif

