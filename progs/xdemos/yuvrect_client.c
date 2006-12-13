/*
 * Test the GL_NV_texture_rectangle and GL_MESA_ycrcb_texture extensions and GLX_MESA_allocate-memory
 *
 * Dave Airlie - Feb 2005
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/glx.h>

#include "../util/readtex.c"   /* I know, this is a hack. */

#define TEXTURE_FILE "../images/girl2.rgb"

static GLfloat Xrot = 0, Yrot = 0, Zrot = 0;
static GLint ImgWidth, ImgHeight;
static GLushort *ImageYUV = NULL;
static void *glx_memory;

static void DrawObject(void)
{
   glBegin(GL_QUADS);

   glTexCoord2f(0, 0);
   glVertex2f(-1.0, -1.0);

   glTexCoord2f(ImgWidth, 0);
   glVertex2f(1.0, -1.0);

   glTexCoord2f(ImgWidth, ImgHeight);
   glVertex2f(1.0, 1.0);

   glTexCoord2f(0, ImgHeight);
   glVertex2f(-1.0, 1.0);

   glEnd();
}


static void scr_Display( void )
{
   glClear( GL_COLOR_BUFFER_BIT );

   glPushMatrix();
      glRotatef(Xrot, 1.0, 0.0, 0.0);
      glRotatef(Yrot, 0.0, 1.0, 0.0);
      glRotatef(Zrot, 0.0, 0.0, 1.0);
      DrawObject();
   glPopMatrix();

}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -1.0, 1.0, 10.0, 100.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -15.0 );
}

static int queryClient(Display *dpy, int screen)
{
#ifdef GLX_MESA_allocate_memory
  char *extensions;

  extensions = (char *)glXQueryExtensionsString(dpy, screen);
  if (!extensions || !strstr(extensions,"GLX_MESA_allocate_memory")) {
    return 0;
  }

  return 1;
#else
  return 0;
#endif
}

static int
query_extension(char* extName) {
    char *p = (char *) glGetString(GL_EXTENSIONS);
    char *end = p + strlen(p);
    while (p < end) {
        int n = strcspn(p, " ");
        if ((strlen(extName) == n) && (strncmp(extName, p, n) == 0))
            return GL_TRUE;
        p += (n + 1);
    }
    return GL_FALSE;
}

static void Init( int argc, char *argv[] , Display *dpy, int screen, Window win)
{
   GLuint texObj = 100;
   const char *file;
   void *glx_memory;

   if (!query_extension("GL_NV_texture_rectangle")) {
      printf("Sorry, GL_NV_texture_rectangle is required\n");
      exit(0);
   }

   if (!query_extension("GL_MESA_ycbcr_texture")) {
      printf("Sorry, GL_MESA_ycbcr_texture is required\n");
      exit(0);
   }

   if (!queryClient(dpy, screen)) {
     printf("Sorry, GLX_MESA_allocate_memory is required\n");
     exit(0);
   }

   glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, 1);
   glBindTexture(GL_TEXTURE_RECTANGLE_NV, texObj);
#ifdef LINEAR_FILTER
   /* linear filtering looks much nicer but is much slower for Mesa */
   glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#else
   glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif

   if (argc > 1)
      file = argv[1];
   else
      file = TEXTURE_FILE;

   ImageYUV = LoadYUVImage(file, &ImgWidth, &ImgHeight);
   if (!ImageYUV) {
      printf("Couldn't read %s\n", TEXTURE_FILE);
      exit(0);
   }
   
   glx_memory = glXAllocateMemoryMESA(dpy, screen, ImgWidth * ImgHeight * 2, 0, 0 ,0);
   if (!glx_memory)
   {
     fprintf(stderr,"Failed to allocate MESA memory\n");
     exit(-1);
   }
   
   memcpy(glx_memory, ImageYUV, ImgWidth * ImgHeight * 2);
   
   printf("Image: %dx%d\n", ImgWidth, ImgHeight);

   glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0,
                GL_YCBCR_MESA, ImgWidth, ImgHeight, 0,
                GL_YCBCR_MESA, GL_UNSIGNED_SHORT_8_8_APPLE, glx_memory);

   assert(glGetError() == GL_NO_ERROR);

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   glEnable(GL_TEXTURE_RECTANGLE_NV);

   glShadeModel(GL_FLAT);
   glClearColor(0.3, 0.3, 0.4, 1.0);

}

