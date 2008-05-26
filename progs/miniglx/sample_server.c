
/*
 * Sample server that just keeps first available window mapped.
 */


#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/miniglx.h>

struct client {
   struct client *next;
   Window windowid;
   int mappable;
};

struct client *clients = 0, *mapped_client = 0;


static struct client *find_client( Window id )
{
   struct client *c;

   for (c = clients ; c ; c = c->next)
      if (c->windowid == id)
	 return c;

   return 0;
}

int main( int argc, char *argv[] )
{
   Display *dpy;
   XEvent ev;

   dpy = __miniglx_StartServer(NULL);
   if (!dpy) {
      fprintf(stderr, "Error: __miniglx_StartServer failed\n");
      return 1;
   }

   while (XNextEvent( dpy, &ev )) {
      struct client *c;

      switch (ev.type) {
      case MapRequest:
	 fprintf(stderr, "MapRequest\n");
	 c = find_client(ev.xmaprequest.window);
	 if (!c) break;
	 c->mappable = True;
	 break;

      case UnmapNotify:
	 fprintf(stderr, "UnmapNotify\n");
	 c = find_client(ev.xunmap.window);
	 if (!c) break;
	 c->mappable = False;
	 if (c == mapped_client)
	    mapped_client = 0;
	 break;

      case CreateNotify: 
	 fprintf(stderr, "CreateNotify\n");
	 c = malloc(sizeof(*c));
	 c->next = clients;
	 c->windowid = ev.xcreatewindow.window;
	 c->mappable = False;
	 clients = c;
	 break;

      case DestroyNotify:
	 fprintf(stderr, "DestroyNotify\n");
	 c = find_client(ev.xdestroywindow.window);
	 if (!c) break;
	 if (c == clients)
	    clients = c->next;
	 else {
	    struct client *t;
	    for (t = clients ; t->next != c ; t = t->next)
	       ;
	    t->next = c->next;
	 }

	 if (c == mapped_client) 
	    mapped_client = 0;

	 free(c);
	 break;

      default:
	 break;
      }

      /* Search for first mappable client if none already mapped.
       */
      if (!mapped_client) {
	 for (c = clients ; c ; c = c->next) {
	    if (c->mappable) {
	       XMapWindow( dpy, c->windowid );
	       mapped_client = c;
	       break;
	    }
	 }
      }
   }

   XCloseDisplay( dpy );

   return 0;
}
