#include <assert.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xvlib.h>
#include <X11/extensions/XvMClib.h>

XvAttribute* XvMCQueryAttributes(Display *dpy, XvMCContext *context, int *number)
{
   return NULL;
}

Status XvMCSetAttribute(Display *dpy, XvMCContext *context, Atom attribute, int value)
{
   return BadImplementation;
}

Status XvMCGetAttribute(Display *dpy, XvMCContext *context, Atom attribute, int *value)
{
   return BadImplementation;
}
