
/* uglpoint.c - WindML/Mesa example program */

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
Draw a single point.
*/

#include <stdio.h>
#include <math.h>

#include <ugl/ugl.h>
#include <ugl/uglevent.h>
#include <ugl/uglinput.h>

#include <GL/uglmesa.h>

#define DOUBLE_BUFFER GL_TRUE

enum {
    BLACK = 0,
    RED,
    GREEN,
    BLUE,
    WHITE
};

UGL_LOCAL GLuint rgb;
UGL_LOCAL UGL_EVENT_SERVICE_ID eventServiceId;
UGL_LOCAL UGL_EVENT_Q_ID qId;
UGL_LOCAL UGL_MESA_CONTEXT umc;
UGL_LOCAL GLint angleT;

UGL_LOCAL void initGL (void)
    {
    /* By passed in RGB mode */
    uglMesaSetColor(BLACK, 0.0, 0.0, 0.0);
    uglMesaSetColor(RED, 1.0, 0.0, 0.0);
    uglMesaSetColor(GREEN, 0.0, 1.0, 0.0);
    uglMesaSetColor(BLUE, 0.0, 0.0, 1.0);
    uglMesaSetColor(WHITE, 1.0, 1.0, 1.0);
    
    glOrtho(0.0, 1.0, 0.0, 1.0, -20.0, 20.0);
    
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearIndex(BLACK);
    }

UGL_LOCAL void drawGL (void)
    {
    GLint i;
    GLfloat x, y;

    /* Avoid blinking in single buffer */
    
    if (DOUBLE_BUFFER)
	glClear(GL_COLOR_BUFFER_BIT);    

    /* Random points */

    glBegin(GL_POINTS);
    (rgb) ? glColor3f(1.0, 0.0, 0.0): glIndexi(RED);

    for (i=0; i<150; i++)
	{
	x = rand() / (RAND_MAX+1.0);
        y = rand() / (RAND_MAX+1.0);
	glVertex2f(x, y);
	}

    (rgb) ? glColor3f(0.0, 1.0, 0.0): glIndexi(GREEN);

    for (i=0; i<150; i++)
	{
	x = (rand() / (RAND_MAX+1.0));
        y = (rand() / (RAND_MAX+1.0));
	glVertex2f(x, y);
	}

    (rgb) ? glColor3f(0.0, 0.0, 1.0): glIndexi(BLUE);
    glVertex2f(0.5,0.5);
    
    for (i=0; i<150; i++)
	{
	x = rand() / (RAND_MAX+1.0);
        y = rand() / (RAND_MAX+1.0);
	glVertex2f(x, y);
	}

    glEnd();

    /* Smooth triangle */

    glPushMatrix();
    glTranslatef(0.5, 0.5, 0);    
    glRotatef(angleT, 1.0, -1.0, 0.0);
    angleT = angleT++ % 360;
    glBegin(GL_TRIANGLES);
    (rgb) ? glColor3f(1.0, 0.0, 0.0): glIndexi(RED);  
    glVertex2f(0.75, 0.25);
    (rgb) ? glColor3f(0.0, 1.0, 0.0): glIndexi(GREEN);  
    glVertex2f(0.75, 0.75);
    (rgb) ? glColor3f(0.0, 0.0, 1.0): glIndexi(BLUE);  
    glVertex2f(0.25, 0.75);
    glEnd();
    glPopMatrix();

    /* Flush and swap */

    glFlush();
    
    uglMesaSwapBuffers();
    }

/************************************************************************
*
* getEvent
*
* RETURNS: true or false
*
* NOMANUAL
*
*/

UGL_LOCAL int getEvent(void)
    {
    UGL_EVENT event;
    UGL_STATUS status;
    int retVal = 0;

    status = uglEventGet (qId, &event, sizeof (event), UGL_NO_WAIT);

    while (status != UGL_STATUS_Q_EMPTY)
        {
	UGL_INPUT_EVENT * pInputEvent = (UGL_INPUT_EVENT *)&event;
	
	if (pInputEvent->modifiers & UGL_KEYBOARD_KEYDOWN)
	    retVal = 1;

	status = uglEventGet (qId, &event, sizeof (event), UGL_NO_WAIT);
        }
 
    return(retVal);
    }

void windMLPoint (UGL_BOOL windMLMode);

void uglpoint (void)
    {
    taskSpawn ("tPoint", 210, VX_FP_TASK, 100000,
	       (FUNCPTR)windMLPoint, UGL_FALSE,1,2,3,4,5,6,7,8,9);
    }

void windMLPoint (UGL_BOOL windMLMode)
    {
    GLubyte pPixels[4];
    GLsizei width, height;
    UGL_INPUT_DEVICE_ID keyboardDevId;
    
    angleT = 0;

    uglInitialize();
    
    uglDriverFind (UGL_KEYBOARD_TYPE, 0, (UGL_UINT32 *)&keyboardDevId);
    
    if (uglDriverFind (UGL_EVENT_SERVICE_TYPE, 0,
                       (UGL_UINT32 *)&eventServiceId) == UGL_STATUS_OK)
        {
        qId = uglEventQCreate (eventServiceId, 100);
        }
    else 
        {
        eventServiceId = UGL_NULL;
        }
    
    if (DOUBLE_BUFFER)
	{
	if (windMLMode)
	    umc = uglMesaCreateNewContext(UGL_MESA_DOUBLE
					  | UGL_MESA_WINDML_EXCLUSIVE, NULL);
	else
	    umc = uglMesaCreateNewContext(UGL_MESA_DOUBLE, NULL);
	}
    else
	{
	if (windMLMode)
	    umc = uglMesaCreateNewContext(UGL_MESA_SINGLE
					  | UGL_MESA_WINDML_EXCLUSIVE, NULL);
	else
	    umc = uglMesaCreateNewContext(UGL_MESA_SINGLE, NULL);
	}

    if (umc == NULL)
        {
	uglDeinitialize();
	return;
        }

    /* Fullscreen */

    uglMesaMakeCurrentContext(umc, 0, 0, UGL_MESA_FULLSCREEN_WIDTH,
			      UGL_MESA_FULLSCREEN_HEIGHT);
    
    /* RGB or CI ? */

    uglMesaGetIntegerv(UGL_MESA_RGB, &rgb);
    
    initGL();

    while (!getEvent())
	drawGL();
    
    uglMesaGetIntegerv(UGL_MESA_WIDTH, &width);
    uglMesaGetIntegerv(UGL_MESA_HEIGHT, &height);
    
    printf ("glReadPixel return ");
    if (rgb)
	{
	glReadPixels(width/2, height/2,
		     1, 1, GL_RGB,
		     GL_UNSIGNED_BYTE, pPixels);  
	glFlush();
	printf ("R:%i G:%i B:%i (RGB)", pPixels[0], pPixels[1], pPixels[2]);
	}
    else
	{
	glReadPixels(width/2, height/2,
		     1, 1, GL_COLOR_INDEX,
		     GL_UNSIGNED_BYTE, pPixels);  
	glFlush();
	if (pPixels[0] == BLUE)
	    printf ("BLUE (CI)");
	else
	    printf ("%i (CI))", pPixels[0]);
	}

    printf(" for %ix%i\n", width/2, height/2);
    
    if (eventServiceId != UGL_NULL)
	uglEventQDestroy (eventServiceId, qId);
    
    uglMesaDestroyContext();
    uglDeinitialize();
    
    return;
    }
