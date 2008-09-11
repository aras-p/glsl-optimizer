/* -*- mode: c; tab-width: 8; -*- */
/* vi: set sw=4 ts=8: */
/* Platform-specific types and definitions for egl.h */

#ifndef __eglplatform_h_
#define __eglplatform_h_

/* Windows calling convention boilerplate */
#if (defined(WIN32) || defined(_WIN32_WCE))
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		/* Exclude rarely-used stuff from Windows headers */
#endif
#include <windows.h>
#endif

#if !defined(_WIN32_WCE)
#include <sys/types.h>
#endif

/* Macros used in EGL function prototype declarations.
 *
 * EGLAPI return-type EGLAPIENTRY eglFunction(arguments);
 * typedef return-type (EXPAPIENTRYP PFNEGLFUNCTIONPROC) (arguments);
 *
 * On Windows, EGLAPIENTRY can be defined like APIENTRY.
 * On most other platforms, it should be empty.
 */

#ifndef EGLAPIENTRY
#define EGLAPIENTRY
#endif
#ifndef EGLAPIENTRYP
#define EGLAPIENTRYP EGLAPIENTRY *
#endif

/* The types NativeDisplayType, NativeWindowType, and NativePixmapType
 * are aliases of window-system-dependent types, such as X Display * or
 * Windows Device Context. They must be defined in platform-specific
 * code below. The EGL-prefixed versions of Native*Type are the same
 * types, renamed in EGL 1.3 so all types in the API start with "EGL".
 */

/* Unix (tentative)
    #include <X headers>
    typedef Display *NativeDisplayType;
      - or maybe, if encoding "hostname:display.head"
    typedef const char *NativeWindowType;
	etc.
 */


#if (defined(WIN32) || defined(_WIN32_WCE))

/** BEGIN Added for Windows **/
#ifndef EGLAPI
#define EGLAPI __declspec(dllexport)
#endif

typedef long	int32_t;
typedef unsigned long u_int32_t;
typedef unsigned char uint8_t;
#define snprintf _snprintf
#define strcasecmp _stricmp
#define vsnprintf _vsnprintf

typedef HDC		NativeDisplayType;
typedef HWND	NativeWindowType;
typedef HBITMAP NativePixmapType;
/** END Added for Windows **/

#elif defined(__gnu_linux__)

/** BEGIN Added for X (Mesa) **/
#ifndef EGLAPI
#define EGLAPI extern
#endif

#include <X11/Xlib.h>
typedef Display *NativeDisplayType;
typedef Window NativeWindowType;
typedef Pixmap NativePixmapType;
/** END Added for X (Mesa) **/

#endif

/* EGL 1.2 types, renamed for consistency in EGL 1.3 */
typedef NativeDisplayType EGLNativeDisplayType;
typedef NativePixmapType EGLNativePixmapType;
typedef NativeWindowType EGLNativeWindowType;

#endif /* __eglplatform_h */
