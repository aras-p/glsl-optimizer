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
	return BadImplementation;
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
	return BadImplementation;
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
	return BadImplementation;
}

Status XvMCDestroySubpicture(Display *display, XvMCSubpicture *subpicture)
{
	return BadImplementation;
}

Status XvMCSetSubpicturePalette(Display *display, XvMCSubpicture *subpicture, unsigned char *palette)
{
	return BadImplementation;
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
	return BadImplementation;
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
	return BadImplementation;
}

Status XvMCSyncSubpicture(Display *display, XvMCSubpicture *subpicture)
{
	return BadImplementation;
}

Status XvMCFlushSubpicture(Display *display, XvMCSubpicture *subpicture)
{
	return BadImplementation;
}

Status XvMCGetSubpictureStatus(Display *display, XvMCSubpicture *subpicture, int *status)
{
	return BadImplementation;
}

