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
 * Nov 20, 1995 use stdlib's rand()/srand() instead of random()/srand48(), etc.
 */

/*
 * Modified by Stephane Raimbault to be able to run in VxWorks 07/18/01
 *
 * Modified by Li Wei(liwei@aiar.xjtu.edu.cn) to be able to run in Windows
 * 6/13
 *
 * Modified by Brian Paul to compile with Windows OR Unix.  7/23/97
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ugl/ugl.h>
#include <ugl/uglevent.h>
#include <ugl/uglinput.h>
#include <ugl/uglucode.h>

#include <GL/uglmesa.h>
#include <GL/glu.h>

#ifndef RAND_MAX
#  define RAND_MAX 32767
#endif

#define XSIZE	100
#define YSIZE	75

#define RINGS 5
#define BLUERING 0
#define BLACKRING 1
#define REDRING 2
#define YELLOWRING 3
#define GREENRING 4

#define BACKGROUND 8

UGL_LOCAL UGL_EVENT_SERVICE_ID eventServiceId;
UGL_LOCAL UGL_EVENT_Q_ID qId;
UGL_LOCAL UGL_MESA_CONTEXT umc;
UGL_LOCAL volatile UGL_BOOL stopWex;

UGL_LOCAL int rgb;
UGL_LOCAL unsigned char rgb_colors[RINGS][3];
UGL_LOCAL int mapped_colors[RINGS];
UGL_LOCAL float dests[RINGS][3];
UGL_LOCAL float offsets[RINGS][3];
UGL_LOCAL float angs[RINGS];
UGL_LOCAL float rotAxis[RINGS][3];
UGL_LOCAL int iters[RINGS];
UGL_LOCAL GLuint theTorus;

enum {
    COLOR_BLACK = 0,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE
};

/*
UGL_LOCAL float RGBMap[9][3] = {
    {0, 0, 0},
    {1, 0, 0},
    {0, 1, 0},
    {1, 1, 0},
    {0, 0, 1},
    {1, 0, 1},
    {0, 1, 1},
    {1, 1, 1},
    {0.5, 0.5, 0.5}
};

UGL_LOCAL void SetColor(int c)
    {
    (rgb) ? glColor3fv(RGBMap[c]): glIndexf(c);
    }

UGL_LOCAL void InitMap(void)
    {
    int i;
    
    if (rgb)
	return;
    
    for (i = 0; i < 9; i++)
	uglMesaSetColor(i, RGBMap[i][0], RGBMap[i][1], RGBMap[i][2]);
    }

UGL_LOCAL void SetFogRamp(int density, int startIndex)
    {
    int fogValues, colorValues;
    int i, j, k;
    float intensity;
    
    fogValues = 1 << density;
    colorValues = 1 << startIndex;
    for (i = 0; i < colorValues; i++)
	{
	for (j = 0; j < fogValues; j++)
	    {
	    k = i * fogValues + j;
	    intensity = (i * fogValues + j * colorValues) / 255.0;
	    uglMesaSetColor(k, intensity, intensity, intensity);
	    }
	}
    }

UGL_LOCAL void SetGreyRamp(void)
    {
    int i;
    float intensity;

    for (i = 0; i < 255; i++)
	{
	intensity = i / 255.0;
	uglMesaSetColor(i, intensity, intensity, intensity);
	}
    }
*/

UGL_LOCAL void FillTorus(float rc, int numc, float rt, int numt)
    {
    int i, j, k;
    double s, t;
    double x, y, z;
    double pi, twopi;

    pi = 3.14159265358979323846;
    twopi = 2 * pi;

    for (i = 0; i < numc; i++)
	{
	glBegin(GL_QUAD_STRIP);
        for (j = 0; j <= numt; j++)
	    {
	    for (k = 1; k >= 0; k--)
		{
		s = (i + k) % numc + 0.5;
		t = j % numt;
		
		x = cos(t*twopi/numt) * cos(s*twopi/numc);
		y = sin(t*twopi/numt) * cos(s*twopi/numc);
		z = sin(s*twopi/numc);
		glNormal3f(x, y, z);
		
		x = (rt + rc * cos(s*twopi/numc)) * cos(t*twopi/numt);
		y = (rt + rc * cos(s*twopi/numc)) * sin(t*twopi/numt);
		z = rc * sin(s*twopi/numc);
		glVertex3f(x, y, z);
		}
	    }
	glEnd();
	}
    }

