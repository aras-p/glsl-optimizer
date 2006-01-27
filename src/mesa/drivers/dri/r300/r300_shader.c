#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "program.h"
#include "r300_context.h"
#include "program_instruction.h"
#if USE_ARB_F_P == 1
#include "r300_fragprog.h"
#endif

static void r300BindProgram(GLcontext *ctx, GLenum target, struct program *prog)
{
	
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_vertex_program *vp=(void *)prog;
	
	
	switch(target){
		case GL_VERTEX_PROGRAM_ARB:
		rmesa->curr_vp = vp;
		vp->ref_count++;
#if 0
		if((vp->ref_count % 1500) == 0) {
			fprintf(stderr, "id %p, ref_count %d\n", vp, vp->ref_count);
			_mesa_print_program(&vp->mesa_program.Base);
		}
#endif
		
#if USE_ARB_F_P == 1
		case GL_FRAGMENT_PROGRAM_ARB:
#endif
			//rmesa->current_vp = vp;
		break;
		default:
			WARN_ONCE("Target not supported yet!\n");
		break;
	}
}

static struct program *r300NewProgram(GLcontext *ctx, GLenum target, GLuint id)
{
	struct r300_vertex_program *vp;
#if USE_ARB_F_P == 1
	struct r300_fragment_program *fp;
#else
	struct fragment_program *fp;
#endif
	struct ati_fragment_shader *afs;
	
	switch(target){
		case GL_VERTEX_STATE_PROGRAM_NV:
		case GL_VERTEX_PROGRAM_ARB:
			vp=CALLOC_STRUCT(r300_vertex_program);
			return _mesa_init_vertex_program(ctx, &vp->mesa_program, target, id);
		case GL_FRAGMENT_PROGRAM_ARB:
#if USE_ARB_F_P == 1
			fp=CALLOC_STRUCT(r300_fragment_program);
			fp->ctx = ctx;
			return _mesa_init_fragment_program(ctx, &fp->mesa_program, target, id);
#else
			fp=CALLOC_STRUCT(fragment_program);
			return _mesa_init_fragment_program(ctx, fp, target, id);
#endif	
		case GL_FRAGMENT_PROGRAM_NV:
#if USE_ARB_F_P == 1
			fp=CALLOC_STRUCT(r300_fragment_program);
			return _mesa_init_fragment_program(ctx, &fp->mesa_program, target, id);
#else
			fp=CALLOC_STRUCT(fragment_program);
			return _mesa_init_fragment_program(ctx, fp, target, id);
#endif
#if 00
                /* _mesa_new_ati_fragment_shader() is now called instead */
		case GL_FRAGMENT_SHADER_ATI:
			afs=CALLOC_STRUCT(ati_fragment_shader);
			return _mesa_init_ati_fragment_shader(ctx, afs, target, id);
#endif
		default:
			_mesa_problem(ctx, "Bad target in r300NewProgram");
	}
	
	return NULL;	
}


static void r300DeleteProgram(GLcontext *ctx, struct program *prog)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_vertex_program *vp=(void *)prog;
	
	/*if(rmesa->curr_vp == vp)
		rmesa->curr_vp = NULL;*/
	
	_mesa_delete_program(ctx, prog);
}

static void r300ProgramStringNotify(GLcontext *ctx, GLenum target, 
				struct program *prog)
{
	struct r300_vertex_program *vp=(void *)prog;
#if USE_ARB_F_P == 1
	struct r300_fragment_program *fp = (struct r300_fragment_program *) prog;
#endif
	
	switch(target) {
	case GL_VERTEX_PROGRAM_ARB:
		vp->translated = GL_FALSE;
		memset(&vp->translated, 0, sizeof(struct r300_vertex_program) - sizeof(struct vertex_program));
		/*translate_vertex_shader(vp);*/
	break;
	case GL_FRAGMENT_PROGRAM_ARB:
#if USE_ARB_F_P == 1
		fp->translated = GL_FALSE;
#endif
	break;
	}
}

static GLboolean r300IsProgramNative(GLcontext *ctx, GLenum target, struct program *prog)
{
	//struct r300_vertex_program *vp=(void *)prog;
	//r300ContextPtr rmesa = R300_CONTEXT(ctx);

	return 1;
}

void r300InitShaderFuncs(struct dd_function_table *functions)
{
	functions->NewProgram=r300NewProgram;
	functions->BindProgram=r300BindProgram;
	functions->DeleteProgram=r300DeleteProgram;
	functions->ProgramStringNotify=r300ProgramStringNotify;
	functions->IsProgramNative=r300IsProgramNative;
}
