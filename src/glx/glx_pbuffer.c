/*
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file glx_pbuffer.c
 * Implementation of pbuffer related functions.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <inttypes.h>
#include "glxclient.h"
#include <X11/extensions/extutil.h>
#include <X11/extensions/Xext.h>
#include <assert.h>
#include <string.h>
#include "glxextensions.h"

#ifdef GLX_USE_APPLEGL
#include <pthread.h>
#include "apple_glx_drawable.h"
#include "glx_error.h"
#endif

#define WARN_ONCE_GLX_1_3(a, b) {		\
		static int warned=1;		\
		if(warned) {			\
			warn_GLX_1_3((a), b );	\
			warned=0;		\
		}				\
	}

/**
 * Emit a warning when clients use GLX 1.3 functions on pre-1.3 systems.
 */
static void
warn_GLX_1_3(Display * dpy, const char *function_name)
{
   __GLXdisplayPrivate *priv = __glXInitialize(dpy);

   if (priv->minorVersion < 3) {
      fprintf(stderr,
              "WARNING: Application calling GLX 1.3 function \"%s\" "
              "when GLX 1.3 is not supported!  This is an application bug!\n",
              function_name);
   }
}

#ifndef GLX_USE_APPLEGL
/**
 * Change a drawable's attribute.
 *
 * This function is used to implement \c glXSelectEvent and
 * \c glXSelectEventSGIX.
 *
 * \note
 * This function dynamically determines whether to use the SGIX_pbuffer
 * version of the protocol or the GLX 1.3 version of the protocol.
 *
 * \todo
 * This function needs to be modified to work with direct-rendering drivers.
 */
static void
ChangeDrawableAttribute(Display * dpy, GLXDrawable drawable,
                        const CARD32 * attribs, size_t num_attribs)
{
   __GLXdisplayPrivate *priv = __glXInitialize(dpy);
   CARD32 *output;
   CARD8 opcode;

   if ((dpy == NULL) || (drawable == 0)) {
      return;
   }

   opcode = __glXSetupForCommand(dpy);
   if (!opcode)
      return;

   LockDisplay(dpy);

   if ((priv->majorVersion > 1) || (priv->minorVersion >= 3)) {
      xGLXChangeDrawableAttributesReq *req;

      GetReqExtra(GLXChangeDrawableAttributes, 8 + (8 * num_attribs), req);
      output = (CARD32 *) (req + 1);

      req->reqType = opcode;
      req->glxCode = X_GLXChangeDrawableAttributes;
      req->drawable = drawable;
      req->numAttribs = (CARD32) num_attribs;
   }
   else {
      xGLXVendorPrivateWithReplyReq *vpreq;

      GetReqExtra(GLXVendorPrivateWithReply, 4 + (8 * num_attribs), vpreq);
      output = (CARD32 *) (vpreq + 1);

      vpreq->reqType = opcode;
      vpreq->glxCode = X_GLXVendorPrivateWithReply;
      vpreq->vendorCode = X_GLXvop_ChangeDrawableAttributesSGIX;

      output[0] = (CARD32) drawable;
      output++;
   }

   (void) memcpy(output, attribs, sizeof(CARD32) * 2 * num_attribs);

   UnlockDisplay(dpy);
   SyncHandle();

   return;
}


/**
 * Destroy a pbuffer.
 *
 * This function is used to implement \c glXDestroyPbuffer and
 * \c glXDestroyGLXPbufferSGIX.
 *
 * \note
 * This function dynamically determines whether to use the SGIX_pbuffer
 * version of the protocol or the GLX 1.3 version of the protocol.
 *
 * \todo
 * This function needs to be modified to work with direct-rendering drivers.
 */
static void
DestroyPbuffer(Display * dpy, GLXDrawable drawable)
{
   __GLXdisplayPrivate *priv = __glXInitialize(dpy);
   CARD8 opcode;

   if ((dpy == NULL) || (drawable == 0)) {
      return;
   }

   opcode = __glXSetupForCommand(dpy);
   if (!opcode)
      return;

   LockDisplay(dpy);

   if ((priv->majorVersion > 1) || (priv->minorVersion >= 3)) {
      xGLXDestroyPbufferReq *req;

      GetReq(GLXDestroyPbuffer, req);
      req->reqType = opcode;
      req->glxCode = X_GLXDestroyPbuffer;
      req->pbuffer = (GLXPbuffer) drawable;
   }
   else {
      xGLXVendorPrivateWithReplyReq *vpreq;
      CARD32 *data;

      GetReqExtra(GLXVendorPrivateWithReply, 4, vpreq);
      data = (CARD32 *) (vpreq + 1);

      data[0] = (CARD32) drawable;

      vpreq->reqType = opcode;
      vpreq->glxCode = X_GLXVendorPrivateWithReply;
      vpreq->vendorCode = X_GLXvop_DestroyGLXPbufferSGIX;
   }

   UnlockDisplay(dpy);
   SyncHandle();

   return;
}


