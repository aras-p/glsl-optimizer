/*
 * Textured cylinder demo: lighting, texturing, reflection mapping.
 *
 * Brian Paul  May 1997  This program is in the public domain.
 *
 * Conversion to UGL/Mesa by Stephane Raimbault
 */

/*
 * $Log: ugltexcyl.c,v $
 * Revision 1.2  2001/09/10 19:21:13  brianp
 * WindML updates (Stephane Raimbault)
 *
 * Revision 1.1  2001/08/20 16:07:11  brianp
 * WindML driver (Stephane Raimbault)
 *
 * Revision 1.5  2001/03/27 17:35:26  brianp
 * set initial window pos
 *
 * Revision 1.4  2000/12/24 22:53:54  pesco
 * * demos/Makefile.am (INCLUDES): Added -I$(top_srcdir)/util.
 * * demos/Makefile.X11, demos/Makefile.BeOS-R4, demos/Makefile.cygnus:
 * Essentially the same.
 * Program files updated to include "readtex.c", not "../util/readtex.c".
 * * demos/reflect.c: Likewise for "showbuffer.c".
 *
 *
 * * Makefile.am (EXTRA_DIST): Added top-level regular files.
 *
 * * include/GL/Makefile.am (INC_X11): Added glxext.h.
 *
 *
 * * src/GGI/include/ggi/mesa/Makefile.am (EXTRA_HEADERS): Include
 * Mesa GGI headers in dist even if HAVE_GGI is not given.
 *
 * * configure.in: Look for GLUT and demo source dirs in $srcdir.
 *
 * * src/swrast/Makefile.am (libMesaSwrast_la_SOURCES): Set to *.[ch].
 * More source list updates in various Makefile.am's.
 *
 * * Makefile.am (dist-hook): Remove CVS directory from distribution.
 * (DIST_SUBDIRS): List all possible subdirs here.
 * (SUBDIRS): Only list subdirs selected for build again.
 * The above two applied to all subdir Makefile.am's also.
 *
 * Revision 1.3  2000/09/29 23:09:39  brianp
 * added fps output
 *
 * Revision 1.2  1999/10/21 16:39:06  brianp
 * added -info command line option
 *
 * Revision 1.1.1.1  1999/08/19 00:55:40  jtg
 * Imported sources
 *
 * Revision 3.3  1999/03/28 18:24:37  brianp
 * minor clean-up
 *
 * Revision 3.2  1998/11/05 04:34:04  brianp
 * moved image files to ../images/ directory
 *
 * Revision 3.1  1998/06/23 03:16:51  brianp
 * added Point/Linear sampling menu items
 *
 * Revision 3.0  1998/02/14 18:42:29  brianp
 * initial rev
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <tickLib.h>

#include <ugl/ugl.h>
#include <ugl/uglucode.h>
#include <ugl/uglevent.h>
#include <ugl/uglinput.h>

#include <GL/uglmesa.h>
#include <GL/glu.h>

#include "../util/readtex.h"

#define TEXTURE_FILE "Mesa/images/reflect.rgb"

#define LIT 1
#define TEXTURED 2
#define REFLECT 3
#define ANIMATE 10
#define POINT_FILTER 20
#define LINEAR_FILTER 21
#define QUIT 100
#define COUNT_FRAMES

UGL_LOCAL UGL_EVENT_SERVICE_ID eventServiceId;
UGL_LOCAL UGL_EVENT_Q_ID qId;
UGL_LOCAL volatile UGL_BOOL stopWex;
UGL_LOCAL UGL_MESA_CONTEXT umc;

UGL_LOCAL GLuint CylinderObj;
UGL_LOCAL GLboolean Animate;
UGL_LOCAL GLboolean linearFilter;

UGL_LOCAL GLfloat Xrot, Yrot, Zrot;
UGL_LOCAL GLfloat DXrot, DYrot;

UGL_LOCAL GLuint limit;
UGL_LOCAL GLuint count;
UGL_LOCAL GLuint tickStart, tickStop, tickBySec;

UGL_LOCAL void cleanUp (void);

UGL_LOCAL void drawGL(void)
    {
#ifdef COUNT_FRAMES
    int time;
#endif

    glClear( GL_COLOR_BUFFER_BIT );
    
    glPushMatrix();
    glRotatef(Xrot, 1.0, 0.0, 0.0);
    glRotatef(Yrot, 0.0, 1.0, 0.0);
    glRotatef(Zrot, 0.0, 0.0, 1.0);
    glScalef(5.0, 5.0, 5.0);
    glCallList(CylinderObj);
    
    glPopMatrix();
    
    uglMesaSwapBuffers();
    
    if (Animate)
	{
	Xrot += DXrot;
	Yrot += DYrot;
	}

#ifdef COUNT_FRAMES
    if (count > limit)
        {
	tickStop = tickGet ();
	time = (tickStop-tickStart)/tickBySec;
	printf (" %i fps\n", count/time);
	tickStart = tickStop;
	count = 0;
        }
    else 
	count++;
#endif

    }

UGL_LOCAL void echoUse(void)
    {
    printf("Keys:\n");
    printf("     Up/Down  Rotate on Y\n");
    printf("  Left/Right  Rotate on X\n");
    printf("           a  Toggle animation\n");
    printf("           f  Toggle point/linear filtered\n");
    printf("           l  Lit\n");
    printf("           t  Textured\n");
    printf("           r  Reflect\n"); 
    printf("         ESC  Exit\n");
    }

UGL_LOCAL void readKey(UGL_WCHAR key)
    {
    float step = 3.0;
    switch (key)
	{
	case 'a':
	    Animate = !Animate;
	    break;
	case 'f':
	    if(linearFilter)
		{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
				GL_NEAREST);
		}
	    else
		{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
				GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
				GL_LINEAR);
		}
	    linearFilter = !linearFilter;
	    break;
	case 'l':
	    glEnable(GL_LIGHTING);
	    glDisable(GL_TEXTURE_2D);
	    glDisable(GL_TEXTURE_GEN_S);
	    glDisable(GL_TEXTURE_GEN_T);
	    break;
	case 't':
	    glDisable(GL_LIGHTING);
	    glEnable(GL_TEXTURE_2D);
	    glDisable(GL_TEXTURE_GEN_S);
	    glDisable(GL_TEXTURE_GEN_T);
	    break;
	case 'r':
	    glDisable(GL_LIGHTING);
	    glEnable(GL_TEXTURE_2D);
	    glEnable(GL_TEXTURE_GEN_S);
	    glEnable(GL_TEXTURE_GEN_T);
	    break;
	case UGL_UNI_UP_ARROW:
	    Xrot += step;
	    break;
	case UGL_UNI_DOWN_ARROW:
	    Xrot -= step;
	    break;
	case UGL_UNI_LEFT_ARROW:
	    Yrot += step;
	    break;
	case UGL_UNI_RIGHT_ARROW:
	    Yrot -= step;
	    break;
	case UGL_UNI_ESCAPE:
	    stopWex = UGL_TRUE;
	    break;
	}
    }

UGL_LOCAL void loopEvent(void)
    {
    UGL_EVENT event;
    UGL_INPUT_EVENT * pInputEvent;
 
    UGL_FOREVER
	{
	if (uglEventGet (qId, &event, sizeof (event), UGL_NO_WAIT)
	    != UGL_STATUS_Q_EMPTY)
            {
	    pInputEvent = (UGL_INPUT_EVENT *)&event;
	    
	    if (pInputEvent->header.type == UGL_EVENT_TYPE_KEYBOARD &&
		pInputEvent->modifiers & UGL_KEYBOARD_KEYDOWN)
		readKey(pInputEvent->type.keyboard.key);
	    }
	
	drawGL();
	if (stopWex)
            break;
	}
    }

UGL_LOCAL void initGL(void)
    {
    GLUquadricObj *q = gluNewQuadric();
    CylinderObj = glGenLists(1);
    glNewList(CylinderObj, GL_COMPILE);

    glTranslatef(0.0, 0.0, -1.0);

    /* cylinder */
    gluQuadricNormals(q, GL_SMOOTH);
    gluQuadricTexture(q, GL_TRUE);
    gluCylinder(q, 0.6, 0.6, 2.0, 24, 1);

    /* end cap */
    glTranslatef(0.0, 0.0, 2.0);
    gluDisk(q, 0.0, 0.6, 24, 1);

    /* other end cap */
    glTranslatef(0.0, 0.0, -2.0);
    gluQuadricOrientation(q, GLU_INSIDE);
    gluDisk(q, 0.0, 0.6, 24, 1);

    glEndList();
    gluDeleteQuadric(q);

    /* lighting */
    glEnable(GL_LIGHTING);
    {
    GLfloat gray[4] = {0.2, 0.2, 0.2, 1.0};
    GLfloat white[4] = {1.0, 1.0, 1.0, 1.0};
    GLfloat teal[4] = { 0.0, 1.0, 0.8, 1.0 };
    glMaterialfv(GL_FRONT, GL_DIFFUSE, teal);
    glLightfv(GL_LIGHT0, GL_AMBIENT, gray);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
    glEnable(GL_LIGHT0);
    }

    /* fitering = nearest, initially */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);

    if (!LoadRGBMipmaps(TEXTURE_FILE, GL_RGB))
	{
	printf("Error: couldn't load texture image\n");
	cleanUp();
	exit(1);
	}
    
    glEnable(GL_CULL_FACE);  /* don't need Z testing for convex objects */

    glEnable(GL_LIGHTING);

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glFrustum( -1.0, 1.0, -1.0, 1.0, 10.0, 100.0 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glTranslatef( 0.0, 0.0, -70.0 );

    printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
    printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
    printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
    printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));

