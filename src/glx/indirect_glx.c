/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Soft-
 * ware"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, provided that the above copyright
 * notice(s) and this permission notice appear in all copies of the Soft-
 * ware and that both the above copyright notice(s) and this permission
 * notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
 * ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN
 * THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSE-
 * QUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFOR-
 * MANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization of
 * the copyright holder.
 *
 * Authors:
 *   Kristian Høgsberg (krh@bitplanet.net)
 */

#include "glapi.h"
#include "glxclient.h"

#ifndef GLX_USE_APPLEGL

extern struct _glapi_table *__glXNewIndirectAPI(void);

/*
** All indirect rendering contexts will share the same indirect dispatch table.
*/
static struct _glapi_table *IndirectAPI = NULL;

static void
indirect_destroy_context(struct glx_context *gc)
{
   __glXFreeVertexArrayState(gc);

   free((char *) gc->vendor);
   free((char *) gc->renderer);
   free((char *) gc->version);
   free((char *) gc->extensions);
   __glFreeAttributeState(gc);
   free((char *) gc->buf);
   free((char *) gc->client_state_private);
   free((char *) gc);
}

static Bool
SendMakeCurrentRequest(Display * dpy, CARD8 opcode,
                       GLXContextID gc_id, GLXContextTag gc_tag,
                       GLXDrawable draw, GLXDrawable read,
                       GLXContextTag *out_tag)
{
   xGLXMakeCurrentReply reply;
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
      struct glx_display *priv = __glXInitialize(dpy);

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

   ret = _XReply(dpy, (xReply *) &reply, 0, False);

   if (out_tag)
      *out_tag = reply.contextTag;

   UnlockDisplay(dpy);
   SyncHandle();

   return ret;
}

static int
indirect_bind_context(struct glx_context *gc, struct glx_context *old,
		      GLXDrawable draw, GLXDrawable read)
{
   GLXContextTag tag;
   __GLXattribute *state;
   Display *dpy = gc->psc->dpy;
   int opcode = __glXSetupForCommand(dpy);
   Bool sent;

   if (old != &dummyContext && !old->isDirect && old->psc->dpy == dpy) {
      tag = old->currentContextTag;
      old->currentContextTag = 0;
   } else {
      tag = 0;
   }

   sent = SendMakeCurrentRequest(dpy, opcode, gc->xid, tag, draw, read,
				 &gc->currentContextTag);

   if (!IndirectAPI)
      IndirectAPI = __glXNewIndirectAPI();
   _glapi_set_dispatch(IndirectAPI);

   state = gc->client_state_private;
   if (state->array_state == NULL) {
      glGetString(GL_EXTENSIONS);
      glGetString(GL_VERSION);
      __glXInitVertexArrayState(gc);
   }

   return !sent;
}

static void
indirect_unbind_context(struct glx_context *gc, struct glx_context *new)
{
   Display *dpy = gc->psc->dpy;
   int opcode = __glXSetupForCommand(dpy);

   if (gc == new)
      return;
   
   /* We are either switching to no context, away from a indirect
    * context to a direct context or from one dpy to another and have
    * to send a request to the dpy to unbind the previous context.
    */
   if (!new || new->isDirect || new->psc->dpy != dpy) {
      SendMakeCurrentRequest(dpy, opcode, None,
			     gc->currentContextTag, None, None, NULL);
      gc->currentContextTag = 0;
   }
}

static void
indirect_wait_gl(struct glx_context *gc)
{
   xGLXWaitGLReq *req;
   Display *dpy = gc->currentDpy;

   /* Flush any pending commands out */
   __glXFlushRenderBuffer(gc, gc->pc);

   /* Send the glXWaitGL request */
   LockDisplay(dpy);
   GetReq(GLXWaitGL, req);
   req->reqType = gc->majorOpcode;
   req->glxCode = X_GLXWaitGL;
   req->contextTag = gc->currentContextTag;
   UnlockDisplay(dpy);
   SyncHandle();
}

static void
indirect_wait_x(struct glx_context *gc)
{
   xGLXWaitXReq *req;
   Display *dpy = gc->currentDpy;

   /* Flush any pending commands out */
   __glXFlushRenderBuffer(gc, gc->pc);

   LockDisplay(dpy);
   GetReq(GLXWaitX, req);
   req->reqType = gc->majorOpcode;
   req->glxCode = X_GLXWaitX;
   req->contextTag = gc->currentContextTag;
   UnlockDisplay(dpy);
   SyncHandle();
}

