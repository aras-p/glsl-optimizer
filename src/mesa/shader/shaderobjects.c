/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2004-2006  Brian Paul   All Rights Reserved.
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

/**
 * \file shaderobjects.c
 * ARB_shader_objects state management functions
 * \author Michal Krol
 */


#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "shaderobjects.h"
#include "shaderobjects_3dlabs.h"


#if FEATURE_ARB_shader_objects

#define RELEASE_GENERIC(x)\
   (**x)._unknown.Release ((struct gl2_unknown_intf **) (x))

#define RELEASE_CONTAINER(x)\
   (**x)._generic._unknown.Release ((struct gl2_unknown_intf **) (x))

#define RELEASE_PROGRAM(x)\
   (**x)._container._generic._unknown.Release ((struct gl2_unknown_intf **) (x))

#define RELEASE_SHADER(x)\
   (**x)._generic._unknown.Release ((struct gl2_unknown_intf **) (x))

static struct gl2_unknown_intf **
lookup_handle (GLcontext *ctx, GLhandleARB handle, enum gl2_uiid uiid, const char *function)
{
   struct gl2_unknown_intf **unk;

   /*
    * Note: _mesa_HashLookup() requires non-zero input values, so the passed-in handle value
    *       must be checked beforehand.
    */
   if (handle == 0) {
      _mesa_error (ctx, GL_INVALID_VALUE, function);
      return NULL;
   }
   _glthread_LOCK_MUTEX (ctx->Shared->Mutex);
   unk = (struct gl2_unknown_intf **) (_mesa_HashLookup (ctx->Shared->GL2Objects, handle));
   _glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);
   if (unk == NULL)
      _mesa_error (ctx, GL_INVALID_VALUE, function);
   else {
      unk = (**unk).QueryInterface (unk, uiid);
      if (unk == NULL)
         _mesa_error (ctx, GL_INVALID_OPERATION, function);
   }
   return unk;
}

#define GET_GENERIC(x, handle, function)\
   struct gl2_generic_intf **x = (struct gl2_generic_intf **)\
                                 lookup_handle (ctx, handle, UIID_GENERIC, function);

#define GET_CONTAINER(x, handle, function)\
   struct gl2_container_intf **x = (struct gl2_container_intf **)\
                                   lookup_handle (ctx, handle, UIID_CONTAINER, function);

#define GET_PROGRAM(x, handle, function)\
   struct gl2_program_intf **x = (struct gl2_program_intf **)\
                                 lookup_handle (ctx, handle, UIID_PROGRAM, function);

#define GET_SHADER(x, handle, function)\
   struct gl2_shader_intf **x = (struct gl2_shader_intf **)\
                                lookup_handle (ctx, handle, UIID_SHADER, function);

#define GET_LINKED_PROGRAM(x, handle, function)\
   GET_PROGRAM(x, handle, function);\
   if (x != NULL && (**x).GetLinkStatus (x) == GL_FALSE) {\
      RELEASE_PROGRAM(x);\
      x = NULL;\
      _mesa_error (ctx, GL_INVALID_OPERATION, function);\
   }

#define GET_CURRENT_LINKED_PROGRAM(x, function)\
   struct gl2_program_intf **x = NULL;\
   if (ctx->ShaderObjects.CurrentProgram == NULL)\
      _mesa_error (ctx, GL_INVALID_OPERATION, function);\
   else {\
      x = ctx->ShaderObjects.CurrentProgram;\
      if (x != NULL && (**x).GetLinkStatus (x) == GL_FALSE) {\
         x = NULL;\
         _mesa_error (ctx, GL_INVALID_OPERATION, function);\
      }\
   }

#define IS_NAME_WITH_GL_PREFIX(x) ((x)[0] == 'g' && (x)[1] == 'l' && (x)[2] == '_')


GLvoid GLAPIENTRY
_mesa_DeleteObjectARB (GLhandleARB obj)
{
   if (obj != 0)
   {
      GET_CURRENT_CONTEXT(ctx);
      GET_GENERIC(gen, obj, "glDeleteObjectARB");

      if (gen != NULL)
      {
         (**gen).Delete (gen);
         RELEASE_GENERIC(gen);
      }
   }
}

