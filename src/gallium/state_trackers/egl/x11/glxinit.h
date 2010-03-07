#ifndef GLXINIT_INCLUDED
#define GLXINIT_INCLUDED

#include <X11/Xlib.h>
#include "glxclient.h"

/* this is used by DRI loaders */
extern void
_gl_context_modes_destroy(__GLcontextModes * modes);

#endif /* GLXINIT_INCLUDED */
