/*
 * $TOG: xdpyinfo.c /main/35 1998/02/09 13:57:05 kaleb $
 * 
 * xdpyinfo - print information about X display connecton
 *
 * 
Copyright 1988, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
 *
 * Author:  Jim Fulton, MIT X Consortium
 * GLX and Overlay support added by Jamie Zawinski <jwz@jwz.org>, 11-Nov-99
 *
 *      To compile:
 *         cc -DHAVE_GLX glxdpyinfo.c -o glxdpyinfo -lGL -lX11 -lXext -lm
 *
 *      Other defines to consider:
 *         -DHAVE_XIE -DHAVE_XTEST -DHAVE_XRECORD
 */

#define HAVE_GLX  /* added by Brian Paul */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h> /* for CARD32 */
#include <X11/extensions/multibuf.h>
#ifdef HAVE_XIE
#include <X11/extensions/XIElib.h>
#endif /* HAVE_XIE */
#ifdef HAVE_XTEST
#include <X11/extensions/XTest.h>
#endif /* HAVE_XTEST */
#include <X11/extensions/sync.h>
#include <X11/extensions/Xdbe.h>
#ifdef HAVE_XRECORD
#include <X11/extensions/record.h>
#endif /* HAVE_XRECORD */
#ifdef MITSHM
#include <X11/extensions/XShm.h>
#endif
#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_GLX
# include <GL/gl.h>
# include <GL/glx.h>
#endif /* HAVE_GLX */

#define HAVE_OVERLAY /* jwz: no compile-time deps, so do this all the time */

char *ProgramName;
Bool queryExtensions = False;

static int StrCmp(a, b)
    char **a, **b;
{
    return strcmp(*a, *b);
}


static int print_event_mask (char *buf, int lastcol, int indent, long mask);



#ifdef HAVE_GLX  /* Added by jwz, 11-Nov-99 */

static void
print_glx_versions (dpy)
    Display *dpy;
{
  /* Note: with Mesa 3.0, this lies: it prints the info from the
     client's GL library, rather than the info from the GLX server.

     Note also that we can't protect these calls by only doing
     them when the GLX extension is present, because with Mesa,
     the server doesn't have that extension (but the GL library
     works anyway.)
   */
      int scr = DefaultScreen (dpy);
      const char *vend, *vers;
      vend = glXQueryServerString (dpy, scr, GLX_VENDOR);
      if (!vend) return;
      vers = glXQueryServerString (dpy, scr, GLX_VERSION);
      printf ("GLX vendor:    %s (%s)\n",
              vend, (vers ? vers : "unknown version"));
}