#ifdef GLX_DIRECT_RENDERING
static GLenum
determineTextureTarget(const int *attribs, int numAttribs)
{
   GLenum target = 0;
   int i;

   for (i = 0; i < numAttribs; i++) {
      if (attribs[2 * i] == GLX_TEXTURE_TARGET_EXT) {
         switch (attribs[2 * i + 1]) {
         case GLX_TEXTURE_2D_EXT:
            target = GL_TEXTURE_2D;
            break;
         case GLX_TEXTURE_RECTANGLE_EXT:
            target = GL_TEXTURE_RECTANGLE_ARB;
            break;
         }
      }
   }

   return target;
}


static GLenum
determineTextureFormat(const int *attribs, int numAttribs)
{
   int i;

   for (i = 0; i < numAttribs; i++) {
      if (attribs[2 * i] == GLX_TEXTURE_FORMAT_EXT)
         return attribs[2 * i + 1];
   }

   return 0;
}
#endif

/**
 * Get a drawable's attribute.
 *
 * This function is used to implement \c glXGetSelectedEvent and
 * \c glXGetSelectedEventSGIX.
 *
 * \note
 * This function dynamically determines whether to use the SGIX_pbuffer
 * version of the protocol or the GLX 1.3 version of the protocol.
 *
 * \todo
 * The number of attributes returned is likely to be small, probably less than
 * 10.  Given that, this routine should try to use an array on the stack to
 * capture the reply rather than always calling Xmalloc.
 *
 * \todo
 * This function needs to be modified to work with direct-rendering drivers.
 */
static int
GetDrawableAttribute(Display * dpy, GLXDrawable drawable,
                     int attribute, unsigned int *value)
{
   __GLXdisplayPrivate *priv;
   xGLXGetDrawableAttributesReply reply;
   CARD32 *data;
   CARD8 opcode;
   unsigned int length;
   unsigned int i;
   unsigned int num_attributes;
   GLboolean use_glx_1_3;

   if ((dpy == NULL) || (drawable == 0)) {
      return 0;
   }

   priv = __glXInitialize(dpy);
   use_glx_1_3 = ((priv->majorVersion > 1) || (priv->minorVersion >= 3));

   *value = 0;


   opcode = __glXSetupForCommand(dpy);
   if (!opcode)
      return 0;

   LockDisplay(dpy);

   if (use_glx_1_3) {
      xGLXGetDrawableAttributesReq *req;

      GetReqExtra(GLXGetDrawableAttributes, 4, req);
      req->reqType = opcode;
      req->glxCode = X_GLXGetDrawableAttributes;
      req->drawable = drawable;
   }
   else {
      xGLXVendorPrivateWithReplyReq *vpreq;

      GetReqExtra(GLXVendorPrivateWithReply, 4, vpreq);
      data = (CARD32 *) (vpreq + 1);
      data[0] = (CARD32) drawable;

      vpreq->reqType = opcode;
      vpreq->glxCode = X_GLXVendorPrivateWithReply;
      vpreq->vendorCode = X_GLXvop_GetDrawableAttributesSGIX;
   }

   _XReply(dpy, (xReply *) & reply, 0, False);

   if (reply.type == X_Error) {
      UnlockDisplay(dpy);
      SyncHandle();
      return 0;
   }

   length = reply.length;
   if (length) {
      num_attributes = (use_glx_1_3) ? reply.numAttribs : length / 2;
      data = (CARD32 *) Xmalloc(length * sizeof(CARD32));
      if (data == NULL) {
         /* Throw data on the floor */
         _XEatData(dpy, length);
      }
      else {
         _XRead(dpy, (char *) data, length * sizeof(CARD32));

         /* Search the set of returned attributes for the attribute requested by
          * the caller.
          */
         for (i = 0; i < num_attributes; i++) {
            if (data[i * 2] == attribute) {
               *value = data[(i * 2) + 1];
               break;
            }
         }

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
         {
            __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable, NULL);

            if (pdraw != NULL && !pdraw->textureTarget)
               pdraw->textureTarget =
                  determineTextureTarget((const int *) data, num_attributes);
            if (pdraw != NULL && !pdraw->textureFormat)
               pdraw->textureFormat =
                  determineTextureFormat((const int *) data, num_attributes);
         }
#endif

         Xfree(data);
      }
   }

   UnlockDisplay(dpy);
   SyncHandle();

   return 0;
}

