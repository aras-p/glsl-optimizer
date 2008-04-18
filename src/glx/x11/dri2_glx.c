/*
 * Copyright © 2008 Red Hat, Inc.
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
 *   Kristian Høgsberg (krh@redhat.com)
 */

#ifdef GLX_DIRECT_RENDERING

#include <unistd.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xdamage.h>
#include "glheader.h"
#include "glxclient.h"
#include "glcontextmodes.h"
#include "xf86dri.h"
#include "sarea.h"
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "xf86drm.h"
#include "dri2.h"


#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif

typedef struct __GLXDRIdisplayPrivateRec __GLXDRIdisplayPrivate;
typedef struct __GLXDRIcontextPrivateRec __GLXDRIcontextPrivate;
typedef struct __GLXDRIconfigPrivateRec  __GLXDRIconfigPrivate;

struct __GLXDRIdisplayPrivateRec {
    __GLXDRIdisplay base;

    /*
    ** XFree86-DRI version information
    */
    int driMajor;
    int driMinor;
    int driPatch;
};

struct __GLXDRIcontextPrivateRec {
    __GLXDRIcontext base;
    __DRIcontext *driContext;
    __GLXscreenConfigs *psc;
};

struct __GLXDRIconfigPrivateRec {
    __GLcontextModes modes;
    const __DRIconfig *driConfig;
};

static void dri2DestroyContext(__GLXDRIcontext *context,
			      __GLXscreenConfigs *psc, Display *dpy)
{
    __GLXDRIcontextPrivate *pcp = (__GLXDRIcontextPrivate *) context;
    const __DRIcoreExtension *core = pcp->psc->core;

    (*core->destroyContext)(pcp->driContext);

    Xfree(pcp);
}

static Bool dri2BindContext(__GLXDRIcontext *context,
			   __GLXDRIdrawable *draw, __GLXDRIdrawable *read)
{
    __GLXDRIcontextPrivate *pcp = (__GLXDRIcontextPrivate *) context;
    const __DRIcoreExtension *core = pcp->psc->core;

    return (*core->bindContext)(pcp->driContext,
				draw->driDrawable,
				read->driDrawable);
}

static void dri2UnbindContext(__GLXDRIcontext *context)
{
    __GLXDRIcontextPrivate *pcp = (__GLXDRIcontextPrivate *) context;
    const __DRIcoreExtension *core = pcp->psc->core;

    (*core->unbindContext)(pcp->driContext);
}

static __GLXDRIcontext *dri2CreateContext(__GLXscreenConfigs *psc,
					 const __GLcontextModes *mode,
					 GLXContext gc,
					 GLXContext shareList, int renderType)
{
    __GLXDRIcontextPrivate *pcp, *pcp_shared;
    __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) mode;
    const __DRIcoreExtension *core = psc->core;
    __DRIcontext *shared = NULL;

    if (shareList) {
	pcp_shared = (__GLXDRIcontextPrivate *) shareList->driContext;
	shared = pcp_shared->driContext;
    }

    pcp = Xmalloc(sizeof *pcp);
    if (pcp == NULL)
	return NULL;

    pcp->psc = psc;
    pcp->driContext = 
	(*core->createNewContext)(psc->__driScreen,
				  config->driConfig, shared, pcp);
    gc->__driContext = pcp->driContext;

    if (pcp->driContext == NULL) {
	Xfree(pcp);
	return NULL;
    }

    pcp->base.destroyContext = dri2DestroyContext;
    pcp->base.bindContext = dri2BindContext;
    pcp->base.unbindContext = dri2UnbindContext;

    return &pcp->base;
}

static void dri2DestroyDrawable(__GLXDRIdrawable *pdraw)
{
    const __DRIcoreExtension *core = pdraw->psc->core;

    (*core->destroyDrawable)(pdraw->driDrawable);
    DRI2DestroyDrawable(pdraw->psc->dpy, pdraw->drawable);
    Xfree(pdraw);
}

static __GLXDRIdrawable *dri2CreateDrawable(__GLXscreenConfigs *psc,
					    XID xDrawable,
					    GLXDrawable drawable,
					    const __GLcontextModes *modes)
{
    __GLXDRIdrawable *pdraw;
    __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) modes;
    unsigned int handle, head;
    const __DRIcoreExtension *core = psc->core;

    pdraw = Xmalloc(sizeof(*pdraw));
    if (!pdraw)
	return NULL;

    pdraw->destroyDrawable = dri2DestroyDrawable;
    pdraw->xDrawable = xDrawable;
    pdraw->drawable = drawable;
    pdraw->psc = psc;

    fprintf(stderr, "calling DRI2CreateDrawable, XID 0x%lx, GLX ID 0x%lx\n",
	    xDrawable, drawable);

    if (!DRI2CreateDrawable(psc->dpy, xDrawable, &handle, &head)) {
	Xfree(pdraw);
	return NULL;
    }

    fprintf(stderr, "success, head 0x%x, handle 0x%x\n", head, handle);

    /* Create a new drawable */
    pdraw->driDrawable =
	(*core->createNewDrawable)(psc->__driScreen,
				   config->driConfig,
				   handle,
				   head,
				   pdraw);

    if (!pdraw->driDrawable) {
	DRI2DestroyDrawable(psc->dpy, drawable);
	Xfree(pdraw);
	return NULL;
    }

    return pdraw;
}

