/* uglcube.c - WindML/Mesa example program */

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
DESCRIPTION
Draw a smooth cube.
*/

#include <stdio.h>
#include <math.h>

#include <ugl/uglevent.h>
#include <ugl/uglinput.h>

#include <GL/uglmesa.h>
#include <GL/glu.h>

UGL_LOCAL UGL_EVENT_SERVICE_ID eventServiceId;
UGL_LOCAL UGL_EVENT_Q_ID qId;
UGL_LOCAL UGL_MESA_CONTEXT umc;
UGL_LOCAL GLfloat rotx;
UGL_LOCAL GLfloat roty;
UGL_LOCAL GLuint theCube;

UGL_LOCAL void cube()
    {

    /* Front */ 
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    glColor3f(1.0f, 1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glEnd();
    
	    
    /* Back */ 
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glColor3f(1.0f, 1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glEnd();
    
    
    /* Right */ 
    glBegin(GL_QUADS);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    glEnd();
    
    /* Left */ 
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glColor3f(1.0f, 1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glColor3f(1.0f, 1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glEnd();
    
    /* Top */ 
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glEnd();
    
	    
    /* Bottom */ 
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    glColor3f(1.0f, 1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glEnd();	
    }

UGL_LOCAL void initGL
    (
    int Width,
    int Height
    ) 
    {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    theCube = glGenLists(1);
    glNewList(theCube, GL_COMPILE);
    cube();
    glEndList();
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat) Width / (GLfloat) Height, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -6.0f);
    } 


/* The main drawing function. */ 

UGL_LOCAL void drawGL() 
    {

    /* Clear The Screen And The Depth Buffer */

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Rotate the cube */
    
    glRotatef(rotx, 1.0f, 0.0f, 0.0f);
    glRotatef(roty, 0.0f, 1.0f, 0.0f);
    
    glCallList(theCube);    

    glFlush();

    uglMesaSwapBuffers();
    }


/* The function called whenever a key is pressed. */ 

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

void windMLCube (UGL_BOOL windMLMode);

void uglcube (void)
    {
    taskSpawn("tCube", 210, VX_FP_TASK, 100000, (FUNCPTR)windMLCube,
              UGL_FALSE,1,2,3,4,5,6,7,8,9);
    }

void windMLCube (UGL_BOOL windMLMode)
    {    
    GLsizei width, height;
    UGL_INPUT_DEVICE_ID keyboardDevId;

    rotx = 2.5f;
    roty = 1.0f;
    
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

    if (windMLMode)
       umc = uglMesaCreateNewContext(UGL_MESA_DOUBLE
				     | UGL_MESA_WINDML_EXCLUSIVE, NULL);
    else
       umc = uglMesaCreateNewContext(UGL_MESA_DOUBLE, NULL);

    /* Fullscreen */

    uglMesaMakeCurrentContext(umc, 0, 0, UGL_MESA_FULLSCREEN_WIDTH,
			      UGL_MESA_FULLSCREEN_HEIGHT);
    
    uglMesaGetIntegerv(UGL_MESA_WIDTH, &width);
    uglMesaGetIntegerv(UGL_MESA_HEIGHT, &height);

    /* Initialize our window. */ 

    initGL(width, height);

    while (!getEvent())
	drawGL();
    
    if (eventServiceId != UGL_NULL)
	uglEventQDestroy (eventServiceId, qId);
    
    uglMesaDestroyContext();
    uglDeinitialize();

    return;
    }
