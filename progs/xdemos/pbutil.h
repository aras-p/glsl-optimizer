
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


extern int
QueryPbuffers(Display *dpy, int screen);


#ifdef GLX_SGIX_fbconfig


extern void
PrintFBConfigInfo(Display *dpy, GLXFBConfigSGIX fbConfig, Bool horizFormat);


extern GLXPbufferSGIX
CreatePbuffer( Display *dpy, GLXFBConfigSGIX fbConfig,
	       int width, int height, int *pbAttribs );


#endif  /*GLX_SGIX_fbconfig*/


#endif  /*PBUTIL_H*/
