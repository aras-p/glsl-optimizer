#ifndef _EGL_H
#define _EGL_H

/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.0 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
**
** http://oss.sgi.com/projects/FreeB
**
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
**
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2004 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
**
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
*/

#if 0/*XXX TEMPORARY HACK*/
#include <GLES/gl.h>
#else
#include <GL/gl.h>
#endif
#include <GLES/egltypes.h>

/* XXX should go in eglext.h */
#define GL_OES_VERSION_1_0  1
#define GL_OES_read_format  1
#define GL_OES_compressed_paletted_texture 1
#define GL_IMPLEMENTATION_COLOR_READ_TYPE_OES 0x8B9A
#define GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES 0x8B9B
#define GL_PALETTE4_RGB8_OES        0x8B90
#define GL_PALETTE4_RGBA8_OES       0x8B91
#define GL_PALETTE4_R5_G6_B5_OES    0x8B92
#define GL_PALETTE4_RGBA4_OES       0x8B93
#define GL_PALETTE4_RGB5_A1_OES     0x8B94
#define GL_PALETTE8_RGB8_OES        0x8B95
#define GL_PALETTE8_RGBA8_OES       0x8B96
#define GL_PALETTE8_R5_G6_B5_OES    0x8B97
#define GL_PALETTE8_RGBA4_OES       0x8B98
#define GL_PALETTE8_RGB5_A1_OES     0x8B99
/* XXX */

/*
** Versioning and extensions
*/
#define EGL_VERSION_1_0		       1
#if 0
#define EGL_VERSION_1_1		       1
#endif

/*
** Boolean
*/
#define EGL_FALSE		       0
#define EGL_TRUE		       1

/*
** Errors
*/
#define EGL_SUCCESS		       0x3000
#define EGL_NOT_INITIALIZED	       0x3001
#define EGL_BAD_ACCESS		       0x3002
#define EGL_BAD_ALLOC		       0x3003
#define EGL_BAD_ATTRIBUTE	       0x3004
#define EGL_BAD_CONFIG		       0x3005
#define EGL_BAD_CONTEXT		       0x3006
#define EGL_BAD_CURRENT_SURFACE        0x3007
#define EGL_BAD_DISPLAY		       0x3008
#define EGL_BAD_MATCH		       0x3009
#define EGL_BAD_NATIVE_PIXMAP	       0x300A
#define EGL_BAD_NATIVE_WINDOW	       0x300B
#define EGL_BAD_PARAMETER	       0x300C
#define EGL_BAD_SURFACE		       0x300D
#define EGL_CONTEXT_LOST	       0x300E
/* 0x300F - 0x301F reserved for additional errors. */

/*
** Config attributes
*/
#define EGL_BUFFER_SIZE		       0x3020
#define EGL_ALPHA_SIZE		       0x3021
#define EGL_BLUE_SIZE		       0x3022
#define EGL_GREEN_SIZE		       0x3023
#define EGL_RED_SIZE		       0x3024
#define EGL_DEPTH_SIZE		       0x3025
#define EGL_STENCIL_SIZE	       0x3026
#define EGL_CONFIG_CAVEAT	       0x3027
#define EGL_CONFIG_ID		       0x3028
#define EGL_LEVEL		       0x3029
#define EGL_MAX_PBUFFER_HEIGHT	       0x302A
#define EGL_MAX_PBUFFER_PIXELS	       0x302B
#define EGL_MAX_PBUFFER_WIDTH	       0x302C
#define EGL_NATIVE_RENDERABLE	       0x302D
#define EGL_NATIVE_VISUAL_ID	       0x302E
#define EGL_NATIVE_VISUAL_TYPE	       0x302F
/*#define EGL_PRESERVED_RESOURCES	 0x3030*/
#define EGL_SAMPLES		       0x3031
#define EGL_SAMPLE_BUFFERS	       0x3032
#define EGL_SURFACE_TYPE	       0x3033
#define EGL_TRANSPARENT_TYPE	       0x3034
#define EGL_TRANSPARENT_BLUE_VALUE     0x3035
#define EGL_TRANSPARENT_GREEN_VALUE    0x3036
#define EGL_TRANSPARENT_RED_VALUE      0x3037
#define EGL_NONE		       0x3038	/* Also a config value */
#define EGL_BIND_TO_TEXTURE_RGB        0x3039
#define EGL_BIND_TO_TEXTURE_RGBA       0x303A
#define EGL_MIN_SWAP_INTERVAL	       0x303B
#define EGL_MAX_SWAP_INTERVAL	       0x303C