GLhandleARB GLAPIENTRY
_mesa_GetHandleARB (GLenum pname)
{
	GET_CURRENT_CONTEXT(ctx);

	switch (pname)
	{
   case GL_PROGRAM_OBJECT_ARB:
      {
         struct gl2_program_intf **pro = ctx->ShaderObjects.CurrentProgram;

         if (pro != NULL)
            return (**pro)._container._generic.GetName ((struct gl2_generic_intf **) (pro));
      }
      break;
	default:
		_mesa_error (ctx, GL_INVALID_ENUM, "glGetHandleARB");
	}

	return 0;
}

GLvoid GLAPIENTRY
_mesa_DetachObjectARB (GLhandleARB containerObj, GLhandleARB attachedObj)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CONTAINER(con, containerObj, "glDetachObjectARB");

	if (con != NULL)
	{
		GET_GENERIC(att, attachedObj, "glDetachObjectARB");

		if (att != NULL)
		{
			(**con).Detach (con, att);
			RELEASE_GENERIC(att);
		}
		RELEASE_CONTAINER(con);
	}
}

GLhandleARB GLAPIENTRY
_mesa_CreateShaderObjectARB (GLenum shaderType)
{
	return _mesa_3dlabs_create_shader_object (shaderType);
}

GLvoid GLAPIENTRY
_mesa_ShaderSourceARB (GLhandleARB shaderObj, GLsizei count, const GLcharARB **string,
					   const GLint *length)
{
	GET_CURRENT_CONTEXT(ctx);
	GLint *offsets;
	GLsizei i;
	GLcharARB *source;
	GET_SHADER(sha, shaderObj, "glShaderSourceARB");

	if (sha == NULL)
		return;

	if (string == NULL)
	{
		RELEASE_SHADER(sha);
		_mesa_error (ctx, GL_INVALID_VALUE, "glShaderSourceARB");
		return;
	}

	/*
	 * This array holds offsets of where the appropriate string ends, thus the last
	 * element will be set to the total length of the source code.
	 */
	offsets = (GLint *) _mesa_malloc (count * sizeof (GLint));
	if (offsets == NULL)
	{
		RELEASE_SHADER(sha);
		_mesa_error (ctx, GL_OUT_OF_MEMORY, "glShaderSourceARB");
		return;
	}

	for (i = 0; i < count; i++)
	{
		if (string[i] == NULL)
		{
			_mesa_free ((GLvoid *) offsets);
			RELEASE_SHADER(sha);
			_mesa_error (ctx, GL_INVALID_VALUE, "glShaderSourceARB");
			return;
		}
		if (length == NULL || length[i] < 0)
			offsets[i] = _mesa_strlen (string[i]);
		else
			offsets[i] = length[i];
		/* accumulate string lengths */
		if (i > 0)
			offsets[i] += offsets[i - 1];
	}

	source = (GLcharARB *) _mesa_malloc ((offsets[count - 1] + 1) * sizeof (GLcharARB));
	if (source == NULL)
	{
		_mesa_free ((GLvoid *) offsets);
		RELEASE_SHADER(sha);
		_mesa_error (ctx, GL_OUT_OF_MEMORY, "glShaderSourceARB");
		return;
	}

	for (i = 0; i < count; i++)
	{
		GLint start = (i > 0) ? offsets[i - 1] : 0;
		_mesa_memcpy (source + start, string[i], (offsets[i] - start) * sizeof (GLcharARB));
	}
	source[offsets[count - 1]] = '\0';

	(**sha).SetSource (sha, source, offsets, count);
	RELEASE_SHADER(sha);
}

GLvoid  GLAPIENTRY
_mesa_CompileShaderARB (GLhandleARB shaderObj)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_SHADER(sha, shaderObj, "glCompileShaderARB");

	if (sha != NULL)
	{
		(**sha).Compile (sha);
		RELEASE_SHADER(sha);
	}
}

GLhandleARB GLAPIENTRY
_mesa_CreateProgramObjectARB (GLvoid)
{
	return _mesa_3dlabs_create_program_object ();
}

GLvoid GLAPIENTRY
_mesa_AttachObjectARB (GLhandleARB containerObj, GLhandleARB obj)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CONTAINER(con, containerObj, "glAttachObjectARB");

	if (con != NULL)
	{
		GET_GENERIC(att, obj, "glAttachObjectARB");

		if (att != NULL)
		{
			(**con).Attach (con, att);
			RELEASE_GENERIC(att);
		}
		RELEASE_CONTAINER(con);
	}
}

