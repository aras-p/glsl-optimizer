
#include "main/glheader.h"

#include "shader/program.h"
#include "tnl/tnl.h"
#include "r600_context.h"
#include "r600_fragprog.h"

static struct gl_program *r600NewProgram(GLcontext * ctx, GLenum target,
					 GLuint id)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	struct r600_vertex_program_cont *vp;
	struct r600_fragment_program *r600_fp;

	switch (target) {
	case GL_VERTEX_STATE_PROGRAM_NV:
	case GL_VERTEX_PROGRAM_ARB:
		vp = CALLOC_STRUCT(r600_vertex_program_cont);
		return _mesa_init_vertex_program(ctx, &vp->mesa_program,
						 target, id);
	case GL_FRAGMENT_PROGRAM_ARB:
		r600_fp = CALLOC_STRUCT(r600_fragment_program);
		return _mesa_init_fragment_program(ctx, &r600_fp->mesa_program,
						   target, id);
	case GL_FRAGMENT_PROGRAM_NV:
		r600_fp = CALLOC_STRUCT(r600_fragment_program);
		return _mesa_init_fragment_program(ctx, &r600_fp->mesa_program,
						   target, id);
	default:
		_mesa_problem(ctx, "Bad target in r600NewProgram");
	}

	return NULL;
}

static void r600DeleteProgram(GLcontext * ctx, struct gl_program *prog)
{
	_mesa_delete_program(ctx, prog);
}

static void
r600ProgramStringNotify(GLcontext * ctx, GLenum target, struct gl_program *prog)
{
	r600ContextPtr rmesa = R600_CONTEXT(ctx);
	struct r600_vertex_program_cont *vp = (void *)prog;
	struct r600_fragment_program *r600_fp = (struct r600_fragment_program *)prog;

	switch (target) {
	case GL_VERTEX_PROGRAM_ARB:
		vp->progs = NULL;
		break;
	case GL_FRAGMENT_PROGRAM_ARB:
		r600_fp->translated = GL_FALSE;
		break;
	}

	/* need this for tcl fallbacks */
	_tnl_program_string(ctx, target, prog);
}

static GLboolean
r600IsProgramNative(GLcontext * ctx, GLenum target, struct gl_program *prog)
{
	return GL_TRUE;
}

void r600InitShaderFuncs(struct dd_function_table *functions)
{
	functions->NewProgram = r600NewProgram;
	functions->DeleteProgram = r600DeleteProgram;
	functions->ProgramStringNotify = r600ProgramStringNotify;
	functions->IsProgramNative = r600IsProgramNative;
}
