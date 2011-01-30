/*
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2010 George Sapountzis <gsapountzis@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file drisw_util.c
 *
 * DRISW utility functions, i.e. dri_util.c stripped from drm-specific bits.
 */

#include "drisw_util.h"
#include "utils.h"


/**
 * Screen functions
 */

static void
setupLoaderExtensions(__DRIscreen *psp,
		      const __DRIextension **extensions)
{
    int i;

    for (i = 0; extensions[i]; i++) {
	if (strcmp(extensions[i]->name, __DRI_SWRAST_LOADER) == 0)
	    psp->swrast_loader = (__DRIswrastLoaderExtension *) extensions[i];
    }
}

static __DRIscreen *
driCreateNewScreen(int scrn, const __DRIextension **extensions,
		   const __DRIconfig ***driver_configs, void *data)
{
    static const __DRIextension *emptyExtensionList[] = { NULL };
    __DRIscreen *psp;

    (void) data;

    psp = CALLOC_STRUCT(__DRIscreenRec);
    if (!psp)
	return NULL;

    setupLoaderExtensions(psp, extensions);

    psp->extensions = emptyExtensionList;
    psp->fd = -1;
    psp->myNum = scrn;

    *driver_configs = driDriverAPI.InitScreen(psp);

    if (*driver_configs == NULL) {
	FREE(psp);
	return NULL;
    }

    return psp;
}

static void driDestroyScreen(__DRIscreen *psp)
{
    if (psp) {
	driDriverAPI.DestroyScreen(psp);

	FREE(psp);
    }
}

static const __DRIextension **driGetExtensions(__DRIscreen *psp)
{
    return psp->extensions;
}


/**
 * Context functions
 */

static __DRIcontext *
driCreateNewContext(__DRIscreen *psp, const __DRIconfig *config,
		    __DRIcontext *shared, void *data)
{
    __DRIcontext *pcp;
    void * const shareCtx = (shared != NULL) ? shared->driverPrivate : NULL;

    pcp = CALLOC_STRUCT(__DRIcontextRec);
    if (!pcp)
	return NULL;

    pcp->loaderPrivate = data;

    pcp->driScreenPriv = psp;
    pcp->driDrawablePriv = NULL;
    pcp->driReadablePriv = NULL;

    if (!driDriverAPI.CreateContext(API_OPENGL,
			    &config->modes, pcp, shareCtx)) {
	FREE(pcp);
	return NULL;
    }

    return pcp;
}

static __DRIcontext *
driCreateNewContextForAPI(__DRIscreen *psp, int api,
                          const __DRIconfig *config,
                          __DRIcontext *shared, void *data)
{
    __DRIcontext *pcp;
    void * const shareCtx = (shared != NULL) ? shared->driverPrivate : NULL;
    gl_api mesa_api;

    switch (api) {
    case __DRI_API_OPENGL:
            mesa_api = API_OPENGL;
            break;
    case __DRI_API_GLES:
            mesa_api = API_OPENGLES;
            break;
    case __DRI_API_GLES2:
            mesa_api = API_OPENGLES2;
            break;
    default:
            return NULL;
    }

    pcp = CALLOC_STRUCT(__DRIcontextRec);
    if (!pcp)
        return NULL;

    pcp->loaderPrivate = data;

    pcp->driScreenPriv = psp;
    pcp->driDrawablePriv = NULL;
    pcp->driReadablePriv = NULL;

    if (!driDriverAPI.CreateContext(mesa_api,
                            &config->modes, pcp, shareCtx)) {
        FREE(pcp);
        return NULL;
    }

    return pcp;
}

static void
driDestroyContext(__DRIcontext *pcp)
{
    if (pcp) {
	driDriverAPI.DestroyContext(pcp);
	FREE(pcp);
    }
}

static int
driCopyContext(__DRIcontext *dst, __DRIcontext *src, unsigned long mask)
{
    return GL_FALSE;
}

static void dri_get_drawable(__DRIdrawable *pdp);
static void dri_put_drawable(__DRIdrawable *pdp);

static int driBindContext(__DRIcontext *pcp,
			  __DRIdrawable *pdp,
			  __DRIdrawable *prp)
{
    /* Bind the drawable to the context */
    if (pcp) {
	pcp->driDrawablePriv = pdp;
	pcp->driReadablePriv = prp;
	if (pdp) {
	    pdp->driContextPriv = pcp;
	    dri_get_drawable(pdp);
	}
	if ( prp && pdp != prp ) {
	    dri_get_drawable(prp);
	}
    }

    return driDriverAPI.MakeCurrent(pcp, pdp, prp);
}

static int driUnbindContext(__DRIcontext *pcp)
{
    __DRIdrawable *pdp;
    __DRIdrawable *prp;

    if (pcp == NULL)
	return GL_FALSE;

    pdp = pcp->driDrawablePriv;
    prp = pcp->driReadablePriv;

    /* already unbound */
    if (!pdp && !prp)
	return GL_TRUE;

    driDriverAPI.UnbindContext(pcp);

    dri_put_drawable(pdp);

    if (prp != pdp) {
	dri_put_drawable(prp);
    }

    pcp->driDrawablePriv = NULL;
    pcp->driReadablePriv = NULL;

    return GL_TRUE;
}


/**
 * Drawable functions
 */

static void dri_get_drawable(__DRIdrawable *pdp)
{
    pdp->refcount++;
}

static void dri_put_drawable(__DRIdrawable *pdp)
{
    if (pdp) {
	pdp->refcount--;
	if (pdp->refcount)
	    return;

	driDriverAPI.DestroyBuffer(pdp);

	FREE(pdp);
    }
}

static __DRIdrawable *
driCreateNewDrawable(__DRIscreen *psp,
		     const __DRIconfig *config, void *data)
{
    __DRIdrawable *pdp;

    pdp = CALLOC_STRUCT(__DRIdrawableRec);
    if (!pdp)
	return NULL;

    pdp->loaderPrivate = data;

    pdp->driScreenPriv = psp;
    pdp->driContextPriv = NULL;

    dri_get_drawable(pdp);

    if (!driDriverAPI.CreateBuffer(psp, pdp, &config->modes, GL_FALSE)) {
	FREE(pdp);
	return NULL;
    }

    pdp->lastStamp = 1; /* const */

    return pdp;
}

static void
driDestroyDrawable(__DRIdrawable *pdp)
{
    dri_put_drawable(pdp);
}

static void driSwapBuffers(__DRIdrawable *pdp)
{
    driDriverAPI.SwapBuffers(pdp);
}

const __DRIcoreExtension driCoreExtension = {
    { __DRI_CORE, __DRI_CORE_VERSION },
    NULL, /* driCreateNewScreen */
    driDestroyScreen,
    driGetExtensions,
    driGetConfigAttrib,
    driIndexConfigAttrib,
    NULL, /* driCreateNewDrawable */
    driDestroyDrawable,
    driSwapBuffers,
    driCreateNewContext,
    driCopyContext,
    driDestroyContext,
    driBindContext,
    driUnbindContext
};

const __DRIswrastExtension driSWRastExtension = {
    { __DRI_SWRAST, __DRI_SWRAST_VERSION },
    driCreateNewScreen,
    driCreateNewDrawable,
    driCreateNewContextForAPI
};
