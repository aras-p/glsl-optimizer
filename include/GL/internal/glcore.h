#ifndef __gl_core_h_
#define __gl_core_h_

/* $XFree86: xc/lib/GL/include/GL/internal/glcore.h,v 1.5 1999/06/14 07:23:42 dawes Exp $ */
/*
** The contents of this file are subject to the GLX Public License Version 1.0
** (the "License"). You may not use this file except in compliance with the
** License. You may obtain a copy of the License at Silicon Graphics, Inc.,
** attn: Legal Services, 2011 N. Shoreline Blvd., Mountain View, CA 94043
** or at http://www.sgi.com/software/opensource/glx/license.html.
**
** Software distributed under the License is distributed on an "AS IS"
** basis. ALL WARRANTIES ARE DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY
** IMPLIED WARRANTIES OF MERCHANTABILITY, OF FITNESS FOR A PARTICULAR
** PURPOSE OR OF NON- INFRINGEMENT. See the License for the specific
** language governing rights and limitations under the License.
**
** The Original Software is GLX version 1.2 source code, released February,
** 1999. The developer of the Original Software is Silicon Graphics, Inc.
** Those portions of the Subject Software created by Silicon Graphics, Inc.
** are Copyright (c) 1991-9 Silicon Graphics, Inc. All Rights Reserved.
**
** $SGI$
*/

#ifndef XFree86LOADER
#include <sys/types.h>
#endif

#define GL_CORE_SGI  1
#define GL_CORE_MESA 2

typedef struct __GLcontextRec __GLcontext;
typedef struct __GLinterfaceRec __GLinterface;

/*
** This file defines the interface between the GL core and the surrounding
** "operating system" that supports it (currently the GLX or WGL extensions).
**
** Members (data and function pointers) are documented as imported or
** exported according to how they are used by the core rendering functions.
** Imported members are initialized by the "operating system" and used by
** the core functions.  Exported members are initialized by the core functions
** and used by the "operating system".
*/

/*
** Mode and limit information for a context.  This information is
** kept around in the context so that values can be used during
** command execution, and for returning information about the
** context to the application.
*/
typedef struct __GLcontextModesRec {
    GLboolean rgbMode;
    GLboolean colorIndexMode;
    GLboolean doubleBufferMode;
    GLboolean stereoMode;

    GLboolean haveAccumBuffer;
    GLboolean haveDepthBuffer;
    GLboolean haveStencilBuffer;

    GLint redBits, greenBits, blueBits, alphaBits;	/* bits per comp */
    GLuint redMask, greenMask, blueMask, alphaMask;
    GLint rgbBits;		/* total bits for rgb */
    GLint indexBits;		/* total bits for colorindex */

    GLint accumRedBits, accumGreenBits, accumBlueBits, accumAlphaBits;
    GLint depthBits;
    GLint stencilBits;

    GLint numAuxBuffers;

    GLint level;

    GLint pixmapMode;
} __GLcontextModes;

/************************************************************************/

/*
** Structure used for allocating and freeing drawable private memory.
** (like software buffers, for example).
**
** The memory allocation routines are provided by the surrounding
** "operating system" code, and they are to be used for allocating
** software buffers and things which are associated with the drawable,
** and used by any context which draws to that drawable.  There are
** separate memory allocation functions for drawables and contexts
** since drawables and contexts can be created and destroyed independently
** of one another, and the "operating system" may want to use separate
** allocation arenas for each.
**
** The freePrivate function is filled in by the core routines when they
** allocates software buffers, and stick them in "private".  The freePrivate
** function will destroy anything allocated to this drawable (to be called
** when the drawable is destroyed).
*/
typedef struct __GLdrawableRegionRec __GLdrawableRegion;
typedef struct __GLdrawableBufferRec __GLdrawableBuffer;
typedef struct __GLdrawablePrivateRec __GLdrawablePrivate;

typedef struct __GLregionRectRec {
    /* lower left (inside the rectangle) */
    GLint x0, y0;
    /* upper right (outside the rectangle) */
    GLint x1, y1;
} __GLregionRect;

struct __GLdrawableRegionRec {
    GLint numRects;
    __GLregionRect *rects;
    __GLregionRect boundingRect;
};

/************************************************************************/

/* masks for the buffers */
#define __GL_FRONT_BUFFER_MASK		0x00000001
#define	__GL_FRONT_LEFT_BUFFER_MASK	0x00000001
#define	__GL_FRONT_RIGHT_BUFFER_MASK	0x00000002
#define	__GL_BACK_BUFFER_MASK		0x00000004
#define __GL_BACK_LEFT_BUFFER_MASK	0x00000004
#define __GL_BACK_RIGHT_BUFFER_MASK	0x00000008
#define	__GL_ACCUM_BUFFER_MASK		0x00000010
#define	__GL_DEPTH_BUFFER_MASK		0x00000020
#define	__GL_STENCIL_BUFFER_MASK	0x00000040
#define	__GL_AUX_BUFFER_MASK(i)		(0x0000080 << (i))