UGL_LOCAL float Clamp(int iters_left, float t)
    {
    if (iters_left < 3)
	{
	return 0.0;
	}
    return (iters_left-2)*t/iters_left;
    }

UGL_LOCAL void drawGL(void)
    {
    int i, j;

    for (i = 0; i < RINGS; i++)
	{
	if (iters[i]) {
	for (j = 0; j < 3; j++)
	    {
	    offsets[i][j] = Clamp(iters[i], offsets[i][j]);
	    }
	angs[i] = Clamp(iters[i], angs[i]);
	iters[i]--;
	}
	}

    glPushMatrix();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gluLookAt(0,0,10, 0,0,0, 0,1,0);

    for (i = 0; i < RINGS; i++)
	{
	if (rgb)
	    {
	    glColor3ubv(rgb_colors[i]);
	    }
	else
	    {
	    glIndexi(mapped_colors[i]);
	    }
	glPushMatrix();
	glTranslatef(dests[i][0]+offsets[i][0], dests[i][1]+offsets[i][1],
		     dests[i][2]+offsets[i][2]);
	glRotatef(angs[i], rotAxis[i][0], rotAxis[i][1], rotAxis[i][2]);
	glCallList(theTorus);
	glPopMatrix();
	}

    glPopMatrix();

    glFlush();

    uglMesaSwapBuffers();
    }

UGL_LOCAL float MyRand(void)
    {
    return 10.0 * ( (float) rand() / (float) RAND_MAX - 0.5 );
    }

UGL_LOCAL void ReInit(void)
    {
    int i;
    float deviation;

    deviation = MyRand() / 2;
    deviation = deviation * deviation;
    for (i = 0; i < RINGS; i++)
	{
	offsets[i][0] = MyRand();
	offsets[i][1] = MyRand();
	offsets[i][2] = MyRand();
	angs[i] = 260.0 * MyRand();
	rotAxis[i][0] = MyRand();
	rotAxis[i][1] = MyRand();
	rotAxis[i][2] = MyRand();
	iters[i] = (deviation * MyRand() + 60.0);
	}
    }

