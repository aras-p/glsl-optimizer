/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

/**
 * \file glxclient.h
 * Direct rendering support added by Precision Insight, Inc.
 *
 * \author Kevin E. Martin <kevin@precisioninsight.com>
 */

#ifndef _GLX_client_h_
#define _GLX_client_h_
#define NEED_REPLIES
#define NEED_EVENTS
#include <X11/Xproto.h>
#include <X11/Xlibint.h>
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/glxext.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#include <stdint.h>
#endif
#include "GL/glxint.h"
#include "GL/glxproto.h"
#include "GL/internal/glcore.h"
#include "glapi/glapitable.h"
#include "glxhash.h"
#if defined( USE_XTHREADS )
# include <X11/Xthreads.h>
#elif defined( PTHREADS )
# include <pthread.h>
#endif

#include "glxextensions.h"


/* If we build the library with gcc's -fvisibility=hidden flag, we'll
 * use the PUBLIC macro to mark functions that are to be exported.
 *
 * We also need to define a USED attribute, so the optimizer doesn't 
 * inline a static function that we later use in an alias. - ajax
 */
#if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 303
#  define PUBLIC __attribute__((visibility("default")))
#  define USED __attribute__((used))
#else
#  define PUBLIC
#  define USED
#endif



#define GLX_MAJOR_VERSION	1	/* current version numbers */
#define GLX_MINOR_VERSION	4

#define __GLX_MAX_TEXTURE_UNITS 32

typedef struct __GLXscreenConfigsRec __GLXscreenConfigs;
typedef struct __GLXcontextRec __GLXcontext;
typedef struct __GLXdrawableRec __GLXdrawable;
typedef struct __GLXdisplayPrivateRec __GLXdisplayPrivate;
typedef struct _glapi_table __GLapi;

/************************************************************************/

#ifdef GLX_DIRECT_RENDERING

#define containerOf(ptr, type, member)			\
    (type *)( (char *)ptr - offsetof(type,member) )

#include <GL/internal/dri_interface.h>


/**
 * Display dependent methods.  This structure is initialized during the
 * \c driCreateDisplay call.
 */
typedef struct __GLXDRIdisplayRec __GLXDRIdisplay;
typedef struct __GLXDRIscreenRec __GLXDRIscreen;
typedef struct __GLXDRIdrawableRec __GLXDRIdrawable;
typedef struct __GLXDRIcontextRec __GLXDRIcontext;

#include "glxextensions.h"

struct __GLXDRIdisplayRec {
    /**
     * Method to destroy the private DRI display data.
     */
    void (*destroyDisplay)(__GLXDRIdisplay *display);

    __GLXDRIscreen *(*createScreen)(__GLXscreenConfigs *psc, int screen,
				    __GLXdisplayPrivate *priv);
};

struct __GLXDRIscreenRec {

    void (*destroyScreen)(__GLXscreenConfigs *psc);

    __GLXDRIcontext *(*createContext)(__GLXscreenConfigs *psc,
				      const __GLcontextModes *mode,
				      GLXContext gc,
				      GLXContext shareList, int renderType);
	
    __GLXDRIdrawable *(*createDrawable)(__GLXscreenConfigs *psc,
					XID drawable,
					GLXDrawable glxDrawable,
					const __GLcontextModes *modes);

    void (*swapBuffers)(__GLXDRIdrawable *pdraw);
    void (*copySubBuffer)(__GLXDRIdrawable *pdraw,
			  int x, int y, int width, int height);
};

struct __GLXDRIcontextRec {
    void (*destroyContext)(__GLXDRIcontext *context, __GLXscreenConfigs *psc,
			   Display *dpy);
    Bool (*bindContext)(__GLXDRIcontext *context,
			__GLXDRIdrawable *pdraw,
			__GLXDRIdrawable *pread);
    
    void (*unbindContext)(__GLXDRIcontext *context);
};

struct __GLXDRIdrawableRec {
    void (*destroyDrawable)(__GLXDRIdrawable *drawable);

    XID xDrawable;
    XID drawable;
    __GLXscreenConfigs *psc;
    GLenum textureTarget;
    __DRIdrawable *driDrawable;
};

/*
** Function to create and DRI display data and initialize the display
** dependent methods.
*/
extern __GLXDRIdisplay *driswCreateDisplay(Display *dpy);
extern __GLXDRIdisplay *driCreateDisplay(Display *dpy);
extern __GLXDRIdisplay *dri2CreateDisplay(Display *dpy);

