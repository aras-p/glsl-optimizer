#ifndef WMESADEF_H
#define WMESADEF_H
#ifdef __MINGW32__
#include <windows.h>
#endif
#if 0
#include "context.h"
#endif
#include "state_tracker/st_context.h"
#include "state_tracker/st_public.h"


/**
 * The Windows Mesa rendering context, derived from GLcontext.
 */
struct wmesa_context {
    struct st_context 	*st;
    HDC                 hDC;
    BYTE		cColorBits;
};

/**
 * Windows framebuffer, derived from gl_framebuffer
 */
struct wmesa_framebuffer
{
    struct st_framebuffer *stfb;
    HDC                 hDC;
    int			pixelformat;
    BYTE		cColorBits;
    HDC                 dib_hDC;
    HBITMAP             hbmDIB;
    HBITMAP             hOldBitmap;
    PBYTE               pbPixels;
    struct wmesa_framebuffer *next;
};

typedef struct wmesa_framebuffer *WMesaFramebuffer;

#endif /* WMESADEF_H */