#define __GL_ALL_BUFFER_MASK		0xffffffff

/* what Resize routines return if resize resorted to fallback case */
#define __GL_BUFFER_FALLBACK	0x10

typedef void (*__GLbufFallbackInitFn)(__GLdrawableBuffer *buf, 
				      __GLdrawablePrivate *glPriv, GLint bits);
typedef void (*__GLbufMainInitFn)(__GLdrawableBuffer *buf, 
				  __GLdrawablePrivate *glPriv, GLint bits,
				  __GLbufFallbackInitFn back);

/*
** A drawable buffer
**
** This data structure describes the context side of a drawable.  
**
** According to the spec there could be multiple contexts bound to the same
** drawable at the same time (from different threads).  In order to avoid
** multiple-access conflicts, locks are used to serialize access.  When a
** thread needs to access (read or write) a member of the drawable, it takes
** a lock first.  Some of the entries in the drawable are treated "mostly
** constant", so we take the freedom of allowing access to them without
** taking a lock (for optimization reasons).
**
** For more details regarding locking, see buffers.h in the GL core
*/
struct __GLdrawableBufferRec {
    /*
    ** Buffer dimensions
    */
    GLint width, height, depth;

    /*
    ** Framebuffer base address
    */
    void *base;

    /*
    ** Framebuffer size (in bytes)
    */
    GLuint size;

    /*
    ** Size (in bytes) of each element in the framebuffer
    */
    GLuint elementSize;
    GLuint elementSizeLog2;

    /*
    ** Element skip from one scanline to the next.
    ** If the buffer is part of another buffer (for example, fullscreen
    ** front buffer), outerWidth is the width of that buffer.
    */
    GLint outerWidth;

    /*
    ** outerWidth * elementSize
    */
    GLint byteWidth;

    /*
    ** Allocation/deallocation is done based on this handle.  A handle
    ** is conceptually different from the framebuffer 'base'.
    */
    void *handle;

    /* imported */
    GLboolean (*resize)(__GLdrawableBuffer *buf,
			GLint x, GLint y, GLuint width, GLuint height, 
			__GLdrawablePrivate *glPriv, GLuint bufferMask);
    void (*lock)(__GLdrawableBuffer *buf, __GLdrawablePrivate *glPriv);
    void (*unlock)(__GLdrawableBuffer *buf, __GLdrawablePrivate *glPriv);
    void (*fill)(__GLdrawableBuffer *buf, __GLdrawablePrivate *glPriv,
    		GLuint val, GLint x, GLint y, GLint w, GLint h);
    void (*free)(__GLdrawableBuffer *buf, __GLdrawablePrivate *glPriv);
    void *other;

    /* exported */
    void (*freePrivate)(__GLdrawableBuffer *buf, __GLdrawablePrivate *glPriv);
    void *private;

    /* private */
    __GLbufMainInitFn mainInit;
    __GLbufFallbackInitFn fallbackInit;
};

/*
** The context side of the drawable private
*/
struct __GLdrawablePrivateRec {
    /*
    ** Drawable Modes
    */
    __GLcontextModes *modes;

    /*
    ** Drawable size
    */
    GLuint width, height;

    /*
    ** Origin in screen coordinates of the drawable
    */
    GLint xOrigin, yOrigin;
#ifdef __GL_ALIGNED_BUFFERS
    /*
    ** Drawable offset from screen origin
    */
    GLint xOffset, yOffset;

    /*
    ** Alignment restriction
    */
    GLint xAlignment, yAlignment;
#endif
    /*
    ** Should we invert the y axis?
    */
    GLint yInverted;

    /*
    ** Mask specifying which buffers are renderable by the hw
    */
    GLuint accelBufferMask;

    /*
    ** the buffers themselves
    */
    __GLdrawableBuffer frontBuffer;
    __GLdrawableBuffer backBuffer;
    __GLdrawableBuffer accumBuffer;
    __GLdrawableBuffer depthBuffer;
    __GLdrawableBuffer stencilBuffer;
#if defined(__GL_NUMBER_OF_AUX_BUFFERS) && (__GL_NUMBER_OF_AUX_BUFFERS > 0)
    __GLdrawableBuffer *auxBuffer;
#endif

    __GLdrawableRegion ownershipRegion;

    /*
    ** Lock for the drawable private structure
    */
    void *lock;
#ifdef DEBUG
    /* lock debugging info */
    int lockRefCount;
    int lockLine[10];
    char *lockFile[10];
#endif

    /* imported */
    void *(*malloc)(size_t size);
    void *(*calloc)(size_t numElem, size_t elemSize);
    void *(*realloc)(void *oldAddr, size_t newSize);
    void (*free)(void *addr);

