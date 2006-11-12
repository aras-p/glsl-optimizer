/**************************************************************************

Copyright 2006 Stephane Marchesin
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#include "glheader.h"
#include "context.h"
#include "simple_list.h"
#include "imports.h"
#include "matrix.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "array_cache/acache.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "nouveau_context.h"
#include "nouveau_driver.h"
//#include "nouveau_state.h"
#include "nouveau_span.h"
#include "nouveau_object.h"
#include "nouveau_fifo.h"
#include "nouveau_tex.h"
#include "nv10_swtcl.h"

#include "vblank.h"
#include "utils.h"
#include "texmem.h"
#include "xmlpool.h" /* for symbolic values of enum-type options */

#ifndef NOUVEAU_DEBUG
int NOUVEAU_DEBUG = 0;
#endif

static const struct dri_debug_control debug_control[] =
{
	{ NULL,    0 }
};

/* Create the device specific context.
 */
GLboolean nouveauCreateContext( const __GLcontextModes *glVisual,
		__DRIcontextPrivate *driContextPriv,
		void *sharedContextPrivate )
{
	GLcontext *ctx, *shareCtx;
	__DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
	struct dd_function_table functions;
	nouveauContextPtr nmesa;
	nouveauScreenPtr screen;

	/* Allocate the context */
	nmesa = (nouveauContextPtr) CALLOC( sizeof(*nmesa) );
	if ( !nmesa )
		return GL_FALSE;

	/* Create the hardware context */
	if (!nouveauFifoInit(nmesa))
	   return GL_FALSE;
	nouveauObjectInit(nmesa);

	/* Init default driver functions then plug in our nouveau-specific functions
	 * (the texture functions are especially important)
	 */
	_mesa_init_driver_functions( &functions );
	nouveauDriverInitFunctions( &functions );
	nouveauTexInitFunctions( &functions );

	/* Allocate the Mesa context */
	if (sharedContextPrivate)
		shareCtx = ((nouveauContextPtr) sharedContextPrivate)->glCtx;
	else 
		shareCtx = NULL;
	nmesa->glCtx = _mesa_create_context(glVisual, shareCtx,
			&functions, (void *) nmesa);
	if (!nmesa->glCtx) {
		FREE(nmesa);
		return GL_FALSE;
	}
	driContextPriv->driverPrivate = nmesa;
	ctx = nmesa->glCtx;

	nmesa->driContext = driContextPriv;
	nmesa->driScreen = sPriv;
	nmesa->driDrawable = NULL;
	nmesa->hHWContext = driContextPriv->hHWContext;
	nmesa->driHwLock = &sPriv->pSAREA->lock;
	nmesa->driFd = sPriv->fd;

	nmesa->screen = (nouveauScreenPtr)(sPriv->private);
	screen=nmesa->screen;

	/* Parse configuration files */
	driParseConfigFiles (&nmesa->optionCache, &screen->optionCache,
			screen->driScreen->myNum, "nouveau");

	nmesa->sarea = (drm_nouveau_sarea_t *)((char *)sPriv->pSAREA +
			screen->sarea_priv_offset);


	nmesa->current_primitive = -1;

	/* Initialize the swrast */
	_swrast_CreateContext( ctx );
	_ac_CreateContext( ctx );
	_tnl_CreateContext( ctx );
	_swsetup_CreateContext( ctx );

	switch(nmesa->screen->card->type)
	{
		case NV_03:
			//nv03TriInitFunctions( ctx );
			break;
		case NV_04:
		case NV_05:
			//nv04TriInitFunctions( ctx );
			break;
		case NV_10:
		case NV_20:
		case NV_30:
		case NV_40:
		case G_70:
		default:
			nv10TriInitFunctions( ctx );
			break;
	}
	nouveauDDInitStateFuncs( ctx );
	nouveauSpanInitFunctions( ctx );
	nouveauDDInitState( nmesa );

	driContextPriv->driverPrivate = (void *)nmesa;

	NOUVEAU_DEBUG = driParseDebugString( getenv( "NOUVEAU_DEBUG" ),
			debug_control );

	if (driQueryOptionb(&nmesa->optionCache, "no_rast")) {
		fprintf(stderr, "disabling 3D acceleration\n");
		FALLBACK(nmesa, NOUVEAU_FALLBACK_DISABLE, 1);
	}

	return GL_TRUE;
}

/* Destroy the device specific context. */
void nouveauDestroyContext( __DRIcontextPrivate *driContextPriv  )
{
	nouveauContextPtr nmesa = (nouveauContextPtr) driContextPriv->driverPrivate;

	assert(nmesa);
	if ( nmesa ) {
		/* free the option cache */
		driDestroyOptionCache (&nmesa->optionCache);

		FREE( nmesa );
	}

}


/* Force the context `c' to be the current context and associate with it
 * buffer `b'.
 */
GLboolean nouveauMakeCurrent( __DRIcontextPrivate *driContextPriv,
		__DRIdrawablePrivate *driDrawPriv,
		__DRIdrawablePrivate *driReadPriv )
{
	if ( driContextPriv ) {
		GET_CURRENT_CONTEXT(ctx);
		nouveauContextPtr oldNOUVEAUCtx = ctx ? NOUVEAU_CONTEXT(ctx) : NULL;
		nouveauContextPtr newNOUVEAUCtx = (nouveauContextPtr) driContextPriv->driverPrivate;

		driDrawableInitVBlank(driDrawPriv, newNOUVEAUCtx->vblank_flags, &newNOUVEAUCtx->vblank_seq );
		newNOUVEAUCtx->driDrawable = driDrawPriv;

		_mesa_make_current( newNOUVEAUCtx->glCtx,
				(GLframebuffer *) driDrawPriv->driverPrivate,
				(GLframebuffer *) driReadPriv->driverPrivate );

	} else {
		_mesa_make_current( NULL, NULL, NULL );
	}

	return GL_TRUE;
}


/* Force the context `c' to be unbound from its buffer.
 */
GLboolean nouveauUnbindContext( __DRIcontextPrivate *driContextPriv )
{
	return GL_TRUE;
}