GLvoid GLAPIENTRY
_mesa_LinkProgramARB (GLhandleARB programObj)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_PROGRAM(pro, programObj, "glLinkProgramARB");

	if (pro != NULL)
	{
		(**pro).Link (pro);
		if (pro == ctx->ShaderObjects.CurrentProgram)
		{
			if ((**pro).GetLinkStatus (pro))
				_mesa_UseProgramObjectARB (programObj);
			else
				_mesa_UseProgramObjectARB (0);
		}
		RELEASE_PROGRAM(pro);
	}
}

GLvoid GLAPIENTRY
_mesa_UseProgramObjectARB (GLhandleARB programObj)
{
	GET_CURRENT_CONTEXT(ctx);
   struct gl2_program_intf **program = NULL;

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (programObj != 0)
	{
		GET_PROGRAM(pro, programObj, "glUseProgramObjectARB");

		if (pro == NULL)
			return;

		if ((**pro).GetLinkStatus (pro) == GL_FALSE)
		{
			RELEASE_PROGRAM(pro);
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUseProgramObjectARB");
			return;
		}

		program = pro;

		ctx->ShaderObjects._VertexShaderPresent = (**pro).IsShaderPresent (pro, GL_VERTEX_SHADER_ARB);
		ctx->ShaderObjects._FragmentShaderPresent = (**pro).IsShaderPresent (pro,
			GL_FRAGMENT_SHADER_ARB);
	}
	else
	{
		ctx->ShaderObjects._VertexShaderPresent = GL_FALSE;
		ctx->ShaderObjects._FragmentShaderPresent = GL_FALSE;
	}

	if (ctx->ShaderObjects.CurrentProgram != NULL)
		RELEASE_PROGRAM(ctx->ShaderObjects.CurrentProgram);
	ctx->ShaderObjects.CurrentProgram = program;
}

GLvoid GLAPIENTRY
_mesa_ValidateProgramARB (GLhandleARB programObj)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_PROGRAM(pro, programObj, "glValidateProgramARB");

	if (pro != NULL)
	{
		(**pro).Validate (pro);
		RELEASE_PROGRAM(pro);
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform1fARB (GLint location, GLfloat v0)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform1fARB");

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!(**pro).WriteUniform (pro, location, 1, &v0, GL_FLOAT))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform1fARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform2fARB (GLint location, GLfloat v0, GLfloat v1)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform2fARB");

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		GLfloat v[2];
		v[0] = v0;
		v[1] = v1;

		if (!(**pro).WriteUniform (pro, location, 1, v, GL_FLOAT_VEC2))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform2fARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform3fARB (GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform3fARB");

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		GLfloat v[3];
		v[0] = v0;
		v[1] = v1;
		v[2] = v2;

		if (!(**pro).WriteUniform (pro, location, 1, v, GL_FLOAT_VEC3))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform3fARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform4fARB (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform4fARB");

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		GLfloat v[4];
		v[0] = v0;
		v[1] = v1;
		v[2] = v2;
		v[3] = v3;

		if (!(**pro).WriteUniform (pro, location, 1, v, GL_FLOAT_VEC4))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform4fARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform1iARB (GLint location, GLint v0)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform1iARB");

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!(**pro).WriteUniform (pro, location, 1, &v0, GL_INT))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform1iARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform2iARB (GLint location, GLint v0, GLint v1)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform2iARB");

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		GLint v[2];
		v[0] = v0;
		v[1] = v1;

		if (!(**pro).WriteUniform (pro, location, 1, v, GL_INT_VEC2))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform2iARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform3iARB (GLint location, GLint v0, GLint v1, GLint v2)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform3iARB");

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		GLint v[3];
		v[0] = v0;
		v[1] = v1;
		v[2] = v2;

		if (!(**pro).WriteUniform (pro, location, 1, v, GL_INT_VEC3))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform3iARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform4iARB (GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform4iARB");

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		GLint v[4];
		v[0] = v0;
		v[1] = v1;
		v[2] = v2;
		v[3] = v3;

		if (!(**pro).WriteUniform (pro, location, 1, v, GL_INT_VEC4))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform4iARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform1fvARB (GLint location, GLsizei count, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform1fvARB");

	if (value == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glUniform1fvARB");
		return;
	}

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!(**pro).WriteUniform (pro, location, count, value, GL_FLOAT))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform1fvARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform2fvARB (GLint location, GLsizei count, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform2fvARB");

	if (value == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glUniform2fvARB");
		return;
	}

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!(**pro).WriteUniform (pro, location, count, value, GL_FLOAT_VEC2))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform2fvARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform3fvARB (GLint location, GLsizei count, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform3fvARB");

	if (value == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glUniform3fvARB");
		return;
	}

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!(**pro).WriteUniform (pro, location, count, value, GL_FLOAT_VEC3))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform3fvARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform4fvARB (GLint location, GLsizei count, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform4fvARB");

	if (value == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glUniform4fvARB");
		return;
	}

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!(**pro).WriteUniform (pro, location, count, value, GL_FLOAT_VEC4))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform4fvARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform1ivARB (GLint location, GLsizei count, const GLint *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform1ivARB");

	if (value == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glUniform1ivARB");
		return;
	}

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!(**pro).WriteUniform (pro, location, count, value, GL_INT))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform1ivARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform2ivARB (GLint location, GLsizei count, const GLint *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform2ivARB");

	if (value == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glUniform2ivARB");
		return;
	}

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!(**pro).WriteUniform (pro, location, count, value, GL_INT_VEC2))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform2ivARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform3ivARB (GLint location, GLsizei count, const GLint *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform3ivARB");

	if (value == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glUniform3ivARB");
		return;
	}

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!(**pro).WriteUniform (pro, location, count, value, GL_INT_VEC3))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform3ivARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform4ivARB (GLint location, GLsizei count, const GLint *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform4ivARB");

	if (value == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glUniform4ivARB");
		return;
	}

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!(**pro).WriteUniform (pro, location, count, value, GL_INT_VEC4))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform4ivARB");
	}
}

