/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 2005  Brian Paul   All Rights Reserved.
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
 * \file shaderobjects_3dlabs.c
 * shader objects definitions for 3dlabs compiler
 * \author Michal Krol
 */


#include "glheader.h"
#include "shaderobjects.h"
#include "shaderobjects_3dlabs.h"
#include "context.h"
#include "macros.h"
#include "hash.h"


struct gl2_unknown_obj
{
	GLuint reference_count;
	void (* _destructor) (struct gl2_unknown_intf **);
};

struct gl2_unknown_impl
{
	struct gl2_unknown_intf *_vftbl;
	struct gl2_unknown_obj _obj;
};

static void
_unknown_destructor (struct gl2_unknown_intf **intf)
{
}

static void
_unknown_AddRef (struct gl2_unknown_intf **intf)
{
	struct gl2_unknown_impl *impl = (struct gl2_unknown_impl *) intf;

	impl->_obj.reference_count++;
}

static void
_unknown_Release (struct gl2_unknown_intf **intf)
{
	struct gl2_unknown_impl *impl = (struct gl2_unknown_impl *) intf;

	impl->_obj.reference_count--;
	if (impl->_obj.reference_count == 0)
	{
		impl->_obj._destructor (intf);
		_mesa_free ((void *) intf);
	}
}

static struct gl2_unknown_intf **
_unknown_QueryInterface (struct gl2_unknown_intf **intf, enum gl2_uiid uiid)
{
	if (uiid == UIID_UNKNOWN)
	{
		(**intf).AddRef (intf);
		return intf;
	}
	return NULL;
}

static struct gl2_unknown_intf _unknown_vftbl = {
	_unknown_AddRef,
	_unknown_Release,
	_unknown_QueryInterface
};

static void
_unknown_constructor (struct gl2_unknown_impl *impl)
{
	impl->_vftbl = &_unknown_vftbl;
	impl->_obj.reference_count = 1;
	impl->_obj._destructor = _unknown_destructor;
}

struct gl2_generic_obj
{
	struct gl2_unknown_obj _unknown;
	GLhandleARB name;
	GLboolean delete_status;
	GLcharARB *info_log;
};

struct gl2_generic_impl
{
	struct gl2_generic_intf *_vftbl;
	struct gl2_generic_obj _obj;
};

static void
_generic_destructor (struct gl2_unknown_intf **intf)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_generic_impl *impl = (struct gl2_generic_impl *) intf;

	_mesa_free ((void *) impl->_obj.info_log);

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	_mesa_HashRemove (ctx->Shared->GL2Objects, impl->_obj.name);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);

	_unknown_destructor (intf);
}

static struct gl2_unknown_intf **
_generic_QueryInterface (struct gl2_unknown_intf **intf, enum gl2_uiid uiid)
{
	if (uiid == UIID_GENERIC)
	{
		(**intf).AddRef (intf);
		return intf;
	}
	return _unknown_QueryInterface (intf, uiid);
}

static void
_generic_Delete (struct gl2_generic_intf **intf)
{
	struct gl2_generic_impl *impl = (struct gl2_generic_impl *) intf;

	if (impl->_obj.delete_status == GL_FALSE)
	{
		impl->_obj.delete_status = GL_TRUE;
		(**intf)._unknown.Release ((struct gl2_unknown_intf **) intf);
	}
}

static GLhandleARB
_generic_GetName (struct gl2_generic_intf **intf)
{
	struct gl2_generic_impl *impl = (struct gl2_generic_impl *) intf;

	return impl->_obj.name;
}

static GLboolean
_generic_GetDeleteStatus (struct gl2_generic_intf **intf)
{
	struct gl2_generic_impl *impl = (struct gl2_generic_impl *) intf;

	return impl->_obj.delete_status;
}

static const GLcharARB *
_generic_GetInfoLog (struct gl2_generic_intf **intf)
{
	struct gl2_generic_impl *impl = (struct gl2_generic_impl *) intf;

	return impl->_obj.info_log;
}

static struct gl2_generic_intf _generic_vftbl = {
	{
		_unknown_AddRef,
		_unknown_Release,
		_generic_QueryInterface
	},
	_generic_Delete,
	NULL,
	_generic_GetName,
	_generic_GetDeleteStatus,
	_generic_GetInfoLog
};

