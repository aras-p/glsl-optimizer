/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 2004-2005  Brian Paul   All Rights Reserved.
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


#define I_UNKNOWN struct gl2_unknown_intf **
#define I_GENERIC struct gl2_generic_intf **
#define I_CONTAINER struct gl2_container_intf **
#define I_PROGRAM struct gl2_program_intf **
#define I_SHADER struct gl2_shader_intf **

#define RELEASE_GENERIC(x)\
	(**x)._unknown.Release ((I_UNKNOWN) x)

#define RELEASE_CONTAINER(x)\
	(**x)._generic._unknown.Release ((I_UNKNOWN) x)

#define RELEASE_PROGRAM(x)\
	(**x)._container._generic._unknown.Release ((I_UNKNOWN) x)

#define RELEASE_SHADER(x)\
	(**x)._generic._unknown.Release ((I_UNKNOWN) x);

#define _LOOKUP_HANDLE(handle, function)\
	I_UNKNOWN unk;\
	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);\
	unk = (I_UNKNOWN) _mesa_HashLookup (ctx->Shared->GL2Objects, handle);\
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);\
	if (unk == NULL) {\
		_mesa_error (ctx, GL_INVALID_VALUE, function);\
		break;\
	}

#define _QUERY_INTERFACE(x, type, uuid, function)\
	x = (type) (**unk).QueryInterface (unk, uuid);\
	if (x == NULL) {\
		_mesa_error (ctx, GL_INVALID_OPERATION, function);\
		break;\
	}

#define GET_GENERIC(x, handle, function)\
	I_GENERIC x = NULL;\
	do {\
		_LOOKUP_HANDLE(handle, function);\
		_QUERY_INTERFACE(x, I_GENERIC, UIID_GENERIC, function);\
	} while (0)

#define GET_CONTAINER(x, handle, function)\
	I_CONTAINER x = NULL;\
	do {\
		_LOOKUP_HANDLE(handle, function);\
		_QUERY_INTERFACE(x, I_CONTAINER, UIID_CONTAINER, function);\
	} while (0)

#define GET_PROGRAM(x, handle, function)\
	I_PROGRAM x = NULL;\
	do {\
		_LOOKUP_HANDLE(handle, function);\
		_QUERY_INTERFACE(x, I_PROGRAM, UIID_PROGRAM, function);\
	} while (0)

#define GET_SHADER(x, handle, function)\
	I_SHADER x = NULL;\
	do {\
		_LOOKUP_HANDLE(handle, function);\
		_QUERY_INTERFACE(x, I_SHADER, UIID_SHADER, function);\
	} while (0)

#define _LINKED_PROGRAM(x, function)\
	if ((**x).GetLinkStatus (x) == GL_FALSE) {\
		RELEASE_PROGRAM(x);\
		_mesa_error (ctx, GL_INVALID_OPERATION, function);\
		break;\
	}

#define GET_LINKED_PROGRAM(x, handle, function)\
	I_PROGRAM x = NULL;\
	do {\
		_LOOKUP_HANDLE(handle, function);\
		_QUERY_INTERFACE(x, I_PROGRAM, UIID_PROGRAM, function);\
		_LINKED_PROGRAM(x, function);\
	} while (0)

#define _CURRENT_PROGRAM(x, function)\
	if (ctx->ShaderObjects.CurrentProgram == NULL) {\
		_mesa_error (ctx, GL_INVALID_OPERATION, function);\
		break;\
	}\
	x = ctx->ShaderObjects.CurrentProgram;

#define GET_CURRENT_LINKED_PROGRAM(x, function)\
	I_PROGRAM x = NULL;\
	do {\
		_CURRENT_PROGRAM(x, function);\
		_LINKED_PROGRAM(x, function);\
	} while (0)


GLvoid GLAPIENTRY
_mesa_DeleteObjectARB (GLhandleARB obj)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_GENERIC(gen, obj, "glDeleteObjectARB");

	if (gen != NULL)
	{
		(**gen).Delete (gen);
		RELEASE_GENERIC(gen);
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
			I_PROGRAM pro = ctx->ShaderObjects.CurrentProgram;

			if (pro != NULL)
				return (**pro)._container._generic.GetName ((I_GENERIC) pro);
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
		if (pro == ctx->ShaderObjects.CurrentProgram)
		{
			/* TODO re-install executable program */
		}
		(**pro).Link (pro);
		RELEASE_PROGRAM(pro);
	}
}

