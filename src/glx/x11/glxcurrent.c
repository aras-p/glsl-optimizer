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
 * \file glxcurrent.c
 * Client-side GLX interface for current context management.
 */

#include "glxclient.h"
#include "glapi.h"
#include "indirect_init.h"

#ifdef GLX_DIRECT_RENDERING
#include "xf86dri.h"
#endif

/*
** We setup some dummy structures here so that the API can be used
** even if no context is current.
*/

static GLubyte dummyBuffer[__GLX_BUFFER_LIMIT_SIZE];

/*
** Dummy context used by small commands when there is no current context.
** All the
** gl and glx entry points are designed to operate as nop's when using
** the dummy context structure.
*/
static __GLXcontext dummyContext = {
   &dummyBuffer[0],
   &dummyBuffer[0],
   &dummyBuffer[0],
   &dummyBuffer[__GLX_BUFFER_LIMIT_SIZE],
   sizeof(dummyBuffer),
};


/*
** All indirect rendering contexts will share the same indirect dispatch table.
*/
static __GLapi *IndirectAPI = NULL;


/*
 * Current context management and locking
 */

#if defined( USE_XTHREADS )

/* thread safe */
static GLboolean TSDinitialized = GL_FALSE;
static xthread_key_t ContextTSD;

_X_HIDDEN __GLXcontext *
__glXGetCurrentContext(void)
{
   if (!TSDinitialized) {
      xthread_key_create(&ContextTSD, NULL);
      TSDinitialized = GL_TRUE;
      return &dummyContext;
   }
   else {
      void *p;
      xthread_get_specific(ContextTSD, &p);
      if (!p)
         return &dummyContext;
      else
         return (__GLXcontext *) p;
   }
}

_X_HIDDEN void
__glXSetCurrentContext(__GLXcontext * c)
{
   if (!TSDinitialized) {
      xthread_key_create(&ContextTSD, NULL);
      TSDinitialized = GL_TRUE;
   }
   xthread_set_specific(ContextTSD, c);
}


/* Used by the __glXLock() and __glXUnlock() macros */
_X_HIDDEN xmutex_rec __glXmutex;

#elif defined( PTHREADS )

_X_HIDDEN pthread_mutex_t __glXmutex = PTHREAD_MUTEX_INITIALIZER;

# if defined( GLX_USE_TLS )

/**
 * Per-thread GLX context pointer.
 * 
 * \c __glXSetCurrentContext is written is such a way that this pointer can
 * \b never be \c NULL.  This is important!  Because of this
 * \c __glXGetCurrentContext can be implemented as trivial macro.
 */
__thread void *__glX_tls_Context __attribute__ ((tls_model("initial-exec")))
   = &dummyContext;

_X_HIDDEN void
__glXSetCurrentContext(__GLXcontext * c)
{
   __glX_tls_Context = (c != NULL) ? c : &dummyContext;
}

# else

static pthread_once_t once_control = PTHREAD_ONCE_INIT;

/**
 * Per-thread data key.
 * 
 * Once \c init_thread_data has been called, the per-thread data key will
 * take a value of \c NULL.  As each new thread is created the default
 * value, in that thread, will be \c NULL.
 */
static pthread_key_t ContextTSD;

/**
 * Initialize the per-thread data key.
 * 
 * This function is called \b exactly once per-process (not per-thread!) to
 * initialize the per-thread data key.  This is ideally done using the
 * \c pthread_once mechanism.
 */
static void
init_thread_data(void)
{
   if (pthread_key_create(&ContextTSD, NULL) != 0) {
      perror("pthread_key_create");
      exit(-1);
   }
}

_X_HIDDEN void
__glXSetCurrentContext(__GLXcontext * c)
{
   pthread_once(&once_control, init_thread_data);
   pthread_setspecific(ContextTSD, c);
}

_X_HIDDEN __GLXcontext *
__glXGetCurrentContext(void)
{
   void *v;

   pthread_once(&once_control, init_thread_data);

   v = pthread_getspecific(ContextTSD);
   return (v == NULL) ? &dummyContext : (__GLXcontext *) v;
}

# endif /* defined( GLX_USE_TLS ) */

#elif defined( THREADS )

#error Unknown threading method specified.

#else

/* not thread safe */
_X_HIDDEN __GLXcontext *__glXcurrentContext = &dummyContext;

#endif


_X_HIDDEN void
__glXSetCurrentContextNull(void)
{
   __glXSetCurrentContext(&dummyContext);
#ifdef GLX_DIRECT_RENDERING
   _glapi_set_dispatch(NULL);   /* no-op functions */
#endif
}


/************************************************************************/

PUBLIC GLXContext
glXGetCurrentContext(void)
{
   GLXContext cx = __glXGetCurrentContext();

   if (cx == &dummyContext) {
      return NULL;
   }
   else {
      return cx;
   }
}

PUBLIC GLXDrawable
glXGetCurrentDrawable(void)
{
   GLXContext gc = __glXGetCurrentContext();
   return gc->currentDrawable;
}


