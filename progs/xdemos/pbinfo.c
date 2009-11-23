
/*
 * Print list of fbconfigs and test each to see if a pbuffer can be created
 * for that config.
 *
 * Brian Paul
 * April 1997
 * Updated on 5 October 2002.
 */


#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>
#include "pbutil.h"




static void
PrintConfigs(Display *dpy, int screen, Bool horizFormat)
{
   FBCONFIG *fbConfigs;
   int nConfigs;
   int i;

   fbConfigs = GetAllFBConfigs(dpy, screen, &nConfigs);
   if (!nConfigs || !fbConfigs) {
      printf("Error: glxGetFBConfigs failed\n");
      XFree(fbConfigs);
      return;
   }

   printf("Number of fbconfigs: %d\n", nConfigs);

   if (horizFormat) {
      printf("  ID        VisualType  Depth Lvl RGB CI DB Stereo  R  G  B  A");
      printf("   Z  S  AR AG AB AA  MSbufs MSnum  Pbuffer  Float\n");
   }

   /* Print config info */
   for (i = 0; i < nConfigs; i++) {
      PrintFBConfigInfo(dpy, screen, fbConfigs[i], horizFormat);
   }

   /* free the list */
   XFree(fbConfigs);
}



static void
PrintUsage(void)
{
   printf("Options:\n");
   printf("  -display <display-name>  specify X display name\n");
   printf("  -t                       print in tabular format\n");
   printf("  -v                       print in verbose format\n");
   printf("  -help                    print this information\n");
}


int
main(int argc, char *argv[])
{
   Display *dpy;
   int scrn;
   char *dpyName = NULL;
   Bool horizFormat = True;
   int i;

   for (i=1; i<argc; i++) {
      if (strcmp(argv[i],"-display")==0) {
	 if (i+1<argc) {
	    dpyName = argv[i+1];
	    i++;
	 }
      }
      else if (strcmp(argv[i],"-t")==0) {
	 /* tabular format */
	 horizFormat = True;
      }
      else if (strcmp(argv[i],"-v")==0) {
	 /* verbose format */
	 horizFormat = False;
      }
      else if (strcmp(argv[i],"-help")==0) {
	 PrintUsage();
	 return 0;
      }
      else {
	 printf("Unknown option: %s\n", argv[i]);
      }
   }

   dpy = XOpenDisplay(dpyName);

   if (!dpy) {
      printf("Error: couldn't open display %s\n", XDisplayName(dpyName));
      return 1;
   }

   scrn = DefaultScreen(dpy);
   PrintConfigs(dpy, scrn, horizFormat);
   XCloseDisplay(dpy);
   return 0;
}