GLvoid GLAPIENTRY
_mesa_UseProgramObjectARB (GLhandleARB programObj)
{
	GET_CURRENT_CONTEXT(ctx);
	I_PROGRAM program = NULL;

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
		if (!_slang_write_uniform (pro, location, 1, &v0, GL_FLOAT))
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
		GLfloat v[2] = { v0, v1 };

		if (!_slang_write_uniform (pro, location, 1, v, GL_FLOAT_VEC2))
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
		GLfloat v[3] = { v0, v1, v2 };

		if (!_slang_write_uniform (pro, location, 1, v, GL_FLOAT_VEC3))
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
		GLfloat v[4] = { v0, v1, v2, v3 };

		if (!_slang_write_uniform (pro, location, 1, v, GL_FLOAT_VEC4))
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
		if (!_slang_write_uniform (pro, location, 1, &v0, GL_INT))
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
		GLint v[2] = { v0, v1 };

		if (!_slang_write_uniform (pro, location, 1, v, GL_INT_VEC2))
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
		GLint v[3] = { v0, v1, v2 };

		if (!_slang_write_uniform (pro, location, 1, v, GL_INT_VEC3))
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
		GLint v[4] = { v0, v1, v2, v3 };

		if (!_slang_write_uniform (pro, location, 1, v, GL_INT_VEC4))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform4iARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform1fvARB (GLint location, GLsizei count, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform1fvARB");

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!_slang_write_uniform (pro, location, count, value, GL_FLOAT))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform1fvARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform2fvARB (GLint location, GLsizei count, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform2fvARB");

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!_slang_write_uniform (pro, location, count, value, GL_FLOAT_VEC2))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform2fvARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform3fvARB (GLint location, GLsizei count, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform3fvARB");

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!_slang_write_uniform (pro, location, count, value, GL_FLOAT_VEC3))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform3fvARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform4fvARB (GLint location, GLsizei count, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform4fvARB");

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!_slang_write_uniform (pro, location, count, value, GL_FLOAT_VEC4))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform4fvARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform1ivARB (GLint location, GLsizei count, const GLint *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform1ivARB");

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!_slang_write_uniform (pro, location, count, value, GL_INT))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform1ivARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform2ivARB (GLint location, GLsizei count, const GLint *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform2ivARB");

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!_slang_write_uniform (pro, location, count, value, GL_INT_VEC2))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform2ivARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform3ivARB (GLint location, GLsizei count, const GLint *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform3ivARB");

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!_slang_write_uniform (pro, location, count, value, GL_INT_VEC3))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform3ivARB");
	}
}

GLvoid GLAPIENTRY
_mesa_Uniform4ivARB (GLint location, GLsizei count, const GLint *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniform4ivARB");

	FLUSH_VERTICES(ctx, _NEW_PROGRAM);

	if (pro != NULL)
	{
		if (!_slang_write_uniform (pro, location, count, value, GL_INT_VEC4))
			_mesa_error (ctx, GL_INVALID_OPERATION, "glUniform4ivARB");
	}
}

GLvoid GLAPIENTRY
_mesa_UniformMatrix2fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniformMatrix2fvARB");

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
			if (!_slang_write_uniform (pro, location, count, trans, GL_FLOAT_MAT2))
				_mesa_error (ctx, GL_INVALID_OPERATION, "glUniformMatrix2fvARB");
			_mesa_free (trans);
		}
		else
		{
			if (!_slang_write_uniform (pro, location, count, value, GL_FLOAT_MAT2))
				_mesa_error (ctx, GL_INVALID_OPERATION, "glUniformMatrix2fvARB");
		}
	}
}

GLvoid GLAPIENTRY
_mesa_UniformMatrix3fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniformMatrix3fvARB");

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
			if (!_slang_write_uniform (pro, location, count, trans, GL_FLOAT_MAT3))
				_mesa_error (ctx, GL_INVALID_OPERATION, "glUniformMatrix3fvARB");
			_mesa_free (trans);
		}
		else
		{
			if (!_slang_write_uniform (pro, location, count, value, GL_FLOAT_MAT3))
				_mesa_error (ctx, GL_INVALID_OPERATION, "glUniformMatrix3fvARB");
		}
	}
}

GLvoid GLAPIENTRY
_mesa_UniformMatrix4fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_CURRENT_LINKED_PROGRAM(pro, "glUniformMatrix4fvARB");

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
			if (!_slang_write_uniform (pro, location, count, trans, GL_FLOAT_MAT4))
				_mesa_error (ctx, GL_INVALID_OPERATION, "glUniformMatrix4fvARB");
			_mesa_free (trans);
		}
		else
		{
			if (!_slang_write_uniform (pro, location, count, value, GL_FLOAT_MAT4))
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
				{
					const GLcharARB *info = (**gen).GetInfoLog (gen);

					if (info == NULL)
						*ipar = 0;
					else
						*ipar = _mesa_strlen (info) + 1;
				}
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
				*ipar = (**pro)._container.GetAttachedCount ((I_CONTAINER) pro);
				break;
			case GL_OBJECT_ACTIVE_UNIFORMS_ARB:
				*ipar = _slang_get_active_uniform_count (pro);
				break;
			case GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB:
				*ipar = _slang_get_active_uniform_max_length (pro);
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
	GLboolean integral;
	GLint size;

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
	GLboolean integral;
	GLint size;

	assert (sizeof (GLfloat) == sizeof (GLint));

	if (_mesa_get_object_parameter (obj, pname, (GLvoid *) params, &integral, &size))
		if (!integral)
		{
			GLint i;

			for (i = 0; i < size; i++)
				params[i] = (GLint) ((GLfloat *) params)[i];
		}
}