    GLboolean (*addSwapRect)(__GLdrawablePrivate *glPriv, 
			     GLint x, GLint y, GLsizei width, GLsizei height);
    void (*setClipRect)(__GLdrawablePrivate *glPriv, 
			GLint x, GLint y, GLsizei width, GLsizei height);
    void (*updateClipRegion)(__GLdrawablePrivate *glPriv);
    GLboolean (*resize)(__GLdrawablePrivate *glPriv);
    void (*getDrawableSize)(__GLdrawablePrivate *glPriv, 
			    GLint *x, GLint *y, GLuint *width, GLuint *height);

    void (*lockDP)(__GLdrawablePrivate *glPriv, __GLcontext *gc);
    void (*unlockDP)(__GLdrawablePrivate *glPriv);


    void *other;

    /* exported */
    void (*freePrivate)(__GLdrawablePrivate *);
    void *private;
};

/*
** Macros to lock/unlock the drawable private
*/
#if defined(DEBUG)
#define __GL_LOCK_DP(glPriv,gc) \
    (*(glPriv)->lockDP)(glPriv,gc); \
    (glPriv)->lockLine[(glPriv)->lockRefCount] = __LINE__; \
    (glPriv)->lockFile[(glPriv)->lockRefCount] = __FILE__; \
    (glPriv)->lockRefCount++
#define __GL_UNLOCK_DP(glPriv) \
    (glPriv)->lockRefCount--; \
    (glPriv)->lockLine[(glPriv)->lockRefCount] = 0; \
    (glPriv)->lockFile[(glPriv)->lockRefCount] = NULL; \
    (*(glPriv)->unlockDP)(glPriv)
#else /* DEBUG */
#define __GL_LOCK_DP(glPriv,gc)		(*(glPriv)->lockDP)(glPriv,gc)
#define	__GL_UNLOCK_DP(glPriv)		(*(glPriv)->unlockDP)(glPriv)
#endif /* DEBUG */


/*
** Procedures which are imported by the GL from the surrounding
** "operating system".  Math functions are not considered part of the
** "operating system".
*/
typedef struct __GLimportsRec {
    /* Memory management */
    void *(*malloc)(__GLcontext *gc, size_t size);
    void *(*calloc)(__GLcontext *gc, size_t numElem, size_t elemSize);
    void *(*realloc)(__GLcontext *gc, void *oldAddr, size_t newSize);
    void (*free)(__GLcontext *gc, void *addr);

    /* Error handling */
    void (*warning)(__GLcontext *gc, char *fmt);
    void (*fatal)(__GLcontext *gc, char *fmt);

    /* other system calls */
    char *(*getenv)(__GLcontext *gc, const char *var);
    int (*sprintf)(__GLcontext *gc, char *str, const char *fmt, ...);
    void *(*fopen)(__GLcontext *gc, const char *path, const char *mode);
    int (*fclose)(__GLcontext *gc, void *stream);
    int (*fprintf)(__GLcontext *gc, void *stream, const char *fmt, ...);

    /* Drawing surface management */
    __GLdrawablePrivate *(*getDrawablePrivate)(__GLcontext *gc);

    /* Operating system dependent data goes here */
    void *other;
} __GLimports;

/************************************************************************/

/*
** Procedures which are exported by the GL to the surrounding "operating
** system" so that it can manage multiple GL context's.
*/
typedef struct __GLexportsRec {
    /* Context management (return GL_FALSE on failure) */
    GLboolean (*destroyContext)(__GLcontext *gc);
    GLboolean (*loseCurrent)(__GLcontext *gc);
    GLboolean (*makeCurrent)(__GLcontext *gc, __GLdrawablePrivate *glPriv);
    GLboolean (*shareContext)(__GLcontext *gc, __GLcontext *gcShare);
    GLboolean (*copyContext)(__GLcontext *dst, const __GLcontext *src, GLuint mask);
    GLboolean (*forceCurrent)(__GLcontext *gc);

    /* Drawing surface notification callbacks */
    GLboolean (*notifyResize)(__GLcontext *gc);
    void (*notifyDestroy)(__GLcontext *gc);
    void (*notifySwapBuffers)(__GLcontext *gc);

    /* Dispatch table override control for external agents like libGLS */
    struct __GLdispatchStateRec* (*dispatchExec)(__GLcontext *gc);
    void (*beginDispatchOverride)(__GLcontext *gc);
    void (*endDispatchOverride)(__GLcontext *gc);
} __GLexports;

/************************************************************************/

/*
** This must be the first member of a __GLcontext structure.  This is the
** only part of a context that is exposed to the outside world; everything
** else is opaque.
*/
struct __GLinterfaceRec {
    __GLimports imports;
    __GLexports exports;
};

extern __GLcontext *__glCoreCreateContext(__GLimports *, __GLcontextModes *);
extern void __glCoreNopDispatch(void);

#endif /* __gl_core_h_ */