static void dri2DestroyScreen(__GLXscreenConfigs *psc)
{
    /* Free the direct rendering per screen data */
    (*psc->core->destroyScreen)(psc->__driScreen);
    drmClose(psc->fd);
    psc->__driScreen = NULL;
}


static void dri2ReemitDrawableInfo(__DRIdrawable *draw, unsigned int *tail,
				    void *loaderPrivate)
{
    __GLXDRIdrawable *pdraw = loaderPrivate;

    DRI2ReemitDrawableInfo(pdraw->psc->dpy, pdraw->drawable, tail);
}

static void dri2PostDamage(__DRIdrawable *draw,
			   struct drm_clip_rect *rects,
			   int numRects, void *loaderPrivate)
{ 
    XRectangle *xrects;
    XserverRegion region;
    __GLXDRIdrawable *glxDraw = loaderPrivate;
    __GLXscreenConfigs *psc = glxDraw->psc;
    Display *dpy = psc->dpy;
    int i;

    xrects = malloc(sizeof(XRectangle) * numRects);
    if (xrects == NULL)
	return;

    for (i = 0; i < numRects; i++) {
	xrects[i].x = rects[i].x1;
	xrects[i].y = rects[i].y1;
	xrects[i].width = rects[i].x2 - rects[i].x1;
	xrects[i].height = rects[i].y2 - rects[i].y1;
    }
    region = XFixesCreateRegion(dpy, xrects, numRects);
    free(xrects);
    XDamageAdd(dpy, glxDraw->xDrawable, region);
    XFixesDestroyRegion(dpy, region);
}

static const __DRIloaderExtension dri2LoaderExtension = {
    { __DRI_LOADER, __DRI_LOADER_VERSION },
    dri2ReemitDrawableInfo,
    dri2PostDamage
};

_X_HIDDEN const __DRIsystemTimeExtension systemTimeExtension;

static const __DRIextension *loader_extensions[] = {
    &dri2LoaderExtension.base,
    &systemTimeExtension.base,
    NULL
};

/* We need a dri_common.h type-of-thing. */

extern void ErrorMessageF(const char *f, ...);

extern void *driOpenDriver(const char *driverName);

extern __GLcontextModes *
driConvertConfigs(const __DRIcoreExtension *core,
		  __GLcontextModes *modes, const __DRIconfig **configs);

extern void driBindExtensions(__GLXscreenConfigs *psc);

void
driBindExtensions(__GLXscreenConfigs *psc)
{
    const __DRIextension **extensions;
    int i;

    extensions = psc->core->getExtensions(psc->__driScreen);

    for (i = 0; extensions[i]; i++) {
#ifdef __DRI_COPY_SUB_BUFFER
	if (strcmp(extensions[i]->name, __DRI_COPY_SUB_BUFFER) == 0) {
	    psc->copySubBuffer = (__DRIcopySubBufferExtension *) extensions[i];
	    __glXEnableDirectExtension(psc, "GLX_MESA_copy_sub_buffer_bit");
	}
#endif

#ifdef __DRI_SWAP_CONTROL
	if (strcmp(extensions[i]->name, __DRI_SWAP_CONTROL) == 0) {
	    psc->swapControl = (__DRIswapControlExtension *) extensions[i];
	    __glXEnableDirectExtension(psc, "GLX_SGI_swap_control");
	    __glXEnableDirectExtension(psc, "GLX_MESA_swap_control");
	}
#endif

#ifdef __DRI_ALLOCATE
	if (strcmp(extensions[i]->name, __DRI_ALLOCATE) == 0) {
	    psc->allocate = (__DRIallocateExtension *) extensions[i];
	    __glXEnableDirectExtension(psc, "GLX_MESA_allocate_memory");
	}
#endif

#ifdef __DRI_FRAME_TRACKING
	if (strcmp(extensions[i]->name, __DRI_FRAME_TRACKING) == 0) {
	    psc->frameTracking = (__DRIframeTrackingExtension *) extensions[i];
	    __glXEnableDirectExtension(psc, "GLX_MESA_swap_frame_usage");
	}
#endif

#ifdef __DRI_MEDIA_STREAM_COUNTER
	if (strcmp(extensions[i]->name, __DRI_MEDIA_STREAM_COUNTER) == 0) {
	    psc->msc = (__DRImediaStreamCounterExtension *) extensions[i];
	    __glXEnableDirectExtension(psc, "GLX_SGI_video_sync");
	}
#endif

#ifdef __DRI_SWAP_BUFFER_COUNTER
	/* No driver supports this at this time and the extension is
	 * not defined in dri_interface.h.  Will enable
	 * GLX_OML_sync_control if implemented. */
#endif

#ifdef __DRI_READ_DRAWABLE
	if (strcmp(extensions[i]->name, __DRI_READ_DRAWABLE) == 0) {
	    __glXEnableDirectExtension(psc, "GLX_SGI_make_current_read");
	}
#endif

#ifdef __DRI_TEX_BUFFER
	if (strcmp(extensions[i]->name, __DRI_TEX_BUFFER) == 0) {
	    psc->texBuffer = (__DRItexBufferExtension *) extensions[i];
	    __glXEnableDirectExtension(psc, "GLX_EXT_texture_from_pixmap");
	}
#endif

	/* Ignore unknown extensions */
    }
}

