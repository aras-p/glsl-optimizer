#ifndef __R300_VERTPROG_H_
#define __R300_VERTPROG_H_

#include "r300_reg.h"


void r300SetupVertexProgram(r300ContextPtr rmesa);

struct r300_vertex_program * r300SelectAndTranslateVertexShader(struct gl_context *ctx);

#endif