extern void DRI_glXUseXFont( Font font, int first, int count, int listbase );

/*
** Functions to obtain driver configuration information from a direct
** rendering client application
*/
extern const char *glXGetScreenDriver (Display *dpy, int scrNum);

extern const char *glXGetDriverConfig (const char *driverName);

#endif

/************************************************************************/

#define __GL_CLIENT_ATTRIB_STACK_DEPTH 16

typedef struct __GLXpixelStoreModeRec {
    GLboolean swapEndian;
    GLboolean lsbFirst;
    GLuint rowLength;
    GLuint imageHeight;
    GLuint imageDepth;
    GLuint skipRows;
    GLuint skipPixels;
    GLuint skipImages;
    GLuint alignment;
} __GLXpixelStoreMode;


typedef struct __GLXattributeRec {
    GLuint mask;

    /**
     * Pixel storage state.  Most of the pixel store mode state is kept
     * here and used by the client code to manage the packing and
     * unpacking of data sent to/received from the server.
     */
    __GLXpixelStoreMode storePack, storeUnpack;

    /**
     * Is EXT_vertex_array / GL 1.1 DrawArrays protocol specifically
     * disabled?
     */
    GLboolean NoDrawArraysProtocol;
    
    /**
     * Vertex Array storage state.  The vertex array component
     * state is stored here and is used to manage the packing of
     * DrawArrays data sent to the server.
     */
    struct array_state_vector * array_state;
} __GLXattribute;

typedef struct __GLXattributeMachineRec {
	__GLXattribute *stack[__GL_CLIENT_ATTRIB_STACK_DEPTH];
	__GLXattribute **stackPointer;
} __GLXattributeMachine;

/**
 * GLX state that needs to be kept on the client.  One of these records
 * exist for each context that has been made current by this client.
 */
struct __GLXcontextRec {
    /**
     * \name Drawing command buffer.
     *
     * Drawing commands are packed into this buffer before being sent as a
     * single GLX protocol request.  The buffer is sent when it overflows or
     * is flushed by \c __glXFlushRenderBuffer.  \c pc is the next location
     * in the buffer to be filled.  \c limit is described above in the buffer
     * slop discussion.
     *
     * Commands that require large amounts of data to be transfered will
     * also use this buffer to hold a header that describes the large
     * command.
     *
     * These must be the first 6 fields since they are static initialized
     * in the dummy context in glxext.c
     */
    /*@{*/
    GLubyte *buf;
    GLubyte *pc;
    GLubyte *limit;
    GLubyte *bufEnd;
    GLint bufSize;
    /*@}*/

    /**
     * The XID of this rendering context.  When the context is created a
     * new XID is allocated.  This is set to None when the context is
     * destroyed but is still current to some thread. In this case the
     * context will be freed on next MakeCurrent.
     */
    XID xid;

    /**
     * The XID of the \c shareList context.
     */
    XID share_xid;

    /**
     * Screen number.
     */
    GLint screen;
    __GLXscreenConfigs *psc;

    /**
     * \c GL_TRUE if the context was created with ImportContext, which
     * means the server-side context was created by another X client.
     */
    GLboolean imported;

    /**
     * The context tag returned by MakeCurrent when this context is made
     * current. This tag is used to identify the context that a thread has
     * current so that proper server context management can be done.  It is
     * used for all context specific commands (i.e., \c Render, \c RenderLarge,
     * \c WaitX, \c WaitGL, \c UseXFont, and \c MakeCurrent (for the old
     * context)).
     */
    GLXContextTag currentContextTag;

    /**
     * \name Rendering mode
     *
     * The rendering mode is kept on the client as well as the server.
     * When \c glRenderMode is called, the buffer associated with the
     * previous rendering mode (feedback or select) is filled.
     */
    /*@{*/
    GLenum renderMode;
    GLfloat *feedbackBuf;
    GLuint *selectBuf;
    /*@}*/

    /**
     * This is \c GL_TRUE if the pixel unpack modes are such that an image
     * can be unpacked from the clients memory by just copying.  It may
     * still be true that the server will have to do some work.  This
     * just promises that a straight copy will fetch the correct bytes.
     */
    GLboolean fastImageUnpack;

    /**
     * Fill newImage with the unpacked form of \c oldImage getting it
     * ready for transport to the server.
     */
    void (*fillImage)(__GLXcontext*, GLint, GLint, GLint, GLint, GLenum,
		      GLenum, const GLvoid*, GLubyte*, GLubyte*);

