/* $XFree86: xc/lib/GL/glx/packsingle.h,v 1.5tsi Exp $ */
#ifndef __GLX_packsingle_h__
#define __GLX_packsingle_h__

/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
*/

#include "packrender.h"

/*
** The macros in this header convert wire protocol data types to the client
** machine's native data types.  The header is part of the porting layer of
** the client library, and it is intended that hardware vendors will rewrite
** this header to suit their own machines.
*/

/*
** Dummy define to make the GetReqExtra macro happy.  The value is not
** used, but instead the code in __GLX_SINGLE_BEGIN issues its own store
** to req->reqType with the proper code (our extension code).
*/
#define X_GLXSingle 0

/* Declare common variables used during a single command */
#define __GLX_SINGLE_DECLARE_VARIABLES()	 \
    __GLXcontext *gc = __glXGetCurrentContext(); \
    GLubyte *pc, *pixelHeaderPC;		 \
    GLuint compsize, cmdlen;			 \
    Display *dpy = gc->currentDpy;		 \
    xGLXSingleReq *req

#define __GLX_SINGLE_LOAD_VARIABLES() \
    pc = gc->pc;           \
    /* Muffle compilers */		     \
    pixelHeaderPC = 0;  (void)pixelHeaderPC; \
    compsize = 0;       (void)compsize;	     \
    cmdlen = 0;         (void)cmdlen

/* Start a single command */
#define __GLX_SINGLE_BEGIN(opcode,bytes)	   \
    if (dpy) {					   \
	(void) __glXFlushRenderBuffer(gc, pc);	   \
	LockDisplay(dpy);			   \
	GetReqExtra(GLXSingle,bytes,req);	   \
	req->reqType = gc->majorOpcode;		   \
	req->glxCode = opcode;			   \
	req->contextTag = gc->currentContextTag;   \
	pc = ((GLubyte *)(req) + sz_xGLXSingleReq)

/* End a single command */
#define __GLX_SINGLE_END()			   \
	UnlockDisplay(dpy);			   \
	SyncHandle();				   \
    }

/* Store data to sending for a single command */
#define __GLX_SINGLE_PUT_CHAR(offset,a)	\
    *((INT8 *) (pc + offset)) = a

#ifndef CRAY
#define __GLX_SINGLE_PUT_SHORT(offset,a) \
    *((INT16 *) (pc + offset)) = a

#define __GLX_SINGLE_PUT_LONG(offset,a)	\
    *((INT32 *) (pc + offset)) = a

#define __GLX_SINGLE_PUT_FLOAT(offset,a) \
    *((FLOAT32 *) (pc + offset)) = a

#else
#define __GLX_SINGLE_PUT_SHORT(offset,a) \
    { GLubyte *cp = (pc+offset); \
      int shift = (64-16) - ((int)(cp) >> (64-6)); \
      *(int *)cp = (*(int *)cp & ~(0xffff << shift)) | ((a & 0xffff) << shift); }

#define __GLX_SINGLE_PUT_LONG(offset,a) \
    { GLubyte *cp = (pc+offset); \
      int shift = (64-32) - ((int)(cp) >> (64-6)); \
      *(int *)cp = (*(int *)cp & ~(0xffffffff << shift)) | ((a & 0xffffffff) << shift); }

#define __GLX_SINGLE_PUT_FLOAT(offset,a) \
    gl_put_float(pc + offset, a)
#endif

/* Read support macros */
#define __GLX_SINGLE_READ_XREPLY()		    \
    (void) _XReply(dpy, (xReply*) &reply, 0, False)

#define __GLX_SINGLE_GET_RETVAL(a,cast)	\
    a = (cast) reply.retval

#define __GLX_SINGLE_GET_SIZE(a) \
    a = (GLint) reply.size

#ifndef _CRAY
#define __GLX_SINGLE_GET_CHAR(p) \
    *p = *(GLbyte *)&reply.pad3;

#define __GLX_SINGLE_GET_SHORT(p) \
    *p = *(GLshort *)&reply.pad3;

#define __GLX_SINGLE_GET_LONG(p) \
    *p = *(GLint *)&reply.pad3;

#define __GLX_SINGLE_GET_FLOAT(p) \
    *p = *(GLfloat *)&reply.pad3;

#else
#define __GLX_SINGLE_GET_CHAR(p) \
    *p = reply.pad3 >> 24;

#define __GLX_SINGLE_GET_SHORT(p) \
    {int t = reply.pad3 >> 16; \
     *p = (t & 0x8000) ? (t | ~0xffff) : (t & 0xffff);}

#define __GLX_SINGLE_GET_LONG(p) \
    {int t = reply.pad3; \
     *p = (t & 0x80000000) ? (t | ~0xffffffff) : (t & 0xffffffff);}

#define PAD3OFFSET 16
#define __GLX_SINGLE_GET_FLOAT(p) \
    *p = gl_ntoh_float((GLubyte *)&reply + PAD3OFFSET);

#define __GLX_SINGLE_GET_DOUBLE(p) \
    *p = gl_ntoh_double((GLubyte *)&reply + PAD3OFFSET);

extern float gl_ntoh_float(GLubyte *);
extern float gl_ntoh_double(GLubyte *);
#endif

#ifndef _CRAY

#ifdef __GLX_ALIGN64
#define __GLX_SINGLE_GET_DOUBLE(p) \
    __GLX_MEM_COPY(p, &reply.pad3, 8)
#else
#define __GLX_SINGLE_GET_DOUBLE(p) \
    *p = *(GLdouble *)&reply.pad3
#endif

#endif
	  
/* Get an array of typed data */
#define __GLX_SINGLE_GET_VOID_ARRAY(a,alen)	\
{						\
    GLint slop = alen*__GLX_SIZE_INT8 & 3;	\
    _XRead(dpy,(char *)a,alen*__GLX_SIZE_INT8);	\
    if (slop) _XEatData(dpy,4-slop);		\
}

#define __GLX_SINGLE_GET_CHAR_ARRAY(a,alen)	\
{						\
    GLint slop = alen*__GLX_SIZE_INT8 & 3;	\
    _XRead(dpy,(char *)a,alen*__GLX_SIZE_INT8);	\
    if (slop) _XEatData(dpy,4-slop);		\
}
					

#define __GLX_SINGLE_GET_SHORT_ARRAY(a,alen)	\
{						\
    GLint slop = (alen*__GLX_SIZE_INT16) & 3;	\
    _XRead(dpy,(char *)a,alen*__GLX_SIZE_INT16);\
    if (slop) _XEatData(dpy,4-slop);		\
}

#define __GLX_SINGLE_GET_LONG_ARRAY(a,alen) \
    _XRead(dpy,(char *)a,alen*__GLX_SIZE_INT32);  	

#ifndef _CRAY
#define __GLX_SINGLE_GET_FLOAT_ARRAY(a,alen) \
    _XRead(dpy,(char *)a,alen*__GLX_SIZE_FLOAT32);  	

#define __GLX_SINGLE_GET_DOUBLE_ARRAY(a,alen) \
    _XRead(dpy,(char *)a,alen*__GLX_SIZE_FLOAT64);  	

#else
#define __GLX_SINGLE_GET_FLOAT_ARRAY(a,alen) \
    gl_get_float_array(dpy,a,alen);

#define __GLX_SINGLE_GET_DOUBLE_ARRAY(a,alen) \
    gl_get_double_array(dpy, a, alen);

extern void gl_get_float_array(Display *dpy, float *a, int alen);
extern void gl_get_double_array(Display *dpy, double *a, int alen);
#endif

#endif /* !__GLX_packsingle_h__ */