static __GLXDRIscreen *dri2CreateScreen(__GLXscreenConfigs *psc, int screen,
					__GLXdisplayPrivate *priv)
{
    const __DRIconfig **driver_configs;
    const __DRIextension **extensions;
    __GLXDRIscreen *psp;
    unsigned int sareaHandle;
    char *driverName, *busID;
    drm_magic_t magic;
    int i;

    psp = Xmalloc(sizeof *psp);
    if (psp == NULL)
	return NULL;

    /* Initialize per screen dynamic client GLX extensions */
    psc->ext_list_first_time = GL_TRUE;

    if (!DRI2Connect(psc->dpy, screen, &driverName, &busID, &sareaHandle))
	return NULL;

    psc->driver = driOpenDriver(driverName);
    if (psc->driver == NULL)
	goto handle_error;

    extensions = dlsym(psc->driver, __DRI_DRIVER_EXTENSIONS);
    if (extensions == NULL) {
	ErrorMessageF("driver exports no extensions (%s)\n", dlerror());
	goto handle_error;
    }
    
    for (i = 0; extensions[i]; i++) {
	if (strcmp(extensions[i]->name, __DRI_CORE) == 0)
	    psc->core = (__DRIcoreExtension *) extensions[i];
    }

    if (psc->core == NULL) {
	ErrorMessageF("core dri extension not found\n");
	goto handle_error;
    }

    psc->fd = drmOpen(NULL, busID);
    if (psc->fd < 0) {
	ErrorMessageF("failed to open drm device: %s\n", strerror(errno));
	return NULL;
    }

    if (drmGetMagic(psc->fd, &magic))
	return NULL;

    if (!DRI2AuthConnection(psc->dpy, screen, magic)) {
	ErrorMessageF("failed to authenticate drm access\n");
	return NULL;
    }

    psc->__driScreen = 
	psc->core->createNewScreen(screen, psc->fd, sareaHandle,
				   loader_extensions, &driver_configs, psc);
    if (psc->__driScreen == NULL) {
	ErrorMessageF("failed to create dri screen\n");
	return NULL;
    }

    driBindExtensions(psc);

    psc->configs = driConvertConfigs(psc->core, psc->configs, driver_configs);
    psc->visuals = driConvertConfigs(psc->core, psc->visuals, driver_configs);

    psp->destroyScreen = dri2DestroyScreen;
    psp->createContext = dri2CreateContext;
    psp->createDrawable = dri2CreateDrawable;

    Xfree(driverName);
    Xfree(busID);

    return psp;

 handle_error:
    Xfree(driverName);
    Xfree(busID);

    /* FIXME: clean up here */

    return NULL;
}

/* Called from __glXFreeDisplayPrivate.
 */
static void dri2DestroyDisplay(__GLXDRIdisplay *dpy)
{
    Xfree(dpy);
}

/*
 * Allocate, initialize and return a __DRIdisplayPrivate object.
 * This is called from __glXInitialize() when we are given a new
 * display pointer.
 */
_X_HIDDEN __GLXDRIdisplay *dri2CreateDisplay(Display *dpy)
{
    __GLXDRIdisplayPrivate *pdp;
    int eventBase, errorBase;

    if (!DRI2QueryExtension(dpy, &eventBase, &errorBase))
	return NULL;

    pdp = Xmalloc(sizeof *pdp);
    if (pdp == NULL)
	return NULL;

    if (!DRI2QueryVersion(dpy, &pdp->driMajor, &pdp->driMinor)) {
	Xfree(pdp);
	return NULL;
    }

    pdp->driPatch = 0;

    pdp->base.destroyDisplay = dri2DestroyDisplay;
    pdp->base.createScreen = dri2CreateScreen;

    return &pdp->base;
}

#endif /* GLX_DIRECT_RENDERING */
