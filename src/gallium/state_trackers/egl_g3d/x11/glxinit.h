#ifndef GLXINIT_INCLUDED
#define GLXINIT_INCLUDED

#include <X11/Xlib.h>

#ifndef GLX_DIRECT_RENDERING
#define GLX_DIRECT_RENDERING
#endif
#include "glxclient.h"

extern void
__glXRelease(__GLXdisplayPrivate *dpyPriv);

#endif /* GLXINIT_INCLUDED */