    /**
     * Client side attribs.
     */
    __GLXattributeMachine attributes;

    /**
     * Client side error code.  This is set when client side gl API
     * routines need to set an error because of a bad enumerant or
     * running out of memory, etc.
     */
    GLenum error;

    /**
     * Whether this context does direct rendering.
     */
    Bool isDirect;

    /**
     * \c dpy of current display for this context.  Will be \c NULL if not
     * current to any display, or if this is the "dummy context".
     */
    Display *currentDpy;

    /**
     * The current drawable for this context.  Will be None if this
     * context is not current to any drawable.  currentReadable is below.
     */
    GLXDrawable currentDrawable;

    /**
     * \name GL Constant Strings
     *
     * Constant strings that describe the server implementation
     * These pertain to GL attributes, not to be confused with
     * GLX versioning attributes.
     */
    /*@{*/
    GLubyte *vendor;
    GLubyte *renderer;
    GLubyte *version;
    GLubyte *extensions;
    /*@}*/

    /**
     * Record the dpy this context was created on for later freeing
     */
    Display *createDpy;

    /**
     * Maximum small render command size.  This is the smaller of 64k and
     * the size of the above buffer.
     */
    GLint maxSmallRenderCommandSize;

    /**
     * Major opcode for the extension.  Copied here so a lookup isn't
     * needed.
     */
    GLint majorOpcode;

    /**
     * Pointer to the mode used to create this context.
     */
    const __GLcontextModes * mode;

#ifdef GLX_DIRECT_RENDERING
    __GLXDRIcontext *driContext;
    __DRIcontext *__driContext;
#endif

    /**
     * The current read-drawable for this context.  Will be None if this
     * context is not current to any drawable.
     *
     * \since Internal API version 20030606.
     */
    GLXDrawable currentReadable;

   /** 
    * Pointer to client-state data that is private to libGL.  This is only
    * used for indirect rendering contexts.
    *
    * No internal API version change was made for this change.  Client-side
    * drivers should NEVER use this data or even care that it exists.
    */
   void * client_state_private;

   /**
    * Stored value for \c glXQueryContext attribute \c GLX_RENDER_TYPE.
    */
   int renderType;
    
   /**
    * \name Raw server GL version
    *
    * True core GL version supported by the server.  This is the raw value
    * returned by the server, and it may not reflect what is actually
    * supported (or reported) by the client-side library.
    */
    /*@{*/
   int server_major;        /**< Major version number. */
   int server_minor;        /**< Minor version number. */
    /*@}*/

    char gl_extension_bits[ __GL_EXT_BYTES ];
};

#define __glXSetError(gc,code) \
    if (!(gc)->error) {	       \
	(gc)->error = code;    \
    }

extern void __glFreeAttributeState(__GLXcontext *);

/************************************************************************/

/**
 * The size of the largest drawing command known to the implementation
 * that will use the GLXRender GLX command.  In this case it is
 * \c glPolygonStipple.
 */
#define __GLX_MAX_SMALL_RENDER_CMD_SIZE	156

/**
 * To keep the implementation fast, the code uses a "limit" pointer
 * to determine when the drawing command buffer is too full to hold
 * another fixed size command.  This constant defines the amount of
 * space that must always be available in the drawing command buffer
 * at all times for the implementation to work.  It is important that
 * the number be just large enough, but not so large as to reduce the
 * efficacy of the buffer.  The "+32" is just to keep the code working
 * in case somebody counts wrong.
 */
#define __GLX_BUFFER_LIMIT_SIZE	(__GLX_MAX_SMALL_RENDER_CMD_SIZE + 32)

/**
 * This implementation uses a smaller threshold for switching
 * to the RenderLarge protocol than the protcol requires so that
 * large copies don't occur.
 */
#define __GLX_RENDER_CMD_SIZE_LIMIT	4096

/**
 * One of these records exists per screen of the display.  It contains
 * a pointer to the config data for that screen (if the screen supports GL).
 */
struct __GLXscreenConfigsRec {
    /**
     * GLX extension string reported by the X-server.
     */
    const char *serverGLXexts;

    /**
     * GLX extension string to be reported to applications.  This is the
     * set of extensions that the application can actually use.
     */
    char *effectiveGLXexts;

#ifdef GLX_DIRECT_RENDERING
    /**
     * Per screen direct rendering interface functions and data.
     */
    __DRIscreen *__driScreen;
    const __DRIcoreExtension *core;
    const __DRIlegacyExtension *legacy;
    const __DRIswrastExtension *swrast;
    const __DRIdri2Extension *dri2;
    __glxHashTable *drawHash;
    Display *dpy;
    int scr, fd;
    void *driver;