#ifdef COUNT_FRAMES
    tickStart = tickGet ();
    tickBySec = sysClkRateGet ();
#endif

    }

UGL_LOCAL void cleanUp (void)
    {
    uglEventQDestroy (eventServiceId, qId);
    
    uglMesaDestroyContext();
    uglDeinitialize ();
    }

void windMLTexCyl (UGL_BOOL windMLMode);

void ugltexcyl (void)
    {
    taskSpawn ("tTexCyl", 210, VX_FP_TASK, 100000, (FUNCPTR)windMLTexCyl,
	       UGL_FALSE,1,2,3,4,5,6,7,8,9);
    }

void windMLTexCyl (UGL_BOOL windMLMode)
    {
    UGL_INPUT_DEVICE_ID keyboardDevId;
    GLsizei displayWidth, displayHeight;
    GLsizei x, y, w, h;

    CylinderObj = 0;
    Animate = GL_TRUE;
    linearFilter = GL_FALSE;
    Xrot = 0.0;
    Yrot = 0.0;
    Zrot = 0.0;
    DXrot = 1.0;
    DYrot = 2.5;
    limit = 100;
    count = 1;
    
    uglInitialize ();

    uglDriverFind (UGL_KEYBOARD_TYPE, 0,
		   (UGL_UINT32 *)&keyboardDevId);

    uglDriverFind (UGL_EVENT_SERVICE_TYPE, 0, (UGL_UINT32 *)&eventServiceId);

    qId = uglEventQCreate (eventServiceId, 100);
    
    /* Double buffering */
    if (windMLMode)
       umc = uglMesaCreateNewContext(UGL_MESA_DOUBLE
				     | UGL_MESA_WINDML_EXCLUSIVE, NULL);
    else
       umc = uglMesaCreateNewContext(UGL_MESA_DOUBLE, NULL);
    
    if (umc == NULL)
        {
	uglDeinitialize ();
	return;
	}

    uglMesaMakeCurrentContext (umc, 0, 0, 1, 1);

    uglMesaGetIntegerv(UGL_MESA_DISPLAY_WIDTH, &displayWidth);
    uglMesaGetIntegerv(UGL_MESA_DISPLAY_HEIGHT, &displayHeight);
    
    h = (displayHeight*3)/4;
    w = h;
    x = (displayWidth-w)/2;
    y = (displayHeight-h)/2;
    
    uglMesaMoveToWindow(x, y);
    uglMesaResizeToWindow(w, h);

    initGL ();
    
    echoUse();

    stopWex = UGL_FALSE;
    loopEvent();

    cleanUp();
    
    return;
    }