/*
** Config values
*/
#define EGL_DONT_CARE		       ((EGLint) -1)

#define EGL_SLOW_CONFIG		       0x3050	/* EGL_CONFIG_CAVEAT value */
#define EGL_NON_CONFORMANT_CONFIG      0x3051	/* " */
#define EGL_TRANSPARENT_RGB	       0x3052	/* EGL_TRANSPARENT_TYPE value */
#define EGL_NO_TEXTURE		       0x305C	/* EGL_TEXTURE_FORMAT/TARGET value */
#define EGL_TEXTURE_RGB		       0x305D	/* EGL_TEXTURE_FORMAT value */
#define EGL_TEXTURE_RGBA	       0x305E	/* " */
#define EGL_TEXTURE_2D		       0x305F	/* EGL_TEXTURE_TARGET value */

/*
** Config attribute mask bits
*/
#define EGL_PBUFFER_BIT		       0x01	/* EGL_SURFACE_TYPE mask bit */
#define EGL_PIXMAP_BIT		       0x02	/* " */
#define EGL_WINDOW_BIT		       0x04	/* " */

/*
** String names
*/
#define EGL_VENDOR		       0x3053	/* eglQueryString target */
#define EGL_VERSION		       0x3054	/* " */
#define EGL_EXTENSIONS		       0x3055	/* " */

/*
** Surface attributes
*/
#define EGL_HEIGHT		       0x3056
#define EGL_WIDTH		       0x3057
#define EGL_LARGEST_PBUFFER	       0x3058
#define EGL_TEXTURE_FORMAT	       0x3080	/* For pbuffers bound as textures */
#define EGL_TEXTURE_TARGET	       0x3081	/* " */
#define EGL_MIPMAP_TEXTURE	       0x3082	/* " */
#define EGL_MIPMAP_LEVEL	       0x3083	/* " */

/*
** BindTexImage / ReleaseTexImage buffer target
*/
#define EGL_BACK_BUFFER		       0x3084

/*
** Current surfaces
*/
#define EGL_DRAW		       0x3059
#define EGL_READ		       0x305A

/*
** Engines
*/
#define EGL_CORE_NATIVE_ENGINE	       0x305B

/* 0x305C-0x3FFFF reserved for future use */