static void
print_glx_visual_info (dpy, vip)
    Display *dpy;
    XVisualInfo *vip;
{
  int status, value = False;

  status = glXGetConfig (dpy, vip, GLX_USE_GL, &value);
  if (status == GLX_NO_EXTENSION)
    /* dpy does not support the GLX extension. */
    return;

  if (status == GLX_BAD_VISUAL || value == False)
    {
      printf ("    GLX supported:     no\n");
      return;
    }
  else
    {
      printf ("    GLX supported:     yes\n");
    }
    
  if (!glXGetConfig (dpy, vip, GLX_LEVEL, &value) &&
      value != 0)
    printf ("    GLX level:         %d\n", value);

  if (!glXGetConfig (dpy, vip, GLX_RGBA, &value) && value)
    {
      int r=0, g=0, b=0, a=0;
      glXGetConfig (dpy, vip, GLX_RED_SIZE,   &r);
      glXGetConfig (dpy, vip, GLX_GREEN_SIZE, &g);
      glXGetConfig (dpy, vip, GLX_BLUE_SIZE,  &b);
      glXGetConfig (dpy, vip, GLX_ALPHA_SIZE, &a);
      printf ("    GLX type:          RGBA (%2d, %2d, %2d, %2d)\n",
              r, g, b, a);

      r=0, g=0, b=0, a=0;
      glXGetConfig (dpy, vip, GLX_ACCUM_RED_SIZE,   &r);
      glXGetConfig (dpy, vip, GLX_ACCUM_GREEN_SIZE, &g);
      glXGetConfig (dpy, vip, GLX_ACCUM_BLUE_SIZE,  &b);
      glXGetConfig (dpy, vip, GLX_ACCUM_ALPHA_SIZE, &a);
      printf ("    GLX accum:         RGBA (%2d, %2d, %2d, %2d)\n",
              r, g, b, a);
    }
  else
    {
      value = 0;
      glXGetConfig (dpy, vip, GLX_BUFFER_SIZE, &value);
      printf ("    GLX type:          indexed (%d)\n", value);
    }

# if 0  /* redundant */
  if (!glXGetConfig (dpy, vip, GLX_X_VISUAL_TYPE_EXT, &value))
      printf ("    GLX class:         %s\n",
              (value == GLX_TRUE_COLOR_EXT ? "TrueColor" :
               value == GLX_DIRECT_COLOR_EXT ? "DirectColor" :
               value == GLX_PSEUDO_COLOR_EXT ? "PseudoColor" :
               value == GLX_STATIC_COLOR_EXT ? "StaticColor" :
               value == GLX_GRAY_SCALE_EXT ? "Grayscale" :
               value == GLX_STATIC_GRAY_EXT ? "StaticGray" : "???"));
# endif

# ifdef GLX_VISUAL_CAVEAT_EXT
  if (!glXGetConfig (dpy, vip, GLX_VISUAL_CAVEAT_EXT, &value) &&
      value != GLX_NONE_EXT)
    printf ("    GLX rating:        %s\n",
            (value == GLX_NONE_EXT ? "none" :
             value == GLX_SLOW_VISUAL_EXT ? "slow" :
#   ifdef GLX_NON_CONFORMANT_EXT
             value == GLX_NON_CONFORMANT_EXT ? "non-conformant" :
#   endif
             "???"));
# endif

  if (!glXGetConfig (dpy, vip, GLX_DOUBLEBUFFER, &value))
    printf ("    GLX double-buffer: %s\n", (value ? "yes" : "no"));

  if (!glXGetConfig (dpy, vip, GLX_STEREO, &value) &&
      value)
    printf ("    GLX stereo:        %s\n", (value ? "yes" : "no"));

  if (!glXGetConfig (dpy, vip, GLX_AUX_BUFFERS, &value) &&
      value != 0)
    printf ("    GLX aux buffers:   %d\n", value);

  if (!glXGetConfig (dpy, vip, GLX_DEPTH_SIZE, &value))
    printf ("    GLX depth size:    %d\n", value);

  if (!glXGetConfig (dpy, vip, GLX_STENCIL_SIZE, &value) &&
      value != 0)
    printf ("    GLX stencil size:  %d\n", value);

# ifdef GLX_SAMPLE_BUFFERS_SGIS
  if (!glXGetConfig (dpy, vip, GLX_SAMPLE_BUFFERS_SGIS, &value) &&
      value != 0)
    {
      int bufs = value;
      if (!glXGetConfig (dpy, vip, GLX_SAMPLES_SGIS, &value))
        printf ("    GLX multisamplers: %d (%d)\n", bufs, value);
    }
# endif

  if (!glXGetConfig (dpy, vip, GLX_TRANSPARENT_TYPE_EXT, &value) &&
      value != GLX_NONE_EXT)
    {
      if (value == GLX_NONE_EXT)
        printf ("    GLX transparency:  none\n");
      else if (value == GLX_TRANSPARENT_INDEX_EXT)
        {
          if (!glXGetConfig (dpy, vip, GLX_TRANSPARENT_INDEX_VALUE_EXT,&value))
            printf ("    GLX transparency:  indexed (%d)\n", value);
        }
      else if (value == GLX_TRANSPARENT_RGB_EXT)
        {
          int r=0, g=0, b=0, a=0;
          glXGetConfig (dpy, vip, GLX_TRANSPARENT_RED_VALUE_EXT,   &r);
          glXGetConfig (dpy, vip, GLX_TRANSPARENT_GREEN_VALUE_EXT, &g);
          glXGetConfig (dpy, vip, GLX_TRANSPARENT_BLUE_VALUE_EXT,  &b);
          glXGetConfig (dpy, vip, GLX_TRANSPARENT_ALPHA_VALUE_EXT, &a);
          printf ("    GLX transparency:  RGBA (%2d, %2d, %2d, %2d)\n",
                  r, g, b, a);
        }
    }
}
#endif /* HAVE_GLX */


#ifdef HAVE_OVERLAY  /* Added by jwz, 11-Nov-99 */

 /* If the server's root window contains a SERVER_OVERLAY_VISUALS property,
    then that identifies the visuals which correspond to the video hardware's
    overlay planes.  Windows created in these kinds of visuals may have
    transparent pixels that let other layers shine through.

    This might not be an X Consortium standard, but it turns out that
    SGI, HP, DEC, and IBM all use this same mechanism.  So that's close
    enough for me.

    Documentation on the SERVER_OVERLAY_VISUALS property can be found at:
    http://www.hp.com/xwindow/sharedInfo/Whitepapers/Visuals/server_overlay_visuals.html
  */

