#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "program.h"
#include "r300_context.h"
#include "nvvertprog.h"

static void r300BindProgram(GLcontext *ctx, GLenum target, struct program *prog)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_vertex_program *vp=(void *)prog;
	
	switch(target){
		case GL_VERTEX_PROGRAM_ARB:
			//rmesa->current_vp = vp;
		break;
		default:
			WARN_ONCE("Target not supported yet!\n");
		break;
	}
}

/* Mesa doesnt seem to have prototype for this */
struct program *
_mesa_init_ati_fragment_shader( GLcontext *ctx, struct ati_fragment_shader *prog,
                           GLenum target, GLuint id);

static struct program *r300NewProgram(GLcontext *ctx, GLenum target, GLuint id)
{
	struct r300_vertex_program *vp;
	struct fragment_program *fp;
	struct ati_fragment_shader *afs;
	
	switch(target){
		case GL_VERTEX_PROGRAM_ARB:
			vp=CALLOC_STRUCT(r300_vertex_program);
		return _mesa_init_vertex_program(ctx, &vp->mesa_program, target, id);
		
		case GL_FRAGMENT_PROGRAM_ARB:
			fp=CALLOC_STRUCT(fragment_program);
		return _mesa_init_fragment_program(ctx, fp, target, id);
		
		case GL_FRAGMENT_PROGRAM_NV:
			fp=CALLOC_STRUCT(fragment_program);
		return _mesa_init_fragment_program(ctx, fp, target, id);
		
		case GL_FRAGMENT_SHADER_ATI:
			afs=CALLOC_STRUCT(ati_fragment_shader);
		return _mesa_init_ati_fragment_shader(ctx, afs, target, id);
	}
	
	return NULL;	
}


static void r300DeleteProgram(GLcontext *ctx, struct program *prog)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_vertex_program *vp=(void *)prog;
	
	/*if(rmesa->current_vp == vp)
		rmesa->current_vp = NULL;*/
	
	_mesa_delete_program(ctx, prog);
}
     
void r300ProgramStringNotify(GLcontext *ctx, GLenum target, 
				struct program *prog)
{
	struct r300_vertex_program *vp=(void *)prog;
		
	switch(target) {
	case GL_VERTEX_PROGRAM_ARB:
		vp->translated=GL_FALSE;
		translate_vertex_shader(vp);
	break;
	
	case GL_FRAGMENT_PROGRAM_ARB:
	break;
	}
}

static GLboolean r300IsProgramNative(GLcontext *ctx, GLenum target, struct program *prog)
{
	struct r300_vertex_program *vp=(void *)prog;
	r300ContextPtr rmesa = R300_CONTEXT(ctx);

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
