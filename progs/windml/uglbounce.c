/*
 * Bouncing ball demo.
 *
 * This program is in the public domain
 *
 * Brian Paul
 *
 * Conversion to GLUT by Mark J. Kilgard
 *
 * Conversion to UGL/Mesa by Stephane Raimbault
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <ugl/ugl.h>
#include <ugl/uglevent.h>
#include <ugl/uglinput.h>

#include <GL/uglmesa.h>

#define COS(X)   cos( (X) * 3.14159/180.0 )
#define SIN(X)   sin( (X) * 3.14159/180.0 )

#define RED 1
#define WHITE 2
#define CYAN 3

UGL_LOCAL UGL_EVENT_SERVICE_ID eventServiceId;
UGL_LOCAL UGL_EVENT_Q_ID qId;
UGL_LOCAL UGL_MESA_CONTEXT umc;

UGL_LOCAL GLuint Ball;
UGL_LOCAL GLfloat Zrot, Zstep;
UGL_LOCAL GLfloat Xpos, Ypos;
UGL_LOCAL GLfloat Xvel, Yvel;
UGL_LOCAL GLfloat Xmin, Xmax;
UGL_LOCAL GLfloat Ymin;
/* UGL_LOCAL GLfloat Ymax = 4.0; */
UGL_LOCAL GLfloat G;

UGL_LOCAL GLuint make_ball(void)
    {
    GLuint list;
    GLfloat a, b;
    GLfloat da = 18.0, db = 18.0;
    GLfloat radius = 1.0;
    GLuint color;
    GLfloat x, y, z;
    
    list = glGenLists(1);
    
    glNewList(list, GL_COMPILE);
    
    color = 0;
    for (a = -90.0; a + da <= 90.0; a += da)
	{
	glBegin(GL_QUAD_STRIP);
	for (b = 0.0; b <= 360.0; b += db)
	    {
	    if (color)
		{
		glIndexi(RED);
		glColor3f(1, 0, 0);
		}
	    else
		{
		glIndexi(WHITE);
		glColor3f(1, 1, 1);
		}

	    x = radius * COS(b) * COS(a);
	    y = radius * SIN(b) * COS(a);
	    z = radius * SIN(a);
	    glVertex3f(x, y, z);
	    
	    x = radius * COS(b) * COS(a + da);
	    y = radius * SIN(b) * COS(a + da);
	    z = radius * SIN(a + da);
	    glVertex3f(x, y, z);
	    
	    color = 1 - color;
	    }
	glEnd();
	
	}
    
    glEndList();
    
    return list;
    }

UGL_LOCAL void initGL(GLsizei width, GLsizei height)
    {
    float aspect = (float) width / (float) height;
    glViewport(0, 0, (GLint) width, (GLint) height);

    uglMesaSetColor(RED, 1.0, 0.0, 0.0);
    uglMesaSetColor(WHITE, 1.0, 1.0, 1.0);
    uglMesaSetColor(CYAN, 0.0, 1.0, 1.0);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-6.0 * aspect, 6.0 * aspect, -6.0, 6.0, -6.0, 6.0);
    glMatrixMode(GL_MODELVIEW);

    }

UGL_LOCAL void drawGL(void)
    {
    GLint i;
    static float vel0 = -100.0;
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    glIndexi(CYAN);
    glColor3f(0, 1, 1);
    glBegin(GL_LINES);
    for (i = -5; i <= 5; i++)
	{
	glVertex2i(i, -5);
	glVertex2i(i, 5);
	}
    for (i = -5; i <= 5; i++)
	{
	glVertex2i(-5, i);
	glVertex2i(5, i);
	}
    for (i = -5; i <= 5; i++)
	{
	glVertex2i(i, -5);
	glVertex2f(i * 1.15, -5.9);
	}
    glVertex2f(-5.3, -5.35);
    glVertex2f(5.3, -5.35);
    glVertex2f(-5.75, -5.9);
    glVertex2f(5.75, -5.9);
    glEnd();
    
    glPushMatrix();
    glTranslatef(Xpos, Ypos, 0.0);
    glScalef(2.0, 2.0, 2.0);
    glRotatef(8.0, 0.0, 0.0, 1.0);
    glRotatef(90.0, 1.0, 0.0, 0.0);
    glRotatef(Zrot, 0.0, 0.0, 1.0);
    
    glCallList(Ball);
    
    glPopMatrix();

    glFlush();
    
    uglMesaSwapBuffers();
    
    Zrot += Zstep;
    
    Xpos += Xvel;
    if (Xpos >= Xmax)
	{
	Xpos = Xmax;
	Xvel = -Xvel;
	Zstep = -Zstep;
	}
    if (Xpos <= Xmin)
	{
	Xpos = Xmin;
	Xvel = -Xvel;
	Zstep = -Zstep;
	}
    Ypos += Yvel;
    Yvel += G;
    if (Ypos < Ymin)
	{
	Ypos = Ymin;
	if (vel0 == -100.0)
	    vel0 = fabs(Yvel);
	Yvel = vel0;
	}
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

void windMLBounce (UGL_BOOL windMLMode);

void uglbounce (void)
    {
    taskSpawn("tBounce", 210, VX_FP_TASK, 100000, (FUNCPTR)windMLBounce,
              UGL_FALSE,1,2,3,4,5,6,7,8,9);
    }

void windMLBounce(UGL_BOOL windMLMode)
    {
    GLsizei width, height;
    UGL_INPUT_DEVICE_ID keyboardDevId;

    Zrot = 0.0;
    Zstep = 6.0;
    Xpos = 0.0;
    Ypos = 1.0;
    Xvel = 0.2;
    Yvel = 0.0;
    Xmin = -4.0;
    Xmax = 4.0;
    Ymin = -3.8;
    G = -0.1;

    uglInitialize();
   
    uglDriverFind (UGL_KEYBOARD_TYPE, 0, (UGL_UINT32 *)&keyboardDevId);
    
    uglDriverFind (UGL_EVENT_SERVICE_TYPE, 0, (UGL_UINT32 *)&eventServiceId);
   
    qId = uglEventQCreate (eventServiceId, 100);

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
    
    Ball = make_ball();
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glDisable(GL_DITHER);
    glShadeModel(GL_FLAT);
    
    uglMesaGetIntegerv(UGL_MESA_WIDTH, &width);
    uglMesaGetIntegerv(UGL_MESA_HEIGHT, &height);
    
    initGL(width, height);
    
    while(!getEvent())
	drawGL();

    uglEventQDestroy (eventServiceId, qId);

    uglMesaDestroyContext();
    uglDeinitialize ();
    
    return;
    }