struct overlay
{
  CARD32 visual_id;
  CARD32 transparency; /* 0: none; 1: pixel; 2: mask */
  CARD32 value;                /* the transparent pixel */
  CARD32 layer;                /* -1: underlay; 0: normal; 1: popup; 2: overlay */
};

struct overlay_list
{
  int count;
  struct overlay *list;
};

static struct overlay_list *overlays = 0;

static void
find_overlay_info (dpy)
  Display *dpy;
{
  int screen;
  Atom OVERLAY = XInternAtom (dpy, "SERVER_OVERLAY_VISUALS", False);

  overlays = (struct overlay_list *) calloc (sizeof (struct overlay_list),
                                             ScreenCount (dpy));

  for (screen = 0; screen < ScreenCount (dpy); screen++)
    {
      Window window = RootWindow (dpy, screen);
      Atom actual_type;
      int actual_format;
      unsigned long nitems, bytes_after;
      struct overlay *data = 0;
      int result = XGetWindowProperty (dpy, window, OVERLAY,
                                       0, (65536 / sizeof (long)), False, 
                                       OVERLAY, &actual_type, &actual_format,
                                       &nitems, &bytes_after,
                                       (unsigned char **) &data);
      if (result == Success &&
          actual_type == OVERLAY &&
          actual_format == 32 &&
          nitems > 0)
        {
          overlays[screen].count = (nitems /
                                    (sizeof(struct overlay) / sizeof(CARD32)));
          overlays[screen].list = data;
        }
      else if (data)
        XFree((char *) data);
    }
}

static void
print_overlay_visual_info (vip)
    XVisualInfo *vip;
{
  int i;
  int vis = vip->visualid;
  int scr = vip->screen;
  if (!overlays) return;
  for (i = 0; i < overlays[scr].count; i++)
    if (vis == overlays[scr].list[i].visual_id)
      {
        struct overlay *ov = &overlays[scr].list[i];
        printf ("    Overlay info:      layer %ld (%s), ",
                (long) ov->layer,
                (ov->layer == -1 ? "underlay" :
                 ov->layer ==  0 ? "normal" :
                 ov->layer ==  1 ? "popup" :
                 ov->layer ==  2 ? "overlay" : "???"));
        if (ov->transparency == 1)
          printf ("transparent pixel %lu\n", (unsigned long) ov->value);
        else if (ov->transparency == 2)
          printf ("transparent mask 0x%x\n", (unsigned) ov->value);
        else
          printf ("opaque\n");
      }
}
#endif /* HAVE_OVERLAY */


static void
print_extension_info (dpy)
    Display *dpy;
{
    int n = 0;
    char **extlist = XListExtensions (dpy, &n);

    printf ("number of extensions:    %d\n", n);

    if (extlist) {
	register int i;
	int opcode, event, error;

	qsort(extlist, n, sizeof(char *), StrCmp);
	for (i = 0; i < n; i++) {
	    if (!queryExtensions) {
		printf ("    %s\n", extlist[i]);
		continue;
	    }
	    XQueryExtension(dpy, extlist[i], &opcode, &event, &error);
	    printf ("    %s  (opcode: %d", extlist[i], opcode);
	    if (event)
		printf (", base event: %d", event);
	    if (error)
		printf (", base error: %d", error);
	    printf(")\n");
	}
	/* do not free, Xlib can depend on contents being unaltered */
	/* XFreeExtensionList (extlist); */
    }
}