/************************************************************************/

/**
 * Sends a GLX protocol message to the specified display to make the context
 * and the drawables current.
 *
 * \param dpy     Display to send the message to.
 * \param opcode  Major opcode value for the display.
 * \param gc_id   Context tag for the context to be made current.
 * \param draw    Drawable ID for the "draw" drawable.
 * \param read    Drawable ID for the "read" drawable.
 * \param reply   Space to store the X-server's reply.
 *
 * \warning
 * This function assumes that \c dpy is locked with \c LockDisplay on entry.
 */
static Bool
SendMakeCurrentRequest(Display * dpy, CARD8 opcode,
                       GLXContextID gc_id, GLXContextTag gc_tag,
                       GLXDrawable draw, GLXDrawable read,
                       xGLXMakeCurrentReply * reply)
{
   Bool ret;


   LockDisplay(dpy);

   if (draw == read) {
      xGLXMakeCurrentReq *req;

      GetReq(GLXMakeCurrent, req);
      req->reqType = opcode;
      req->glxCode = X_GLXMakeCurrent;
      req->drawable = draw;
      req->context = gc_id;
      req->oldContextTag = gc_tag;
   }
   else {
      __GLXdisplayPrivate *priv = __glXInitialize(dpy);

      /* If the server can support the GLX 1.3 version, we should
       * perfer that.  Not only that, some servers support GLX 1.3 but
       * not the SGI extension.
       */

      if ((priv->majorVersion > 1) || (priv->minorVersion >= 3)) {
         xGLXMakeContextCurrentReq *req;

         GetReq(GLXMakeContextCurrent, req);
         req->reqType = opcode;
         req->glxCode = X_GLXMakeContextCurrent;
         req->drawable = draw;
         req->readdrawable = read;
         req->context = gc_id;
         req->oldContextTag = gc_tag;
      }
      else {
         xGLXVendorPrivateWithReplyReq *vpreq;
         xGLXMakeCurrentReadSGIReq *req;

         GetReqExtra(GLXVendorPrivateWithReply,
                     sz_xGLXMakeCurrentReadSGIReq -
                     sz_xGLXVendorPrivateWithReplyReq, vpreq);
         req = (xGLXMakeCurrentReadSGIReq *) vpreq;
         req->reqType = opcode;
         req->glxCode = X_GLXVendorPrivateWithReply;
         req->vendorCode = X_GLXvop_MakeCurrentReadSGI;
         req->drawable = draw;
         req->readable = read;
         req->context = gc_id;
         req->oldContextTag = gc_tag;
      }
   }

   ret = _XReply(dpy, (xReply *) reply, 0, False);

   UnlockDisplay(dpy);
   SyncHandle();

   return ret;
}


#ifdef GLX_DIRECT_RENDERING
static __GLXDRIdrawable *
FetchDRIDrawable(Display * dpy,
                 GLXDrawable glxDrawable, GLXContext gc, Bool pre13)
{
   __GLXdisplayPrivate *const priv = __glXInitialize(dpy);
   __GLXDRIdrawable *pdraw;
   __GLXscreenConfigs *psc;
   XID drawable;

   if (priv == NULL)
      return NULL;

   psc = &priv->screenConfigs[gc->screen];
   if (psc->drawHash == NULL)
      return NULL;

   if (__glxHashLookup(psc->drawHash, glxDrawable, (void *) &pdraw) == 0)
      return pdraw;

   /* If this is glXMakeCurrent (pre GLX 1.3) we allow creating the
    * GLX drawable on the fly.  Otherwise we pass None as the X
    * drawable */
   if (pre13)
      drawable = glxDrawable;
   else
      drawable = None;

   pdraw = psc->driScreen->createDrawable(psc, drawable,
                                          glxDrawable, gc->mode);
   if (__glxHashInsert(psc->drawHash, glxDrawable, pdraw)) {
      (*pdraw->destroyDrawable) (pdraw);
      return NULL;
   }

   return pdraw;
}
#endif /* GLX_DIRECT_RENDERING */


/**
 * Make a particular context current.
 * 
 * \note This is in this file so that it can access dummyContext.
 */