/**
 * Create a non-pbuffer GLX drawable.
 *
 * \todo
 * This function needs to be modified to work with direct-rendering drivers.
 */
static GLXDrawable
CreateDrawable(Display * dpy, const __GLcontextModes * fbconfig,
               Drawable drawable, const int *attrib_list, CARD8 glxCode)
{
   xGLXCreateWindowReq *req;
   CARD32 *data;
   unsigned int i;
   CARD8 opcode;

   i = 0;
   if (attrib_list) {
      while (attrib_list[i * 2] != None)
         i++;
   }

   opcode = __glXSetupForCommand(dpy);
   if (!opcode)
      return None;

   LockDisplay(dpy);
   GetReqExtra(GLXCreateWindow, 8 * i, req);
   data = (CARD32 *) (req + 1);

   req->reqType = opcode;
   req->glxCode = glxCode;
   req->screen = (CARD32) fbconfig->screen;
   req->fbconfig = fbconfig->fbconfigID;
   req->window = (CARD32) drawable;
   req->glxwindow = (GLXWindow) XAllocID(dpy);
   req->numAttribs = (CARD32) i;

   if (attrib_list)
      memcpy(data, attrib_list, 8 * i);

   UnlockDisplay(dpy);
   SyncHandle();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   do {
      /* FIXME: Maybe delay __DRIdrawable creation until the drawable
       * is actually bound to a context... */

      __GLXdisplayPrivate *const priv = __glXInitialize(dpy);
      __GLXDRIdrawable *pdraw;
      __GLXscreenConfigs *psc;

      psc = &priv->screenConfigs[fbconfig->screen];
      if (psc->driScreen == NULL)
         break;
      pdraw = psc->driScreen->createDrawable(psc, drawable,
                                             req->glxwindow, fbconfig);
      if (pdraw == NULL) {
         fprintf(stderr, "failed to create drawable\n");
         break;
      }

      if (__glxHashInsert(psc->drawHash, req->glxwindow, pdraw)) {
         (*pdraw->destroyDrawable) (pdraw);
         return None;           /* FIXME: Check what we're supposed to do here... */
      }

      pdraw->textureTarget = determineTextureTarget(attrib_list, i);
      pdraw->textureFormat = determineTextureFormat(attrib_list, i);
   } while (0);
#endif

   return (GLXDrawable) req->glxwindow;
}


/**
 * Destroy a non-pbuffer GLX drawable.
 *
 * \todo
 * This function needs to be modified to work with direct-rendering drivers.
 */
static void
DestroyDrawable(Display * dpy, GLXDrawable drawable, CARD32 glxCode)
{
   xGLXDestroyPbufferReq *req;
   CARD8 opcode;

   if ((dpy == NULL) || (drawable == 0)) {
      return;
   }


   opcode = __glXSetupForCommand(dpy);
   if (!opcode)
      return;

   LockDisplay(dpy);

   GetReqExtra(GLXDestroyPbuffer, 4, req);
   req->reqType = opcode;
   req->glxCode = glxCode;
   req->pbuffer = (GLXPbuffer) drawable;

   UnlockDisplay(dpy);
   SyncHandle();

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)
   {
      int screen;
      __GLXdisplayPrivate *const priv = __glXInitialize(dpy);
      __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable, &screen);
      __GLXscreenConfigs *psc = &priv->screenConfigs[screen];

      if (pdraw != NULL) {
         (*pdraw->destroyDrawable) (pdraw);
         __glxHashDelete(psc->drawHash, drawable);
      }
   }
#endif

   return;
}


