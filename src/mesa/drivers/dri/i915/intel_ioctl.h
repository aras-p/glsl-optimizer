/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 **************************************************************************/

#ifndef INTEL_IOCTL_H
#define INTEL_IOCTL_H

#include "intel_context.h"

extern void intelWaitAgeLocked( intelContextPtr intel, int age, GLboolean unlock );

extern void intelClear(GLcontext *ctx, GLbitfield mask, GLboolean all,
		       GLint cx, GLint cy, GLint cw, GLint ch);

extern void intelPageFlip( const __DRIdrawablePrivate *dpriv );
extern void intelWaitForIdle( intelContextPtr intel );
extern void intelFlushBatch( intelContextPtr intel, GLboolean refill );
extern void intelFlushBatchLocked( intelContextPtr intel,
				   GLboolean ignore_cliprects,
				   GLboolean refill,
				   GLboolean allow_unlock);
extern void intelRefillBatchLocked( intelContextPtr intel, GLboolean allow_unlock );
extern void intelFinish( GLcontext *ctx );
extern void intelFlush( GLcontext *ctx );

extern void *intelAllocateAGP( intelContextPtr intel, GLsizei size );
extern void intelFreeAGP( intelContextPtr intel, void *pointer );

extern void *intelAllocateMemoryMESA( __DRInativeDisplay *dpy, int scrn, 
				      GLsizei size, GLfloat readfreq,
				      GLfloat writefreq, GLfloat priority );

extern void intelFreeMemoryMESA( __DRInativeDisplay *dpy, int scrn, 
				 GLvoid *pointer );

extern GLuint intelGetMemoryOffsetMESA( __DRInativeDisplay *dpy, int scrn, const GLvoid *pointer );
extern GLboolean intelIsAgpMemory( intelContextPtr intel, const GLvoid *pointer,
				  GLint size );

extern GLuint intelAgpOffsetFromVirtual( intelContextPtr intel, const GLvoid *p );


#endif
