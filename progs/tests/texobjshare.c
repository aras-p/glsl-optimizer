/*
 * Create several OpenGL rendering contexts, sharing textures, display
 * lists, etc.  Exercise binding, deleting, etc.
 *
 * Brian Paul
 * 21 December 2004
 */


#include <GL/gl.h>
#include <GL/glx.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/keysym.h>


/*
 * Each display/window/context:
 */
struct context {
   char DisplayName[1000];
   Display *Dpy;
   Window Win;
   GLXContext Context;
};


#define MAX_CONTEXTS 200
static struct context Contexts[MAX_CONTEXTS];
static int NumContexts = 0;


static void
Error(const char *display, const char *msg)
{
   fprintf(stderr, "Error on display %s - %s\n", display, msg);
   exit(1);
}


static struct context *
CreateContext(const char *displayName, const char *name)
{
   Display *dpy;
   Window win;
   GLXContext ctx;
   int attrib[] = { GLX_RGBA,
		    GLX_RED_SIZE, 1,
		    GLX_GREEN_SIZE, 1,
		    GLX_BLUE_SIZE, 1,
		    GLX_DOUBLEBUFFER,
		    None };
   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   XVisualInfo *visinfo;
   int width = 90, height = 90;
   int xpos = 0, ypos = 0;

   if (NumContexts >= MAX_CONTEXTS)
      return NULL;

   dpy = XOpenDisplay(displayName);
   if (!dpy) {
      Error(displayName, "Unable to open display");
      return NULL;
   }

   scrnum = DefaultScreen(dpy);
   root = RootWindow(dpy, scrnum);

   visinfo = glXChooseVisual(dpy, scrnum, attrib);
   if (!visinfo) {
      Error(displayName, "Unable to find RGB, double-buffered visual");
      return NULL;
   }

   /* window attributes */
   xpos = (NumContexts % 10) * 100;
   ypos = (NumContexts / 10) * 100;
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow(dpy, root, xpos, ypos, width, height,
		        0, visinfo->depth, InputOutput,
		        visinfo->visual, mask, &attr);
   if (!win) {
      Error(displayName, "Couldn't create window");
      return NULL;
   }

   {
      XSizeHints sizehints;
      sizehints.x = xpos;
      sizehints.y = ypos;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(dpy, win, &sizehints);
      XSetStandardProperties(dpy, win, name, name,
                              None, (char **)NULL, 0, &sizehints);
   }

   if (NumContexts == 0) {
      ctx = glXCreateContext(dpy, visinfo, NULL, True);
   }
   else {
      /* share textures & dlists with 0th context */
      ctx = glXCreateContext(dpy, visinfo, Contexts[0].Context, True);
   }
   if (!ctx) {
      Error(displayName, "Couldn't create GLX context");
      return NULL;
   }

   XMapWindow(dpy, win);

   if (!glXMakeCurrent(dpy, win, ctx)) {
      Error(displayName, "glXMakeCurrent failed");
      return NULL;
   }

   if (NumContexts == 0) {
      printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   }

   /* save the info for this context */
   {
      struct context *h = &Contexts[NumContexts];
      strcpy(h->DisplayName, name);
      h->Dpy = dpy;
      h->Win = win;
      h->Context = ctx;
      NumContexts++;
      return &Contexts[NumContexts-1];
   }
}


static void
MakeCurrent(int i)
{
   if (!glXMakeCurrent(Contexts[i].Dpy, Contexts[i].Win, Contexts[i].Context)) {
      fprintf(stderr, "glXMakeCurrent failed!\n");
   }
}



static void
DestroyContext(int i)
{
   XDestroyWindow(Contexts[i].Dpy, Contexts[i].Win);
   glXDestroyContext(Contexts[i].Dpy, Contexts[i].Context);
   XCloseDisplay(Contexts[i].Dpy);
}


int
main(int argc, char *argv[])
{
   char *dpyName = NULL;
   int i;
   GLuint t;
   GLint tb;

   for (i = 0; i < 2; i++) {
      CreateContext(dpyName, "context");
   }

   /* Create texture and bind it in context 0 */
   MakeCurrent(0);
   glGenTextures(1, &t);
   printf("Generated texture ID %u\n", t);
   assert(!glIsTexture(t));
   glBindTexture(GL_TEXTURE_2D, t);
   assert(glIsTexture(t));
   glGetIntegerv(GL_TEXTURE_BINDING_2D, &tb);
   assert(tb == t);

   /* Bind texture in context 1 */
   MakeCurrent(1);
   assert(glIsTexture(t));
   glBindTexture(GL_TEXTURE_2D, t);
   glGetIntegerv(GL_TEXTURE_BINDING_2D, &tb);
   assert(tb == t);

   /* Delete texture from context 0 */
   MakeCurrent(0);
   glDeleteTextures(1, &t);
   assert(!glIsTexture(t));
   glGetIntegerv(GL_TEXTURE_BINDING_2D, &tb);
   printf("After delete, binding = %d\n", tb);

   /* Check texture state from context 1 */
   MakeCurrent(1);
   assert(!glIsTexture(t));
   glGetIntegerv(GL_TEXTURE_BINDING_2D, &tb);
   printf("In second context, binding = %d\n", tb);
   glBindTexture(GL_TEXTURE_2D, 0);
   glGetIntegerv(GL_TEXTURE_BINDING_2D, &tb);
   assert(tb == 0);
   

   for (i = 0; i < NumContexts; i++) {
      DestroyContext(i);
   }

   printf("Success!\n");

   return 0;
}