static GLvoid
_mesa_get_string (const GLcharARB *src, GLsizei maxLength, GLsizei *length, GLcharARB *str)
{
	GLsizei len;

	if (maxLength == 0)
	{
		if (length != NULL)
			*length = 0;
		return;
	}

	if (src == NULL)
		src = "";

	len = _mesa_strlen (src);
	if (len >= maxLength)
		len = maxLength - 1;

	_mesa_memcpy (str, src, len * sizeof (GLcharARB));
	str[len] = '\0';
	if (length != NULL)
		*length = len;
}

GLvoid GLAPIENTRY
_mesa_GetInfoLogARB (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_GENERIC(gen, obj, "glGetInfoLogARB");

	if (gen != NULL)
	{
		_mesa_get_string ((**gen).GetInfoLog (gen), maxLength, length, infoLog);
		RELEASE_GENERIC(gen);
	}
}

GLvoid GLAPIENTRY
_mesa_GetAttachedObjectsARB (GLhandleARB containerObj, GLsizei maxCount, GLsizei *count,
							 GLhandleARB *obj)
{
	GET_CURRENT_CONTEXT(ctx);
	GLsizei cnt, i;
	GET_CONTAINER(con, containerObj, "glGetAttachedObjectsARB");

	if (con != NULL)
	{
		cnt = (**con).GetAttachedCount (con);
		if (cnt > maxCount)
			cnt = maxCount;
		if (count != NULL)
			*count = cnt;

		for (i = 0; i < cnt; i++)
		{
			I_GENERIC x = (**con).GetAttached (con, i);
			obj[i] = (**x).GetName (x);
			RELEASE_GENERIC(x);
		}
		RELEASE_CONTAINER(con);
	}
}

GLint GLAPIENTRY
_mesa_GetUniformLocationARB (GLhandleARB programObj, const GLcharARB *name)
{
	GET_CURRENT_CONTEXT(ctx);
	GLint loc = -1;
	GET_LINKED_PROGRAM(pro, programObj, "glGetUniformLocationARB");

	if (pro != NULL)
	{
		if (name != NULL && (name[0] != 'g' || name[1] != 'l' || name[2] != '_'))
			loc = _slang_get_uniform_location (pro, name);
		RELEASE_PROGRAM(pro);
	}
	return loc;
}

GLvoid GLAPIENTRY
_mesa_GetActiveUniformARB (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length,
	GLint *size, GLenum *type, GLcharARB *name)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_PROGRAM(pro, programObj, "glGetActiveUniformARB");

	if (pro != NULL)
	{
		if (index < _slang_get_active_uniform_count (pro))
			_slang_get_active_uniform (pro, index, maxLength, length, size, type, name);
		else
			_mesa_error (ctx, GL_INVALID_VALUE, "glGetActiveUniformARB");
		RELEASE_PROGRAM(pro);
	}
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

	if (sha != NULL)
	{
		_mesa_get_string ((**sha).GetSource (sha), maxLength, length, source);
		RELEASE_SHADER(sha);
	}
}

/* GL_ARB_vertex_shader */

GLvoid GLAPIENTRY
_mesa_BindAttribLocationARB (GLhandleARB programObj, GLuint index, const GLcharARB *name)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_PROGRAM(pro, programObj, "glBindAttribLocationARB");

	if (pro != NULL)
	{
		if (name != NULL && (name[0] != 'g' || name[1] != 'l' || name[2] != '_'))
		{
			/* TODO */
		}
		RELEASE_PROGRAM(pro);
	}
}

GLvoid GLAPIENTRY
_mesa_GetActiveAttribARB (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length,
						  GLint *size, GLenum *type, GLcharARB *name)
{
	GET_CURRENT_CONTEXT(ctx);
	GET_PROGRAM(pro, programObj, "glGetActiveAttribARB");

	if (pro != NULL)
	{
		/* TODO */
		RELEASE_PROGRAM(pro);
	}
}

GLint GLAPIENTRY
_mesa_GetAttribLocationARB (GLhandleARB programObj, const GLcharARB *name)
{
	GET_CURRENT_CONTEXT(ctx);
	GLint loc = -1;
	GET_PROGRAM(pro, programObj, "glGetAttribLocationARB");

	if (pro != NULL)
	{
		if (name != NULL && (name[0] != 'g' || name[1] != 'l' || name[2] != '_'))
		{
			/* TODO */
		}
		RELEASE_PROGRAM(pro);
	}
	return loc;
}

GLvoid
_mesa_init_shaderobjects (GLcontext *ctx)
{
	ctx->ShaderObjects.CurrentProgram = NULL;

	_mesa_init_shaderobjects_3dlabs (ctx);
}