static void
print_display_info (dpy)
    Display *dpy;
{
    char dummybuf[40];
    char *cp;
    int minkeycode, maxkeycode;
    int i, n;
    long req_size;
    XPixmapFormatValues *pmf;
    Window focuswin;
    int focusrevert;

    printf ("name of display:    %s\n", DisplayString (dpy));
    printf ("version number:    %d.%d\n",
	    ProtocolVersion (dpy), ProtocolRevision (dpy));
    printf ("vendor string:    %s\n", ServerVendor (dpy));
    printf ("vendor release number:    %d\n", VendorRelease (dpy));

#ifdef HAVE_GLX
    print_glx_versions (dpy);
#endif /* HAVE_GLX */

    req_size = XExtendedMaxRequestSize (dpy);
    if (!req_size) req_size = XMaxRequestSize (dpy);
    printf ("maximum request size:  %ld bytes\n", req_size * 4);
    printf ("motion buffer size:  %d\n", (int) XDisplayMotionBufferSize(dpy));

    switch (BitmapBitOrder (dpy)) {
      case LSBFirst:    cp = "LSBFirst"; break;
      case MSBFirst:    cp = "MSBFirst"; break;
      default:    
	sprintf (dummybuf, "unknown order %d", BitmapBitOrder (dpy));
	cp = dummybuf;
	break;
    }
    printf ("bitmap unit, bit order, padding:    %d, %s, %d\n",
	    BitmapUnit (dpy), cp, BitmapPad (dpy));

    switch (ImageByteOrder (dpy)) {
      case LSBFirst:    cp = "LSBFirst"; break;
      case MSBFirst:    cp = "MSBFirst"; break;
      default:    
	sprintf (dummybuf, "unknown order %d", ImageByteOrder (dpy));
	cp = dummybuf;
	break;
    }
    printf ("image byte order:    %s\n", cp);

    pmf = XListPixmapFormats (dpy, &n);
    printf ("number of supported pixmap formats:    %d\n", n);
    if (pmf) {
	printf ("supported pixmap formats:\n");
	for (i = 0; i < n; i++) {
	    printf ("    depth %d, bits_per_pixel %d, scanline_pad %d\n",
		    pmf[i].depth, pmf[i].bits_per_pixel, pmf[i].scanline_pad);
	}
	XFree ((char *) pmf);
    }


    /*
     * when we get interfaces to the PixmapFormat stuff, insert code here
     */

    XDisplayKeycodes (dpy, &minkeycode, &maxkeycode);
    printf ("keycode range:    minimum %d, maximum %d\n",
	    minkeycode, maxkeycode);

    XGetInputFocus (dpy, &focuswin, &focusrevert);
    printf ("focus:  ");
    switch (focuswin) {
      case PointerRoot:
	printf ("PointerRoot\n");
	break;
      case None:
	printf ("None\n");
	break;
      default:
	printf("window 0x%lx, revert to ", focuswin);
	switch (focusrevert) {
	  case RevertToParent:
	    printf ("Parent\n");
	    break;
	  case RevertToNone:
	    printf ("None\n");
	    break;
	  case RevertToPointerRoot:
	    printf ("PointerRoot\n");
	    break;
	  default:			/* should not happen */
	    printf ("%d\n", focusrevert);
	    break;
	}
	break;
    }

    print_extension_info (dpy);

    printf ("default screen number:    %d\n", DefaultScreen (dpy));
    printf ("number of screens:    %d\n", ScreenCount (dpy));
}

static void
print_visual_info (vip)
    XVisualInfo *vip;
{
    char errorbuf[40];			/* for sprintfing into */
    char *class = NULL;			/* for printing */

    switch (vip->class) {
      case StaticGray:    class = "StaticGray"; break;
      case GrayScale:    class = "GrayScale"; break;
      case StaticColor:    class = "StaticColor"; break;
      case PseudoColor:    class = "PseudoColor"; break;
      case TrueColor:    class = "TrueColor"; break;
      case DirectColor:    class = "DirectColor"; break;
      default:    
	sprintf (errorbuf, "unknown class %d", vip->class);
	class = errorbuf;
	break;
    }

    printf ("  visual:\n");
    printf ("    visual id:    0x%lx\n", vip->visualid);
    printf ("    class:    %s\n", class);
    printf ("    depth:    %d plane%s\n", vip->depth, 
	    vip->depth == 1 ? "" : "s");
    if (vip->class == TrueColor || vip->class == DirectColor)
	printf ("    available colormap entries:    %d per subfield\n",
		vip->colormap_size);
    else
	printf ("    available colormap entries:    %d\n",
		vip->colormap_size);
    printf ("    red, green, blue masks:    0x%lx, 0x%lx, 0x%lx\n",
	    vip->red_mask, vip->green_mask, vip->blue_mask);
    printf ("    significant bits in color specification:    %d bits\n",
	    vip->bits_per_rgb);
}