static Bool
MakeContextCurrent(Display * dpy, GLXDrawable draw,
                   GLXDrawable read, GLXContext gc, Bool pre13)
{
   xGLXMakeCurrentReply reply;
   const GLXContext oldGC = __glXGetCurrentContext();
   const CARD8 opcode = __glXSetupForCommand(dpy);
   const CARD8 oldOpcode = ((gc == oldGC) || (oldGC == &dummyContext))
      ? opcode : __glXSetupForCommand(oldGC->currentDpy);
   Bool bindReturnValue;
   __GLXattribute *state;

   if (!opcode || !oldOpcode) {
      return GL_FALSE;
   }

   /* Make sure that the new context has a nonzero ID.  In the request,
    * a zero context ID is used only to mean that we bind to no current
    * context.
    */
   if ((gc != NULL) && (gc->xid == None)) {
      return GL_FALSE;
   }

   _glapi_check_multithread();

#ifdef GLX_DIRECT_RENDERING
   /* Bind the direct rendering context to the drawable */
   if (gc && gc->driContext) {
      __GLXDRIdrawable *pdraw = FetchDRIDrawable(dpy, draw, gc, pre13);
      __GLXDRIdrawable *pread = FetchDRIDrawable(dpy, read, gc, pre13);

      bindReturnValue =
         (gc->driContext->bindContext) (gc->driContext, pdraw, pread);
   }
   else
#endif
   {
      /* Send a glXMakeCurrent request to bind the new context. */
      bindReturnValue =
         SendMakeCurrentRequest(dpy, opcode, gc ? gc->xid : None,
                                ((dpy != oldGC->currentDpy)
                                 || oldGC->isDirect)
                                ? None : oldGC->currentContextTag, draw, read,
                                &reply);
   }


   if (!bindReturnValue) {
      return False;
   }

#ifdef GLX_DIRECT_RENDERING
   if ((dpy != oldGC->currentDpy || (gc && gc->driContext)) &&
       !oldGC->isDirect && oldGC != &dummyContext) {
#else
   if ((dpy != oldGC->currentDpy) && oldGC != &dummyContext) {
#endif
      xGLXMakeCurrentReply dummy_reply;

      /* We are either switching from one dpy to another and have to
       * send a request to the previous dpy to unbind the previous
       * context, or we are switching away from a indirect context to
       * a direct context and have to send a request to the dpy to
       * unbind the previous context.
       */
      (void) SendMakeCurrentRequest(oldGC->currentDpy, oldOpcode, None,
                                    oldGC->currentContextTag, None, None,
                                    &dummy_reply);
   }
#ifdef GLX_DIRECT_RENDERING
   else if (oldGC->driContext) {
      oldGC->driContext->unbindContext(oldGC->driContext);
   }
#endif


   /* Update our notion of what is current */
   __glXLock();
   if (gc == oldGC) {
      /* Even though the contexts are the same the drawable might have
       * changed.  Note that gc cannot be the dummy, and that oldGC
       * cannot be NULL, therefore if they are the same, gc is not
       * NULL and not the dummy.
       */
      gc->currentDrawable = draw;
      gc->currentReadable = read;
   }
   else {
      if (oldGC != &dummyContext) {
         /* Old current context is no longer current to anybody */
         oldGC->currentDpy = 0;
         oldGC->currentDrawable = None;
         oldGC->currentReadable = None;
         oldGC->currentContextTag = 0;

         if (oldGC->xid == None) {
            /* We are switching away from a context that was
             * previously destroyed, so we need to free the memory
             * for the old handle.
             */
#ifdef GLX_DIRECT_RENDERING
            /* Destroy the old direct rendering context */
            if (oldGC->driContext) {
               oldGC->driContext->destroyContext(oldGC->driContext,
                                                 oldGC->psc,
                                                 oldGC->createDpy);
               oldGC->driContext = NULL;
            }
#endif
            __glXFreeContext(oldGC);
         }
      }
      if (gc) {
         __glXSetCurrentContext(gc);

         gc->currentDpy = dpy;
         gc->currentDrawable = draw;
         gc->currentReadable = read;

#ifdef GLX_DIRECT_RENDERING
         if (!gc->driContext) {
#endif
            if (!IndirectAPI)
               IndirectAPI = __glXNewIndirectAPI();
            _glapi_set_dispatch(IndirectAPI);

#ifdef GLX_USE_APPLEGL
            do {
               extern void XAppleDRIUseIndirectDispatch(void);
               XAppleDRIUseIndirectDispatch();
            } while (0);
#endif

            state = (__GLXattribute *) (gc->client_state_private);

            gc->currentContextTag = reply.contextTag;
            if (state->array_state == NULL) {
               (void) glGetString(GL_EXTENSIONS);
               (void) glGetString(GL_VERSION);
               __glXInitVertexArrayState(gc);
            }
#ifdef GLX_DIRECT_RENDERING
         }
         else {
            gc->currentContextTag = -1;
         }
#endif
      }
      else {
         __glXSetCurrentContextNull();
      }
   }
   __glXUnlock();
   return GL_TRUE;
}


PUBLIC Bool
glXMakeCurrent(Display * dpy, GLXDrawable draw, GLXContext gc)
{
   return MakeContextCurrent(dpy, draw, draw, gc, True);
}

PUBLIC
GLX_ALIAS(Bool, glXMakeCurrentReadSGI,
          (Display * dpy, GLXDrawable d, GLXDrawable r, GLXContext ctx),
          (dpy, d, r, ctx, False), MakeContextCurrent)

     PUBLIC GLX_ALIAS(Bool, glXMakeContextCurrent,
                      (Display * dpy, GLXDrawable d, GLXDrawable r,
                       GLXContext ctx), (dpy, d, r, ctx, False),
                      MakeContextCurrent)
