/* -*- c++ -*- */
/*
 * Copyright © 2009 Intel Corporation
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
#ifndef GLSL_TYPES_H
#define GLSL_TYPES_H

#include <cstring>

#define GLSL_TYPE_UINT          0
#define GLSL_TYPE_INT           1
#define GLSL_TYPE_FLOAT         2
#define GLSL_TYPE_BOOL          3
#define GLSL_TYPE_SAMPLER       4
#define GLSL_TYPE_STRUCT        5
#define GLSL_TYPE_ARRAY         6
#define GLSL_TYPE_FUNCTION      7
#define GLSL_TYPE_VOID          8
#define GLSL_TYPE_ERROR         9

extern const struct glsl_type *const glsl_error_type;
extern const struct glsl_type *const glsl_int_type;
extern const struct glsl_type *const glsl_uint_type;
extern const struct glsl_type *const glsl_float_type;
extern const struct glsl_type *const glsl_bool_type;

#define is_numeric_base_type(b) \
   (((b) >= GLSL_TYPE_UINT) && ((b) <= GLSL_TYPE_FLOAT))

#define is_integer_base_type(b) \
   (((b) == GLSL_TYPE_UINT) || ((b) == GLSL_TYPE_INT))

#define is_error_type(t) ((t)->base_type == GLSL_TYPE_ERROR)

enum glsl_sampler_dim {
   GLSL_SAMPLER_DIM_1D = 0,
   GLSL_SAMPLER_DIM_2D,
   GLSL_SAMPLER_DIM_3D,
   GLSL_SAMPLER_DIM_CUBE,
   GLSL_SAMPLER_DIM_RECT,
   GLSL_SAMPLER_DIM_BUF
};


struct glsl_type {
   unsigned base_type:4;

   unsigned sampler_dimensionality:3;
   unsigned sampler_shadow:1;
   unsigned sampler_array:1;
   unsigned sampler_type:2;    /**< Type of data returned using this sampler.
				* only \c GLSL_TYPE_FLOAT, \c GLSL_TYPE_INT,
				* and \c GLSL_TYPE_UINT are valid.
				*/

   unsigned vector_elements:3; /**< 0, 2, 3, or 4 vector elements. */
   unsigned matrix_columns:3;  /**< 0, 2, 3, or 4 matrix columns. */

   /**
    * Name of the data type
    *
    * This may be \c NULL for anonymous structures, for arrays, or for
    * function types.
    */
   const char *name;

   /**
    * For \c GLSL_TYPE_ARRAY, this is the length of the array.  For
    * \c GLSL_TYPE_STRUCT, it is the number of elements in the structure and
    * the number of values pointed to by \c fields.structure (below).
    *
    * For \c GLSL_TYPE_FUNCTION, it is the number of parameters to the
    * function.  The return value from a function is implicitly the first
    * parameter.  The types of the parameters are stored in
    * \c fields.parameters (below).
    */
   unsigned length;

   /**
    * Subtype of composite data types.
    */
   union {
      const struct glsl_type *array;            /**< Type of array elements. */
      const struct glsl_type *parameters;       /**< Parameters to function. */
      const struct glsl_struct_field *structure;/**< List of struct fields. */
   } fields;


   glsl_type(unsigned base_type, unsigned vector_elements,
	     unsigned matrix_columns, const char *name) :
      base_type(base_type), 
      sampler_dimensionality(0), sampler_shadow(0), sampler_array(0),
      sampler_type(0),
      vector_elements(vector_elements), matrix_columns(matrix_columns),
      name(name),
      length(0)
   {
      memset(& fields, 0, sizeof(fields));
   }

   glsl_type(enum glsl_sampler_dim dim, bool shadow, bool array,
	     unsigned type, const char *name) :
      base_type(GLSL_TYPE_SAMPLER),
      sampler_dimensionality(dim), sampler_shadow(shadow),
      sampler_array(array), sampler_type(type),
      vector_elements(0), matrix_columns(0),
      name(name),
      length(0)
   {
      memset(& fields, 0, sizeof(fields));
   }

   glsl_type(const glsl_struct_field *fields, unsigned num_fields,
	     const char *name) :
      base_type(GLSL_TYPE_STRUCT),
      sampler_dimensionality(0), sampler_shadow(0), sampler_array(0),
      sampler_type(0),
      vector_elements(0), matrix_columns(0),
      name(name),
      length(num_fields)
   {
      this->fields.structure = fields;
   }

   /**
    * For numeric and boolean derrived types returns the basic scalar type
    *
    * If the type is a numeric or boolean scalar, vector, or matrix type,
    * this function gets the scalar type of the individual components.  For
    * all other types, including arrays of numeric or boolean types, the
    * error type is returned.
    */
   const glsl_type *get_base_type() const;

   /**
    * Get the instance of a built-in scalar, vector, or matrix type
    */
   static const glsl_type *get_instance(unsigned base_type, unsigned rows,
					unsigned columns);

   /**
    * Query whether or not a type is a scalar (non-vector and non-matrix).
    */
   bool is_scalar() const
   {
      return (vector_elements == 0)
	 && (base_type >= GLSL_TYPE_UINT)
	 && (base_type <= GLSL_TYPE_BOOL);
   }

   /**
    * Query whether or not a type is a vector
    */
   bool is_vector() const
   {
      return (vector_elements > 0)
	 && (matrix_columns == 0)
	 && (base_type >= GLSL_TYPE_UINT)
	 && (base_type <= GLSL_TYPE_BOOL);
   }

   /**
    * Query whether or not a type is a matrix
    */
   bool is_matrix() const
   {
      /* GLSL only has float matrices. */
      return (matrix_columns > 0) && (base_type == GLSL_TYPE_FLOAT);
   }

   /**
    * Query whether or not a type is a non-array numeric type
    */
   bool is_numeric() const
   {
      return (base_type >= GLSL_TYPE_UINT) && (base_type <= GLSL_TYPE_FLOAT);
   }

   /**
    * Query whether or not a type is a non-array boolean type
    */
   bool is_boolean() const
   {
      return base_type == GLSL_TYPE_BOOL;
   }

   /**
    * Query whether or not a type is a sampler
    */
   bool is_sampler() const
   {
      return base_type == GLSL_TYPE_SAMPLER;
   }

   /**
    * Query whether or not a type is the void type singleton.
    */
   bool is_void() const
   {
      return base_type == GLSL_TYPE_VOID;
   }

   /**
    * Query whether or not a type is the error type singleton.
    */
   bool is_error() const
   {
      return base_type == GLSL_TYPE_ERROR;
   }

   /**
    * Query the full type of a matrix row
    *
    * \return
    * If the type is not a matrix, \c glsl_error_type is returned.  Otherwise
    * a type matching the rows of the matrix is returned.
    */
   const glsl_type *row_type() const
   {
      return is_matrix()
	 ? get_instance(base_type, matrix_columns, 1)
	 : glsl_error_type;
   }

   /**
    * Query the full type of a matrix column
    *
    * \return
    * If the type is not a matrix, \c glsl_error_type is returned.  Otherwise
    * a type matching the columns of the matrix is returned.
    */
   const glsl_type *column_type() const
   {
      return is_matrix()
	 ? get_instance(base_type, vector_elements, 1)
	 : glsl_error_type;
   }

private:
   /**
    * \name Pointers to various type singletons
    */
   /*@{*/
   static const glsl_type *const mat2_type;
   static const glsl_type *const mat2x3_type;
   static const glsl_type *const mat2x4_type;
   static const glsl_type *const mat3x2_type;
   static const glsl_type *const mat3_type;
   static const glsl_type *const mat3x4_type;
   static const glsl_type *const mat4x2_type;
   static const glsl_type *const mat4x3_type;
   static const glsl_type *const mat4_type;
   /*@}*/
};

struct glsl_struct_field {
   const struct glsl_type *type;
   const char *name;
};

struct _mesa_glsl_parse_state;

#ifdef __cplusplus
extern "C" {
#endif

extern void
_mesa_glsl_initialize_types(struct _mesa_glsl_parse_state *state);

#ifdef __cplusplus
}
#endif

#endif /* GLSL_TYPES_H */
