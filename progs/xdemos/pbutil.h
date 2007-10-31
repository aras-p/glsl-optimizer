/*
 * OpenGL pbuffers utility functions.
 *
 * Brian Paul
 * April 1997
 */


#ifndef PBUTIL_H
#define PBUTIL_H


#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>


#if defined(GLX_VERSION_1_3)
#define PBUFFER GLXPbuffer
#define FBCONFIG GLXFBConfig
#elif defined(GLX_SGIX_fbconfig) && defined(GLX_SGIX_pbuffer)
#define PBUFFER GLXPbufferSGIX
#define FBCONFIG GLXFBConfigSGIX
#else
#define PBUFFER int
#define FBCONFIG int
#endif


extern int
QueryFBConfig(Display *dpy, int screen);

extern int
QueryPbuffers(Display *dpy, int screen);


extern void
PrintFBConfigInfo(Display *dpy, int screen, FBCONFIG config, Bool horizFormat);


extern FBCONFIG *
ChooseFBConfig(Display *dpy, int screen, const int attribs[], int *nConfigs);


extern FBCONFIG *
GetAllFBConfigs(Display *dpy, int screen, int *nConfigs);


extern XVisualInfo *
GetVisualFromFBConfig(Display *dpy, int screen, FBCONFIG config);


extern GLXContext
CreateContext(Display *dpy, int screen, FBCONFIG config);


extern void
DestroyContext(Display *dpy, GLXContext ctx);


extern PBUFFER
CreatePbuffer(Display *dpy, int screen, FBCONFIG config,
	      int width, int height, Bool preserve, Bool largest);


extern void
DestroyPbuffer(Display *dpy, int screen, PBUFFER pbuffer);


#endif  /*PBUTIL_H*/
