#ifndef _I810_STATE_H
#define _I810_STATE_H

#include "i810context.h"

extern void i810InitState( struct gl_context *ctx );
extern void i810InitStateFuncs( struct gl_context *ctx );
extern void i810PrintDirty( const char *msg, GLuint state );
extern void i810DrawBuffer(struct gl_context *ctx, GLenum mode );

extern void i810Fallback( i810ContextPtr imesa, GLuint bit, GLboolean mode );
#define FALLBACK( imesa, bit, mode ) i810Fallback( imesa, bit, mode )


#endif