/**
 * Create a pbuffer.
 *
 * This function is used to implement \c glXCreatePbuffer and
 * \c glXCreateGLXPbufferSGIX.
 *
 * \note
 * This function dynamically determines whether to use the SGIX_pbuffer
 * version of the protocol or the GLX 1.3 version of the protocol.
 *
 * \todo
 * This function needs to be modified to work with direct-rendering drivers.
 */
static GLXDrawable
CreatePbuffer(Display * dpy, const __GLcontextModes * fbconfig,
              unsigned int width, unsigned int height,
              const int *attrib_list, GLboolean size_in_attribs)
{
   __GLXdisplayPrivate *priv = __glXInitialize(dpy);
   GLXDrawable id = 0;
   CARD32 *data;
   CARD8 opcode;
   unsigned int i;

   i = 0;
   if (attrib_list) {
      while (attrib_list[i * 2])
         i++;
   }

   opcode = __glXSetupForCommand(dpy);
   if (!opcode)
      return None;

   LockDisplay(dpy);
   id = XAllocID(dpy);

   if ((priv->majorVersion > 1) || (priv->minorVersion >= 3)) {
      xGLXCreatePbufferReq *req;
      unsigned int extra = (size_in_attribs) ? 0 : 2;

      GetReqExtra(GLXCreatePbuffer, (8 * (i + extra)), req);
      data = (CARD32 *) (req + 1);

      req->reqType = opcode;
      req->glxCode = X_GLXCreatePbuffer;
      req->screen = (CARD32) fbconfig->screen;
      req->fbconfig = fbconfig->fbconfigID;
      req->pbuffer = (GLXPbuffer) id;
      req->numAttribs = (CARD32) (i + extra);

      if (!size_in_attribs) {
         data[(2 * i) + 0] = GLX_PBUFFER_WIDTH;
         data[(2 * i) + 1] = width;
         data[(2 * i) + 2] = GLX_PBUFFER_HEIGHT;
         data[(2 * i) + 3] = height;
         data += 4;
      }
   }
   else {
      xGLXVendorPrivateReq *vpreq;

      GetReqExtra(GLXVendorPrivate, 20 + (8 * i), vpreq);
      data = (CARD32 *) (vpreq + 1);

      vpreq->reqType = opcode;
      vpreq->glxCode = X_GLXVendorPrivate;
      vpreq->vendorCode = X_GLXvop_CreateGLXPbufferSGIX;

      data[0] = (CARD32) fbconfig->screen;
      data[1] = (CARD32) fbconfig->fbconfigID;
      data[2] = (CARD32) id;
      data[3] = (CARD32) width;
      data[4] = (CARD32) height;
      data += 5;
   }

   (void) memcpy(data, attrib_list, sizeof(CARD32) * 2 * i);

   UnlockDisplay(dpy);
   SyncHandle();

   return id;
}

/**
 * Create a new pbuffer.
 */
PUBLIC GLXPbufferSGIX
glXCreateGLXPbufferSGIX(Display * dpy, GLXFBConfigSGIX config,
                        unsigned int width, unsigned int height,
                        int *attrib_list)
{
   return (GLXPbufferSGIX) CreatePbuffer(dpy, (__GLcontextModes *) config,
                                         width, height,
                                         attrib_list, GL_FALSE);
}

#endif /* GLX_USE_APPLEGL */

/**
 * Create a new pbuffer.
 */
PUBLIC GLXPbuffer
glXCreatePbuffer(Display * dpy, GLXFBConfig config, const int *attrib_list)
{
   int i, width, height;
#ifdef GLX_USE_APPLEGL
   GLXPbuffer result;
   int errorcode;
#endif

   width = 0;
   height = 0;

   WARN_ONCE_GLX_1_3(dpy, __func__);

#ifdef GLX_USE_APPLEGL
   for (i = 0; attrib_list[i]; ++i) {
      switch (attrib_list[i]) {
      case GLX_PBUFFER_WIDTH:
         width = attrib_list[i + 1];
         ++i;
         break;

      case GLX_PBUFFER_HEIGHT:
         height = attrib_list[i + 1];
         ++i;
         break;

      case GLX_LARGEST_PBUFFER:
         /* This is a hint we should probably handle, but how? */
         ++i;
         break;

      case GLX_PRESERVED_CONTENTS:
         /* The contents are always preserved with AppleSGLX with CGL. */
         ++i;
         break;

      default:
         return None;
      }
   }

   if (apple_glx_pbuffer_create(dpy, config, width, height, &errorcode,
                                &result)) {
      /* 
       * apple_glx_pbuffer_create only sets the errorcode to core X11
       * errors. 
       */
      __glXSendError(dpy, errorcode, 0, X_GLXCreatePbuffer, true);

      return None;
   }

   return result;
#else
   for (i = 0; attrib_list[i * 2]; i++) {
      switch (attrib_list[i * 2]) {
      case GLX_PBUFFER_WIDTH:
         width = attrib_list[i * 2 + 1];
         break;
      case GLX_PBUFFER_HEIGHT:
         height = attrib_list[i * 2 + 1];
         break;
      }
   }

   return (GLXPbuffer) CreatePbuffer(dpy, (__GLcontextModes *) config,
                                     width, height, attrib_list, GL_TRUE);
#endif
}