static void
_generic_constructor (struct gl2_generic_impl *impl)
{
	GET_CURRENT_CONTEXT(ctx);

	_unknown_constructor ((struct gl2_unknown_impl *) impl);
	impl->_vftbl = &_generic_vftbl;
	impl->_obj._unknown._destructor = _generic_destructor;
	impl->_obj.delete_status = GL_FALSE;
	impl->_obj.info_log = NULL;

	_glthread_LOCK_MUTEX (ctx->Shared->Mutex);
	impl->_obj.name = _mesa_HashFindFreeKeyBlock (ctx->Shared->GL2Objects, 1);
	_mesa_HashInsert (ctx->Shared->GL2Objects, impl->_obj.name, (void *) impl);
	_glthread_UNLOCK_MUTEX (ctx->Shared->Mutex);
}

struct gl2_container_obj
{
	struct gl2_generic_obj _generic;
	struct gl2_generic_intf ***attached;
	GLuint attached_count;
};

struct gl2_container_impl
{
	struct gl2_container_intf *_vftbl;
	struct gl2_container_obj _obj;
};

static void
_container_destructor (struct gl2_unknown_intf **intf)
{
	struct gl2_container_impl *impl = (struct gl2_container_impl *) intf;
	GLuint i;

	for (i = 0; i < impl->_obj.attached_count; i++)
	{
		struct gl2_generic_intf **x = impl->_obj.attached[i];
		(**x)._unknown.Release ((struct gl2_unknown_intf **) x);
	}

	_generic_destructor (intf);
}

static struct gl2_unknown_intf **
_container_QueryInterface (struct gl2_unknown_intf **intf, enum gl2_uiid uiid)
{
	if (uiid == UIID_CONTAINER)
	{
		(**intf).AddRef (intf);
		return intf;
	}
	return _generic_QueryInterface (intf, uiid);
}

static GLboolean
_container_Attach (struct gl2_container_intf **intf, struct gl2_generic_intf **att)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_container_impl *impl = (struct gl2_container_impl *) intf;
	GLuint i;

	for (i = 0; i < impl->_obj.attached_count; i++)
		if (impl->_obj.attached[i] == att)
		{
			_mesa_error (ctx, GL_INVALID_OPERATION, "_container_Attach");
			return GL_FALSE;
		}

	impl->_obj.attached = (struct gl2_generic_intf ***) _mesa_realloc (impl->_obj.attached,
		impl->_obj.attached_count * sizeof (*impl->_obj.attached), (impl->_obj.attached_count + 1) *
		sizeof (*impl->_obj.attached));
	if (impl->_obj.attached == NULL)
		return GL_FALSE;

	impl->_obj.attached[impl->_obj.attached_count] = att;
	impl->_obj.attached_count++;
	(**att)._unknown.AddRef ((struct gl2_unknown_intf **) att);
	return GL_TRUE;
}

static GLboolean
_container_Detach (struct gl2_container_intf **intf, struct gl2_generic_intf **att)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_container_impl *impl = (struct gl2_container_impl *) intf;
	GLuint i, j;

	for (i = 0; i < impl->_obj.attached_count; i++)
		if (impl->_obj.attached[i] == att)
		{
			for (j = i; j < impl->_obj.attached_count - 1; j++)
				impl->_obj.attached[j] = impl->_obj.attached[j + 1];
			impl->_obj.attached = (struct gl2_generic_intf ***) _mesa_realloc (impl->_obj.attached,
				impl->_obj.attached_count * sizeof (*impl->_obj.attached),
				(impl->_obj.attached_count - 1) * sizeof (*impl->_obj.attached));
			impl->_obj.attached_count--;
			(**att)._unknown.Release ((struct gl2_unknown_intf **) att);
			return GL_TRUE;
		}

	_mesa_error (ctx, GL_INVALID_OPERATION, "_container_Detach");
	return GL_FALSE;
}

static GLsizei
_container_GetAttachedCount (struct gl2_container_intf **intf)
{
	struct gl2_container_impl *impl = (struct gl2_container_impl *) intf;

	return impl->_obj.attached_count;
}