GLvoid GLAPIENTRY
_mesa_UniformMatrix2fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniformMatrix2fvARB");

	if (value == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glUniformMatrix2fvARB");
		return;
	}

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (transpose)
		{
			GLfloat *trans, *pt;
			const GLfloat *pv;

			trans = (GLfloat *) _mesa_malloc (count * 4 * sizeof (GLfloat));
			if (trans == NULL)
			{
				_mesa_error (ctx, GL_OUT_OF_MEMORY, "glUniformMatrix2fvARB");
				return;
			}
			for (pt = trans, pv = value; pt != trans + count * 4; pt += 4, pv += 4)
			{
				pt[0] = pv[0];
				pt[1] = pv[2];
				pt[2] = pv[1];
				pt[3] = pv[3];
			}
			if (!(**pro).WriteUniform (pro, location, count, trans, GL_FLOAT_MAT2))
				_mesa_error (ctx, GL_INVALID_OPERATION, "glUniformMatrix2fvARB");
			_mesa_free (trans);
		}
		else
		{
			if (!(**pro).WriteUniform (pro, location, count, value, GL_FLOAT_MAT2))
				_mesa_error (ctx, GL_INVALID_OPERATION, "glUniformMatrix2fvARB");
		}
	}
}

GLvoid GLAPIENTRY
_mesa_UniformMatrix3fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniformMatrix3fvARB");

	if (value == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glUniformMatrix3fvARB");
		return;
	}

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (transpose)
		{
			GLfloat *trans, *pt;
			const GLfloat *pv;

			trans = (GLfloat *) _mesa_malloc (count * 9 * sizeof (GLfloat));
			if (trans == NULL)
			{
				_mesa_error (ctx, GL_OUT_OF_MEMORY, "glUniformMatrix3fvARB");
				return;
			}
			for (pt = trans, pv = value; pt != trans + count * 9; pt += 9, pv += 9)
			{
				pt[0] = pv[0];
				pt[1] = pv[3];
				pt[2] = pv[6];
				pt[3] = pv[1];
				pt[4] = pv[4];
				pt[5] = pv[7];
				pt[6] = pv[2];
				pt[7] = pv[5];
				pt[8] = pv[8];
			}
			if (!(**pro).WriteUniform (pro, location, count, trans, GL_FLOAT_MAT3))
				_mesa_error (ctx, GL_INVALID_OPERATION, "glUniformMatrix3fvARB");
			_mesa_free (trans);
		}
		else
		{
			if (!(**pro).WriteUniform (pro, location, count, value, GL_FLOAT_MAT3))
				_mesa_error (ctx, GL_INVALID_OPERATION, "glUniformMatrix3fvARB");
		}
	}
}

