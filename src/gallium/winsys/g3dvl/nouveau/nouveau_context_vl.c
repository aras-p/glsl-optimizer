#include "nouveau_context_vl.h"
#include <pipe/p_defines.h>
#include <pipe/p_context.h>
#include <pipe/p_screen.h>
#include <util/u_memory.h>
#include <common/nouveau_dri.h>
#include <common/nouveau_local.h>
#include <common/nouveau_winsys_pipe.h>
#include "nouveau_screen_vl.h"

/*
#ifdef DEBUG
static const struct dri_debug_control debug_control[] = {
	{ "bo", DEBUG_BO },
	{ NULL, 0 }
};
int __nouveau_debug = 0;
#endif
*/

int
nouveau_context_create(dri_context_t *dri_context)
{
	dri_screen_t			*dri_screen;
	struct nouveau_screen_vl	*nv_screen;
	struct nouveau_context_vl	*nv;

	assert (dri_context);

	dri_screen = dri_context->dri_screen;
	nv_screen = dri_screen->private;
	nv = CALLOC_STRUCT(nouveau_context_vl);

	if (!nv)
		return 1;

	if (nouveau_context_init(&nv_screen->base, dri_context->drm_context,
	                        (drmLock*)&dri_screen->sarea->lock, NULL, &nv->base))
	{
		FREE(nv);
		return 1;
	}

	dri_context->private = (void*)nv;
	nv->dri_context = dri_context;
	nv->nv_screen  = nv_screen;

	/*
	driParseConfigFiles(&nv->dri_option_cache, &nv_screen->option_cache,
			    nv->dri_screen->myNum, "nouveau");
#ifdef DEBUG
	__nouveau_debug = driParseDebugString(getenv("NOUVEAU_DEBUG"),
					      debug_control);
#endif
	*/

	nv->base.nvc->pctx[nv->base.pctx_id]->priv = nv;

	return 0;
}

void
nouveau_context_destroy(dri_context_t *dri_context)
{
	struct nouveau_context_vl *nv = dri_context->private;

	assert(dri_context);

	nouveau_context_cleanup(&nv->base);

	FREE(nv);
}

int
nouveau_context_bind(struct nouveau_context_vl *nv, dri_drawable_t *dri_drawable)
{
	assert(nv);
	assert(dri_drawable);

	if (nv->dri_drawable != dri_drawable)
	{
		nv->dri_drawable = dri_drawable;
		dri_drawable->private = nv;
	}

	return 0;
}

int
nouveau_context_unbind(struct nouveau_context_vl *nv)
{
	assert(nv);

	nv->dri_drawable = NULL;

	return 0;
}

/* Show starts here */

int bind_pipe_drawable(struct pipe_context *pipe, Drawable drawable)
{
	struct nouveau_context_vl	*nv;
	dri_drawable_t			*dri_drawable;

	assert(pipe);

	nv = pipe->priv;

	driCreateDrawable(nv->nv_screen->dri_screen, drawable, &dri_drawable);

	nouveau_context_bind(nv, dri_drawable);

	return 0;
}

int unbind_pipe_drawable(struct pipe_context *pipe)
{
	assert (pipe);

	nouveau_context_unbind(pipe->priv);

	return 0;
}

struct pipe_context* create_pipe_context(Display *display, int screen)
{
	dri_screen_t			*dri_screen;
	dri_framebuffer_t		dri_framebuf;
	dri_context_t			*dri_context;
	struct nouveau_context_vl	*nv;

	assert(display);

	driCreateScreen(display, screen, &dri_screen, &dri_framebuf);
	driCreateContext(dri_screen, XDefaultVisual(display, screen), &dri_context);

	nouveau_screen_create(dri_screen, &dri_framebuf);
	nouveau_context_create(dri_context);

	nv = dri_context->private;

	return nv->base.nvc->pctx[nv->base.pctx_id];
}

int destroy_pipe_context(struct pipe_context *pipe)
{
	struct pipe_screen		*screen;
	struct pipe_winsys		*winsys;
	struct nouveau_context_vl	*nv;
	dri_screen_t			*dri_screen;
	dri_context_t			*dri_context;

	assert(pipe);

	screen = pipe->screen;
	winsys = pipe->winsys;
	nv = pipe->priv;
	dri_context = nv->dri_context;
	dri_screen = dri_context->dri_screen;

	pipe->destroy(pipe);
	screen->destroy(screen);
	FREE(winsys);

	nouveau_context_destroy(dri_context);
	nouveau_screen_destroy(dri_screen);
	driDestroyContext(dri_context);
	driDestroyScreen(dri_screen);

	return 0;
}