static void
print_screen_info (dpy, scr)
    Display *dpy;
    int scr;
{
    Screen *s = ScreenOfDisplay (dpy, scr);  /* opaque structure */
    XVisualInfo viproto;		/* fill in for getting info */
    XVisualInfo *vip;			/* retured info */
    int nvi;				/* number of elements returned */
    int i;				/* temp variable: iterator */
    char eventbuf[80];			/* want 79 chars per line + nul */
    static char *yes = "YES", *no = "NO", *when = "WHEN MAPPED";
    double xres, yres;
    int ndepths = 0, *depths = NULL;
    unsigned int width, height;


    /*
     * there are 2.54 centimeters to an inch; so there are 25.4 millimeters.
     *
     *     dpi = N pixels / (M millimeters / (25.4 millimeters / 1 inch))
     *         = N pixels / (M inch / 25.4)
     *         = N * 25.4 pixels / M inch
     */

    xres = ((((double) DisplayWidth(dpy,scr)) * 25.4) / 
	    ((double) DisplayWidthMM(dpy,scr)));
    yres = ((((double) DisplayHeight(dpy,scr)) * 25.4) / 
	    ((double) DisplayHeightMM(dpy,scr)));

    printf ("\n");
    printf ("screen #%d:\n", scr);
    printf ("  dimensions:    %dx%d pixels (%dx%d millimeters)\n",
	    DisplayWidth (dpy, scr), DisplayHeight (dpy, scr),
	    DisplayWidthMM(dpy, scr), DisplayHeightMM (dpy, scr));
    printf ("  resolution:    %dx%d dots per inch\n", 
	    (int) (xres + 0.5), (int) (yres + 0.5));
    depths = XListDepths (dpy, scr, &ndepths);
    if (!depths) ndepths = 0;
    printf ("  depths (%d):    ", ndepths);
    for (i = 0; i < ndepths; i++) {
	printf ("%d", depths[i]);
	if (i < ndepths - 1) { 
	    putchar (',');
	    putchar (' ');
	}
    }
    putchar ('\n');
    if (depths) XFree ((char *) depths);
    printf ("  root window id:    0x%lx\n", RootWindow (dpy, scr));
    printf ("  depth of root window:    %d plane%s\n",
	    DisplayPlanes (dpy, scr),
	    DisplayPlanes (dpy, scr) == 1 ? "" : "s");
    printf ("  number of colormaps:    minimum %d, maximum %d\n",
	    MinCmapsOfScreen(s), MaxCmapsOfScreen(s));
    printf ("  default colormap:    0x%lx\n", DefaultColormap (dpy, scr));
    printf ("  default number of colormap cells:    %d\n",
	    DisplayCells (dpy, scr));
    printf ("  preallocated pixels:    black %ld, white %ld\n",
	    BlackPixel (dpy, scr), WhitePixel (dpy, scr));
    printf ("  options:    backing-store %s, save-unders %s\n",
	    (DoesBackingStore (s) == NotUseful) ? no :
	    ((DoesBackingStore (s) == Always) ? yes : when),
	    DoesSaveUnders (s) ? yes : no);
    XQueryBestSize (dpy, CursorShape, RootWindow (dpy, scr), 65535, 65535,
		    &width, &height);
    if (width == 65535 && height == 65535)
	printf ("  largest cursor:    unlimited\n");
    else
	printf ("  largest cursor:    %dx%d\n", width, height);
    printf ("  current input event mask:    0x%lx\n", EventMaskOfScreen (s));
    (void) print_event_mask (eventbuf, 79, 4, EventMaskOfScreen (s));
		      

    nvi = 0;
    viproto.screen = scr;
    vip = XGetVisualInfo (dpy, VisualScreenMask, &viproto, &nvi);
    printf ("  number of visuals:    %d\n", nvi);
    printf ("  default visual id:  0x%lx\n", 
	    XVisualIDFromVisual (DefaultVisual (dpy, scr)));
    for (i = 0; i < nvi; i++) {
	print_visual_info (vip+i);
#ifdef HAVE_OVERLAY
       print_overlay_visual_info (vip+i);
#endif /* HAVE_OVERLAY */
#ifdef HAVE_GLX
       print_glx_visual_info (dpy, vip+i);
#endif /* HAVE_GLX */
    }
    if (vip) XFree ((char *) vip);
}

/*
 * The following routine prints out an event mask, wrapping events at nice
 * boundaries.
 */

#define MASK_NAME_WIDTH 25

static struct _event_table {
    char *name;
    long value;
} event_table[] = {
    { "KeyPressMask             ", KeyPressMask },
    { "KeyReleaseMask           ", KeyReleaseMask },
    { "ButtonPressMask          ", ButtonPressMask },
    { "ButtonReleaseMask        ", ButtonReleaseMask },
    { "EnterWindowMask          ", EnterWindowMask },
    { "LeaveWindowMask          ", LeaveWindowMask },
    { "PointerMotionMask        ", PointerMotionMask },
    { "PointerMotionHintMask    ", PointerMotionHintMask },
    { "Button1MotionMask        ", Button1MotionMask },
    { "Button2MotionMask        ", Button2MotionMask },
    { "Button3MotionMask        ", Button3MotionMask },
    { "Button4MotionMask        ", Button4MotionMask },
    { "Button5MotionMask        ", Button5MotionMask },
    { "ButtonMotionMask         ", ButtonMotionMask },
    { "KeymapStateMask          ", KeymapStateMask },
    { "ExposureMask             ", ExposureMask },
    { "VisibilityChangeMask     ", VisibilityChangeMask },
    { "StructureNotifyMask      ", StructureNotifyMask },
    { "ResizeRedirectMask       ", ResizeRedirectMask },
    { "SubstructureNotifyMask   ", SubstructureNotifyMask },
    { "SubstructureRedirectMask ", SubstructureRedirectMask },
    { "FocusChangeMask          ", FocusChangeMask },
    { "PropertyChangeMask       ", PropertyChangeMask },
    { "ColormapChangeMask       ", ColormapChangeMask },
    { "OwnerGrabButtonMask      ", OwnerGrabButtonMask },
    { NULL, 0 }};