/**
 * Destroy an existing pbuffer.
 */
PUBLIC void
glXDestroyPbuffer(Display * dpy, GLXPbuffer pbuf)
{
#ifdef GLX_USE_APPLEGL
   if (apple_glx_pbuffer_destroy(dpy, pbuf)) {
      __glXSendError(dpy, GLXBadPbuffer, pbuf, X_GLXDestroyPbuffer, false);
   }
#else
   DestroyPbuffer(dpy, pbuf);
#endif
}


/**
 * Query an attribute of a drawable.
 */
PUBLIC void
glXQueryDrawable(Display * dpy, GLXDrawable drawable,
                 int attribute, unsigned int *value)
{
   WARN_ONCE_GLX_1_3(dpy, __func__);
#ifdef GLX_USE_APPLEGL
   Window root;
   int x, y;
   unsigned int width, height, bd, depth;

   if (apple_glx_pixmap_query(drawable, attribute, value))
      return;                   /*done */

   if (apple_glx_pbuffer_query(drawable, attribute, value))
      return;                   /*done */

   /*
    * The OpenGL spec states that we should report GLXBadDrawable if
    * the drawable is invalid, however doing so would require that we
    * use XSetErrorHandler(), which is known to not be thread safe.
    * If we use a round-trip call to validate the drawable, there could
    * be a race, so instead we just opt in favor of letting the
    * XGetGeometry request fail with a GetGeometry request X error 
    * rather than GLXBadDrawable, in what is hoped to be a rare
    * case of an invalid drawable.  In practice most and possibly all
    * X11 apps using GLX shouldn't notice a difference.
    */
   if (XGetGeometry
       (dpy, drawable, &root, &x, &y, &width, &height, &bd, &depth)) {
      switch (attribute) {
      case GLX_WIDTH:
         *value = width;
         break;

      case GLX_HEIGHT:
         *value = height;
         break;
      }
   }
#else
   GetDrawableAttribute(dpy, drawable, attribute, value);
#endif
}


#ifndef GLX_USE_APPLEGL
/**
 * Query an attribute of a pbuffer.
 */
PUBLIC int
glXQueryGLXPbufferSGIX(Display * dpy, GLXPbufferSGIX drawable,
                       int attribute, unsigned int *value)
{
   return GetDrawableAttribute(dpy, drawable, attribute, value);
}
#endif

/**
 * Select the event mask for a drawable.
 */
PUBLIC void
glXSelectEvent(Display * dpy, GLXDrawable drawable, unsigned long mask)
{
#ifdef GLX_USE_APPLEGL
   XWindowAttributes xwattr;

   if (apple_glx_pbuffer_set_event_mask(drawable, mask))
      return;                   /*done */

   /* 
    * The spec allows a window, but currently there are no valid
    * events for a window, so do nothing.
    */
   if (XGetWindowAttributes(dpy, drawable, &xwattr))
      return;                   /*done */
   /* The drawable seems to be invalid.  Report an error. */

   __glXSendError(dpy, GLXBadDrawable, drawable,
                  X_GLXChangeDrawableAttributes, false);
#else
   CARD32 attribs[2];

   attribs[0] = (CARD32) GLX_EVENT_MASK;
   attribs[1] = (CARD32) mask;

   ChangeDrawableAttribute(dpy, drawable, attribs, 1);
#endif
}


/**
 * Get the selected event mask for a drawable.
 */