/*
 * Create an RGB, double-buffered window.
 * Return the window and context handles.
 */
static void
make_window( Display *dpy, const char *name,
             int x, int y, int width, int height,
             Window *winRet, GLXContext *ctxRet)
{
   int attribs[] = { GLX_RGBA,
                     GLX_RED_SIZE, 1,
                     GLX_GREEN_SIZE, 1,
                     GLX_BLUE_SIZE, 1,
                     GLX_DOUBLEBUFFER,
                     GLX_DEPTH_SIZE, 1,
                     None };
   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;
   GLXContext ctx;
   XVisualInfo *visinfo;

   scrnum = DefaultScreen( dpy );
   root = RootWindow( dpy, scrnum );
   
   visinfo = glXChooseVisual( dpy, scrnum, attribs );
   if (!visinfo) {
     printf("Error: couldn't get an RGB, Double-buffered visual\n");
     exit(1);
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   attr.override_redirect = 0;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect;

   win = XCreateWindow( dpy, root, 0, 0, width, height,
		        0, visinfo->depth, InputOutput,
		        visinfo->visual, mask, &attr );

   /* set hints and properties */
   {
      XSizeHints sizehints;
      sizehints.x = x;
      sizehints.y = y;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(dpy, win, &sizehints);
      XSetStandardProperties(dpy, win, name, name,
                              None, (char **)NULL, 0, &sizehints);
   }

   ctx = glXCreateContext( dpy, visinfo, NULL, True );
   if (!ctx) {
      printf("Error: glXCreateContext failed\n");
      exit(1);
   }

   XFree(visinfo);

   *winRet = win;
   *ctxRet = ctx;
}


static void
event_loop(Display *dpy, Window win)
{
   while (1) {
      while (XPending(dpy) > 0) {
         XEvent event;
         XNextEvent(dpy, &event);
         switch (event.type) {
	 case Expose:
            /* we'll redraw below */
	    break;
	 case ConfigureNotify:
	   Reshape(event.xconfigure.width, event.xconfigure.height);
	    break;
         case KeyPress:
            {
               char buffer[10];
               int r, code;
               code = XLookupKeysym(&event.xkey, 0);
	       r = XLookupString(&event.xkey, buffer, sizeof(buffer),
				 NULL, NULL);
	       if (buffer[0] == 27) {
		 /* escape */
		 return;
                 
               }
            }
         }
      }

   }
}


int
main(int argc, char *argv[])
{
   Display *dpy;
   Window win;
   GLXContext ctx;
   char *dpyName = NULL;
   GLboolean printInfo = GL_FALSE;
   int i;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-display") == 0) {
         dpyName = argv[i+1];
         i++;
      }
      else if (strcmp(argv[i], "-info") == 0) {
         printInfo = GL_TRUE;
      }
      else
	 printf("Warrning: unknown parameter: %s\n", argv[i]);
   }

   dpy = XOpenDisplay(dpyName);
   if (!dpy) {
      printf("Error: couldn't open display %s\n",
	     XDisplayName(dpyName));
      return -1;
   }

   make_window(dpy, "yuvrect_client", 0, 0, 300, 300, &win, &ctx);
   XMapWindow(dpy, win);
   glXMakeCurrent(dpy, win, ctx);

   if (printInfo) {
      printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
      printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
      printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
   }

   Init(argc, argv, dpy, DefaultScreen(dpy), win);

   scr_Display();			       
   glXSwapBuffers(dpy, win);
   event_loop(dpy, win);

   glXFreeMemoryMESA(dpy, DefaultScreen(dpy), glx_memory);
   glXDestroyContext(dpy, ctx);
   XDestroyWindow(dpy, win);
   XCloseDisplay(dpy);

   return 0;
}
