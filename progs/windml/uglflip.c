
/* uglflip.c - WindML/Mesa example program */

/* Copyright (C) 2001 by Wind River Systems, Inc */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * The MIT License
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 * Stephane Raimbault <stephane.raimbault@windriver.com> 
 */

/*
DESCRIPTION
Draw a triangle and flip the screen
*/

#include <stdio.h>
#include <math.h>

#include <ugl/ugl.h>
#include <ugl/uglucode.h>
#include <ugl/uglevent.h>
#include <ugl/uglinput.h>

#include <GL/uglmesa.h>
#include <GL/glu.h>

#define BLACK     (0)
#define RED       (1)
#define GREEN     (2)
#define BLUE      (3)
#define CI_OFFSET  4

UGL_LOCAL GLuint rgb;
UGL_LOCAL UGL_EVENT_SERVICE_ID eventServiceId;
UGL_LOCAL UGL_EVENT_Q_ID qId;
UGL_LOCAL volatile UGL_BOOL stopWex;

UGL_LOCAL UGL_MESA_CONTEXT umc;

UGL_LOCAL void initGL (void)
    {
    uglMesaSetColor(BLACK, 0.0, 0.0, 0.0);
    uglMesaSetColor(RED, 1.0, 0.3, 0.3);
    uglMesaSetColor(GREEN, 0.3, 1.0, 0.3);
    uglMesaSetColor(BLUE, 0.3, 0.3, 1.0);
    
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearIndex(BLACK);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    }

UGL_LOCAL void drawGL (void)
    {
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBegin(GL_TRIANGLES);
        (rgb) ? glColor3f(1.0, 0.3, 0.3) : glIndexi(RED);
	glVertex2f(0.75, -0.50);
	(rgb) ? glColor3f(0.3, 1.0, 0.3) : glIndexi(GREEN);
	glVertex2f(0.0, 0.75);
	(rgb) ? glColor3f(0.3, 0.3, 1.0) : glIndexi(BLUE);
	glVertex2f(-0.75, -0.50);
    glEnd();
    
    glBegin(GL_LINES);
        (rgb) ? glColor3f(1.0, 0.3, 0.3) : glIndexi(RED);
	glVertex2f(-1.0, 1.0);
	(rgb) ? glColor3f(0.3, 0.3, 1.0) : glIndexi(BLUE);
	glVertex2f(1.0, -1.0);
    glEnd();

    glFlush();

    uglMesaSwapBuffers();
    }    

UGL_LOCAL void echoUse(void)
    {
    printf("tFlip keys:\n");
    printf("             d  Toggle dithering\n");
    printf("            up  Reduce the window\n");
    printf("          down  Enlarge the window\n");
    printf("       page up  Y==0 is the bottom line and increases upward\n");
    printf("     page down  Y==0 is the bottom line and increases downward\n");
    printf("           ESC  Exit\n");
    }

UGL_LOCAL void readKey (UGL_WCHAR key)
    {
    
    switch(key)
	{
	case UGL_UNI_UP_ARROW:
	    uglMesaResizeWindow(8, 8);
	    break;
	case UGL_UNI_DOWN_ARROW:
	    glDrawBuffer(GL_FRONT_LEFT);
	    glClear(GL_COLOR_BUFFER_BIT);
	    glDrawBuffer(GL_BACK_LEFT);
	    uglMesaResizeWindow(-8, -8);
	    break;
	case UGL_UNI_PAGE_UP:
	    uglMesaPixelStore(UGL_MESA_Y_UP, GL_TRUE);
	    break;
	case UGL_UNI_PAGE_DOWN:
	    uglMesaPixelStore(UGL_MESA_Y_UP, GL_FALSE);
	    break;
	case UGL_UNI_ESCAPE:
	    stopWex = UGL_TRUE;
	    break;
	case 'd':
	    if (glIsEnabled(GL_DITHER))
		glDisable(GL_DITHER);
	    else
		glEnable(GL_DITHER);
	    break;
	}
    }

UGL_LOCAL void loopEvent(void)
    {
    UGL_EVENT event;
    UGL_INPUT_EVENT * pInputEvent;
 
    drawGL();

    UGL_FOREVER
	{
	if (uglEventGet (qId, &event, sizeof (event), UGL_WAIT_FOREVER)
	    != UGL_STATUS_Q_EMPTY)
            {
	    pInputEvent = (UGL_INPUT_EVENT *)&event;
	    
	    if (pInputEvent->header.type == UGL_EVENT_TYPE_KEYBOARD &&
		pInputEvent->modifiers & UGL_KEYBOARD_KEYDOWN)
		{
		readKey(pInputEvent->type.keyboard.key);
		drawGL();
		}
	    }

	if (stopWex)
            break;
	}
    }

void windMLFlip (UGL_BOOL windMLMode);

void uglflip (void)
    {
    taskSpawn ("tFlip", 210, VX_FP_TASK, 100000, (FUNCPTR)windMLFlip,
	       UGL_FALSE,1,2,3,4,5,6,7,8,9);
    }

void windMLFlip (UGL_BOOL windMLMode)
    {

    UGL_INPUT_DEVICE_ID keyboardDevId;

    uglInitialize();

    uglDriverFind (UGL_KEYBOARD_TYPE, 0, (UGL_UINT32 *)&keyboardDevId);

    uglDriverFind (UGL_EVENT_SERVICE_TYPE, 0, (UGL_UINT32 *)&eventServiceId);

    qId = uglEventQCreate (eventServiceId, 100);

    if (windMLMode)
       umc = uglMesaCreateNewContext(UGL_MESA_SINGLE
				     | UGL_MESA_WINDML_EXCLUSIVE, NULL);
    else
       umc = uglMesaCreateNewContext(UGL_MESA_DOUBLE_SOFTWARE, NULL);

    if (umc == NULL)
        {
	uglDeinitialize();
	return;
        }

    uglMesaMakeCurrentContext(umc, 0, 0, UGL_MESA_FULLSCREEN_WIDTH,
			      UGL_MESA_FULLSCREEN_HEIGHT);
    
    uglMesaGetIntegerv(UGL_MESA_RGB, &rgb);
    
    initGL();

    echoUse();
    stopWex = UGL_FALSE;	    
    loopEvent();
    
    uglEventQDestroy (eventServiceId, qId);

    uglMesaDestroyContext();
    uglDeinitialize();

    return;
    }