UGL_LOCAL void initGL(void)
    {
    float base, height;
    float aspect, x, y;
    int i;

    float top_y = 1.0;
    float bottom_y = 0.0;
    float top_z = 0.15;
    float bottom_z = 0.69;
    float spacing = 2.5;
    static float lmodel_ambient[] = {0.0, 0.0, 0.0, 0.0};
    static float lmodel_twoside[] = {GL_FALSE};
    static float lmodel_local[] = {GL_FALSE};
    static float light0_ambient[] = {0.1, 0.1, 0.1, 1.0};
    static float light0_diffuse[] = {1.0, 1.0, 1.0, 0.0};
    static float light0_position[] = {0.8660254, 0.5, 1, 0};
    static float light0_specular[] = {1.0, 1.0, 1.0, 0.0};
    static float bevel_mat_ambient[] = {0.0, 0.0, 0.0, 1.0};
    static float bevel_mat_shininess[] = {40.0};
    static float bevel_mat_specular[] = {1.0, 1.0, 1.0, 0.0};
    static float bevel_mat_diffuse[] = {1.0, 0.0, 0.0, 0.0};

    ReInit();

    for (i = 0; i < RINGS; i++)
	{
	rgb_colors[i][0] = rgb_colors[i][1] = rgb_colors[i][2] = 0;
	}
    rgb_colors[BLUERING][2] = 255;
    rgb_colors[REDRING][0] = 255;
    rgb_colors[GREENRING][1] = 255;
    rgb_colors[YELLOWRING][0] = 255;
    rgb_colors[YELLOWRING][1] = 255;
    mapped_colors[BLUERING] = COLOR_BLUE;
    mapped_colors[REDRING] = COLOR_RED;
    mapped_colors[GREENRING] = COLOR_GREEN;
    mapped_colors[YELLOWRING] = COLOR_YELLOW;
    mapped_colors[BLACKRING] = COLOR_BLACK;

    dests[BLUERING][0] = -spacing;
    dests[BLUERING][1] = top_y;
    dests[BLUERING][2] = top_z;

    dests[BLACKRING][0] = 0.0;
    dests[BLACKRING][1] = top_y;
    dests[BLACKRING][2] = top_z;

    dests[REDRING][0] = spacing;
    dests[REDRING][1] = top_y;
    dests[REDRING][2] = top_z;

    dests[YELLOWRING][0] = -spacing / 2.0;
    dests[YELLOWRING][1] = bottom_y;
    dests[YELLOWRING][2] = bottom_z;

    dests[GREENRING][0] = spacing / 2.0;
    dests[GREENRING][1] = bottom_y;
    dests[GREENRING][2] = bottom_z;

    base = 2.0;
    height = 2.0;
    theTorus = glGenLists(1);
    glNewList(theTorus, GL_COMPILE);
    FillTorus(0.1, 8, 1.0, 25);
    glEndList();

    x = (float)XSIZE;
    y = (float)YSIZE;
    aspect = x / y;
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glClearDepth(1.0);

    if (rgb)
	{
	glClearColor(0.5, 0.5, 0.5, 0.0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
	glEnable(GL_LIGHT0);

	glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_local);
	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glEnable(GL_LIGHTING);

	glMaterialfv(GL_FRONT, GL_AMBIENT, bevel_mat_ambient);
	glMaterialfv(GL_FRONT, GL_SHININESS, bevel_mat_shininess);
	glMaterialfv(GL_FRONT, GL_SPECULAR, bevel_mat_specular);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, bevel_mat_diffuse);

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glShadeModel(GL_SMOOTH);
	}
    else
	{
	glClearIndex(BACKGROUND);
	glShadeModel(GL_FLAT);
	}

    glMatrixMode(GL_PROJECTION);
    gluPerspective(45, 1.33, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
    }

UGL_LOCAL void echoUse(void)
    {
    printf("tOlympic keys:\n");
    printf("       SPACE  Reinitialize\n");
    printf("         ESC  Exit\n");
    }

UGL_LOCAL void readKey (UGL_WCHAR key)
    {
    switch(key)
	{
	case UGL_UNI_SPACE:
	    ReInit();
	    break;
	case UGL_UNI_ESCAPE:
	    stopWex = 1;
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

void windMLOlympic (UGL_BOOL windMLMode);

void uglolympic (void)
    {
    taskSpawn("tOlympic", 210, VX_FP_TASK, 100000, (FUNCPTR)windMLOlympic,
              0,1,2,3,4,5,6,7,8,9);
    }

void windMLOlympic(UGL_BOOL windMLMode)
    {
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

    uglMesaMakeCurrentContext(umc, 0, 0, UGL_MESA_FULLSCREEN_WIDTH,
			      UGL_MESA_FULLSCREEN_HEIGHT);

    uglMesaGetIntegerv(UGL_MESA_RGB, &rgb);
    
    initGL();

    echoUse();

    stopWex = 0;
    loopEvent();
    
    if (eventServiceId != UGL_NULL)
	uglEventQDestroy (eventServiceId, qId);
    
    uglMesaDestroyContext();
    uglDeinitialize();
    
    return;
    }