static int print_event_mask (char *buf, int lastcol, int indent, long mask)
{
    struct _event_table *etp;
    int len;
    int bitsfound = 0;

    buf[0] = buf[lastcol] = '\0';	/* just in case */

#define INDENT() { register int i; len = indent; \
		   for (i = 0; i < indent; i++) buf[i] = ' '; }

    INDENT ();

    for (etp = event_table; etp->name; etp++) {
	if (mask & etp->value) {
	    if (len + MASK_NAME_WIDTH > lastcol) {
		puts (buf);
		INDENT ();
	    }
	    strcpy (buf+len, etp->name);
	    len += MASK_NAME_WIDTH;
	    bitsfound++;
	}
    }

    if (bitsfound) puts (buf);

#undef INDENT

    return (bitsfound);
}

static void
print_standard_extension_info(dpy, extname, majorrev, minorrev)
    Display *dpy;
    char *extname;
    int majorrev, minorrev;
{
    int opcode, event, error;

    printf("%s version %d.%d ", extname, majorrev, minorrev);

    XQueryExtension(dpy, extname, &opcode, &event, &error);
    printf ("opcode: %d", opcode);
    if (event)
	printf (", base event: %d", event);
    if (error)
	printf (", base error: %d", error);
    printf("\n");
}

static int
print_multibuf_info(dpy, extname)
    Display *dpy;
    char *extname;
{
    int i, j;			/* temp variable: iterator */
    int nmono, nstereo;		/* count */
    XmbufBufferInfo *mono_info = NULL, *stereo_info = NULL; /* arrays */
    static char *fmt = 
	"    visual id, max buffers, depth:    0x%lx, %d, %d\n";
    int scr = 0;
    int majorrev, minorrev;

    if (!XmbufGetVersion(dpy, &majorrev, &minorrev))
	return 0;

    print_standard_extension_info(dpy, extname, majorrev, minorrev);

    for (i = 0; i < ScreenCount (dpy); i++)
    {
	if (!XmbufGetScreenInfo (dpy, RootWindow(dpy, scr), &nmono, &mono_info,
				 &nstereo, &stereo_info)) {
	    fprintf (stderr,
		     "%s:  unable to get multibuffer info for screen %d\n",
		     ProgramName, scr);
	} else {
	    printf ("  screen %d number of mono multibuffer types:    %d\n", i, nmono);
	    for (j = 0; j < nmono; j++) {
		printf (fmt, mono_info[j].visualid, mono_info[j].max_buffers,
			mono_info[j].depth);
	    }
	    printf ("  number of stereo multibuffer types:    %d\n", nstereo);
	    for (j = 0; j < nstereo; j++) {
		printf (fmt, stereo_info[j].visualid,
			stereo_info[j].max_buffers, stereo_info[j].depth);
	    }
	    if (mono_info) XFree ((char *) mono_info);
	    if (stereo_info) XFree ((char *) stereo_info);
	}
    }
    return 1;
} /* end print_multibuf_info */


/* XIE stuff */

#ifdef HAVE_XIE

char *subset_names[] = { NULL, "FULL", "DIS" };
char *align_names[] = { NULL, "Alignable", "Arbitrary" };
char *group_names[] = { /* 0  */ "Default",
			    /* 2  */ "ColorAlloc",
			    /* 4  */ "Constrain",
			    /* 6  */ "ConvertFromRGB",
			    /* 8  */ "ConvertToRGB",
			    /* 10 */ "Convolve",
			    /* 12 */ "Decode",
			    /* 14 */ "Dither",
			    /* 16 */ "Encode",
			    /* 18 */ "Gamut",
			    /* 20 */ "Geometry",
			    /* 22 */ "Histogram",
			    /* 24 */ "WhiteAdjust"
			    };

