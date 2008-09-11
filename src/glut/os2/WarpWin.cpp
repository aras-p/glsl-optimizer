/* WarpWin.c     */
/* glut for Warp */
#include <stdio.h>
#include <string.h>

#include "WarpWin.h"
#include "WarpGL.h"

#define POKA 0

/* global variables that must be set for some functions to operate
   correctly. */
HDC XHDC;
HWND XHWND;


void
XStoreColor(Display* display, Colormap colormap, XColor* color)
{
  /* KLUDGE: set XHDC to 0 if the palette should NOT be realized after
     setting the color.  set XHDC to the correct HDC if it should. */

  LONG pe;
  ULONG cclr;
  int r,g,b;
  /* X11 stores color from 0-65535, Win32 expects them to be 0-256, so
     twiddle the bits ( / 256). */
  r = color->red / 256;
  g = color->green / 256;
  b = color->blue / 256;
  pe = LONGFromRGB(r,g,b);
  /* make sure we use this flag, otherwise the colors might get mapped
     to another place in the colormap, and when we glIndex() that
     color, it may have moved (argh!!) */
  pe |= (PC_NOCOLLAPSE<<24);
/* This function changes the entries in a palette.  */
#if POKA
OS2:
 rc = GpiSetPaletteEntries(colormap,LCOLF_CONSECRGB, color->pixel, 1, &pe);
 GpiSelectPalette(hps,colormap);
 WinRealizePalette(hwnd,hps,&cclr);
source Win:
  if (XHDC) {
    UnrealizeObject(colormap);
    SelectPalette(XHDC, colormap, FALSE);
    RealizePalette(XHDC);

   }
#endif
}

void
XSetWindowColormap(Display* display, Window window, Colormap colormap)
{
#if POKA
  HDC hdc = GetDC(window);

  /* if the third parameter is FALSE, the logical colormap is copied
     into the device palette when the application is in the
     foreground, if it is TRUE, the colors are mapped into the current
     palette in the best possible way. */
  SelectPalette(hdc, colormap, FALSE);
  RealizePalette(hdc);

  /* note that we don't have to release the DC, since our window class
     uses the WC_OWNDC flag! */
#endif
}


/* display, root and visual - don't used at all */
Colormap
XCreateColormap(Display* display, Window root, Visual* visual, int alloc)
{
  /* KLUDGE: this function needs XHDC to be set to the HDC currently
     being operated on before it is invoked! */

  HPAL    palette;
  int n;
#if POKA
  PIXELFORMATDESCRIPTOR pfd;
  LOGPALETTE *logical;

  /* grab the pixel format */
  memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
  DescribePixelFormat(XHDC, GetPixelFormat(XHDC),
                     sizeof(PIXELFORMATDESCRIPTOR), &pfd);

  if (!(pfd.dwFlags & PFD_NEED_PALETTE ||
      pfd.iPixelType == PFD_TYPE_COLORINDEX))
  {
    return 0;
  }

  n = 1 << pfd.cColorBits;

  /* allocate a bunch of memory for the logical palette (assume 256
     colors in a Win32 palette */
  logical = (LOGPALETTE*)malloc(sizeof(LOGPALETTE) +
                               sizeof(PALETTEENTRY) * n);
  memset(logical, 0, sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * n);

  /* set the entries in the logical palette */
  logical->palVersion = 0x300;
  logical->palNumEntries = n;

  /* start with a copy of the current system palette */
  GetSystemPaletteEntries(XHDC, 0, 256, &logical->palPalEntry[0]);

  if (pfd.iPixelType == PFD_TYPE_RGBA) {
    int redMask = (1 << pfd.cRedBits) - 1;
    int greenMask = (1 << pfd.cGreenBits) - 1;
    int blueMask = (1 << pfd.cBlueBits) - 1;
    int i;

    /* fill in an RGBA color palette */
    for (i = 0; i < n; ++i) {
      logical->palPalEntry[i].peRed =
       (((i >> pfd.cRedShift)   & redMask)   * 255) / redMask;
      logical->palPalEntry[i].peGreen =
       (((i >> pfd.cGreenShift) & greenMask) * 255) / greenMask;
       logical->palPalEntry[i].peBlue =
       (((i >> pfd.cBlueShift)  & blueMask)  * 255) / blueMask;
      logical->palPalEntry[i].peFlags = 0;
    }
  }

  palette = CreatePalette(logical);
  free(logical);

  SelectPalette(XHDC, palette, FALSE);
  RealizePalette(XHDC);
#endif /* POKA */

  return palette;
}