static struct gl2_generic_intf **
_container_GetAttached (struct gl2_container_intf **intf, GLuint index)
{
	struct gl2_container_impl *impl = (struct gl2_container_impl *) intf;

	(**impl->_obj.attached[index])._unknown.AddRef (
		(struct gl2_unknown_intf **)impl->_obj.attached[index]);
	return impl->_obj.attached[index];
}

static struct gl2_container_intf _container_vftbl = {
   {
      {
	_unknown_AddRef,
	_unknown_Release,
	_container_QueryInterface,
      },
	_generic_Delete,
	NULL,
	_generic_GetName,
	_generic_GetDeleteStatus,
	_generic_GetInfoLog,
   },
	_container_Attach,
	_container_Detach,
	_container_GetAttachedCount,
	_container_GetAttached
};

static void
_container_constructor (struct gl2_container_impl *impl)
{
	_generic_constructor ((struct gl2_generic_impl *) impl);
	impl->_vftbl = &_container_vftbl;
	impl->_obj._generic._unknown._destructor = _container_destructor;
	impl->_obj.attached = NULL;
	impl->_obj.attached_count = 0;
}

struct gl2_shader_obj
{
	struct gl2_generic_obj _generic;
	GLboolean compile_status;
	GLcharARB *source;
	GLint *offsets;
	GLsizei offset_count;
};

struct gl2_shader_impl
{
	struct gl2_shader_intf *_vftbl;
	struct gl2_shader_obj _obj;
};

static void
_shader_destructor (struct gl2_unknown_intf **intf)
{
	struct gl2_shader_impl *impl = (struct gl2_shader_impl *) intf;

	_mesa_free ((void *) impl->_obj.source);
	_mesa_free ((void *) impl->_obj.offsets);
	_generic_destructor (intf);
}

static struct gl2_unknown_intf **
_shader_QueryInterface (struct gl2_unknown_intf **intf, enum gl2_uiid uiid)
{
	if (uiid == UIID_SHADER)
	{
		(**intf).AddRef (intf);
		return intf;
	}
	return _generic_QueryInterface (intf, uiid);
}

static GLenum
_shader_GetType (struct gl2_generic_intf **intf)
{
	return GL_SHADER_OBJECT_ARB;
}

static GLboolean
_shader_GetCompileStatus (struct gl2_shader_intf **intf)
{
	struct gl2_shader_impl *impl = (struct gl2_shader_impl *) intf;

	return impl->_obj.compile_status;
}

static GLvoid
_shader_SetSource (struct gl2_shader_intf **intf, GLcharARB *src, GLint *off, GLsizei cnt)
{
	struct gl2_shader_impl *impl = (struct gl2_shader_impl *) intf;

	_mesa_free ((void *) impl->_obj.source);
	impl->_obj.source = src;
	_mesa_free ((void *) impl->_obj.offsets);
	impl->_obj.offsets = off;
	impl->_obj.offset_count = cnt;
}

static const GLcharARB *
_shader_GetSource (struct gl2_shader_intf **intf)
{
	struct gl2_shader_impl *impl = (struct gl2_shader_impl *) intf;

	return impl->_obj.source;
}

static GLvoid
_shader_Compile (struct gl2_shader_intf **intf)
{
	struct gl2_shader_impl *impl = (struct gl2_shader_impl *) intf;

	impl->_obj.compile_status = GL_FALSE;
	_mesa_free ((void *) impl->_obj._generic.info_log);
	impl->_obj._generic.info_log = NULL;

	/* TODO compile */
}

static struct gl2_shader_intf _shader_vftbl = {
   {
      {
	_unknown_AddRef,
	_unknown_Release,
	_shader_QueryInterface,
      },
	_generic_Delete,
	_shader_GetType,
	_generic_GetName,
	_generic_GetDeleteStatus,
	_generic_GetInfoLog,
   },
	NULL,
	_shader_GetCompileStatus,
	_shader_SetSource,
	_shader_GetSource,
	_shader_Compile
};

static void
_shader_constructor (struct gl2_shader_impl *impl)
{
	_generic_constructor ((struct gl2_generic_impl *) impl);
	impl->_vftbl = &_shader_vftbl;
	impl->_obj._generic._unknown._destructor = _shader_destructor;
	impl->_obj.compile_status = GL_FALSE;
	impl->_obj.source = NULL;
	impl->_obj.offsets = NULL;
	impl->_obj.offset_count = 0;
}

