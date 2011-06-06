/*
 * Copyright © 2010 Intel Corporation
 * Copyright © 2011 Apple Inc.
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

#if defined(GLX_USE_APPLEGL)

#include <stdbool.h>

#include "glxclient.h"
#include "apple_glx_context.h"
#include "apple_glx.h"
#include "glx_error.h"

static void
applegl_destroy_context(struct glx_context *gc)
{
   apple_glx_destroy_context(&gc->driContext, gc->currentDpy);
}

static int
applegl_bind_context(struct glx_context *gc, struct glx_context *old,
		     GLXDrawable draw, GLXDrawable read)
{
   Display *dpy = gc->psc->dpy;
   bool error = apple_glx_make_current_context(dpy,
					       (oldGC && oldGC != &dummyContext) ? oldGC->driContext : NULL
					       gc ? gc->driContext : NULL, draw);

   apple_glx_diagnostic("%s: error %s\n", __func__, error ? "YES" : "NO");
   if (error)
      return 1; /* GLXBadContext is the same as Success (0) */

   return Success;
}

static void
applegl_unbind_context(struct glx_context *gc, struct glx_context *new)
{
}

static void
applegl_wait_gl(struct glx_context *gc)
{
   glFinish();
}

static void
applegl_wait_x(struct glx_context *gc)
{
   Display *dpy = gc->psc->dpy;
   apple_glx_waitx(dpy, gc->driContext);
}

static const struct glx_context_vtable applegl_context_vtable = {
   applegl_destroy_context,
   applegl_bind_context,
   applegl_unbind_context,
   applegl_wait_gl,
   applegl_wait_x,
   DRI_glXUseXFont,
   NULL, /* bind_tex_image, */
   NULL, /* release_tex_image, */
};

struct glx_context *
applegl_create_context(struct glx_screen *psc,
		       struct glx_config *mode,
		       struct glx_context *shareList, int renderType)
{
   struct glx_context *gc;
   int errorcode;
   bool x11error;
   Display *dpy;

   /* TODO: Integrate this with apple_glx_create_context and make
    * struct apple_glx_context inherit from struct glx_context. */

   gc = Xcalloc(1, sizeof (*gc));
   if (gc == NULL)
      return NULL;

   if (!glx_context_init(gc, psc, mode)) {
      Xfree(gc);
      return NULL;
   }

   dpy = gc->psc->dpy;

   gc->vtable = &applegl_context_vtable;
   gc->driContext = NULL;
   gc->do_destroy = False;

   /* TODO: darwin: Integrate with above to do indirect */
   if(apple_glx_create_context(&gc->driContext, dpy, screen, fbconfig, 
			       shareList ? shareList->driContext : NULL,
			       &errorcode, &x11error)) {
      __glXSendError(dpy, errorcode, 0, X_GLXCreateContext, x11error);
      gc->vtable->destroy(gc);
      return NULL;
   }

   gc->currentContextTag = -1;
   gc->mode = fbconfig;
   gc->isDirect = 1;
   gc->xid = 1; /* Just something not None, so we know when to destroy
		 * it in MakeContextCurrent. */

   return gc;
}

struct glx_screen_vtable applegl_screen_vtable = {
   applegl_create_context
};

_X_HIDDEN struct glx_screen *
applegl_create_screen(int screen, struct glx_display * priv)
{
   struct glx_screen *psc;

   psc = Xmalloc(sizeof *psc);
   if (psc == NULL)
      return NULL;

   memset(psc, 0, sizeof *psc);
   glx_screen_init(psc, screen, priv);
   psc->vtable = &applegl_screen_vtable;

   return psc;
}

_X_HIDDEN int
applegl_create_display(struct glx_display *glx_dpy)
{
   if(!apple_init_glx(glx_dpy->dpy))
      return 1;

   return GLXBadContext;
}

#endif
