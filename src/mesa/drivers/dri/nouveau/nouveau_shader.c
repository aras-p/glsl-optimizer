#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "program.h"
#include "nouveau_context.h"
#include "nouveau_shader.h"

static struct program *
nv40NewProgram(GLcontext *ctx, GLenum target, GLuint id)
{
}

static void
nv40BindProgram(GLcontext *ctx, GLenum target, struct program *prog)
{
}

static void
nv40DeleteProgram(GLcontext *ctx, struct program *prog)
{
}

static void
nv40ProgramStringNotify(GLcontext *ctx, GLenum target,
		                struct program *prog)
{
}

static GLboolean
nv40IsProgramNative(GLcontext *ctx, GLenum target, struct program *prog)
{
}

void
nouveauInitShaderFuncs(GLcontext *ctx)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);

	if (nmesa->screen->card_type == NV_40) {
		ctx->Driver.NewProgram          = nv40NewProgram;
		ctx->Driver.BindProgram         = nv40BindProgram;
		ctx->Driver.DeleteProgram       = nv40DeleteProgram;
		ctx->Driver.ProgramStringNotify = nv40ProgramStringNotify;
		ctx->Driver.IsProgramNative     = nv40IsProgramNative;
	}
}

#define LONGBITS (sizeof(long) * 8)
void
nvsBitSet(long *rec, int bit)
{
	int ri = bit / LONGBITS;
	int rb = bit % LONGBITS;

	rec[ri] |= (1 << rb);
}

void
nvsBitClear(long *rec, int bit)
{
	int ri = bit / LONGBITS;
	int rb = bit % LONGBITS;

	rec[ri] &= ~(1 << rb);
}

void
nvsRecInit(long **rec, int max)
{
	int c = (max / LONGBITS) + ((max % LONGBITS) ? 1 : 0);
	*rec = calloc(c, sizeof(long));
}

int
nvsAllocIndex(long *rec, int max)
{
	int c = (max / LONGBITS) + ((max % LONGBITS) ? 1 : 0);
	int i, idx = 0;

	for (i=0;i<c;i++) {
		idx = ffsl(~rec[i]);
		if (idx) {
			nvsBitSet(rec, (idx - 1));
			break;
		}
	}

	return (idx - 1);
}

