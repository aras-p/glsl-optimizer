
/* Copyright (c) Mark J. Kilgard, 1994. */

/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
/*  stencil.c
 *  This program draws two rotated tori in a window.
 *  A diamond in the center of the window masks out part
 *  of the scene.  Within this mask, a different model
 *  (a sphere) is drawn in a different color.
 */

/*
 * Conversion to UGL/Mesa by Stephane Raimbault, 2001
 */

#include <stdio.h>
#include <math.h>

#include <ugl/ugl.h>
#include <ugl/uglevent.h>
#include <ugl/uglinput.h>

#include <GL/uglmesa.h>
#include <GL/glu.h>
#include <GL/uglglutshapes.h>

#define YELLOWMAT   1
#define BLUEMAT 2

UGL_LOCAL UGL_EVENT_SERVICE_ID eventServiceId;
UGL_LOCAL UGL_EVENT_Q_ID qId;
UGL_LOCAL UGL_MESA_CONTEXT umc;

UGL_LOCAL void initGL (GLsizei w, GLsizei h)
    {
    GLfloat yellow_diffuse[] = { 0.7, 0.7, 0.0, 1.0 };
    GLfloat yellow_specular[] = { 1.0, 1.0, 1.0, 1.0 };

    GLfloat blue_diffuse[] = { 0.1, 0.1, 0.7, 1.0 };
    GLfloat blue_specular[] = { 0.1, 1.0, 1.0, 1.0 };

    GLfloat position_one[] = { 1.0, 1.0, 1.0, 0.0 };

    glNewList(YELLOWMAT, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, yellow_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, yellow_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, 64.0);
    glEndList();

    glNewList(BLUEMAT, GL_COMPILE);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, blue_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, blue_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, 45.0);
    glEndList();

    glLightfv(GL_LIGHT0, GL_POSITION, position_one);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

    glClearStencil(0x0);
    glEnable(GL_STENCIL_TEST);

    glClear(GL_STENCIL_BUFFER_BIT);

/* create a diamond shaped stencil area */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-3.0, 3.0, -3.0, 3.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glStencilFunc (GL_ALWAYS, 0x1, 0x1);
    glStencilOp (GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glBegin(GL_QUADS);
	glVertex3f (-1.0, 0.0, 0.0);
	glVertex3f (0.0, 1.0, 0.0);
	glVertex3f (1.0, 0.0, 0.0);
	glVertex3f (0.0, -1.0, 0.0);
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (GLfloat) w/(GLfloat) h, 3.0, 7.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -5.0);
    }

/*  Draw a sphere in a diamond-shaped section in the
 *  middle of a window with 2 tori.
 */
UGL_LOCAL void drawGL(void)
    {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glStencilOp (GL_KEEP, GL_KEEP, GL_KEEP);

/* draw blue sphere where the stencil is 1 */
    glStencilFunc (GL_EQUAL, 0x1, 0x1);
    glCallList (BLUEMAT);
    glutSolidSphere (0.5, 15, 15);
 
/* draw the tori where the stencil is not 1 */
    glStencilFunc (GL_NOTEQUAL, 0x1, 0x1);
    glPushMatrix();
       glRotatef (45.0, 0.0, 0.0, 1.0);
       glRotatef (45.0, 0.0, 1.0, 0.0);
       glCallList (YELLOWMAT);
       glutSolidTorus (0.275, 0.85, 15, 15);
       glPushMatrix();
          glRotatef (90.0, 1.0, 0.0, 0.0);
	  glutSolidTorus (0.275, 0.85, 15, 15);
       glPopMatrix();
    glPopMatrix();

    glFlush();

    uglMesaSwapBuffers();
    }

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

void windMLStencil (UGL_BOOL windMLMode);

void uglstencil (void)
    {
    taskSpawn("tStencil", 210, VX_FP_TASK, 100000,
	      (FUNCPTR)windMLStencil,UGL_FALSE,1,2,3,4,5,6,7,8,9);
    }

void windMLStencil(UGL_BOOL windMLMode)
    {
    UGL_INPUT_DEVICE_ID keyboardDevId;
    GLsizei width, height;
    
    uglInitialize();
    
    uglDriverFind (UGL_KEYBOARD_TYPE, 0, (UGL_UINT32 *)&keyboardDevId);
    
    uglDriverFind (UGL_EVENT_SERVICE_TYPE, 0, (UGL_UINT32 *)&eventServiceId);
	    
    qId = uglEventQCreate (eventServiceId, 100);

    if (windMLMode)
       umc = uglMesaCreateNewContextExt(UGL_MESA_SINGLE
					| UGL_MESA_WINDML_EXCLUSIVE,
					16,
					8,
					0,0,0,0,
					NULL);
    else
       umc = uglMesaCreateNewContextExt(UGL_MESA_SINGLE,
					16,
					8,
					0,0,0,0,
					NULL);

    if (umc == NULL)
	{
	uglDeinitialize();
	return;
	}

    /* Fullscreen */

    uglMesaMakeCurrentContext(umc, 0, 0, UGL_MESA_FULLSCREEN_WIDTH,
			      UGL_MESA_FULLSCREEN_HEIGHT);
    
    uglMesaGetIntegerv(UGL_MESA_WIDTH, &width);
    uglMesaGetIntegerv(UGL_MESA_HEIGHT, &height);
    
    initGL(width, height);

    drawGL();
	    
    while (!getEvent());
	    
    uglEventQDestroy (eventServiceId, qId);
    
    uglMesaDestroyContext();
    uglDeinitialize();
    
    return;
    }

