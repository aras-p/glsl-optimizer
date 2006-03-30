#ifndef WMESADEF_H
#define WMESADEF_H

#include "context.h"

typedef struct _dibSection {
    HDC		hDC;
    HANDLE	hFileMap;
    BOOL	fFlushed;
    LPVOID	base;
} WMDIBSECTION, *PWMDIBSECTION;

/**
 * The Windows Mesa rendering context, derived from GLcontext.
 */
struct wmesa_context {
    GLcontext           gl_ctx;	        /* The core GL/Mesa context */
    HDC                 hDC;
    COLORREF		clearColorRef;
    HPEN                clearPen;
    HBRUSH              clearBrush;
    GLuint		ScanWidth; /* XXX move into wmesa_framebuffer */
    GLboolean		rgb_flag; /* XXX remove - use gl_visual field */
    GLboolean		db_flag; /* XXX remove - use gl_visual field */
    GLboolean		alpha_flag; /* XXX remove - use gl_visual field */
    WMDIBSECTION	dib;
    BITMAPINFO          bmi;
    HBITMAP             hbmDIB;
    HBITMAP             hOldBitmap;
    PBYTE               pbPixels;
    BYTE		cColorBits;
    int			pixelformat;
};


/**
 * Windows framebuffer, derived from gl_framebuffer
 */
struct wmesa_framebuffer
{
    struct gl_framebuffer Base;
    HDC hdc;
    struct wmesa_framebuffer *next;
};

typedef struct wmesa_framebuffer *WMesaFramebuffer;


#endif /* WMESADEF_H */