static void
indirect_use_x_font(struct glx_context *gc,
		    Font font, int first, int count, int listBase)
{
   xGLXUseXFontReq *req;
   Display *dpy = gc->currentDpy;

   /* Flush any pending commands out */
   __glXFlushRenderBuffer(gc, gc->pc);

   /* Send the glXUseFont request */
   LockDisplay(dpy);
   GetReq(GLXUseXFont, req);
   req->reqType = gc->majorOpcode;
   req->glxCode = X_GLXUseXFont;
   req->contextTag = gc->currentContextTag;
   req->font = font;
   req->first = first;
   req->count = count;
   req->listBase = listBase;
   UnlockDisplay(dpy);
   SyncHandle();
}

static void
indirect_bind_tex_image(Display * dpy,
			GLXDrawable drawable,
			int buffer, const int *attrib_list)
{
   xGLXVendorPrivateReq *req;
   struct glx_context *gc = __glXGetCurrentContext();
   CARD32 *drawable_ptr;
   INT32 *buffer_ptr;
   CARD32 *num_attrib_ptr;
   CARD32 *attrib_ptr;
   CARD8 opcode;
   unsigned int i;

   i = 0;
   if (attrib_list) {
      while (attrib_list[i * 2] != None)
         i++;
   }

   opcode = __glXSetupForCommand(dpy);
   if (!opcode)
      return;

   LockDisplay(dpy);
   GetReqExtra(GLXVendorPrivate, 12 + 8 * i, req);
   req->reqType = opcode;
   req->glxCode = X_GLXVendorPrivate;
   req->vendorCode = X_GLXvop_BindTexImageEXT;
   req->contextTag = gc->currentContextTag;

   drawable_ptr = (CARD32 *) (req + 1);
   buffer_ptr = (INT32 *) (drawable_ptr + 1);
   num_attrib_ptr = (CARD32 *) (buffer_ptr + 1);
   attrib_ptr = (CARD32 *) (num_attrib_ptr + 1);

   *drawable_ptr = drawable;
   *buffer_ptr = buffer;
   *num_attrib_ptr = (CARD32) i;

   i = 0;
   if (attrib_list) {
      while (attrib_list[i * 2] != None) {
         *attrib_ptr++ = (CARD32) attrib_list[i * 2 + 0];
         *attrib_ptr++ = (CARD32) attrib_list[i * 2 + 1];
         i++;
      }
   }

   UnlockDisplay(dpy);
   SyncHandle();
}

static void
indirect_release_tex_image(Display * dpy, GLXDrawable drawable, int buffer)
{
   xGLXVendorPrivateReq *req;
   struct glx_context *gc = __glXGetCurrentContext();
   CARD32 *drawable_ptr;
   INT32 *buffer_ptr;
   CARD8 opcode;

   opcode = __glXSetupForCommand(dpy);
   if (!opcode)
      return;

   LockDisplay(dpy);
   GetReqExtra(GLXVendorPrivate, sizeof(CARD32) + sizeof(INT32), req);
   req->reqType = opcode;
   req->glxCode = X_GLXVendorPrivate;
   req->vendorCode = X_GLXvop_ReleaseTexImageEXT;
   req->contextTag = gc->currentContextTag;

   drawable_ptr = (CARD32 *) (req + 1);
   buffer_ptr = (INT32 *) (drawable_ptr + 1);

   *drawable_ptr = drawable;
   *buffer_ptr = buffer;

   UnlockDisplay(dpy);
   SyncHandle();
}

static const struct glx_context_vtable indirect_context_vtable = {
   .destroy             = indirect_destroy_context,
   .bind                = indirect_bind_context,
   .unbind              = indirect_unbind_context,
   .wait_gl             = indirect_wait_gl,
   .wait_x              = indirect_wait_x,
   .use_x_font          = indirect_use_x_font,
   .bind_tex_image      = indirect_bind_tex_image,
   .release_tex_image   = indirect_release_tex_image,
   .get_proc_address    = NULL,
};

/**
 * \todo Eliminate \c __glXInitVertexArrayState.  Replace it with a new
 * function called \c __glXAllocateClientState that allocates the memory and
 * does all the initialization (including the pixel pack / unpack).
 *
 * \note
 * This function is \b not the place to validate the context creation
 * parameters.  It is just the allocator for the \c glx_context.
 */
