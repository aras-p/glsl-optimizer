
/* uglline.c - WindML/Mesa example program */

/*
 * Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the name of
 * Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF
 * ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

/*
modification history
--------------------
01a,jun01,sra  Ported to UGL/Mesa and modifications
*/

/*
DESCRIPTION
Draw circular lines
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
#define YELLOW    (1)
#define GREEN     (2)
#define BLUE      (3)
#define CI_OFFSET  4

UGL_LOCAL GLuint rgb;
UGL_LOCAL UGL_EVENT_SERVICE_ID eventServiceId;
UGL_LOCAL UGL_EVENT_Q_ID qId;
UGL_LOCAL volatile UGL_BOOL stopWex;
UGL_LOCAL UGL_MESA_CONTEXT umc;

UGL_LOCAL GLboolean mode1, mode2;
UGL_LOCAL GLint size;

UGL_LOCAL GLfloat pntA[3] = {
    -10.0, 0.0, 0.0
};
UGL_LOCAL GLfloat pntB[3] = {
    -5.0, 0.0, 0.0
};

UGL_LOCAL GLint angleA;

UGL_LOCAL void initGL (void)
    {    
    GLint i;

    uglMesaSetColor(BLACK, 0.0, 0.0, 0.0);
    uglMesaSetColor(YELLOW, 1.0, 1.0, 0.0);
    uglMesaSetColor(GREEN, 0.0, 1.0, 0.0);
    uglMesaSetColor(BLUE, 0.0, 0.0, 1.0);
    
    for (i = 0; i < 16; i++)
	{
	uglMesaSetColor(CI_OFFSET+i, i/15.0, i/15.0, 0.0);
	}
    
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearIndex(BLACK);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-10, 10, -10, 10, -10.0, 10.0);

    glMatrixMode(GL_MODELVIEW);
    
    glLineStipple(1, 0xF0E0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    mode1 = GL_FALSE;
    mode2 = GL_FALSE;
    size = 1;
    }

UGL_LOCAL void drawGL (void)
    {
    
    GLint ci, i;

    glClear(GL_COLOR_BUFFER_BIT);

    glLineWidth(size);

    if (mode1) {
	glEnable(GL_LINE_STIPPLE);
    } else {
	glDisable(GL_LINE_STIPPLE);
    }

    if (mode2) {
	ci = CI_OFFSET;
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
    } else {
	ci = YELLOW;
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
    }

    glPushMatrix();

    glRotatef(angleA, 1, 0, 1);
    angleA = angleA++ % 360;
   
    for (i = 0; i < 360; i += 5) {
	glRotatef(5.0, 0, 0, 1);

	glColor3f(1.0, 1.0, 0.0);
	glBegin(GL_LINE_STRIP);
	    glVertex3fv(pntA);
	    glVertex3fv(pntB);
	glEnd();

	glPointSize(1);
	
	glColor3f(0.0, 1.0, 0.0);
	glBegin(GL_POINTS);
	glVertex3fv(pntA);
	glVertex3fv(pntB);
	glEnd();
    }
    
    glPopMatrix();

    glFlush();

    uglMesaSwapBuffers();

    }    

UGL_LOCAL void echoUse(void)
    {
    printf("tLine keys:\n");
    printf("           b  Blending/antialiasing\n");
    printf("           n  Line stipple\n");
    printf("     Up/Down  Pixel size\n");
    printf("         ESC  Exit\n");
    }

UGL_LOCAL void readKey (UGL_WCHAR key)
    {
    switch(key)
	{
	case 'n':
	    mode1 = (mode1) ? GL_FALSE: GL_TRUE;
	    break;
	case 'b':
	    mode2 = (mode2) ? GL_FALSE: GL_TRUE;
	    break;
	case UGL_UNI_DOWN_ARROW:
	    if(size>0)
		size--;
	    break;
	case UGL_UNI_UP_ARROW:
	    size++;
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

void windMLLine (UGL_BOOL windMLMode);

void uglline (void)
    {
    taskSpawn("tLine", 210, VX_FP_TASK, 100000, (FUNCPTR)windMLLine,
              UGL_FALSE,1,2,3,4,5,6,7,8,9);
    }


void windMLLine(UGL_BOOL windMLMode)
    {

    UGL_INPUT_DEVICE_ID keyboardDevId;

    angleA = 0;
    
    uglInitialize();

    uglDriverFind (UGL_KEYBOARD_TYPE, 0, (UGL_UINT32 *)&keyboardDevId);

    uglDriverFind (UGL_EVENT_SERVICE_TYPE, 0, (UGL_UINT32 *)&eventServiceId);

    qId = uglEventQCreate (eventServiceId, 100);

    /* Double buffer */

    if (windMLMode)
       umc = uglMesaCreateNewContext(UGL_MESA_DOUBLE
				     | UGL_MESA_WINDML_EXCLUSIVE, NULL);
	else
	   umc = uglMesaCreateNewContext(UGL_MESA_DOUBLE, NULL);

    if (umc == NULL)
        {
	uglDeinitialize();
	return;
        }

    /* Fullscreen */

    uglMesaMakeCurrentContext(umc, 0, 0, UGL_MESA_FULLSCREEN_WIDTH,
			      UGL_MESA_FULLSCREEN_HEIGHT);

    uglMesaGetIntegerv(UGL_MESA_RGB, &rgb);
    
    initGL();

    echoUse();

    stopWex = UGL_FALSE;
    loopEvent();
        
    uglEventQDestroy(eventServiceId, qId);

    uglMesaDestroyContext();
    uglDeinitialize();

    return;
    }
