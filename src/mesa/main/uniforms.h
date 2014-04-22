/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2010  VMware, Inc.  All Rights Reserved.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef UNIFORMS_H
#define UNIFORMS_H

#include "glheader.h"
#include "program/prog_parameter.h"
#include "../glsl/glsl_types.h"
#include "../glsl/ir_uniform.h"

#ifdef __cplusplus
extern "C" {
#endif


struct gl_program;
struct _glapi_table;

void GLAPIENTRY
_mesa_Uniform1f(GLint, GLfloat);
void GLAPIENTRY
_mesa_Uniform2f(GLint, GLfloat, GLfloat);
void GLAPIENTRY
_mesa_Uniform3f(GLint, GLfloat, GLfloat, GLfloat);
void GLAPIENTRY
_mesa_Uniform4f(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
void GLAPIENTRY
_mesa_Uniform1i(GLint, GLint);
void GLAPIENTRY
_mesa_Uniform2i(GLint, GLint, GLint);
void GLAPIENTRY
_mesa_Uniform3i(GLint, GLint, GLint, GLint);
void GLAPIENTRY
_mesa_Uniform4i(GLint, GLint, GLint, GLint, GLint);
void GLAPIENTRY
_mesa_Uniform1fv(GLint, GLsizei, const GLfloat *);
void GLAPIENTRY
_mesa_Uniform2fv(GLint, GLsizei, const GLfloat *);
void GLAPIENTRY
_mesa_Uniform3fv(GLint, GLsizei, const GLfloat *);
void GLAPIENTRY
_mesa_Uniform4fv(GLint, GLsizei, const GLfloat *);
void GLAPIENTRY
_mesa_Uniform1iv(GLint, GLsizei, const GLint *);
void GLAPIENTRY
_mesa_Uniform2iv(GLint, GLsizei, const GLint *);
void GLAPIENTRY
_mesa_Uniform3iv(GLint, GLsizei, const GLint *);
void GLAPIENTRY
_mesa_Uniform4iv(GLint, GLsizei, const GLint *);
void GLAPIENTRY
_mesa_Uniform1ui(GLint location, GLuint v0);
void GLAPIENTRY
_mesa_Uniform2ui(GLint location, GLuint v0, GLuint v1);
void GLAPIENTRY
_mesa_Uniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2);
void GLAPIENTRY
_mesa_Uniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
void GLAPIENTRY
_mesa_Uniform1uiv(GLint location, GLsizei count, const GLuint *value);
void GLAPIENTRY
_mesa_Uniform2uiv(GLint location, GLsizei count, const GLuint *value);
void GLAPIENTRY
_mesa_Uniform3uiv(GLint location, GLsizei count, const GLuint *value);
void GLAPIENTRY
_mesa_Uniform4uiv(GLint location, GLsizei count, const GLuint *value);
void GLAPIENTRY
_mesa_UniformMatrix2fv(GLint, GLsizei, GLboolean, const GLfloat *);
void GLAPIENTRY
_mesa_UniformMatrix3fv(GLint, GLsizei, GLboolean, const GLfloat *);
void GLAPIENTRY
_mesa_UniformMatrix4fv(GLint, GLsizei, GLboolean, const GLfloat *);
void GLAPIENTRY
_mesa_UniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);
void GLAPIENTRY
_mesa_UniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);
void GLAPIENTRY
_mesa_UniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);
void GLAPIENTRY
_mesa_UniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);
void GLAPIENTRY
_mesa_UniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);
void GLAPIENTRY
_mesa_UniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);

void GLAPIENTRY
_mesa_ProgramUniform1f(GLuint program, GLint, GLfloat);
void GLAPIENTRY
_mesa_ProgramUniform2f(GLuint program, GLint, GLfloat, GLfloat);
void GLAPIENTRY
_mesa_ProgramUniform3f(GLuint program, GLint, GLfloat, GLfloat, GLfloat);
void GLAPIENTRY
_mesa_ProgramUniform4f(GLuint program, GLint, GLfloat, GLfloat, GLfloat, GLfloat);
void GLAPIENTRY
_mesa_ProgramUniform1i(GLuint program, GLint, GLint);
void GLAPIENTRY
_mesa_ProgramUniform2i(GLuint program, GLint, GLint, GLint);
void GLAPIENTRY
_mesa_ProgramUniform3i(GLuint program, GLint, GLint, GLint, GLint);
void GLAPIENTRY
_mesa_ProgramUniform4i(GLuint program, GLint, GLint, GLint, GLint, GLint);
void GLAPIENTRY
_mesa_ProgramUniform1fv(GLuint program, GLint, GLsizei, const GLfloat *);
void GLAPIENTRY
_mesa_ProgramUniform2fv(GLuint program, GLint, GLsizei, const GLfloat *);
void GLAPIENTRY
_mesa_ProgramUniform3fv(GLuint program, GLint, GLsizei, const GLfloat *);
void GLAPIENTRY
_mesa_ProgramUniform4fv(GLuint program, GLint, GLsizei, const GLfloat *);
void GLAPIENTRY
_mesa_ProgramUniform1iv(GLuint program, GLint, GLsizei, const GLint *);
void GLAPIENTRY
_mesa_ProgramUniform2iv(GLuint program, GLint, GLsizei, const GLint *);
void GLAPIENTRY
_mesa_ProgramUniform3iv(GLuint program, GLint, GLsizei, const GLint *);
void GLAPIENTRY
_mesa_ProgramUniform4iv(GLuint program, GLint, GLsizei, const GLint *);
void GLAPIENTRY
_mesa_ProgramUniform1ui(GLuint program, GLint location, GLuint v0);
void GLAPIENTRY
_mesa_ProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1);
void GLAPIENTRY
_mesa_ProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1,
                        GLuint v2);
