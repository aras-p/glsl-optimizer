#ifndef _I810_SPAN_H
#define _I810_SPAN_H

#include "drirenderbuffer.h"

extern void i810InitSpanFuncs( struct gl_context *ctx );

extern void i810SpanRenderFinish( struct gl_context *ctx );
extern void i810SpanRenderStart( struct gl_context *ctx );

extern void
i810SetSpanFunctions(driRenderbuffer *rb, const struct gl_config *vis);

#endif