int  GetSystemMetrics( int mode)
{  RECTL rect;

   switch(mode)
   {   case   SM_CXSCREEN:
        WinQueryWindowRect(HWND_DESKTOP,&rect);
        return (rect.xRight-rect.xLeft);
      break;
       case   SM_CYSCREEN:
        WinQueryWindowRect(HWND_DESKTOP,&rect);
        return (rect.yTop-rect.yBottom);
      break;
       default: ;
   }
   return 0;
}
/*
 *    XParseGeometry parses strings of the form
 *   "=<width>x<height>{+-}<xoffset>{+-}<yoffset>", where
 *   width, height, xoffset, and yoffset are unsigned integers.
 *   Example:  "=80x24+300-49"
 *   The equal sign is optional.
 *   It returns a bitmask that indicates which of the four values
 *   were actually found in the string.  For each value found,
 *   the corresponding argument is updated;  for each value
 *   not found, the corresponding argument is left unchanged.
 */

static int
ReadInteger(char *string, char **NextString)
{
    register int Result = 0;
    int Sign = 1;

    if (*string == '+')
       string++;
    else if (*string == '-')
    {
       string++;
       Sign = -1;
    }
    for (; (*string >= '0') && (*string <= '9'); string++)
    {
       Result = (Result * 10) + (*string - '0');
    }
    *NextString = string;
    if (Sign >= 0)
       return (Result);
    else
       return (-Result);
}

int XParseGeometry(char *string, int *x, int *y, unsigned int *width, unsigned int *height)
{
       int mask = NoValue;
       register char *strind;
       unsigned int tempWidth, tempHeight;
       int tempX, tempY;
       char *nextCharacter;

       if ( (string == NULL) || (*string == '\0')) return(mask);
       if (*string == '=')
               string++;  /* ignore possible '=' at beg of geometry spec */

       strind = (char *)string;
       if (*strind != '+' && *strind != '-' && *strind != 'x') {
               tempWidth = ReadInteger(strind, &nextCharacter);
               if (strind == nextCharacter)
                   return (0);
               strind = nextCharacter;
               mask |= WidthValue;
       }

       if (*strind == 'x' || *strind == 'X') {
               strind++;
               tempHeight = ReadInteger(strind, &nextCharacter);
               if (strind == nextCharacter)
                   return (0);
               strind = nextCharacter;
               mask |= HeightValue;
       }

       if ((*strind == '+') || (*strind == '-')) {
               if (*strind == '-') {
                       strind++;
                       tempX = -ReadInteger(strind, &nextCharacter);
                       if (strind == nextCharacter)
                           return (0);
                       strind = nextCharacter;
                       mask |= XNegative;

               }
               else
               {       strind++;
                       tempX = ReadInteger(strind, &nextCharacter);
                       if (strind == nextCharacter)
                           return(0);
                       strind = nextCharacter;
               }
               mask |= XValue;
               if ((*strind == '+') || (*strind == '-')) {
                       if (*strind == '-') {
                               strind++;
                               tempY = -ReadInteger(strind, &nextCharacter);
                               if (strind == nextCharacter)
                                   return(0);
                               strind = nextCharacter;
                               mask |= YNegative;

                       }
                       else
                       {
                               strind++;
                               tempY = ReadInteger(strind, &nextCharacter);
                               if (strind == nextCharacter)
                                   return(0);
                               strind = nextCharacter;
                       }
                       mask |= YValue;
               }
       }

       /* If strind isn't at the end of the string the it's an invalid
               geometry specification. */

       if (*strind != '\0') return (0);

       if (mask & XValue)
           *x = tempX;
       if (mask & YValue)
           *y = tempY;
       if (mask & WidthValue)
            *width = tempWidth;
       if (mask & HeightValue)
            *height = tempHeight;
       return (mask);
}

int gettimeofday(struct timeval* tp, void* tzp)
{
 DATETIME    DateTime;
 APIRET       ulrc;  /*  Return Code. */

 ulrc = DosGetDateTime(&DateTime);
 tp->tv_sec  = 60 * (60*DateTime.hours + DateTime.minutes) + DateTime.seconds;
 tp->tv_usec = DateTime.hundredths * 10000;
 return 0;
}