void GLAPIENTRY
_mesa_ProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1,
                        GLuint v2, GLuint v3);
void GLAPIENTRY
_mesa_ProgramUniform1uiv(GLuint program, GLint location, GLsizei count,
                         const GLuint *value);
void GLAPIENTRY
_mesa_ProgramUniform2uiv(GLuint program, GLint location, GLsizei count,
                         const GLuint *value);
void GLAPIENTRY
_mesa_ProgramUniform3uiv(GLuint program, GLint location, GLsizei count,
                         const GLuint *value);
void GLAPIENTRY
_mesa_ProgramUniform4uiv(GLuint program, GLint location, GLsizei count,
                         const GLuint *value);
void GLAPIENTRY
_mesa_ProgramUniformMatrix2fv(GLuint program, GLint, GLsizei, GLboolean,
                              const GLfloat *);
void GLAPIENTRY
_mesa_ProgramUniformMatrix3fv(GLuint program, GLint, GLsizei, GLboolean,
                              const GLfloat *);
void GLAPIENTRY
_mesa_ProgramUniformMatrix4fv(GLuint program, GLint, GLsizei, GLboolean,
                              const GLfloat *);
void GLAPIENTRY
_mesa_ProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count,
                                GLboolean transpose, const GLfloat *value);
void GLAPIENTRY
_mesa_ProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count,
                                GLboolean transpose, const GLfloat *value);
void GLAPIENTRY
_mesa_ProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count,
                                GLboolean transpose, const GLfloat *value);
void GLAPIENTRY
_mesa_ProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count,
                                GLboolean transpose, const GLfloat *value);
void GLAPIENTRY
_mesa_ProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count,
                                GLboolean transpose, const GLfloat *value);
void GLAPIENTRY
_mesa_ProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count,
                                GLboolean transpose, const GLfloat *value);

void GLAPIENTRY
_mesa_GetnUniformfvARB(GLuint, GLint, GLsizei, GLfloat *);
void GLAPIENTRY
_mesa_GetUniformfv(GLuint, GLint, GLfloat *);
void GLAPIENTRY
_mesa_GetnUniformivARB(GLuint, GLint, GLsizei, GLint *);
void GLAPIENTRY
_mesa_GetUniformuiv(GLuint, GLint, GLuint *);
void GLAPIENTRY
_mesa_GetnUniformuivARB(GLuint, GLint, GLsizei, GLuint *);
void GLAPIENTRY
_mesa_GetUniformuiv(GLuint program, GLint location, GLuint *params);
void GLAPIENTRY
_mesa_GetnUniformdvARB(GLuint, GLint, GLsizei, GLdouble *);
void GLAPIENTRY
_mesa_GetUniformdv(GLuint, GLint, GLdouble *);
GLint GLAPIENTRY
_mesa_GetUniformLocation(GLuint, const GLcharARB *);
GLuint GLAPIENTRY
_mesa_GetUniformBlockIndex(GLuint program,
			   const GLchar *uniformBlockName);
void GLAPIENTRY
_mesa_GetUniformIndices(GLuint program,
			GLsizei uniformCount,
			const GLchar * const *uniformNames,
			GLuint *uniformIndices);
void GLAPIENTRY
_mesa_UniformBlockBinding(GLuint program,
			  GLuint uniformBlockIndex,
			  GLuint uniformBlockBinding);
void GLAPIENTRY
_mesa_GetActiveAtomicCounterBufferiv(GLuint program, GLuint bufferIndex,
                                     GLenum pname, GLint *params);
void GLAPIENTRY
_mesa_GetActiveUniformBlockiv(GLuint program,
			      GLuint uniformBlockIndex,
			      GLenum pname,
			      GLint *params);
void GLAPIENTRY
_mesa_GetActiveUniformBlockName(GLuint program,
				GLuint uniformBlockIndex,
				GLsizei bufSize,
				GLsizei *length,
				GLchar *uniformBlockName);
void GLAPIENTRY
_mesa_GetActiveUniformName(GLuint program, GLuint uniformIndex,
			   GLsizei bufSize, GLsizei *length,
			   GLchar *uniformName);
