/*
 * glDrawPixels demo/test/benchmark
 * 
 * Brian Paul   September 25, 1997  This file is in the public domain.
 *
 * Conversion to UGL/Mesa by Stephane Raimbault july, 2001
 */

/*
 * $Log: ugldrawpix.c,v $
 * Revision 1.2  2001/09/10 19:21:13  brianp
 * WindML updates (Stephane Raimbault)
 *
 * Revision 1.1  2001/08/20 16:07:11  brianp
 * WindML driver (Stephane Raimbault)
 *
 * Revision 1.5  2000/12/24 22:53:54  pesco
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
 * Revision 1.4  2000/09/08 21:45:21  brianp
 * added dither key option
 *
 * Revision 1.3  1999/10/28 18:23:29  brianp
 * minor changes to Usage() function
 *
 * Revision 1.2  1999/10/21 22:13:58  brianp
 * added f key to toggle front/back drawing
 *
 * Revision 1.1.1.1  1999/08/19 00:55:40  jtg
 * Imported sources
 *
 * Revision 3.3  1999/03/28 18:18:33  brianp
 * minor clean-up
 *
 * Revision 3.2  1998/11/05 04:34:04  brianp
 * moved image files to ../images/ directory
 *
 * Revision 3.1  1998/02/22 16:43:17  brianp
 * added a few casts to silence compiler warnings
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

#define IMAGE_FILE "Mesa/images/wrs_logo.rgb"

UGL_LOCAL UGL_EVENT_SERVICE_ID eventServiceId;
UGL_LOCAL UGL_EVENT_Q_ID qId;
UGL_LOCAL volatile UGL_BOOL stopWex;
UGL_LOCAL UGL_MESA_CONTEXT umc;

UGL_LOCAL int ImgWidth, ImgHeight;
UGL_LOCAL GLenum ImgFormat;
UGL_LOCAL GLubyte *Image;

UGL_LOCAL int Xpos, Ypos;
UGL_LOCAL int SkipPixels, SkipRows;
UGL_LOCAL int DrawWidth, DrawHeight;
UGL_LOCAL float Xzoom, Yzoom;
UGL_LOCAL GLboolean Scissor;
UGL_LOCAL GLboolean DrawFront;
UGL_LOCAL GLboolean Dither;

UGL_LOCAL void cleanUp (void);

UGL_LOCAL void reset(void)
    {
    Xpos = Ypos = 20;
    DrawWidth = ImgWidth;
    DrawHeight = ImgHeight;
    SkipPixels = SkipRows = 0;
    Scissor = GL_FALSE;
    Xzoom = Yzoom = 1.0;
    }

UGL_LOCAL void initGL(GLboolean ciMode, GLsizei width, GLsizei height)
    {
    printf("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));
    printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
    printf("GL_VENDOR = %s\n", (char *) glGetString(GL_VENDOR));
    
    Image = LoadRGBImage(IMAGE_FILE, &ImgWidth, &ImgHeight, &ImgFormat);
    if (!Image)
	{
	printf("Couldn't read %s\n", IMAGE_FILE);
	cleanUp();
	exit(1);
	}
    
    glScissor(width/4, height/4, width/2, height/2);

    if (ciMode)
	{
	/* Convert RGB image to grayscale */
	GLubyte *indexImage = malloc( ImgWidth * ImgHeight );
	GLint i;
	for (i=0; i<ImgWidth*ImgHeight; i++)
	    {
	    int gray = Image[i*3] + Image[i*3+1] + Image[i*3+2];
	    indexImage[i] = gray / 3;
	    }
	free(Image);
	Image = indexImage;
	ImgFormat = GL_COLOR_INDEX;

	for (i=0;i<255;i++)
	    {
	    float g = i / 255.0;
	    uglMesaSetColor(i, g, g, g);
	    }
	}
    
    printf("Loaded %d by %d image\n", ImgWidth, ImgHeight );

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, ImgWidth);

    reset();
    
    glViewport( 0, 0, width, height );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( 0.0, width, 0.0, height, -1.0, 1.0 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();   
    }

UGL_LOCAL void drawGL(void)
    {
    glClear(GL_COLOR_BUFFER_BIT);

    /* This allows negative raster positions: */
    glRasterPos2i(0, 0);
    glBitmap(0, 0, 0, 0, Xpos, Ypos, NULL);

    glPixelStorei(GL_UNPACK_SKIP_PIXELS, SkipPixels);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, SkipRows);

    glPixelZoom( Xzoom, Yzoom );

    if (Scissor)
	    glEnable(GL_SCISSOR_TEST);

    glDrawPixels(DrawWidth, DrawHeight, ImgFormat, GL_UNSIGNED_BYTE, Image);

    glDisable(GL_SCISSOR_TEST);

    uglMesaSwapBuffers();
    }


UGL_LOCAL void benchmark( void )
    {
    int startTick, endTick, ticksBySec;
    int draws;
    double seconds, pixelsPerSecond;

    printf("Benchmarking (4 sec)...\n");

    /* GL set-up */
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, SkipPixels);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, SkipRows);
    glPixelZoom( Xzoom, Yzoom );
    if (Scissor)
	    glEnable(GL_SCISSOR_TEST);

    if (DrawFront)
	    glDrawBuffer(GL_FRONT);
    else
	    glDrawBuffer(GL_BACK);

    /* Run timing test */
    draws = 0;

    ticksBySec = sysClkRateGet ();
    startTick = tickGet();
    
    do {
       glDrawPixels(DrawWidth, DrawHeight, ImgFormat, GL_UNSIGNED_BYTE, Image);
       draws++;
       endTick = tickGet ();
    } while ((endTick - startTick)/ticksBySec < 4);   /* 4 seconds */

    /* GL clean-up */
    glDisable(GL_SCISSOR_TEST);

    /* Results */
    seconds = (endTick - startTick)/ticksBySec;
    pixelsPerSecond = draws * DrawWidth * DrawHeight / seconds;
    printf("Result: %d draws in %f seconds = %f pixels/sec\n",
	   draws, seconds, pixelsPerSecond);
    }