struct gl2_program_obj
{
	struct gl2_container_obj _container;
	GLboolean link_status;
	GLboolean validate_status;
};

struct gl2_program_impl
{
	struct gl2_program_intf *_vftbl;
	struct gl2_program_obj _obj;
};

static void
_program_destructor (struct gl2_unknown_intf **intf)
{
	struct gl2_program_impl *impl = (struct gl2_program_impl *) intf;
	(void) impl;
	_container_destructor (intf);
}

static struct gl2_unknown_intf **
_program_QueryInterface (struct gl2_unknown_intf **intf, enum gl2_uiid uiid)
{
	if (uiid == UIID_PROGRAM)
	{
		(**intf).AddRef (intf);
		return intf;
	}
	return _container_QueryInterface (intf, uiid);
}

static GLenum
_program_GetType (struct gl2_generic_intf **intf)
{
	return GL_PROGRAM_OBJECT_ARB;
}

static GLboolean
_program_Attach (struct gl2_container_intf **intf, struct gl2_generic_intf **att)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_unknown_intf **sha;

	sha = (**att)._unknown.QueryInterface ((struct gl2_unknown_intf **) att, UIID_SHADER);
	if (sha == NULL)
	{
		_mesa_error (ctx, GL_INVALID_OPERATION, "_program_Attach");
		return GL_FALSE;
	}

	(**sha).Release (sha);
	return _container_Attach (intf, att);
}

static GLboolean
_program_GetLinkStatus (struct gl2_program_intf **intf)
{
	struct gl2_program_impl *impl = (struct gl2_program_impl *) intf;

	return impl->_obj.link_status;
}

static GLboolean
_program_GetValidateStatus (struct gl2_program_intf **intf)
{
	struct gl2_program_impl *impl = (struct gl2_program_impl *) intf;

	return impl->_obj.validate_status;
}

static GLvoid
_program_Link (struct gl2_program_intf **intf)
{
	struct gl2_program_impl *impl = (struct gl2_program_impl *) intf;

	impl->_obj.link_status = GL_FALSE;
	_mesa_free ((void *) impl->_obj._container._generic.info_log);
	impl->_obj._container._generic.info_log = NULL;

	/* TODO link */
}

static GLvoid
_program_Validate (struct gl2_program_intf **intf)
{
	struct gl2_program_impl *impl = (struct gl2_program_impl *) intf;

	impl->_obj.validate_status = GL_FALSE;

	/* TODO validate */
}

static struct gl2_program_intf _program_vftbl = {
	{
		{
			{
				_unknown_AddRef,
				_unknown_Release,
				_program_QueryInterface,
			},
			_generic_Delete,
			_program_GetType,
			_generic_GetName,
			_generic_GetDeleteStatus,
			_generic_GetInfoLog,
		},
		_program_Attach,
		_container_Detach,
		_container_GetAttachedCount,
		_container_GetAttached,
	},
	_program_GetLinkStatus,
	_program_GetValidateStatus,
	_program_Link,
	_program_Validate
};

static void
_program_constructor (struct gl2_program_impl *impl)
{
	_container_constructor ((struct gl2_container_impl *) impl);
	impl->_vftbl = &_program_vftbl;
	impl->_obj._container._generic._unknown._destructor = _program_destructor;
	impl->_obj.link_status = GL_FALSE;
	impl->_obj.validate_status = GL_FALSE;
}

struct gl2_fragment_shader_obj
{
	struct gl2_shader_obj _shader;
};

struct gl2_fragment_shader_impl
{
	struct gl2_fragment_shader_intf *_vftbl;
	struct gl2_fragment_shader_obj _obj;
};

static void
_fragment_shader_destructor (struct gl2_unknown_intf **intf)
{
	struct gl2_fragment_shader_impl *impl = (struct gl2_fragment_shader_impl *) intf;
	(void) impl;
	/* TODO free fragment shader data */

	_shader_destructor (intf);
}

static struct gl2_unknown_intf **
_fragment_shader_QueryInterface (struct gl2_unknown_intf **intf, enum gl2_uiid uiid)
{
	if (uiid == UIID_FRAGMENT_SHADER)
	{
		(**intf).AddRef (intf);
		return intf;
	}
	return _shader_QueryInterface (intf, uiid);
}

