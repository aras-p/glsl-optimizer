#ifndef _I810_SPAN_H
#define _I810_SPAN_H

#include "drirenderbuffer.h"

extern void i810InitSpanFuncs( GLcontext *ctx );

extern void i810SpanRenderFinish( GLcontext *ctx );
extern void i810SpanRenderStart( GLcontext *ctx );

extern void
i810SetSpanFunctions(driRenderbuffer *rb, const struct gl_config *vis);

#endif