GLvoid GLAPIENTRY
_mesa_UniformMatrix4fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniformMatrix4fvARB");

	if (value == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glUniformMatrix4fvARB");
		return;
	}

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (transpose)
		{
			GLfloat *trans, *pt;
			const GLfloat *pv;

			trans = (GLfloat *) _mesa_malloc (count * 16 * sizeof (GLfloat));
			if (trans == NULL)
			{
				_mesa_error (ctx, GL_OUT_OF_MEMORY, "glUniformMatrix4fvARB");
				return;
			}
			for (pt = trans, pv = value; pt != trans + count * 16; pt += 16, pv += 16)
			{
				_math_transposef (pt, pv);
			}
			if (!(**pro).WriteUniform (pro, location, count, trans, GL_FLOAT_MAT4))
				_mesa_error (ctx, GL_INVALID_OPERATION, "glUniformMatrix4fvARB");
			_mesa_free (trans);
		}
		else
		{
			if (!(**pro).WriteUniform (pro, location, count, value, GL_FLOAT_MAT4))
				_mesa_error (ctx, GL_INVALID_OPERATION, "glUniformMatrix4fvARB");
		}
	}
}

static GLboolean
_mesa_get_object_parameter (GLhandleARB obj, GLenum pname, GLvoid *params, GLboolean *integral,
	GLint *size)
{
	GET_CURRENT_CONTEXT(ctx);
	GLint *ipar = (GLint *) params;

	/* set default values */
	*integral = GL_TRUE;	/* indicates param type, TRUE: GLint, FALSE: GLfloat */
	*size = 1;				/* param array size */

	switch (pname)
	{
	case GL_OBJECT_TYPE_ARB:
	case GL_OBJECT_DELETE_STATUS_ARB:
	case GL_OBJECT_INFO_LOG_LENGTH_ARB:
		{
			GET_GENERIC(gen, obj, "glGetObjectParameterivARB");

			if (gen == NULL)
				return GL_FALSE;

			switch (pname)
			{
			case GL_OBJECT_TYPE_ARB:
				*ipar = (**gen).GetType (gen);
				break;
			case GL_OBJECT_DELETE_STATUS_ARB:
				*ipar = (**gen).GetDeleteStatus (gen);
				break;
         case GL_OBJECT_INFO_LOG_LENGTH_ARB:
            *ipar = (**gen).GetInfoLogLength (gen);
            break;
			}

			RELEASE_GENERIC(gen);
		}
		break;
	case GL_OBJECT_SUBTYPE_ARB:
	case GL_OBJECT_COMPILE_STATUS_ARB:
	case GL_OBJECT_SHADER_SOURCE_LENGTH_ARB:
		{
			GET_SHADER(sha, obj, "glGetObjectParameterivARB");

			if (sha == NULL)
				return GL_FALSE;

			switch (pname)
			{
			case GL_OBJECT_SUBTYPE_ARB:
				*ipar = (**sha).GetSubType (sha);
				break;
			case GL_OBJECT_COMPILE_STATUS_ARB:
				*ipar = (**sha).GetCompileStatus (sha);
				break;
			case GL_OBJECT_SHADER_SOURCE_LENGTH_ARB:
				{
					const GLcharARB *src = (**sha).GetSource (sha);

					if (src == NULL)
						*ipar = 0;
					else
						*ipar = _mesa_strlen (src) + 1;
				}
				break;
			}

			RELEASE_SHADER(sha);
		}
		break;
	case GL_OBJECT_LINK_STATUS_ARB:
	case GL_OBJECT_VALIDATE_STATUS_ARB:
	case GL_OBJECT_ATTACHED_OBJECTS_ARB:
	case GL_OBJECT_ACTIVE_UNIFORMS_ARB:
	case GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB:
		{
			GET_PROGRAM(pro, obj, "glGetObjectParameterivARB");

			if (pro == NULL)
				return GL_FALSE;

			switch (pname)
			{
			case GL_OBJECT_LINK_STATUS_ARB:
				*ipar = (**pro).GetLinkStatus (pro);
				break;
			case GL_OBJECT_VALIDATE_STATUS_ARB:
				*ipar = (**pro).GetValidateStatus (pro);
				break;
         case GL_OBJECT_ATTACHED_OBJECTS_ARB:
            *ipar = (**pro)._container.GetAttachedCount ((struct gl2_container_intf **) (pro));
            break;
			case GL_OBJECT_ACTIVE_UNIFORMS_ARB:
				*ipar = (**pro).GetActiveUniformCount (pro);
				break;
			case GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB:
				*ipar = (**pro).GetActiveUniformMaxLength (pro);
				break;
			case GL_OBJECT_ACTIVE_ATTRIBUTES_ARB:
				*ipar = (**pro).GetActiveAttribCount (pro);
				break;
			case GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB:
				*ipar = (**pro).GetActiveAttribMaxLength (pro);
				break;
			}

			RELEASE_PROGRAM(pro);
		}
		break;
	default:
		_mesa_error (ctx, GL_INVALID_ENUM, "glGetObjectParameterivARB");
		return GL_FALSE;
	}

	return GL_TRUE;
}

