/*
 * Mesa 3-D graphics library
 *
 * Copyright © 2013 Gregory Hainaut <gregory.hainaut@gmail.com>
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * \file pipelineobj.c
 * \author Hainaut Gregory <gregory.hainaut@gmail.com>
 *
 * Implementation of pipeline object related API functions. Based on
 * GL_ARB_separate_shader_objects extension.
 *
 */

#include "main/glheader.h"
#include "main/context.h"
#include "main/dispatch.h"
#include "main/enums.h"
#include "main/hash.h"
#include "main/mtypes.h"
#include "main/pipelineobj.h"
#include "main/shaderapi.h"
#include "main/shaderobj.h"
#include "main/transformfeedback.h"
#include "main/uniforms.h"
#include "program/program.h"
#include "program/prog_parameter.h"
#include "ralloc.h"
#include <stdbool.h>
#include "../glsl/glsl_parser_extras.h"
#include "../glsl/ir_uniform.h"


/**
 * Bound program to severals stages of the pipeline
 */
void GLAPIENTRY
_mesa_UseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program)
{
}

/**
 * Use the named shader program for subsequent glUniform calls (if pipeline
 * bound)
 */
void GLAPIENTRY
_mesa_ActiveShaderProgram(GLuint pipeline, GLuint program)
{
}

/**
 * Make program of the pipeline current
 */
void GLAPIENTRY
_mesa_BindProgramPipeline(GLuint pipeline)
{
}

/**
 * Delete a set of pipeline objects.
 *
 * \param n      Number of pipeline objects to delete.
 * \param ids    pipeline of \c n pipeline object IDs.
 */
void GLAPIENTRY
_mesa_DeleteProgramPipelines(GLsizei n, const GLuint *pipelines)
{
}

/**
 * Generate a set of unique pipeline object IDs and store them in \c pipelines.
 * \param n       Number of IDs to generate.
 * \param pipelines  pipeline of \c n locations to store the IDs.
 */
void GLAPIENTRY
_mesa_GenProgramPipelines(GLsizei n, GLuint *pipelines)
{
}

/**
 * Determine if ID is the name of an pipeline object.
 *
 * \param id  ID of the potential pipeline object.
 * \return  \c GL_TRUE if \c id is the name of a pipeline object,
 *          \c GL_FALSE otherwise.
 */
GLboolean GLAPIENTRY
_mesa_IsProgramPipeline(GLuint pipeline)
{
   return GL_FALSE;
}

/**
 * glGetProgramPipelineiv() - get pipeline shader state.
 */
void GLAPIENTRY
_mesa_GetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint *params)
{
}

/**
 * Check compatibility of pipeline's program
 */
void GLAPIENTRY
_mesa_ValidateProgramPipeline(GLuint pipeline)
{
}

void GLAPIENTRY
_mesa_GetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize,
                                GLsizei *length, GLchar *infoLog)
{
}