int
XPending(Display* display)
{
  /* similar functionality...I don't think that it is exact, but this
     will have to do. */
  QMSG msg;
  extern HAB   hab;      /* PM anchor block handle         */

//?? WinPeekMsg(hab
  return WinPeekMsg(hab, &msg, NULLHANDLE, 0, 0, PM_NOREMOVE);
}

void
__glutAdjustCoords(Window parent, int* x, int* y, int* width, int* height)
{
  RECTL rect;

  /* adjust the window rectangle because Win32 thinks that the x, y,
     width & height are the WHOLE window (including decorations),
     whereas GLUT treats the x, y, width & height as only the CLIENT
     area of the window. */
  rect.xLeft = *x; rect.yTop = *y;
  rect.xRight = *x + *width; rect.yBottom = *y + *height;

  /* must adjust the coordinates according to the correct style
     because depending on the style, there may or may not be
     borders. */
//??  AdjustWindowRect(&rect, WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
//??              (parent ? WS_CHILD : WS_OVERLAPPEDWINDOW),
//??              FALSE);
  /* FALSE in the third parameter = window has no menu bar */

  /* readjust if the x and y are offscreen */
  if(rect.xLeft < 0) {
    *x = 0;
  } else {
    *x = rect.xLeft;
  }

  if(rect.yTop < 0) {
    *y = 0;
  } else {
    *y = rect.yTop;
  }

  *width = rect.xRight - rect.xLeft;     /* adjusted width */
  *height = -(rect.yBottom - rect.yTop);    /* adjusted height */
}


int
__glutGetTransparentPixel(Display * dpy, XVisualInfo * vinfo)
{
  /* the transparent pixel on Win32 is always index number 0.  So if
     we put this routine in this file, we can avoid compiling the
     whole of layerutil.c which is where this routine normally comes
     from. */
  return 0;
}

/* Translate point coordinates src_x and src_y from src to dst */

Bool
XTranslateCoordinates(Display *display, Window src, Window dst,
                     int src_x, int src_y,
                     int* dest_x_return, int* dest_y_return,
                     Window* child_return)
{
  SWP swp_src,swp_dst;

  WinQueryWindowPos(src,&swp_src);
  WinQueryWindowPos(dst,&swp_dst);

  *dest_x_return =  src_x + swp_src.x - swp_dst.x;
  *dest_y_return =  src_y + swp_src.y - swp_dst.y;

  /* just to make compilers happy...we don't use the return value. */
  return True;
}

Status
XGetGeometry(Display* display, Window window, Window* root_return,
            int* x_return, int* y_return,
            unsigned int* width_return, unsigned int* height_return,
            unsigned int *border_width_return, unsigned int* depth_return)
{
  /* KLUDGE: doesn't return the border_width or depth or root, x & y
     are in screen coordinates. */
  SWP swp_src;
  WinQueryWindowPos(window,&swp_src);

  *x_return = swp_src.x;
  *y_return = swp_src.y;
  *width_return = swp_src.cx;
  *height_return = swp_src.cy;

  /* just to make compilers happy...we don't use the return value. */
  return 1;
}

/* Get Display Width in millimeters */
int
DisplayWidthMM(Display* display, int screen)
{
  int width;
  LONG *pVC_Caps;
  pVC_Caps = GetVideoConfig(NULLHANDLE);
  width = (int)( 0.001 * pVC_Caps[CAPS_WIDTH]  / pVC_Caps[CAPS_HORIZONTAL_RESOLUTION]);/* mm */
  return width;
}

/* Get Display Height in millimeters */
int
DisplayHeightMM(Display* display, int screen)
{
  int height;
  LONG *pVC_Caps;
  pVC_Caps = GetVideoConfig(NULLHANDLE);
  height = (int)( 0.001 * pVC_Caps[CAPS_HEIGHT] / pVC_Caps[CAPS_VERTICAL_RESOLUTION]); /* mm */
  return height;
}

void ScreenToClient( HWND hwnd,   POINTL *point)
{
  SWP swp_src;
  WinQueryWindowPos(hwnd,&swp_src);
  point->x -= swp_src.x;
  point->y -= swp_src.y;
}