GLvoid GLAPIENTRY
_mesa_GetObjectParameterfvARB (GLhandleARB obj, GLenum pname, GLfloat *params)
{
	GET_CURRENT_CONTEXT(ctx);
	GLboolean integral;
	GLint size;

	if (params == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetObjectParameterfvARB");
		return;
	}

	assert (sizeof (GLfloat) == sizeof (GLint));

	if (_mesa_get_object_parameter (obj, pname, (GLvoid *) params, &integral, &size))
		if (integral)
		{
			GLint i;

			for (i = 0; i < size; i++)
				params[i] = (GLfloat) ((GLint *) params)[i];
		}
}

GLvoid GLAPIENTRY
_mesa_GetObjectParameterivARB (GLhandleARB obj, GLenum pname, GLint *params)
{
	GET_CURRENT_CONTEXT(ctx);
	GLboolean integral;
	GLint size;

	if (params == NULL)
	{
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetObjectParameterivARB");
		return;
	}

	assert (sizeof (GLfloat) == sizeof (GLint));

	if (_mesa_get_object_parameter (obj, pname, (GLvoid *) params, &integral, &size))
		if (!integral)
		{
			GLint i;

			for (i = 0; i < size; i++)
				params[i] = (GLint) ((GLfloat *) params)[i];
		}
}


/**
 * Copy string from <src> to <dst>, up to maxLength characters, returning
 * length of <dst> in <length>.
 * \param src  the strings source
 * \param maxLength  max chars to copy
 * \param length  returns numberof chars copied
 * \param dst  the string destination
 */
static GLvoid
copy_string(const GLcharARB *src, GLsizei maxLength, GLsizei *length,
            GLcharARB *dst)
{
	GLsizei len;
	for (len = 0; len < maxLength - 1 && src && src[len]; len++)
		dst[len] = src[len];
	if (maxLength > 0)
		dst[len] = 0;
	if (length)
		*length = len;
}


GLvoid GLAPIENTRY
_mesa_GetInfoLogARB (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_GENERIC(gen, obj, "glGetInfoLogARB");

	if (gen == NULL)
		return;

	if (infoLog == NULL)
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetInfoLogARB");
   else {
      GLsizei actualsize = (**gen).GetInfoLogLength (gen);
      if (actualsize > maxLength)
         actualsize = maxLength;
		(**gen).GetInfoLog (gen, actualsize, infoLog);
      if (length != NULL)
         *length = (actualsize > 0) ? actualsize - 1 : 0;
   }
	RELEASE_GENERIC(gen);
}

GLvoid GLAPIENTRY
_mesa_GetAttachedObjectsARB (GLhandleARB containerObj, GLsizei maxCount, GLsizei *count,
							 GLhandleARB *obj)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CONTAINER(con, containerObj, "glGetAttachedObjectsARB");

	if (con == NULL)
		return;

	if (obj == NULL)
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetAttachedObjectsARB");
	else
	{
		GLsizei cnt, i;

		cnt = (**con).GetAttachedCount (con);
		if (cnt > maxCount)
			cnt = maxCount;
		if (count != NULL)
			*count = cnt;

		for (i = 0; i < cnt; i++)
		{
         struct gl2_generic_intf **x = (**con).GetAttached (con, i);
			obj[i] = (**x).GetName (x);
			RELEASE_GENERIC(x);
		}
	}
	RELEASE_CONTAINER(con);
}