UGL_LOCAL void echoUse(void)
    {
    printf("Keys:\n");
    printf("       SPACE  Reset Parameters\n");
    printf("     Up/Down  Move image up/down\n");
    printf("  Left/Right  Move image left/right\n");
    printf("           x  Decrease X-axis PixelZoom\n");
    printf("           X  Increase X-axis PixelZoom\n");
    printf("           y  Decrease Y-axis PixelZoom\n");
    printf("           Y  Increase Y-axis PixelZoom\n");
    printf("           w  Decrease glDrawPixels width*\n");
    printf("           W  Increase glDrawPixels width*\n");
    printf("           h  Decrease glDrawPixels height*\n");
    printf("           H  Increase glDrawPixels height*\n");
    printf("           p  Decrease GL_UNPACK_SKIP_PIXELS*\n");
    printf("           P  Increase GL_UNPACK_SKIP_PIXELS*\n");
    printf("           r  Decrease GL_UNPACK_SKIP_ROWS*\n");
    printf("           R  Increase GL_UNPACK_SKIP_ROWS*\n");
    printf("           s  Toggle GL_SCISSOR_TEST\n");
    printf("           f  Toggle front/back buffer drawing\n");
    printf("           d  Toggle dithering\n");
    printf("           b  Benchmark test\n");
    printf("         ESC  Exit\n");
    printf("* Warning: no limits are imposed on these parameters so it's\n");
    printf("  possible to cause a segfault if you go too far.\n");
    }


UGL_LOCAL void readKey(UGL_WCHAR key)
    {
    switch (key)
	{
	case UGL_UNI_SPACE:
	    reset();
	    break;
	case 'd':
	    Dither = !Dither;
	    if (Dither)
		glEnable(GL_DITHER);
	    else
		glDisable(GL_DITHER);
	    break;
	case 'w':
	    if (DrawWidth > 0)
		DrawWidth--;
	    break;
	case 'W':
	    DrawWidth++;
	    break;
	case 'h':
	    if (DrawHeight > 0)
		DrawHeight--;
	    break;
	case 'H':
	    DrawHeight++;
	    break;
	case 'p':
	    if (SkipPixels > 0)
		SkipPixels--;
	    break;
	case 'P':
	    SkipPixels++;
	    break;
	case 'r':
	    if (SkipRows > 0)
		SkipRows--;
	    break;
	case 'R':
	    SkipRows++;
	    break;
	case 's':
	    Scissor = !Scissor;
	    break;
	case 'x':
	    Xzoom -= 0.1;
	    break;
	case 'X':
	    Xzoom += 0.1;
	    break;
	case 'y':
	    Yzoom -= 0.1;
	    break;
	case 'Y':
	    Yzoom += 0.1;
	    break;
	case 'b':
	    benchmark();
	    break;
	case 'f':
	    DrawFront = !DrawFront;
	    if (DrawFront)
		glDrawBuffer(GL_FRONT);
	    else
		glDrawBuffer(GL_BACK);
	    printf("glDrawBuffer(%s)\n", DrawFront ? "GL_FRONT" : "GL_BACK");
	    break;
	case UGL_UNI_UP_ARROW:
	    Ypos += 1;
	    break;
	case UGL_UNI_DOWN_ARROW:
	    Ypos -= 1;
	    break;
	case UGL_UNI_LEFT_ARROW:
	    Xpos -= 1;
	    break;
	case UGL_UNI_RIGHT_ARROW:
	    Xpos += 1;
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

UGL_LOCAL void cleanUp (void)
    {
    uglEventQDestroy (eventServiceId, qId);
    
    uglMesaDestroyContext();
    uglDeinitialize ();
    }

void windMLDrawPix (UGL_BOOL windMLMode);

void ugldrawpix (void)
    {
    taskSpawn ("tDrawPix", 210, VX_FP_TASK, 100000, (FUNCPTR)windMLDrawPix,
	       UGL_FALSE,1,2,3,4,5,6,7,8,9);
    }

void windMLDrawPix (UGL_BOOL windMLMode)
    {
    UGL_INPUT_DEVICE_ID keyboardDevId;
    GLuint ciMode;
    GLsizei width, height;

    Image = NULL;
    Scissor = GL_FALSE;
    DrawFront = GL_FALSE;
    Dither = GL_TRUE;

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
    
    uglMesaMakeCurrentContext(umc, 0, 0, UGL_MESA_FULLSCREEN_WIDTH,
			      UGL_MESA_FULLSCREEN_HEIGHT);
    
    uglMesaGetIntegerv(UGL_MESA_COLOR_INDEXED, &ciMode);
    uglMesaGetIntegerv(UGL_MESA_WIDTH, &width);
    uglMesaGetIntegerv(UGL_MESA_HEIGHT, &height);
    
    initGL(ciMode, width, height);

    echoUse();

    stopWex = UGL_FALSE;
    loopEvent();

    cleanUp();
    free(Image);
    
    return;
    }