int
print_xie_info(dpy, extname)
    Display *dpy;
    char *extname;
{
    XieExtensionInfo *xieInfo;
    int i;
    int ntechs;
    XieTechnique *techs;
    XieTechniqueGroup prevGroup;

    if (!XieInitialize(dpy, &xieInfo ))
	return 0;

    print_standard_extension_info(dpy, extname,
	xieInfo->server_major_rev, xieInfo->server_minor_rev);

    printf("  service class: %s\n", subset_names[xieInfo->service_class]);
    printf("  alignment: %s\n", align_names[xieInfo->alignment]);
    printf("  uncnst_mantissa: %d\n", xieInfo->uncnst_mantissa);
    printf("  uncnst_min_exp: %d\n", xieInfo->uncnst_min_exp);
    printf("  uncnst_max_exp: %d\n", xieInfo->uncnst_max_exp);
    printf("  cnst_levels:"); 
    for (i = 0; i < xieInfo->n_cnst_levels; i++)
	printf(" %d", xieInfo->cnst_levels[i]);
    printf("\n");

    if (!XieQueryTechniques(dpy, xieValAll, &ntechs, &techs))
	return 1;

    prevGroup = -1;

    for (i = 0; i < ntechs; i++)
    {
	if (techs[i].group != prevGroup)
	{
	    printf("  technique group: %s\n", group_names[techs[i].group >> 1]);
	    prevGroup = techs[i].group;
	}
	printf("    %s\tspeed: %d  needs_param: %s  number: %d\n",
	       techs[i].name,
	       techs[i].speed, (techs[i].needs_param ? "True " : "False"),
	       techs[i].number);
    }
    return 1;
} /* end print_xie_info */

#endif /* HAVE_XIE */


#ifdef HAVE_XTEST
int
print_xtest_info(dpy, extname)
    Display *dpy;
    char *extname;
{
    int majorrev, minorrev, foo;

    if (!XTestQueryExtension(dpy, &foo, &foo, &majorrev, &minorrev))
	return 0;
    print_standard_extension_info(dpy, extname, majorrev, minorrev);
    return 1;
}
#endif /* HAVE_XTEST */

static int
print_sync_info(dpy, extname)
    Display *dpy;
    char *extname;
{
    int majorrev, minorrev;
    XSyncSystemCounter *syscounters;
    int ncounters, i;

    if (!XSyncInitialize(dpy, &majorrev, &minorrev))
	return 0;
    print_standard_extension_info(dpy, extname, majorrev, minorrev);

    syscounters = XSyncListSystemCounters(dpy, &ncounters);
    printf("  system counters: %d\n", ncounters);
    for (i = 0; i < ncounters; i++)
    {
	printf("    %s  id: 0x%08x  resolution_lo: %d  resolution_hi: %d\n",
	       syscounters[i].name, (int) syscounters[i].counter,
	       (int) XSyncValueLow32(syscounters[i].resolution),
               (int) XSyncValueHigh32(syscounters[i].resolution));
    }
    XSyncFreeSystemCounterList(syscounters);
    return 1;
}

static int
print_shape_info(dpy, extname)
    Display *dpy;
    char *extname;
{
    int majorrev, minorrev;

    if (!XShapeQueryVersion(dpy, &majorrev, &minorrev))
	return 0;
    print_standard_extension_info(dpy, extname, majorrev, minorrev);
    return 1;
}

#ifdef MITSHM
static int
print_mitshm_info(dpy, extname)
    Display *dpy;
    char *extname;
{
    int majorrev, minorrev;
    Bool sharedPixmaps;

    if (!XShmQueryVersion(dpy, &majorrev, &minorrev, &sharedPixmaps))
	return 0;
    print_standard_extension_info(dpy, extname, majorrev, minorrev);
    printf("  shared pixmaps: ");
    if (sharedPixmaps)
    {
	int format = XShmPixmapFormat(dpy);
	printf("yes, format: %d\n", format);
    }
    else
    {
	printf("no\n");
    }
    return 1;
}
#endif /* MITSHM */

static int
print_dbe_info(dpy, extname)
    Display *dpy;
    char *extname;
{
    int majorrev, minorrev;
    XdbeScreenVisualInfo *svi;
    int numscreens = 0;
    int iscrn, ivis;

    if (!XdbeQueryExtension(dpy, &majorrev, &minorrev))
	return 0;

    print_standard_extension_info(dpy, extname, majorrev, minorrev);
    svi = XdbeGetVisualInfo(dpy, (Drawable *)NULL, &numscreens);
    for (iscrn = 0; iscrn < numscreens; iscrn++)
    {
	printf("  Double-buffered visuals on screen %d\n", iscrn);
	for (ivis = 0; ivis < svi[iscrn].count; ivis++)
	{
	    printf("    visual id 0x%lx  depth %d  perflevel %d\n",
		   svi[iscrn].visinfo[ivis].visual,
		   svi[iscrn].visinfo[ivis].depth,
		   svi[iscrn].visinfo[ivis].perflevel);
	}
    }
    XdbeFreeVisualInfo(svi);
    return 1;
}