PUBLIC void
glXGetSelectedEvent(Display * dpy, GLXDrawable drawable, unsigned long *mask)
{
#ifdef GLX_USE_APPLEGL
   XWindowAttributes xwattr;

   if (apple_glx_pbuffer_get_event_mask(drawable, mask))
      return;                   /*done */

   /* 
    * The spec allows a window, but currently there are no valid
    * events for a window, so do nothing, but set the mask to 0.
    */
   if (XGetWindowAttributes(dpy, drawable, &xwattr)) {
      /* The window is valid, so set the mask to 0. */
      *mask = 0;
      return;                   /*done */
   }
   /* The drawable seems to be invalid.  Report an error. */

   __glXSendError(dpy, GLXBadDrawable, drawable, X_GLXGetDrawableAttributes,
                  true);
#else
   unsigned int value;


   /* The non-sense with value is required because on LP64 platforms
    * sizeof(unsigned int) != sizeof(unsigned long).  On little-endian
    * we could just type-cast the pointer, but why?
    */

   GetDrawableAttribute(dpy, drawable, GLX_EVENT_MASK_SGIX, &value);
   *mask = value;
#endif
}


PUBLIC GLXPixmap
glXCreatePixmap(Display * dpy, GLXFBConfig config, Pixmap pixmap,
                const int *attrib_list)
{
   WARN_ONCE_GLX_1_3(dpy, __func__);

#ifdef GLX_USE_APPLEGL
   const __GLcontextModes *modes = (const __GLcontextModes *) config;

   if (apple_glx_pixmap_create(dpy, modes->screen, pixmap, modes))
      return None;

   return pixmap;
#else
   return CreateDrawable(dpy, (__GLcontextModes *) config,
                         (Drawable) pixmap, attrib_list, X_GLXCreatePixmap);
#endif
}


PUBLIC GLXWindow
glXCreateWindow(Display * dpy, GLXFBConfig config, Window win,
                const int *attrib_list)
{
   WARN_ONCE_GLX_1_3(dpy, __func__);
#ifdef GLX_USE_APPLEGL
   XWindowAttributes xwattr;
   XVisualInfo *visinfo;

   (void) attrib_list;          /*unused according to GLX 1.4 */

   XGetWindowAttributes(dpy, win, &xwattr);

   visinfo = glXGetVisualFromFBConfig(dpy, config);

   if (NULL == visinfo) {
      __glXSendError(dpy, GLXBadFBConfig, 0, X_GLXCreateWindow, false);
      return None;
   }

   if (visinfo->visualid != XVisualIDFromVisual(xwattr.visual)) {
      __glXSendError(dpy, BadMatch, 0, X_GLXCreateWindow, true);
      return None;
   }

   XFree(visinfo);

   return win;
#else
   return CreateDrawable(dpy, (__GLcontextModes *) config,
                         (Drawable) win, attrib_list, X_GLXCreateWindow);
#endif
}


PUBLIC void
glXDestroyPixmap(Display * dpy, GLXPixmap pixmap)
{
   WARN_ONCE_GLX_1_3(dpy, __func__);
#ifdef GLX_USE_APPLEGL
   if (apple_glx_pixmap_destroy(dpy, pixmap))
      __glXSendError(dpy, GLXBadPixmap, pixmap, X_GLXDestroyPixmap, false);
#else
   DestroyDrawable(dpy, (GLXDrawable) pixmap, X_GLXDestroyPixmap);
#endif
}


PUBLIC void
glXDestroyWindow(Display * dpy, GLXWindow win)
{
   WARN_ONCE_GLX_1_3(dpy, __func__);
#ifndef GLX_USE_APPLEGL
   DestroyDrawable(dpy, (GLXDrawable) win, X_GLXDestroyWindow);
#endif
}

#ifndef GLX_USE_APPLEGL
PUBLIC
GLX_ALIAS_VOID(glXDestroyGLXPbufferSGIX,
               (Display * dpy, GLXPbufferSGIX pbuf),
               (dpy, pbuf), glXDestroyPbuffer)

PUBLIC
GLX_ALIAS_VOID(glXSelectEventSGIX,
               (Display * dpy, GLXDrawable drawable,
                unsigned long mask), (dpy, drawable, mask), glXSelectEvent)

PUBLIC
GLX_ALIAS_VOID(glXGetSelectedEventSGIX,
               (Display * dpy, GLXDrawable drawable,
                unsigned long *mask), (dpy, drawable, mask),
               glXGetSelectedEvent)
#endif