    __GLXDRIscreen *driScreen;

#ifdef __DRI_COPY_SUB_BUFFER
    const __DRIcopySubBufferExtension *driCopySubBuffer;
#endif

#ifdef __DRI_SWAP_CONTROL
    const __DRIswapControlExtension *swapControl;
#endif

#ifdef __DRI_ALLOCATE
    const __DRIallocateExtension *allocate;
#endif

#ifdef __DRI_FRAME_TRACKING
    const __DRIframeTrackingExtension *frameTracking;
#endif

#ifdef __DRI_MEDIA_STREAM_COUNTER
    const __DRImediaStreamCounterExtension *msc;
#endif

#ifdef __DRI_TEX_BUFFER
    const __DRItexBufferExtension *texBuffer;
#endif

#endif

    /**
     * Linked list of glx visuals and  fbconfigs for this screen.
     */
    __GLcontextModes *visuals, *configs;

    /**
     * Per-screen dynamic GLX extension tracking.  The \c direct_support
     * field only contains enough bits for 64 extensions.  Should libGL
     * ever need to track more than 64 GLX extensions, we can safely grow
     * this field.  The \c __GLXscreenConfigs structure is not used outside
     * libGL.
     */
    /*@{*/
    unsigned char direct_support[8];
    GLboolean ext_list_first_time;
    /*@}*/

};

/**
 * Per display private data.  One of these records exists for each display
 * that is using the OpenGL (GLX) extension.
 */
struct __GLXdisplayPrivateRec {
    /**
     * Back pointer to the display
     */
    Display *dpy;

    /**
     * The \c majorOpcode is common to all connections to the same server.
     * It is also copied into the context structure.
     */
    int majorOpcode;

    /**
     * \name Server Version
     *
     * Major and minor version returned by the server during initialization.
     */
    /*@{*/
    int majorVersion, minorVersion;
    /*@}*/

    /**
     * \name Storage for the servers GLX vendor and versions strings.
     * 
     * These are the same for all screens on this display. These fields will
     * be filled in on demand.
     */
    /*@{*/
    const char *serverGLXvendor;
    const char *serverGLXversion;
    /*@}*/

    /**
     * Configurations of visuals for all screens on this display.
     * Also, per screen data which now includes the server \c GLX_EXTENSION
     * string.
     */
    __GLXscreenConfigs *screenConfigs;

#ifdef GLX_DIRECT_RENDERING
    /**
     * Per display direct rendering interface functions and data.
     */
    __GLXDRIdisplay *driswDisplay;
    __GLXDRIdisplay *driDisplay;
    __GLXDRIdisplay *dri2Display;
#endif
};


extern GLubyte *__glXFlushRenderBuffer(__GLXcontext*, GLubyte*);

extern void __glXSendLargeChunk(__GLXcontext *gc, GLint requestNumber, 
				GLint totalRequests,
				const GLvoid * data, GLint dataLen);

extern void __glXSendLargeCommand(__GLXcontext *, const GLvoid *, GLint,
				  const GLvoid *, GLint);

/* Initialize the GLX extension for dpy */
extern __GLXdisplayPrivate * __glXGetPrivateFromDisplay(Display *dpy);
extern __GLXdisplayPrivate *__glXInitialize(Display*);

/************************************************************************/

extern int __glXDebug;

/* This is per-thread storage in an MT environment */
#if defined( USE_XTHREADS ) || defined( PTHREADS )

extern void __glXSetCurrentContext(__GLXcontext *c);

# if defined( GLX_USE_TLS )

extern __thread void * __glX_tls_Context
    __attribute__((tls_model("initial-exec")));

#  define __glXGetCurrentContext()	__glX_tls_Context

# else

extern __GLXcontext *__glXGetCurrentContext(void);

# endif /* defined( GLX_USE_TLS ) */

#else

extern __GLXcontext *__glXcurrentContext;
#define __glXGetCurrentContext()	__glXcurrentContext
#define __glXSetCurrentContext(gc)	__glXcurrentContext = gc

#endif /* defined( USE_XTHREADS ) || defined( PTHREADS ) */

extern void __glXSetCurrentContextNull(void);

extern void __glXFreeContext(__GLXcontext*);


