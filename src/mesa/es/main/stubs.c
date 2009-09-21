/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
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
 * TUNGSTEN GRAPHICS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **************************************************************************/


/**
 * Temporary stubs for "missing" mesa functions.
 */


#include "main/mtypes.h"
#include "main/imports.h"
#include "vbo/vbo.h"

#define NEED_IMPLEMENT() do { \
      GET_CURRENT_CONTEXT(ctx); \
      _mesa_error(ctx, GL_INVALID_OPERATION, __FUNCTION__); \
   } while (0)

#if FEATURE_accum
/* This is a sanity check that to be sure we're using the correct mfeatures.h
 * header.  We don't want to accidentally use the one from mainline Mesa.
 */
#error "The wrong mfeatures.h file is being included!"
#endif


/* silence compiler warnings */
extern void GLAPIENTRY _vbo_Materialf(GLenum face, GLenum pname, GLfloat param);
extern void GLAPIENTRY _mesa_GetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
extern void GLAPIENTRY _mesa_ReleaseShaderCompiler(void);
extern void GLAPIENTRY _mesa_ShaderBinary(GLint n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLint length);
extern void GLAPIENTRY _vbo_VertexAttrib1f(GLuint indx, GLfloat x);
extern void GLAPIENTRY _vbo_VertexAttrib1fv(GLuint indx, const GLfloat* values);
extern void GLAPIENTRY _vbo_VertexAttrib2f(GLuint indx, GLfloat x, GLfloat y);
extern void GLAPIENTRY _vbo_VertexAttrib2fv(GLuint indx, const GLfloat* values);
extern void GLAPIENTRY _vbo_VertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z);
extern void GLAPIENTRY _vbo_VertexAttrib3fv(GLuint indx, const GLfloat* values);
extern void GLAPIENTRY _vbo_VertexAttrib4fv(GLuint indx, const GLfloat* values);


void GLAPIENTRY
_vbo_Materialf(GLenum face, GLenum pname, GLfloat param)
{
   _vbo_Materialfv(face, pname, &param);
}


void GLAPIENTRY
_mesa_GetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype,
                               GLint* range, GLint* precision)
{
   NEED_IMPLEMENT();
}


void GLAPIENTRY
_mesa_ReleaseShaderCompiler(void)
{
   NEED_IMPLEMENT();
}


void GLAPIENTRY
_mesa_ShaderBinary(GLint n, const GLuint* shaders, GLenum binaryformat,
                   const void* binary, GLint length)
{
   NEED_IMPLEMENT();
}


void GLAPIENTRY
_vbo_VertexAttrib1f(GLuint indx, GLfloat x)
{
   _vbo_VertexAttrib4f(indx, x, 0.0, 0.0, 1.0f);
}


void GLAPIENTRY
_vbo_VertexAttrib1fv(GLuint indx, const GLfloat* values)
{
   _vbo_VertexAttrib4f(indx, values[0], 0.0, 0.0, 1.0f);
}


void GLAPIENTRY
_vbo_VertexAttrib2f(GLuint indx, GLfloat x, GLfloat y)
{
   _vbo_VertexAttrib4f(indx, x, y, 0.0, 1.0f);
}


void GLAPIENTRY
_vbo_VertexAttrib2fv(GLuint indx, const GLfloat* values)
{
   _vbo_VertexAttrib4f(indx, values[0], values[1], 0.0, 1.0f);
}


void GLAPIENTRY
_vbo_VertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z)
{
   _vbo_VertexAttrib4f(indx, x, y, z, 1.0f);
}


void GLAPIENTRY
_vbo_VertexAttrib3fv(GLuint indx, const GLfloat* values)
{
   _vbo_VertexAttrib4f(indx, values[0], values[1], values[2], 1.0f);
}


void GLAPIENTRY
_vbo_VertexAttrib4fv(GLuint indx, const GLfloat* values)
{
   _vbo_VertexAttrib4f(indx, values[0], values[1], values[2], values[3]);
}
