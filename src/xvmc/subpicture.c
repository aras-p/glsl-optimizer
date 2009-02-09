#include <assert.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xvlib.h>
#include <X11/extensions/XvMC.h>

Status XvMCCreateSubpicture
(
	Display *display,
	XvMCContext *context,
	XvMCSubpicture *subpicture,
	unsigned short width,
	unsigned short height,
	int xvimage_id
)
{
	assert(display);
	
	if (!context)
		return XvMCBadContext;
	
	assert(subpicture);
	
	if (width > 2048 || height > 2048)
		return BadValue;
	
	if (xvimage_id != 123)
		return BadMatch;
	
	subpicture->subpicture_id = XAllocID(display);
	subpicture->context_id = context->context_id;
	subpicture->xvimage_id = xvimage_id;
	subpicture->width = width;
	subpicture->height = height;
	subpicture->num_palette_entries = 0;
	subpicture->entry_bytes = 0;
	subpicture->component_order[0] = 0;
	subpicture->component_order[1] = 0;
	subpicture->component_order[2] = 0;
	subpicture->component_order[3] = 0;
	/* TODO: subpicture->privData = ;*/
	
	return Success;
}

Status XvMCClearSubpicture
(
	Display *display,
	XvMCSubpicture *subpicture,
	short x,
	short y,
	unsigned short width,
	unsigned short height,
	unsigned int color
)
{
	assert(display);
	
	if (!subpicture)
		return XvMCBadSubpicture;
	
	/* TODO: Assert clear rect is within bounds? Or clip? */
	
	return Success;
}

Status XvMCCompositeSubpicture
(
	Display *display,
	XvMCSubpicture *subpicture,
	XvImage *image,
	short srcx,
	short srcy,
	unsigned short width,
	unsigned short height,
	short dstx,
	short dsty
)
{
	assert(display);
	
	if (!subpicture)
		return XvMCBadSubpicture;
	
	assert(image);
	
	if (subpicture->xvimage_id != image->id)
		return BadMatch;
	
	/* TODO: Assert rects are within bounds? Or clip? */
	
	return Success;
}

Status XvMCDestroySubpicture(Display *display, XvMCSubpicture *subpicture)
{
	assert(display);
	
	if (!subpicture)
		return XvMCBadSubpicture;
	
	return BadImplementation;
}

Status XvMCSetSubpicturePalette(Display *display, XvMCSubpicture *subpicture, unsigned char *palette)
{
	assert(display);
	
	if (!subpicture)
		return XvMCBadSubpicture;
	
	assert(palette);
	
	/* We don't support paletted subpictures */
	return BadMatch;
}

Status XvMCBlendSubpicture
(
	Display *display,
	XvMCSurface *target_surface,
	XvMCSubpicture *subpicture,
	short subx,
	short suby,
	unsigned short subw,
	unsigned short subh,
	short surfx,
	short surfy,
	unsigned short surfw,
	unsigned short surfh
)
{
	assert(display);
	
	if (!target_surface)
		return XvMCBadSurface;
	
	if (!subpicture)
		return XvMCBadSubpicture;
	
	if (target_surface->context_id != subpicture->context_id)
		return BadMatch;
	
	/* TODO: Assert rects are within bounds? Or clip? */
	return Success;
}

Status XvMCBlendSubpicture2
(
	Display *display,
	XvMCSurface *source_surface,
	XvMCSurface *target_surface,
	XvMCSubpicture *subpicture,
	short subx,
	short suby,
	unsigned short subw,
	unsigned short subh,
	short surfx,
	short surfy,
	unsigned short surfw,
	unsigned short surfh
)
{
	assert(display);
	
	if (!source_surface || !target_surface)
		return XvMCBadSurface;
	
	if (!subpicture)
		return XvMCBadSubpicture;
	
	if (source_surface->context_id != subpicture->context_id)
		return BadMatch;
		
	if (source_surface->context_id != subpicture->context_id)
		return BadMatch;
	
	/* TODO: Assert rects are within bounds? Or clip? */
	return Success;
}

Status XvMCSyncSubpicture(Display *display, XvMCSubpicture *subpicture)
{
	assert(display);
	
	if (!subpicture)
		return XvMCBadSubpicture;
	
	return Success;
}

Status XvMCFlushSubpicture(Display *display, XvMCSubpicture *subpicture)
{
	assert(display);
	
	if (!subpicture)
		return XvMCBadSubpicture;
	
	return Success;
}

Status XvMCGetSubpictureStatus(Display *display, XvMCSubpicture *subpicture, int *status)
{
	assert(display);
	
	if (!subpicture)
		return XvMCBadSubpicture;
	
	assert(status);
	
	/* TODO */
	*status = 0;
	
	return Success;
}

