/* uglicotorus.c - WindML/Mesa example program */

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
modification history
--------------------
01a,jun01,sra
*/

#include <stdio.h>
#include <math.h>

#include <ugl/uglevent.h>
#include <ugl/uglinput.h>
#include <ugl/uglucode.h>

#include <GL/uglmesa.h>
#include <GL/glu.h>

/* Need GLUT_SHAPES */

#include <GL/uglglutshapes.h>

UGL_LOCAL UGL_EVENT_SERVICE_ID eventServiceId;
UGL_LOCAL UGL_EVENT_Q_ID qId;
UGL_LOCAL UGL_MESA_CONTEXT umc;
UGL_LOCAL volatile UGL_BOOL stopWex;

UGL_LOCAL GLfloat angle;
UGL_LOCAL GLboolean chaos_on;
UGL_LOCAL GLboolean color_on;

UGL_LOCAL GLuint theIco, theTorus, theSphere, theCube; 

UGL_LOCAL void initGL
    (
    int w,
    int h
    )
    {
    glViewport(0,0,(GLsizei)w,(GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0,(GLfloat)w/(GLfloat)h,1.0,60.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0,0.0,25.0,0.0,0.0,0.0,0.0,1.0,0.0);
    
    glClearColor(0.0,0.0,0.0,0.0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glEnable(GL_COLOR_MATERIAL);

    theIco = glGenLists(1);
    glNewList(theIco, GL_COMPILE);
    glutSolidIcosahedron();
    glEndList();

    theTorus = glGenLists(1);
    glNewList(theTorus, GL_COMPILE);
    glutSolidTorus(0.2,1.0,10,10);
    glEndList();

    theSphere = glGenLists(1);
    glNewList(theSphere, GL_COMPILE);
    glutSolidSphere(2.5,20,20);
    glEndList();

    theCube = glGenLists(1);
    glNewList(theCube, GL_COMPILE);
    glutSolidCube(4.0);
    glEndList();
    
    }

UGL_LOCAL void createIcoToruses
    (
    int i
    )
    {
    glPushMatrix();
    glRotatef(angle,1.0,1.0,1.0);
    glCallList(theIco);   

    switch (i)
	{
	case 9 :
	    glColor3f(1.0,0.0,0.0);
	    break;
	case 0 :
	    glColor3f(1.0,0.1,0.7);
	    break;
	case 1 :
	    glColor3f(1.0,0.0,1.0);
	    break;
	case 2 :
	    glColor3f(0.0,0.0,1.0);
	    break;
	case 3 :
	    glColor3f(0.0,0.5,1.0);
	    break;
	case 4 :
	    glColor3f(0.0,1.0,0.7);
	    break;
	case 5 :
	    glColor3f(0.0,1.0,0.0);
	    break;
	case 6 :
	    glColor3f(0.5,1.0,0.0);
	    break;
	case 7 :
	    glColor3f(1.0,1.0,0.0);
	    break;
	case 8 :
	    glColor3f(1.0,0.5,0.0);
	    break;
	}

    glRotatef(angle,1.0,1.0,1.0);
    glCallList(theTorus);   
    glRotatef(-2*angle,1.0,1.0,1.0);
    glCallList(theTorus);
    glPopMatrix();
    }

UGL_LOCAL void drawGL (void)
    {
    int i;

    if (color_on)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    else
	glClear(GL_DEPTH_BUFFER_BIT);
    
    glPushMatrix();

    if (chaos_on)	
	glRotatef(angle,1.0,1.0,1.0); 
    
    glPushMatrix();
    glRotatef(angle,1.0,1.0,1.0);
    glColor3f(1.0,0.5,0.0);
    glCallList(theSphere);
    glColor3f(1.0,0.0,0.0);
    glCallList(theCube);
    glPopMatrix(); 
        
    glRotatef(-angle,0.0,0.0,1.0);
    glPushMatrix();
    /* draw ten icosahedrons */
    for (i = 0; i < 10; i++)
	{
	glPushMatrix();
	glRotatef(36*i,0.0,0.0,1.0);
	glTranslatef(10.0,0.0,0.0);
	glRotatef(2*angle,0.0,1.0,0.0);
	glTranslatef(0.0,0.0,2.0);
	
	createIcoToruses(i); 
	glPopMatrix();
	}
    glPopMatrix();
    
    glPopMatrix();
    
    uglMesaSwapBuffers();

    angle += 1.0;

    }

UGL_LOCAL void echoUse(void)
    {
    printf("tIcoTorus keys:\n");
    printf("           c  Toggle color buffer clear\n");
    printf("       SPACE  Toggle chaos mode\n");
    printf("         ESC  Exit\n");
    }

UGL_LOCAL void readKey (UGL_WCHAR key)
    {
    
    switch(key)
	{
	case 'c':
	    color_on = !color_on;
	    break;    
	case UGL_UNI_SPACE:
	    chaos_on = !chaos_on;
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

void windMLIcoTorus (UGL_BOOL windMLMode);

void uglicotorus (void)
    {
    taskSpawn ("tIcoTorus", 210, VX_FP_TASK, 100000, (FUNCPTR)windMLIcoTorus,
	       UGL_FALSE,1,2,3,4,5,6,7,8,9);
    }

void windMLIcoTorus (UGL_BOOL windMLMode)
    {
    GLsizei width, height;
    UGL_INPUT_DEVICE_ID keyboardDevId;

    angle = 0.0;
    chaos_on = GL_TRUE;
    color_on = GL_TRUE;

    uglInitialize ();

    uglDriverFind (UGL_KEYBOARD_TYPE, 0,
		   (UGL_UINT32 *)&keyboardDevId);

    if (uglDriverFind (UGL_EVENT_SERVICE_TYPE, 0,
                       (UGL_UINT32 *)&eventServiceId) == UGL_STATUS_OK)
        {
        qId = uglEventQCreate (eventServiceId, 100);
        }
    else 
        {
        eventServiceId = UGL_NULL;
        }

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
    
    uglMesaMakeCurrentContext (umc, 0, 0, UGL_MESA_FULLSCREEN_WIDTH,
			       UGL_MESA_FULLSCREEN_HEIGHT);

    uglMesaGetIntegerv(UGL_MESA_WIDTH, &width);
    uglMesaGetIntegerv(UGL_MESA_HEIGHT, &height);
    
    initGL (width, height);

    echoUse();

    stopWex = UGL_FALSE;
    loopEvent();
        
    if (eventServiceId != UGL_NULL)
        uglEventQDestroy (eventServiceId, qId);

    uglMesaDestroyContext ();
    uglDeinitialize ();
    
    return;
    }