#ifdef HAVE_XRECORD
static int
print_record_info(dpy, extname)
    Display *dpy;
    char *extname;
{
    int majorrev, minorrev;

    if (!XRecordQueryVersion(dpy, &majorrev, &minorrev))
	return 0;
    print_standard_extension_info(dpy, extname, majorrev, minorrev);
    return 1;
}
#endif /* HAVE_XRECORD */

/* utilities to manage the list of recognized extensions */


typedef int (*ExtensionPrintFunc)(
#if NeedFunctionPrototypes
    Display *, char *
#endif
);

typedef struct {
    char *extname;
    ExtensionPrintFunc printfunc;
    Bool printit;
} ExtensionPrintInfo;

ExtensionPrintInfo known_extensions[] =
{
#ifdef MITSHM
    {"MIT-SHM",	print_mitshm_info, False},
#endif /* MITSHM */
    {MULTIBUFFER_PROTOCOL_NAME,	print_multibuf_info, False},
    {"SHAPE", print_shape_info, False},
    {SYNC_NAME, print_sync_info, False},
#ifdef HAVE_XIE
    {xieExtName, print_xie_info, False},
#endif /* HAVE_XIE */
#ifdef HAVE_XTEST
    {XTestExtensionName, print_xtest_info, False},
#endif /* HAVE_XTEST */
    {"DOUBLE-BUFFER", print_dbe_info, False},
#ifdef HAVE_XRECORD
    {"RECORD", print_record_info, False}    
#endif /* HAVE_XRECORD */
    /* add new extensions here */
    /* wish list: PEX XKB LBX */
};

int num_known_extensions = sizeof known_extensions / sizeof known_extensions[0];

static void
print_known_extensions(f)
    FILE *f;
{
    int i;
    for (i = 0; i < num_known_extensions; i++)
    {
	fprintf(f, "%s ", known_extensions[i].extname);
    }
}

static void
mark_extension_for_printing(extname)
    char *extname;
{
    int i;

    if (strcmp(extname, "all") == 0)
    {
	for (i = 0; i < num_known_extensions; i++)
	    known_extensions[i].printit = True;
    }
    else
    {
	for (i = 0; i < num_known_extensions; i++)
	{
	    if (strcmp(extname, known_extensions[i].extname) == 0)
	    {
		known_extensions[i].printit = True;
		return;
	    }
	}
	printf("%s extension not supported by %s\n", extname, ProgramName);
    }
}

static void
print_marked_extensions(dpy)
    Display *dpy;
{
    int i;
    for (i = 0; i < num_known_extensions; i++)
    {
	if (known_extensions[i].printit)
	{
	    printf("\n");
	    if (! (*known_extensions[i].printfunc)(dpy,
					known_extensions[i].extname))
	    {
		printf("%s extension not supported by server\n",
		       known_extensions[i].extname);
	    }
	}
    }
}

static void usage ()
{
    fprintf (stderr, "usage:  %s [options]\n", ProgramName);
    fprintf (stderr, "-display displayname\tserver to query\n");
    fprintf (stderr, "-queryExtensions\tprint info returned by XQueryExtension\n");
    fprintf (stderr, "-ext all\t\tprint detailed info for all supported extensions\n");
    fprintf (stderr, "-ext extension-name\tprint detailed info for extension-name if one of:\n     ");
    print_known_extensions(stderr);
    fprintf (stderr, "\n");
    exit (1);
}

int main (argc, argv)
    int argc;
    char *argv[];
{
    Display *dpy;			/* X connection */
    char *displayname = NULL;		/* server to contact */
    int i;				/* temp variable:  iterator */

    ProgramName = argv[0];

    for (i = 1; i < argc; i++) {
	char *arg = argv[i];
	int len = strlen(arg);
	
	if (!strncmp("-display", arg, len)) {
	    if (++i >= argc) usage ();
	    displayname = argv[i];
	} else if (!strncmp("-queryExtensions", arg, len)) {
	    queryExtensions = True;
	} else if (!strncmp("-ext", arg, len)) {
	    if (++i >= argc) usage ();
	    mark_extension_for_printing(argv[i]);
	} else
	    usage ();
    }

    dpy = XOpenDisplay (displayname);
    if (!dpy) {
	fprintf (stderr, "%s:  unable to open display \"%s\".\n",
		 ProgramName, XDisplayName (displayname));
	exit (1);
    }

#ifdef HAVE_OVERLAY
       find_overlay_info (dpy);
#endif /* HAVE_OVERLAY */

    print_display_info (dpy);
    for (i = 0; i < ScreenCount (dpy); i++) {
	print_screen_info (dpy, i);
    }

    print_marked_extensions(dpy);

    XCloseDisplay (dpy);
    exit (0);
}