GLint GLAPIENTRY
_mesa_GetUniformLocationARB (GLhandleARB programObj, const GLcharARB *name)
{
	GET_CURRENT_CONTEXT(ctx);
	GLint loc = -1;
	GET_LINKED_PROGRAM(pro, programObj, "glGetUniformLocationARB");

	if (pro == NULL)
		return -1;

	if (name == NULL)
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetUniformLocationARB");
	else
	{
		if (!IS_NAME_WITH_GL_PREFIX(name))
			loc = (**pro).GetUniformLocation (pro, name);
	}
	RELEASE_PROGRAM(pro);
	return loc;
}

GLvoid GLAPIENTRY
_mesa_GetActiveUniformARB (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length,
	GLint *size, GLenum *type, GLcharARB *name)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_PROGRAM(pro, programObj, "glGetActiveUniformARB");

	if (pro == NULL)
		return;

	if (size == NULL || type == NULL || name == NULL)
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetActiveUniformARB");
	else
	{
		if (index < (**pro).GetActiveUniformCount (pro))
			(**pro).GetActiveUniform (pro, index, maxLength, length, size, type, name);
		else
			_mesa_error (ctx, GL_INVALID_VALUE, "glGetActiveUniformARB");
	}
	RELEASE_PROGRAM(pro);
}

GLvoid GLAPIENTRY
_mesa_GetUniformfvARB (GLhandleARB programObj, GLint location, GLfloat *params)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_LINKED_PROGRAM(pro, programObj, "glGetUniformfvARB");

	if (pro != NULL)
	{
		/* TODO */
		RELEASE_PROGRAM(pro);
	}
}

GLvoid GLAPIENTRY
_mesa_GetUniformivARB (GLhandleARB programObj, GLint location, GLint *params)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_LINKED_PROGRAM(pro, programObj, "glGetUniformivARB");

	if (pro != NULL)
	{
		/* TODO */
		RELEASE_PROGRAM(pro);
	}
}

GLvoid GLAPIENTRY
_mesa_GetShaderSourceARB (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_SHADER(sha, obj, "glGetShaderSourceARB");

	if (sha == NULL)
		return;

	if (source == NULL)
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetShaderSourceARB");
	else
		copy_string ((**sha).GetSource (sha), maxLength, length, source);
	RELEASE_SHADER(sha);
}

/* GL_ARB_vertex_shader */

GLvoid GLAPIENTRY
_mesa_BindAttribLocationARB (GLhandleARB programObj, GLuint index, const GLcharARB *name)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_PROGRAM(pro, programObj, "glBindAttribLocationARB");

	if (pro == NULL)
		return;

	if (name == NULL || index >= MAX_VERTEX_ATTRIBS)
		_mesa_error (ctx, GL_INVALID_VALUE, "glBindAttribLocationARB");
	else if (IS_NAME_WITH_GL_PREFIX(name))
		_mesa_error (ctx, GL_INVALID_OPERATION, "glBindAttribLocationARB");
	else
		(**pro).OverrideAttribBinding (pro, index, name);
	RELEASE_PROGRAM(pro);
}

GLvoid GLAPIENTRY
_mesa_GetActiveAttribARB (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length,
                          GLint *size, GLenum *type, GLcharARB *name)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_PROGRAM(pro, programObj, "glGetActiveAttribARB");

	if (pro == NULL)
		return;

	if (name == NULL || index >= (**pro).GetActiveAttribCount (pro))
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetActiveAttribARB");
	else
		(**pro).GetActiveAttrib (pro, index, maxLength, length, size, type, name);
	RELEASE_PROGRAM(pro);
}

GLint GLAPIENTRY
_mesa_GetAttribLocationARB (GLhandleARB programObj, const GLcharARB *name)
{
	GET_CURRENT_CONTEXT(ctx);
	GLint loc = -1;
	GET_LINKED_PROGRAM(pro, programObj, "glGetAttribLocationARB");

	if (pro == NULL)
		return -1;

	if (name == NULL)
		_mesa_error (ctx, GL_INVALID_VALUE, "glGetAttribLocationARB");
	else if (!IS_NAME_WITH_GL_PREFIX(name))
		loc = (**pro).GetAttribLocation (pro, name);
	RELEASE_PROGRAM(pro);
	return loc;
}

#endif

GLvoid
_mesa_init_shaderobjects (GLcontext *ctx)
{
	ctx->ShaderObjects.CurrentProgram = NULL;
	ctx->ShaderObjects._FragmentShaderPresent = GL_FALSE;
	ctx->ShaderObjects._VertexShaderPresent = GL_FALSE;

	_mesa_init_shaderobjects_3dlabs (ctx);
}