static GLenum
_fragment_shader_GetSubType (struct gl2_shader_intf **intf)
{
	return GL_FRAGMENT_SHADER_ARB;
}

static struct gl2_fragment_shader_intf _fragment_shader_vftbl = {
	{
		{
			{
				_unknown_AddRef,
				_unknown_Release,
				_fragment_shader_QueryInterface,
			},
			_generic_Delete,
			_shader_GetType,
			_generic_GetName,
			_generic_GetDeleteStatus,
			_generic_GetInfoLog,
		},
		_fragment_shader_GetSubType,
		_shader_GetCompileStatus,
		_shader_SetSource,
		_shader_GetSource,
		_shader_Compile
	}
};

static void
_fragment_shader_constructor (struct gl2_fragment_shader_impl *impl)
{
	_shader_constructor ((struct gl2_shader_impl *) impl);
	impl->_vftbl = &_fragment_shader_vftbl;
	impl->_obj._shader._generic._unknown._destructor = _fragment_shader_destructor;
}

struct gl2_vertex_shader_obj
{
	struct gl2_shader_obj _shader;
};

struct gl2_vertex_shader_impl
{
	struct gl2_vertex_shader_intf *_vftbl;
	struct gl2_vertex_shader_obj _obj;
};

static void
_vertex_shader_destructor (struct gl2_unknown_intf **intf)
{
	struct gl2_vertex_shader_impl *impl = (struct gl2_vertex_shader_impl *) intf;
	(void) impl;
	/* TODO free vertex shader data */

	_shader_destructor (intf);
}

static struct gl2_unknown_intf **
_vertex_shader_QueryInterface (struct gl2_unknown_intf **intf, enum gl2_uiid uiid)
{
	if (uiid == UIID_VERTEX_SHADER)
	{
		(**intf).AddRef (intf);
		return intf;
	}
	return _shader_QueryInterface (intf, uiid);
}

static GLenum
_vertex_shader_GetSubType (struct gl2_shader_intf **intf)
{
	return GL_VERTEX_SHADER_ARB;
}

static struct gl2_vertex_shader_intf _vertex_shader_vftbl = {
	{
		{
			{
				_unknown_AddRef,
				_unknown_Release,
				_vertex_shader_QueryInterface,
			},
			_generic_Delete,
			_shader_GetType,
			_generic_GetName,
			_generic_GetDeleteStatus,
			_generic_GetInfoLog,
		},
		_vertex_shader_GetSubType,
		_shader_GetCompileStatus,
		_shader_SetSource,
		_shader_GetSource,
		_shader_Compile
	}
};

static void
_vertex_shader_constructor (struct gl2_vertex_shader_impl *impl)
{
	_shader_constructor ((struct gl2_shader_impl *) impl);
	impl->_vftbl = &_vertex_shader_vftbl;
	impl->_obj._shader._generic._unknown._destructor = _vertex_shader_destructor;
}

GLhandleARB
_mesa_3dlabs_create_shader_object (GLenum shaderType)
{
	GET_CURRENT_CONTEXT(ctx);
	(void) ctx;
	switch (shaderType)
	{
	case GL_FRAGMENT_SHADER_ARB:
		{
			struct gl2_fragment_shader_impl *x = (struct gl2_fragment_shader_impl *) _mesa_malloc (
				sizeof (struct gl2_fragment_shader_impl));

			if (x != NULL)
			{
				_fragment_shader_constructor (x);
				return x->_obj._shader._generic.name;
			}
		}
		break;
	case GL_VERTEX_SHADER_ARB:
		{
			struct gl2_vertex_shader_impl *x = (struct gl2_vertex_shader_impl *) _mesa_malloc (
				sizeof (struct gl2_vertex_shader_impl));

			if (x != NULL)
			{
				_vertex_shader_constructor (x);
				return x->_obj._shader._generic.name;
			}
		}
		break;
	}

	return 0;
}

GLhandleARB
_mesa_3dlabs_create_program_object (void)
{
	GET_CURRENT_CONTEXT(ctx);
	struct gl2_program_impl *x = (struct gl2_program_impl *)
		_mesa_malloc (sizeof (struct gl2_program_impl));

	(void) ctx;
	if (x != NULL)
	{
		_program_constructor (x);
		return x->_obj._container._generic.name;
	}

	return 0;
}

