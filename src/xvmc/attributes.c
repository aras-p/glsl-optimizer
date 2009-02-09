#include <assert.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xvlib.h>
#include <X11/extensions/XvMC.h>

XvAttribute* XvMCQueryAttributes(Display *display, XvMCContext *context, int *number)
{
	return NULL;
}

Status XvMCSetAttribute(Display *display, XvMCContext *context, Atom attribute, int value)
{
	return BadImplementation;
}

Status XvMCGetAttribute(Display *display, XvMCContext *context, Atom attribute, int *value)
{
	return BadImplementation;
}