_X_HIDDEN struct glx_context *
indirect_create_context(struct glx_screen *psc,
			struct glx_config *mode,
			struct glx_context *shareList, int renderType)
{
   struct glx_context *gc;
   int bufSize;
   CARD8 opcode;
   __GLXattribute *state;

   opcode = __glXSetupForCommand(psc->dpy);
   if (!opcode) {
      return NULL;
   }

   /* Allocate our context record */
   gc = calloc(1, sizeof *gc);
   if (!gc) {
      /* Out of memory */
      return NULL;
   }

   glx_context_init(gc, psc, mode);
   gc->isDirect = GL_FALSE;
   gc->vtable = &indirect_context_vtable;
   state = calloc(1, sizeof(struct __GLXattributeRec));
   gc->renderType = renderType;

   if (state == NULL) {
      /* Out of memory */
      free(gc);
      return NULL;
   }
   gc->client_state_private = state;
   state->NoDrawArraysProtocol = (getenv("LIBGL_NO_DRAWARRAYS") != NULL);

   /*
    ** Create a temporary buffer to hold GLX rendering commands.  The size
    ** of the buffer is selected so that the maximum number of GLX rendering
    ** commands can fit in a single X packet and still have room in the X
    ** packet for the GLXRenderReq header.
    */

   bufSize = (XMaxRequestSize(psc->dpy) * 4) - sz_xGLXRenderReq;
   gc->buf = malloc(bufSize);
   if (!gc->buf) {
      free(gc->client_state_private);
      free(gc);
      return NULL;
   }
   gc->bufSize = bufSize;

   /* Fill in the new context */
   gc->renderMode = GL_RENDER;

   state->storePack.alignment = 4;
   state->storeUnpack.alignment = 4;

   gc->attributes.stackPointer = &gc->attributes.stack[0];

   /*
    ** PERFORMANCE NOTE: A mode dependent fill image can speed things up.
    */
   gc->fillImage = __glFillImage;
   gc->pc = gc->buf;
   gc->bufEnd = gc->buf + bufSize;
   gc->isDirect = GL_FALSE;
   if (__glXDebug) {
      /*
       ** Set limit register so that there will be one command per packet
       */
      gc->limit = gc->buf;
   }
   else {
      gc->limit = gc->buf + bufSize - __GLX_BUFFER_LIMIT_SIZE;
   }
   gc->majorOpcode = opcode;

   /*
    ** Constrain the maximum drawing command size allowed to be
    ** transfered using the X_GLXRender protocol request.  First
    ** constrain by a software limit, then constrain by the protocl
    ** limit.
    */
   if (bufSize > __GLX_RENDER_CMD_SIZE_LIMIT) {
      bufSize = __GLX_RENDER_CMD_SIZE_LIMIT;
   }
   if (bufSize > __GLX_MAX_RENDER_CMD_SIZE) {
      bufSize = __GLX_MAX_RENDER_CMD_SIZE;
   }
   gc->maxSmallRenderCommandSize = bufSize;
   

   return gc;
}

_X_HIDDEN struct glx_context *
indirect_create_context_attribs(struct glx_screen *base,
				struct glx_config *config_base,
				struct glx_context *shareList,
				unsigned num_attribs,
				const uint32_t *attribs,
				unsigned *error)
{
   int renderType = GLX_RGBA_TYPE;
   unsigned i;

   /* The error parameter is only used on the server so that correct GLX
    * protocol errors can be generated.  On the client, it can be ignored.
    */
   (void) error;

   /* All of the attribute validation for indirect contexts is handled on the
    * server, so there's not much to do here. Still, we need to parse the
    * attributes to correctly set renderType.
    */
   for (i = 0; i < num_attribs; i++) {
      if (attribs[i * 2] == GLX_RENDER_TYPE)
         renderType = attribs[i * 2 + 1];
   }

   return indirect_create_context(base, config_base, shareList, renderType);
}

static const struct glx_screen_vtable indirect_screen_vtable = {
   .create_context         = indirect_create_context,
   .create_context_attribs = indirect_create_context_attribs,
   .query_renderer_integer = NULL,
   .query_renderer_string  = NULL,
};

_X_HIDDEN struct glx_screen *
indirect_create_screen(int screen, struct glx_display * priv)
{
   struct glx_screen *psc;

   psc = calloc(1, sizeof *psc);
   if (psc == NULL)
      return NULL;

   glx_screen_init(psc, screen, priv);
   psc->vtable = &indirect_screen_vtable;

   return psc;
}

#endif
