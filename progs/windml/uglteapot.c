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
 * Linux Magazine July 2001
 * Conversion to UGL/Mesa from GLUT by Stephane Raimbault, 2001
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <ugl/ugl.h>
#include <ugl/uglevent.h>
#include <ugl/uglinput.h>
#include <ugl/uglucode.h>

#include <GL/uglmesa.h>
#include <GL/glu.h>

/* Need GLUT_SHAPES */

#include <GL/uglglutshapes.h>

#ifndef PI
#define PI 3.14159265
#endif

UGL_LOCAL UGL_EVENT_SERVICE_ID eventServiceId;
UGL_LOCAL UGL_EVENT_Q_ID qId;
UGL_LOCAL UGL_MESA_CONTEXT umc;
UGL_LOCAL volatile UGL_BOOL stopWex;

UGL_LOCAL GLint angle;
UGL_LOCAL GLfloat Sin[360], Cos[360];
UGL_LOCAL GLfloat L0pos[]={0.0, 2.0, -1.0};
UGL_LOCAL GLfloat L0dif[]={0.3, 0.3, 0.8};
UGL_LOCAL GLfloat L1pos[]={2.0, 2.0, 2.0};
UGL_LOCAL GLfloat L1dif[]={0.5, 0.5, 0.5};
UGL_LOCAL GLfloat Mspec[3];
UGL_LOCAL GLfloat Mshiny;
UGL_LOCAL GLuint theTeapot;

UGL_LOCAL void calcTableCosSin()
{
	int i;
	for(i=0;i<360;i++) {
		Cos[i] = cos(((float)i)/180.0*PI);
		Sin[i] = sin(((float)i)/180.0*PI);
	}
}

UGL_LOCAL void initGL(void)
    {
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glColor3f(1.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    
    glShadeModel(GL_SMOOTH);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, L0dif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, L0dif);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, L1dif);
    glLightfv(GL_LIGHT1, GL_SPECULAR, L1dif);
    
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, Mspec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, Mshiny);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 1.0, 0.1, 10.0);
    glMatrixMode(GL_MODELVIEW);

    theTeapot = glGenLists(1);
    glNewList(theTeapot, GL_COMPILE);
    glutSolidTeapot(1.0);
    glEndList();

    }

UGL_LOCAL void drawGL()
    {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glLoadIdentity();
    
    gluLookAt(4.5*Cos[angle], 2.0,4.5*Sin[angle],0.0,0.0,0.0,0.0,
	      1.0,0.0);
    glLightfv(GL_LIGHT0, GL_POSITION, L0pos);
    glLightfv(GL_LIGHT1, GL_POSITION, L1pos);

    glCallList(theTeapot);

    glFlush();
    
    uglMesaSwapBuffers();
    }

UGL_LOCAL void echoUse(void)
    {
    printf("tTeapot keys:\n");
    printf("        Left  Counter clockwise rotation (y-axis)\n");
    printf("       Right  Clockwise rotation (y-axis)\n");
    printf("           j  Enable/disable Light0\n");
    printf("           k  Enable/disable Light1\n");
    printf("           m  Add specular\n");
    printf("           l  Remove specular\n");
    printf("           o  Add shininess\n");
    printf("           p  Remove shininess\n");
    printf("         ESC  Exit\n");
    }


UGL_LOCAL void readKey (UGL_WCHAR key)
    {
    switch(key)
	{
	case UGL_UNI_RIGHT_ARROW:
	    angle +=2;
	    if (angle>= 360)
		angle-=360;
	    break;
	case UGL_UNI_LEFT_ARROW:
	    angle -=2;
	    if (angle<0)
		angle+=360;
	    break;
	case 'j':
	    glIsEnabled(GL_LIGHT0) ?
		glDisable(GL_LIGHT0) : glEnable(GL_LIGHT0);
	    break;
	case 'k':
	    glIsEnabled(GL_LIGHT1) ?
		glDisable(GL_LIGHT1) : glEnable(GL_LIGHT1);
	    break;
	case 'm':
	    Mspec[0]+=0.1;
	    if(Mspec[0]>1)
		Mspec[0]=1;
	    Mspec[1]+=0.1;
	    if(Mspec[1]>1)
		Mspec[1]=1;
	    Mspec[2]+=0.1;
	    if(Mspec[2]>1)
		Mspec[2]=1;
	    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, Mspec);
	    break;
	case 'l':
	    Mspec[0]-=0.1;
	    if(Mspec[0]>1)
		Mspec[0]=1;
	    Mspec[1]-=0.1;
	    if(Mspec[1]>1)
		Mspec[1]=1;
	    Mspec[2]-=0.1;
	    if(Mspec[2]>1)
		Mspec[2]=1;
	    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, Mspec);
	    break;
	case 'o':
	    Mshiny -= 1;
	    if (Mshiny<0)
		Mshiny=0;
	    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, Mshiny);
	    break;
	case 'p':
	    Mshiny += 1;
	    if (Mshiny>128)
		Mshiny=128;
	    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, Mshiny);
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

void windMLTeapot (UGL_BOOL windMLMode);

void uglteapot (void)
    {
    taskSpawn ("tTeapot", 210, VX_FP_TASK, 100000, (FUNCPTR)windMLTeapot,
	       UGL_FALSE,1,2,3,4,5,6,7,8,9);
    }

void windMLTeapot (UGL_BOOL windMLMode)
    {
    UGL_INPUT_DEVICE_ID keyboardDevId;
    GLsizei displayWidth, displayHeight;
    GLsizei x, y, w, h;
    
    angle = 45;
    Mspec[0] = 0.5;
    Mspec[1] = 0.5;
    Mspec[2] = 0.5;
    Mshiny = 50;
    
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
    
    h = (displayHeight*2)/3;
    w = h;
    x = (displayWidth-w)/2;
    y = (displayHeight-h)/2;
    
    uglMesaMoveToWindow(x, y);
    uglMesaResizeToWindow(w, h);
    
    calcTableCosSin();
    
    initGL ();
    
    echoUse();

    stopWex = UGL_FALSE;
    loopEvent();
    
    uglEventQDestroy (eventServiceId, qId);
    
    uglMesaDestroyContext();
    uglDeinitialize ();

    return;
    }
