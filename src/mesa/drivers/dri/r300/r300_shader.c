/*
 * Copyright 2009 Maciej Cencora <m.cencora@gmail.com>
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "main/glheader.h"

#include "shader/program.h"
#include "tnl/tnl.h"
#include "r300_context.h"
#include "r300_fragprog_common.h"

static void freeFragProgCache(GLcontext *ctx, struct r300_fragment_program_cont *cache)
{
	struct r300_fragment_program *tmp, *fp = cache->progs;

	while (fp) {
		tmp = fp->next;
		rc_constants_destroy(&fp->code.constants);
		free(fp);
		fp = tmp;
	}
}

static void freeVertProgCache(GLcontext *ctx, struct r300_vertex_program_cont *cache)
{
	struct r300_vertex_program *tmp, *vp = cache->progs;

	while (vp) {
		tmp = vp->next;
		rc_constants_destroy(&vp->code.constants);
		_mesa_reference_vertprog(ctx, &vp->Base, NULL);
		free(vp);
		vp = tmp;
	}
}

static struct gl_program *r300NewProgram(GLcontext * ctx, GLenum target,
					 GLuint id)
{
	struct r300_vertex_program_cont *vp;
	struct r300_fragment_program_cont *fp;

	switch (target) {
	case GL_VERTEX_STATE_PROGRAM_NV:
	case GL_VERTEX_PROGRAM_ARB:
		vp = CALLOC_STRUCT(r300_vertex_program_cont);
		return _mesa_init_vertex_program(ctx, &vp->mesa_program, target, id);

	case GL_FRAGMENT_PROGRAM_NV:
	case GL_FRAGMENT_PROGRAM_ARB:
		fp = CALLOC_STRUCT(r300_fragment_program_cont);
		return _mesa_init_fragment_program(ctx, &fp->Base, target, id);

	default:
		_mesa_problem(ctx, "Bad target in r300NewProgram");
	}

	return NULL;
}

static void r300DeleteProgram(GLcontext * ctx, struct gl_program *prog)
{
	struct r300_vertex_program_cont *vp = (struct r300_vertex_program_cont *)prog;
	struct r300_fragment_program_cont *fp = (struct r300_fragment_program_cont *)prog;

	switch (prog->Target) {
		case GL_VERTEX_PROGRAM_ARB:
			freeVertProgCache(ctx, vp);
			break;
		case GL_FRAGMENT_PROGRAM_ARB:
			freeFragProgCache(ctx, fp);
			break;
	}

	_mesa_delete_program(ctx, prog);
}

static GLboolean
r300ProgramStringNotify(GLcontext * ctx, GLenum target, struct gl_program *prog)
{
	struct r300_vertex_program_cont *vp = (struct r300_vertex_program_cont *)prog;
	struct r300_fragment_program_cont *fp = (struct r300_fragment_program_cont *)prog;

	switch (target) {
	case GL_VERTEX_PROGRAM_ARB:
		freeVertProgCache(ctx, vp);
		vp->progs = NULL;
		break;
	case GL_FRAGMENT_PROGRAM_ARB:
		freeFragProgCache(ctx, fp);
		fp->progs = NULL;
		break;
	}

	/* need this for tcl fallbacks */
	(void) _tnl_program_string(ctx, target, prog);

	/* XXX check if program is legal, within limits */
	return GL_TRUE;
}

static GLboolean
r300IsProgramNative(GLcontext * ctx, GLenum target, struct gl_program *prog)
{
	if (target == GL_FRAGMENT_PROGRAM_ARB) {
		struct r300_fragment_program *fp = r300SelectAndTranslateFragmentShader(ctx);

		return !fp->error;
	} else {
		struct r300_vertex_program *vp = r300SelectAndTranslateVertexShader(ctx);

		return !vp->error;
	}
}

void r300InitShaderFuncs(struct dd_function_table *functions)
{
	functions->NewProgram = r300NewProgram;
	functions->DeleteProgram = r300DeleteProgram;
	functions->ProgramStringNotify = r300ProgramStringNotify;
	functions->IsProgramNative = r300IsProgramNative;
}
