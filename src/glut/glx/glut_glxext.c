
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <string.h>
#include "glutint.h"

#if defined(GLX_VERSION_1_1)
int
__glutIsSupportedByGLX(char *extension)
{
  static const char *extensions = NULL;
  const char *start;
  char *where, *terminator;
  int major, minor;

  glXQueryVersion(__glutDisplay, &major, &minor);
  /* Be careful not to call glXQueryExtensionsString if it
     looks like the server doesn't support GLX 1.1.
     Unfortunately, the original GLX 1.0 didn't have the notion
     of GLX extensions. */
  if ((major == 1 && minor >= 1) || (major > 1)) {
    if (!extensions)
      extensions = glXQueryExtensionsString(__glutDisplay, __glutScreen);
    /* It takes a bit of care to be fool-proof about parsing
       the GLX extensions string.  Don't be fooled by
       sub-strings,  etc. */
    start = extensions;
    for (;;) {
      where = strstr(start, extension);
      if (!where)
        return 0;
      terminator = where + strlen(extension);
      if (where == start || *(where - 1) == ' ') {
        if (*terminator == ' ' || *terminator == '\0') {
          return 1;
        }
      }
      start = terminator;
    }
  }
  return 0;
}
#endif



/*
 * Wrapping of GLX extension functions.
 * Technically, we should do a runtime test to see if we've got the
 * glXGetProcAddressARB() function.  I think GLX_ARB_get_proc_address
 * is pretty widely supported now and any system that has
 * GLX_ARB_get_proc_address defined in its header files should be OK
 * at runtime.
 */

int
__glut_glXBindChannelToWindowSGIX(Display *dpy, int screen,
                                  int channel, Window window)
{
#ifdef GLX_ARB_get_proc_address
  typedef int (*glXBindChannelToWindowSGIX_t) (Display *, int, int, Window);
  static glXBindChannelToWindowSGIX_t glXBindChannelToWindowSGIX_ptr = NULL;
  if (!glXBindChannelToWindowSGIX_ptr) {
    glXBindChannelToWindowSGIX_ptr = (glXBindChannelToWindowSGIX_t)
      glXGetProcAddressARB((const GLubyte *) "glXBindChannelToWindowSGIX");
  }
  if (glXBindChannelToWindowSGIX_ptr)
    return (*glXBindChannelToWindowSGIX_ptr)(dpy, screen, channel, window);
  else
    return 0;
#elif defined(GLX_SGIX_video_resize)
  return glXBindChannelToWindowSGIX(dpy, screen, channel, window);
#else
  return 0;
#endif   
}


int
__glut_glXChannelRectSGIX(Display *dpy, int screen, int channel,
                          int x, int y, int w, int h)
{
#ifdef GLX_ARB_get_proc_address
  typedef int (*glXChannelRectSGIX_t)(Display *, int, int, int, int, int, int);
  static glXChannelRectSGIX_t glXChannelRectSGIX_ptr = NULL;
  if (!glXChannelRectSGIX_ptr) {
    glXChannelRectSGIX_ptr = (glXChannelRectSGIX_t)
      glXGetProcAddressARB((const GLubyte *) "glXChannelRectSGIX");
  }
  if (glXChannelRectSGIX_ptr)
    return (*glXChannelRectSGIX_ptr)(dpy, screen, channel, x, y, w, h);
  else
    return 0;
#elif defined(GLX_SGIX_video_resize)
  return glXChannelRectSGIX(dpy, screen, channel, x, y, w, h);
#else
  return 0;
#endif   
}


int
__glut_glXQueryChannelRectSGIX(Display *dpy, int screen, int channel,
                               int *x, int *y, int *w, int *h)
{
#ifdef GLX_ARB_get_proc_address
  typedef int (*glXQueryChannelRectSGIX_t)(Display *, int, int,
                                           int *, int *, int *, int *);
  static glXQueryChannelRectSGIX_t glXQueryChannelRectSGIX_ptr = NULL;
  if (!glXQueryChannelRectSGIX_ptr) {
    glXQueryChannelRectSGIX_ptr = (glXQueryChannelRectSGIX_t)
      glXGetProcAddressARB((const GLubyte *) "glXQueryChannelRectSGIX");
  }
  if (glXQueryChannelRectSGIX_ptr)
    return (*glXQueryChannelRectSGIX_ptr)(dpy, screen, channel, x, y, w, h);
  else
    return 0;
#elif defined(GLX_SGIX_video_resize)
  return glXQueryChannelRectSGIX(dpy, screen, channel, x, y, w, h);
#else
  return 0;
#endif   
}


