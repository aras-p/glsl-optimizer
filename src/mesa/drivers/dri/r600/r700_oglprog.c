/*
 * Copyright (C) 2008-2009  Advanced Micro Devices, Inc.
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
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Richard Li <RichardZ.Li@amd.com>, <richardradeon@gmail.com>
 */

#include <string.h>

#include "main/glheader.h"
#include "main/imports.h"

#include "shader/program.h"
#include "tnl/tnl.h"

#include "r600_context.h"
#include "r600_emit.h"

#include "r700_oglprog.h"
#include "r700_fragprog.h"
#include "r700_vertprog.h"


static void freeVertProgCache(GLcontext *ctx, struct r700_vertex_program_cont *cache)
{
	struct r700_vertex_program *tmp, *vp = cache->progs;

	while (vp) {
		tmp = vp->next;
		/* Release DMA region */
		r600DeleteShader(ctx, vp->shaderbo);
		/* Clean up */
		Clean_Up_Assembler(&(vp->r700AsmCode));
		Clean_Up_Shader(&(vp->r700Shader));
		
		_mesa_reference_vertprog(ctx, &vp->mesa_program, NULL);
		free(vp);
		vp = tmp;
	}
}

static struct gl_program *r700NewProgram(GLcontext * ctx, 
                                         GLenum target,
					                     GLuint id)
{
	struct gl_program *pProgram = NULL;

    struct r700_vertex_program_cont *vpc;
	struct r700_fragment_program *fp;

	radeon_print(RADEON_SHADER, RADEON_VERBOSE,
			"%s %u, %u\n", __func__, target, id);

    switch (target) 
    {
    case GL_VERTEX_STATE_PROGRAM_NV:
    case GL_VERTEX_PROGRAM_ARB:	    
        vpc       = CALLOC_STRUCT(r700_vertex_program_cont);
	    pProgram = _mesa_init_vertex_program(ctx, 
                                             &vpc->mesa_program,
					                         target, 
                                             id);
	    break;
    case GL_FRAGMENT_PROGRAM_NV:
    case GL_FRAGMENT_PROGRAM_ARB:
		fp       = CALLOC_STRUCT(r700_fragment_program);
		pProgram = _mesa_init_fragment_program(ctx, 
                                               &fp->mesa_program,
						                       target, 
                                               id);
        fp->translated = GL_FALSE;
        fp->loaded     = GL_FALSE;

        fp->shaderbo   = NULL;

	    break;
    default:
	    _mesa_problem(ctx, "Bad target in r700NewProgram");
    }

	return pProgram;
}

static void r700DeleteProgram(GLcontext * ctx, struct gl_program *prog)
{
    struct r700_vertex_program_cont *vpc = (struct r700_vertex_program_cont *)prog;
    struct r700_fragment_program * fp;

	radeon_print(RADEON_SHADER, RADEON_VERBOSE,
			"%s %p\n", __func__, prog);

    switch (prog->Target) 
    {
    case GL_VERTEX_STATE_PROGRAM_NV:
    case GL_VERTEX_PROGRAM_ARB:	    
	    freeVertProgCache(ctx, vpc);
	    break;
    case GL_FRAGMENT_PROGRAM_NV:
    case GL_FRAGMENT_PROGRAM_ARB:
		fp = (struct r700_fragment_program*)prog;
        /* Release DMA region */

        r600DeleteShader(ctx, fp->shaderbo);

        /* Clean up */
        Clean_Up_Assembler(&(fp->r700AsmCode));
        Clean_Up_Shader(&(fp->r700Shader));
	    break;
    default:
	    _mesa_problem(ctx, "Bad target in r700NewProgram");
    }

	_mesa_delete_program(ctx, prog);
}

static GLboolean
r700ProgramStringNotify(GLcontext * ctx, GLenum target, struct gl_program *prog)
{
	struct r700_vertex_program_cont *vpc = (struct r700_vertex_program_cont *)prog;
	struct r700_fragment_program * fp = (struct r700_fragment_program*)prog;

	switch (target) {
	case GL_VERTEX_PROGRAM_ARB:
		freeVertProgCache(ctx, vpc);
		vpc->progs = NULL;
		break;
	case GL_FRAGMENT_PROGRAM_ARB:
		r600DeleteShader(ctx, fp->shaderbo);
		Clean_Up_Assembler(&(fp->r700AsmCode));
		Clean_Up_Shader(&(fp->r700Shader));
		fp->translated = GL_FALSE;
		fp->loaded     = GL_FALSE;
		fp->shaderbo   = NULL;
		break;
	}
		
	/* XXX check if program is legal, within limits */
	return GL_TRUE;
}

static GLboolean r700IsProgramNative(GLcontext * ctx, GLenum target, struct gl_program *prog)
{

	return GL_TRUE;
}

void r700InitShaderFuncs(struct dd_function_table *functions)
{
	functions->NewProgram = r700NewProgram;
	functions->DeleteProgram = r700DeleteProgram;
	functions->ProgramStringNotify = r700ProgramStringNotify;
	functions->IsProgramNative = r700IsProgramNative;
}
