/* $Id: genkgi_visual.c,v 1.1 1999/08/21 05:59:29 jtaylor Exp $
******************************************************************************

   display-fbdev-mesa-generic-kgi: visual handling

   Copyright (C) 1998 Andrew Apted	[andrew@ggi-project.org]
   Copyright (C) 1999 Marcus Sundberg	[marcus@ggi-project.org]
   Copyright (C) 1999 Creative Labs

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************************
*/

#include <ggi/internal/ggi-dl.h>
#include <ggi/mesa/ggimesa_int.h>
#include <ggi/mesa/display/fbdev.h>
#include "genkgi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_VT_H
#include <sys/vt.h>
#else
#include <linux/vt.h>
#endif
#ifdef HAVE_LINUX_KDEV_T_H
#include <linux/kdev_t.h>
#endif
#include <linux/tty.h>


static int refcount = 0;
static int vtnum;
static void *_ggi_fbdev_lock = NULL;
#ifdef FBIOGET_CON2FBMAP
static struct fb_con2fbmap origconmap;
#endif

#define MAX_DEV_LEN	63
#define DEFAULT_FBNUM	0

static char accel_prefix[] = "tgt-fbdev-kgicon-";
#define PREFIX_LEN	(sizeof(accel_prefix))

typedef struct {
	int   async;
	char *str;
} accel_info;

static accel_info accel_strings[] = 
{
	{ 0, "savage4" },		/* S3 Savage4			*/
};

#define NUM_ACCELS	(sizeof(accel_strings)/sizeof(accel_info))

/* FIXME: These should be defined in the makefile system */
#define CONF_FILE "/usr/local/etc/ggi/mesa/targets/genkgi.conf"
void *_configHandle;
char confstub[512] = CONF_FILE;
char *conffile = confstub;

static int changed(ggi_visual_t vis, int whatchanged)
{
	gl_ggiDEBUG("Entered ggimesa_genkgi_changed\n");
	switch (whatchanged)
	{
		case GGI_CHG_APILIST:
		{
			char api[256];
			char args[256];
			int i;
			const char *fname;
			ggi_dlhandle *lib;
			
			for (i = 0; ggiGetAPI(vis, i, api, args) == 0; i++)
			{
				strcat(api, "-mesa");
				gl_ggiDEBUG("ggimesa_genkgi_changed: api=%s, i=%d\n", api, i);
				fname = ggMatchConfig(_configHandle, api, NULL);
				if (fname == NULL)
				{
					/* No special implementation for this sublib */
					continue;
				}
				
				lib = ggiExtensionLoadDL(vis, fname, args, NULL);
			}
		} 
		break;
	}
	return 0;
}

int GGIdlinit(ggi_visual *vis, const char *args, void *argptr)
{
	genkgi_hook_mesa *priv;
	char libname[256], libargs[256];
	int id, err;
	struct stat junk;
	ggifunc_getapi *oldgetapi;

	gl_ggiDEBUG("display-fbdev-kgicon-mesa: GGIdlinit start\n");
	
	GENKGI_PRIVATE(vis) = priv = malloc(sizeof(genkgi_hook_mesa));
	if (priv == NULL) 
	{
		fprintf(stderr, "Failed to allocate genkgi_hook!\n");
		return GGI_DL_ERROR;
	}
	
	err = ggLoadConfig(conffile, &_configHandle);
	if (err != GGI_OK)
	{
		gl_ggiPrint("display-fbdev: Couldn't open %s\n", conffile);
		return err;
	}

	/* Hack city here.  We need to probe the KGI driver properly to discover
	 * the acceleration type.
	 */
	priv->have_accel = 0;
#if 1
	if (stat("/proc/savage4", &junk) == 0)
	{
		sprintf(priv->accel, "%s%s", accel_prefix, "savage4");
		priv->have_accel = 1;
		gl_ggiDEBUG("display-fbdev-kgicon-mesa: Using accel: \"%s\"\n", priv->accel);
	}

#endif	
	/* Mode management */
	vis->opdisplay->getapi = GGIMesa_genkgi_getapi;
	
	ggiIndicateChange(vis, GGI_CHG_APILIST);

	
	/* Give the accel sublibs a chance to set up a driver */
	if (priv->have_accel == 1)
	{
		oldgetapi = vis->opdisplay->getapi;
		vis->opdisplay->getapi = GGIMesa_genkgi_getapi;
		changed(vis, GGI_CHG_APILIST);
		/* If the accel sublibs didn't produce, back up
		 * and keep looking */
		if ((LIBGGI_MESAEXT(vis)->update_state == NULL) ||
		    (LIBGGI_MESAEXT(vis)->setup_driver == NULL))
		  vis->opdisplay->getapi = oldgetapi;
	}

	gl_ggiDEBUG("display-fbdev-kgicon-mesa: GGIdlinit finished\n");

	return 0;
}

int GGIdlcleanup(ggi_visual *vis)
{
	return 0;
}

#include <ggi/internal/ggidlinit.h>