int
__glut_glXQueryChannelDeltasSGIX(Display *dpy, int screen, int channel,
                                 int *dx, int *dy, int *dw, int *dh)
{
#ifdef GLX_ARB_get_proc_address
  typedef int (*glXQueryChannelDeltasSGIX_t)(Display *, int, int,
                                             int *, int *, int *, int *);
  static glXQueryChannelDeltasSGIX_t glXQueryChannelDeltasSGIX_ptr = NULL;
  if (!glXQueryChannelDeltasSGIX_ptr) {
    glXQueryChannelDeltasSGIX_ptr = (glXQueryChannelDeltasSGIX_t)
      glXGetProcAddressARB((const GLubyte *) "glXQueryChannelDeltasSGIX");
  }
  if (glXQueryChannelDeltasSGIX_ptr)
    return (*glXQueryChannelDeltasSGIX_ptr)(dpy, screen, channel,
                                            dx, dy, dw, dh);
  else
    return 0;
#elif defined(GLX_SGIX_video_resize)
  return glXQueryChannelDeltasSGIX(dpy, screen, channel, dx, dy, dw, dh);
#else
  return 0;
#endif   
}


int
__glut_glXChannelRectSyncSGIX(Display *dpy, int screen,
                              int channel, GLenum synctype)
{
#ifdef GLX_ARB_get_proc_address
  typedef int (*glXChannelRectSyncSGIX_t)(Display *, int, int, GLenum);
  static glXChannelRectSyncSGIX_t glXChannelRectSyncSGIX_ptr = NULL;
  if (!glXChannelRectSyncSGIX_ptr) {
    glXChannelRectSyncSGIX_ptr = (glXChannelRectSyncSGIX_t)
      glXGetProcAddressARB((const GLubyte *) "glXChannelRectSyncSGIX");
  }
  if (glXChannelRectSyncSGIX_ptr)
    return (*glXChannelRectSyncSGIX_ptr)(dpy, screen, channel, synctype);
  else
    return 0;
#elif defined(GLX_SGIX_video_resize)
  return glXChannelRectSyncSGIX(dpy, screen, channel, synctype);
#else
  return 0;
#endif   
}



GLXContext
__glut_glXCreateContextWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config,
                                      int render_type, GLXContext share_list,
                                      Bool direct)
{
#ifdef GLX_ARB_get_proc_address
  typedef GLXContext (*glXCreateContextWithConfigSGIX_t)(Display *,
                                 GLXFBConfigSGIX, int, GLXContext, Bool);
  static glXCreateContextWithConfigSGIX_t glXCreateContextWithConfig_ptr = NULL;
  if (!glXCreateContextWithConfig_ptr) {
    glXCreateContextWithConfig_ptr = (glXCreateContextWithConfigSGIX_t)
       glXGetProcAddressARB((const GLubyte *) "glXCreateContextWithConfigSGIX");
  }
  if (glXCreateContextWithConfig_ptr)
    return (*glXCreateContextWithConfig_ptr)(dpy, config, render_type,
                                             share_list, direct);
  else
    return 0;
#elif defined(GLX_SGIX_fbconfig)
  return glXCreateContextWithConfigSGIX(dpy, config, render_type,
                                        share_list, direct);
#else
  return 0;
#endif
}


int
__glut_glXGetFBConfigAttribSGIX(Display *dpy, GLXFBConfigSGIX config,
                                int attribute, int *value)
{
#ifdef GLX_ARB_get_proc_address
  typedef int (*glXGetFBConfigAttribSGIX_t)(Display *,
                                            GLXFBConfigSGIX, int, int *);
  static glXGetFBConfigAttribSGIX_t glXGetFBConfigAttrib_ptr = NULL;
  if (!glXGetFBConfigAttrib_ptr) {
    glXGetFBConfigAttrib_ptr = (glXGetFBConfigAttribSGIX_t)
       glXGetProcAddressARB((const GLubyte *) "glXGetFBConfigAttribSGIX");
  }
  if (glXGetFBConfigAttrib_ptr)
    return (*glXGetFBConfigAttrib_ptr)(dpy, config, attribute, value);
  else
    return 0;
#elif defined(GLX_SGIX_fbconfig)
  return glXGetFBConfigAttribSGIX(dpy, config, attribute, value);
#else
  return 0;
#endif
}


GLXFBConfigSGIX
__glut_glXGetFBConfigFromVisualSGIX(Display *dpy, XVisualInfo *vis)
{
#ifdef GLX_ARB_get_proc_address
  typedef GLXFBConfigSGIX (*glXGetFBConfigFromVisualSGIX_t)(Display *,
                                                            XVisualInfo *);
  static glXGetFBConfigFromVisualSGIX_t glXGetFBConfigFromVisual_ptr = NULL;
  if (!glXGetFBConfigFromVisual_ptr) {
    glXGetFBConfigFromVisual_ptr = (glXGetFBConfigFromVisualSGIX_t)
       glXGetProcAddressARB((const GLubyte *) "glXGetFBConfigFromVisualSGIX");
  }
  if (glXGetFBConfigFromVisual_ptr)
    return (*glXGetFBConfigFromVisual_ptr)(dpy, vis);
  else
    return 0;
#elif defined(GLX_SGIX_fbconfig)
  return glXGetFBConfigFromVisualSGIX(dpy, vis);
#else
  return 0;
#endif
}