/*
** Functions
*/
#ifdef __cplusplus
extern "C" {
#endif

GLAPI EGLint APIENTRY eglGetError (void);

GLAPI EGLDisplay APIENTRY eglGetDisplay (NativeDisplayType display);
GLAPI EGLBoolean APIENTRY eglInitialize (EGLDisplay dpy, EGLint *major, EGLint *minor);
GLAPI EGLBoolean APIENTRY eglTerminate (EGLDisplay dpy);
GLAPI const char * APIENTRY eglQueryString (EGLDisplay dpy, EGLint name);
GLAPI void (* APIENTRY eglGetProcAddress (const char *procname))(void);

GLAPI EGLBoolean APIENTRY eglGetConfigs (EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);
GLAPI EGLBoolean APIENTRY eglChooseConfig (EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
GLAPI EGLBoolean APIENTRY eglGetConfigAttrib (EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);

GLAPI EGLSurface APIENTRY eglCreateWindowSurface (EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list);
GLAPI EGLSurface APIENTRY eglCreatePixmapSurface (EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap, const EGLint *attrib_list);
GLAPI EGLSurface APIENTRY eglCreatePbufferSurface (EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
GLAPI EGLBoolean APIENTRY eglDestroySurface (EGLDisplay dpy, EGLSurface surface);
GLAPI EGLBoolean APIENTRY eglQuerySurface (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value);

/* EGL 1.1 render-to-texture APIs */
GLAPI EGLBoolean APIENTRY eglSurfaceAttrib (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value);
GLAPI EGLBoolean APIENTRY eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
GLAPI EGLBoolean APIENTRY eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);

/* EGL 1.1 swap control API */
GLAPI EGLBoolean APIENTRY eglSwapInterval(EGLDisplay dpy, EGLint interval);

GLAPI EGLContext APIENTRY eglCreateContext (EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list);
GLAPI EGLBoolean APIENTRY eglDestroyContext (EGLDisplay dpy, EGLContext ctx);
GLAPI EGLBoolean APIENTRY eglMakeCurrent (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
GLAPI EGLContext APIENTRY eglGetCurrentContext (void);
GLAPI EGLSurface APIENTRY eglGetCurrentSurface (EGLint readdraw);
GLAPI EGLDisplay APIENTRY eglGetCurrentDisplay (void);
GLAPI EGLBoolean APIENTRY eglQueryContext (EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value);

GLAPI EGLBoolean APIENTRY eglWaitGL (void);
GLAPI EGLBoolean APIENTRY eglWaitNative (EGLint engine);
GLAPI EGLBoolean APIENTRY eglSwapBuffers (EGLDisplay dpy, EGLSurface draw);
GLAPI EGLBoolean APIENTRY eglCopyBuffers (EGLDisplay dpy, EGLSurface surface, NativePixmapType target);



/* EGL_MESA_screen extension  >>> PRELIMINARY <<< */
#ifndef EGL_MESA_screen_surface
#define EGL_MESA_screen_surface 1

#define EGL_BAD_SCREEN_MESA        0x4000
#define EGL_BAD_MODE_MESA          0x4001
#define EGL_SCREEN_COUNT_MESA      0x4002
#define EGL_SCREEN_POSITION_MESA   0x4003
#define EGL_MODE_ID_MESA           0x4004
#define EGL_REFRESH_RATE_MESA      0x4005
#define EGL_OPTIMAL_MODE_MESA      0x4006
#define EGL_SCREEN_BIT_MESA        0x08

GLAPI EGLBoolean APIENTRY eglChooseModeMESA(EGLDisplay dpy, EGLScreenMESA screen, const EGLint *attrib_list, EGLModeMESA *modes, EGLint modes_size, EGLint *num_modes);
GLAPI EGLBoolean APIENTRY eglGetModesMESA(EGLDisplay dpy, EGLScreenMESA screen, EGLModeMESA *modes, EGLint modes_size, EGLint *num_modes);
GLAPI EGLBoolean APIENTRY eglGetModeAttribMESA(EGLDisplay dpy, EGLModeMESA mode, EGLint attribute, EGLint *value);
GLAPI EGLBoolean APIENTRY eglGetScreensMESA(EGLDisplay dpy, EGLScreenMESA *screens, EGLint max_screens, EGLint *num_screens);
GLAPI EGLSurface APIENTRY eglCreateScreenSurfaceMESA(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
GLAPI EGLBoolean APIENTRY eglShowSurfaceMESA(EGLDisplay dpy, EGLint screen, EGLSurface surface, EGLModeMESA mode);
GLAPI EGLBoolean APIENTRY eglScreenPositionMESA(EGLDisplay dpy, EGLScreenMESA screen, EGLint x, EGLint y);
GLAPI EGLBoolean APIENTRY eglQueryScreenMESA(EGLDisplay dpy, EGLScreenMESA screen, EGLint attribute, EGLint *value);
GLAPI EGLBoolean APIENTRY eglQueryScreenSurfaceMESA(EGLDisplay dpy, EGLScreenMESA screen, EGLSurface *surface);
GLAPI EGLBoolean APIENTRY eglQueryScreenModeMESA(EGLDisplay dpy, EGLScreenMESA screen, EGLModeMESA *mode);
GLAPI const char * APIENTRY eglQueryModeStringMESA(EGLDisplay dpy, EGLModeMESA mode);

#endif /* EGL_MESA_screen_surface */


#ifndef EGL_MESA_copy_context
#define EGL_MESA_copy_context 1

GLAPI EGLBoolean APIENTRY eglCopyContextMESA(EGLDisplay dpy, EGLContext source, EGLContext dest, EGLint mask);

#endif /* EGL_MESA_copy_context */


#ifdef __cplusplus
}
#endif

#endif /* _EGL_H */