/*
** Global lock for all threads in this address space using the GLX
** extension
*/
#if defined( USE_XTHREADS )
extern xmutex_rec __glXmutex;
#define __glXLock()    xmutex_lock(&__glXmutex)
#define __glXUnlock()  xmutex_unlock(&__glXmutex)
#elif defined( PTHREADS )
extern pthread_mutex_t __glXmutex;
#define __glXLock()    pthread_mutex_lock(&__glXmutex)
#define __glXUnlock()  pthread_mutex_unlock(&__glXmutex)
#else
#define __glXLock()
#define __glXUnlock()
#endif

/*
** Setup for a command.  Initialize the extension for dpy if necessary.
*/
extern CARD8 __glXSetupForCommand(Display *dpy);

/************************************************************************/

/*
** Data conversion and packing support.
*/

extern const GLuint __glXDefaultPixelStore[9];

/* Send an image to the server using RenderLarge. */
extern void __glXSendLargeImage(__GLXcontext *gc, GLint compsize, GLint dim,
    GLint width, GLint height, GLint depth, GLenum format, GLenum type,
    const GLvoid *src, GLubyte *pc, GLubyte *modes);

/* Return the size, in bytes, of some pixel data */
extern GLint __glImageSize(GLint, GLint, GLint, GLenum, GLenum, GLenum);

/* Return the number of elements per group of a specified format*/
extern GLint __glElementsPerGroup(GLenum format, GLenum type);

/* Return the number of bytes per element, based on the element type (other
** than GL_BITMAP).
*/
extern GLint __glBytesPerElement(GLenum type);

/*
** Fill the transport buffer with the data from the users buffer,
** applying some of the pixel store modes (unpack modes) to the data
** first.  As a side effect of this call, the "modes" field is
** updated to contain the modes needed by the server to decode the
** sent data.
*/
extern void __glFillImage(__GLXcontext*, GLint, GLint, GLint, GLint, GLenum,
			  GLenum, const GLvoid*, GLubyte*, GLubyte*);

/* Copy map data with a stride into a packed buffer */
extern void __glFillMap1f(GLint, GLint, GLint, const GLfloat *, GLubyte *);
extern void __glFillMap1d(GLint, GLint, GLint, const GLdouble *, GLubyte *);
extern void __glFillMap2f(GLint, GLint, GLint, GLint, GLint,
			  const GLfloat *, GLfloat *);
extern void __glFillMap2d(GLint, GLint, GLint, GLint, GLint,
			  const GLdouble *, GLdouble *);

/*
** Empty an image out of the reply buffer into the clients memory applying
** the pack modes to pack back into the clients requested format.
*/
extern void __glEmptyImage(__GLXcontext*, GLint, GLint, GLint, GLint, GLenum,
		           GLenum, const GLubyte *, GLvoid *);


/*
** Allocate and Initialize Vertex Array client state, and free.
*/
extern void __glXInitVertexArrayState(__GLXcontext *);
extern void __glXFreeVertexArrayState(__GLXcontext *);

/*
** Inform the Server of the major and minor numbers and of the client
** libraries extension string.
*/
extern void __glXClientInfo (  Display *dpy, int opcode );

/************************************************************************/

/*
** Declarations that should be in Xlib
*/
#ifdef __GL_USE_OUR_PROTOTYPES
extern void _XFlush(Display*);
extern Status _XReply(Display*, xReply*, int, Bool);
extern void _XRead(Display*, void*, long);
extern void _XSend(Display*, const void*, long);
#endif


extern void __glXInitializeVisualConfigFromTags( __GLcontextModes *config,
    int count, const INT32 *bp, Bool tagged_only, Bool fbconfig_style_tags );

extern char * __glXQueryServerString(Display* dpy, int opcode,
                                     CARD32 screen, CARD32 name);
extern char * __glXGetString(Display* dpy, int opcode,
                             CARD32 screen, CARD32 name);

extern char *__glXstrdup(const char *str);


extern const char __glXGLClientVersion[];
extern const char __glXGLClientExtensions[];

/* Get the unadjusted system time */
extern int __glXGetUST( int64_t * ust );

extern GLboolean __glXGetMscRateOML(Display * dpy, GLXDrawable drawable,
				    int32_t * numerator, int32_t * denominator);

#ifdef GLX_DIRECT_RENDERING
GLboolean
__driGetMscRateOML(__DRIdrawable *draw,
		   int32_t *numerator, int32_t *denominator, void *private);
#endif

#endif /* !__GLX_client_h__ */