void GLAPIENTRY
_mesa_GetActiveUniform(GLuint, GLuint, GLsizei, GLsizei *,
                       GLint *, GLenum *, GLcharARB *);
void GLAPIENTRY
_mesa_GetActiveUniformsiv(GLuint program,
			  GLsizei uniformCount,
			  const GLuint *uniformIndices,
			  GLenum pname,
			  GLint *params);
void GLAPIENTRY
_mesa_GetUniformiv(GLuint, GLint, GLint *);

long
_mesa_parse_program_resource_name(const GLchar *name,
                                  const GLchar **out_base_name_end);

unsigned
_mesa_get_uniform_location(struct gl_context *ctx, struct gl_shader_program *shProg,
			   const GLchar *name, unsigned *offset);

void
_mesa_uniform(struct gl_context *ctx, struct gl_shader_program *shader_program,
	      GLint location, GLsizei count,
              const GLvoid *values, GLenum type);

void
_mesa_uniform_matrix(struct gl_context *ctx, struct gl_shader_program *shProg,
		     GLuint cols, GLuint rows,
                     GLint location, GLsizei count,
                     GLboolean transpose, const GLfloat *values);

void
_mesa_get_uniform(struct gl_context *ctx, GLuint program, GLint location,
		  GLsizei bufSize, enum glsl_base_type returnType,
		  GLvoid *paramsOut);

extern void
_mesa_uniform_attach_driver_storage(struct gl_uniform_storage *,
				    unsigned element_stride,
				    unsigned vector_stride,
				    enum gl_uniform_driver_format format,
				    void *data);

extern void
_mesa_uniform_detach_all_driver_storage(struct gl_uniform_storage *uni);

extern void
_mesa_propagate_uniforms_to_driver_storage(struct gl_uniform_storage *uni,
					   unsigned array_index,
					   unsigned count);

extern void
_mesa_update_shader_textures_used(struct gl_shader_program *shProg,
				  struct gl_program *prog);

extern bool
_mesa_sampler_uniforms_are_valid(const struct gl_shader_program *shProg,
				 char *errMsg, size_t errMsgLength);
extern bool
_mesa_sampler_uniforms_pipeline_are_valid(struct gl_pipeline_object *);

extern const struct gl_program_parameter *
get_uniform_parameter(struct gl_shader_program *shProg, GLint index);

extern void
_mesa_get_uniform_name(const struct gl_uniform_storage *uni,
                       GLsizei maxLength, GLsizei *length,
                       GLchar *nameOut);

struct gl_builtin_uniform_element {
   const char *field;
   int tokens[STATE_LENGTH];
   int swizzle;
};

struct gl_builtin_uniform_desc {
   const char *name;
   const struct gl_builtin_uniform_element *elements;
   unsigned int num_elements;
};

/**
 * \name GLSL uniform arrays and structs require special handling.
 *
 * The GL_ARB_shader_objects spec says that if you use
 * glGetUniformLocation to get the location of an array, you CANNOT
 * access other elements of the array by adding an offset to the
 * returned location.  For example, you must call
 * glGetUniformLocation("foo[16]") if you want to set the 16th element
 * of the array with glUniform().
 *
 * HOWEVER, some other OpenGL drivers allow accessing array elements
 * by adding an offset to the returned array location.  And some apps
 * seem to depend on that behaviour.
 *
 * Mesa's gl_uniform_list doesn't directly support this since each
 * entry in the list describes one uniform variable, not one uniform
 * element.  We could insert dummy entries in the list for each array
 * element after [0] but that causes complications elsewhere.
 *
 * We solve this problem by creating multiple entries for uniform arrays
 * in the UniformRemapTable so that their elements get sequential locations.
 *
 * Utility functions below offer functionality to split UniformRemapTable
 * location in to location of the uniform in UniformStorage + offset to the
 * array element (0 if not an array) and also merge it back again as the
 * UniformRemapTable location.
 *
 */
/*@{*/
/**
 * Combine the uniform's storage index and the array index
 */
static inline GLint
_mesa_uniform_merge_location_offset(const struct gl_shader_program *prog,
                                    unsigned storage_index,
                                    unsigned uniform_array_index)
{
   /* location in remap table + array element offset */
   return prog->UniformStorage[storage_index].remap_location +
      uniform_array_index;
}

/**
 * Separate the uniform storage index and array index
 */
static inline void
_mesa_uniform_split_location_offset(const struct gl_shader_program *prog,
                                    GLint location, unsigned *storage_index,
				    unsigned *uniform_array_index)
{
   *storage_index = prog->UniformRemapTable[location] - prog->UniformStorage;
   *uniform_array_index = location -
      prog->UniformRemapTable[location]->remap_location;

   /*gl_uniform_storage in UniformStorage with the calculated base_location
    * must match with the entry in remap table
    */
   assert(&prog->UniformStorage[*storage_index] ==
          prog->UniformRemapTable[location]);
}
/*@}*/


#ifdef __cplusplus
}
#endif


#endif /* UNIFORMS_H */
