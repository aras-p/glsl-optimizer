
/* ugltexcube.c - WindML/Mesa example program */

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
Draw a textured cube
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <ugl/ugl.h>
#include <ugl/uglevent.h>
#include <ugl/uglinput.h>
#include <GL/uglmesa.h>
#include <GL/glu.h>

#include "../util/readtex.h"

#define IMAGE_FILE "Mesa/images/wrs_logo.rgb"

UGL_LOCAL UGL_EVENT_SERVICE_ID eventServiceId;
UGL_LOCAL UGL_EVENT_Q_ID qId;
UGL_LOCAL UGL_MESA_CONTEXT umc;

UGL_LOCAL GLfloat xrot, yrot, zrot;
UGL_LOCAL GLuint texture[1];
UGL_LOCAL GLuint theTexCube;

typedef struct {
   GLubyte *data;
   int width, height;
   GLenum format;
   } TEX_IMAGE;

UGL_LOCAL void cleanUp (void);

UGL_LOCAL void loadGLTexture()
    {
    TEX_IMAGE * texImage=NULL;

    texImage = (TEX_IMAGE *) malloc(sizeof(TEX_IMAGE));

    if (texImage == NULL)
	{
	printf("Error allocating space for image");
	cleanUp();
	exit(1);
	}
    
    texImage->data = LoadRGBImage(IMAGE_FILE, &texImage->width,
				  &texImage->height, &texImage->format);
    if (!texImage->data)
	{
	printf("Couldn't read %s\n", IMAGE_FILE);
	free(texImage);
	cleanUp();
	exit(1);
	}
	
    /* Create Texture */
    glGenTextures(1, &texture[0]);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, 3,
		 texImage->width, texImage->height,
		 0, GL_RGB, GL_UNSIGNED_BYTE, texImage->data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    free(texImage->data);
    free(texImage);
    }

UGL_LOCAL void initGL(int width, int height)
    {

    /* Load the texture(s) */
    loadGLTexture();

    /* Enable texture mapping */
    glEnable(GL_TEXTURE_2D);

    /* Clear the background color to black */
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glEnable(GL_CULL_FACE);
    
    /* Enables smooth color shading */
    glShadeModel(GL_SMOOTH);

/* glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); */	 
/* glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST); */
 
    theTexCube = glGenLists(1);
    glNewList(theTexCube, GL_COMPILE);

    /* Choose the texture to use */
    glBindTexture(GL_TEXTURE_2D, texture[0]);

    /* Begin drawing a cube */
    glBegin(GL_QUADS);
        
    /* Front face (note that the texture's corners have to match the
       quad's corners) */

    /* Bottom left of the texture and quad */
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);

    /* Bottom Right Of The Texture and Quad */
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);

    /* Top Right Of The Texture and Quad */
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    /* Top Left Of The Texture and Quad	*/
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    
    /* Back Face */

    /* Bottom Right Of The Texture and Quad */
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);

    /* Top Right Of The Texture and Quad */
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);

    /* Top Left Of The Texture and Quad */
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);

    /* Bottom Left Of The Texture and Quad */
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);

    
    /* Top Face */

    /* Top Left Of The Texture and Quad */
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);

    /* Bottom Left Of The Texture and Quad */
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);

    /* Bottom Right Of The Texture and Quad */
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);

    /* Top Right Of The Texture and Quad */
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);
    
    /* Bottom Face */

    /* Top Right Of The Texture and Quad */
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);

    /* Top Left Of The Texture and Quad */
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);

    /* Bottom Left Of The Texture and Quad */
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);

    /* Bottom Right Of The Texture and Quad */
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);
	
	
    /* Right face */
    /* Bottom Right Of The Texture and Quad */
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);

    /* Top Right Of The Texture and Quad */
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, -1.0f);

    /* Top Left Of The Texture and Quad */
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);

    /* Bottom Left Of The Texture and Quad */
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, 1.0f);
    
	
    /* Left Face */
    /* Bottom Left Of The Texture and Quad */
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);

    /* Bottom Right Of The Texture and Quad */
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, 1.0f);

    /* Top Right Of The Texture and Quad */
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);

    /* Top Left Of The Texture and Quad */
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, -1.0f);
						   
    glEnd();		/* done with the polygon */
    glEndList();

    glDisable(GL_DITHER);
    glMatrixMode(GL_PROJECTION);
    /* Reset the projection matrix */
    glLoadIdentity();
    /* Calculate the aspect ratio of the window */
    gluPerspective(45.0f, (GLfloat) width / (GLfloat) height, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    }

UGL_LOCAL void drawGL() 
    {
    glClear(GL_COLOR_BUFFER_BIT);

    /* Reset The View */
    glPushMatrix();
    
    /* Move 8 units into the screen */
    glTranslatef(0.0f, 0.0f, -8.0f);

    /* Rotate on the X axis */
    glRotatef(xrot, 1.0f, 0.0f, 0.0f);
    
    /* Rotate on the Y axis */
    glRotatef(yrot, 0.0f, 1.0f, 0.0f);

    /* Rotate On The Z Axis */
    glRotatef(zrot, 0.0f, 0.0f, 1.0f);

    glCallList(theTexCube);
    
    glFlush();
        
    uglMesaSwapBuffers();

    glPopMatrix();

    xrot += 1.6f;
    yrot += 1.6f;
    zrot += 1.6f;
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

UGL_LOCAL void cleanUp (void)
    {
    if (eventServiceId != UGL_NULL)
	uglEventQDestroy (eventServiceId, qId);

    uglMesaDestroyContext();
    uglDeinitialize();
    }

void windMLTexCube (UGL_BOOL windMLMode);

void ugltexcube (void)
    {
    taskSpawn("tTexCube", 210, VX_FP_TASK, 100000, (FUNCPTR)windMLTexCube,
	      UGL_FALSE,1,2,3,4,5,6,7,8,9);
    }


void windMLTexCube(UGL_BOOL windMLMode)
    {
    GLuint width, height;
    UGL_INPUT_DEVICE_ID keyboardDevId;
    
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

    if (umc == NULL)
        {
	uglDeinitialize();
	return;
        }

    uglMesaMakeCurrentContext(umc, 0, 0,
			      UGL_MESA_FULLSCREEN_WIDTH,
			      UGL_MESA_FULLSCREEN_HEIGHT);
			      

    uglMesaGetIntegerv(UGL_MESA_WIDTH, &width);
    uglMesaGetIntegerv(UGL_MESA_HEIGHT, &height);

    initGL(width, height);

    while(!getEvent())
	drawGL();
        
    cleanUp();

    return;
    }


