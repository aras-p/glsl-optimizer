
/* uglgears.c - WindML/Mesa example program */

/*
 * 3-D gear wheels.  This program is in the public domain.
 *
 * Brian Paul
 *
 * Conversion to GLUT by Mark J. Kilgard
 * Conversion to UGL/Mesa from GLUT by Stephane Raimbault
 */

/*
DESCRIPTION
Spinning gears demo
*/

#include <stdio.h>
#include <math.h>
#include <tickLib.h>

#include <ugl/ugl.h>
#include <ugl/uglucode.h>
#include <ugl/uglevent.h>
#include <ugl/uglinput.h>
#include <GL/uglmesa.h>
#include <GL/glu.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define COUNT_FRAMES

UGL_LOCAL UGL_EVENT_SERVICE_ID eventServiceId;
UGL_LOCAL UGL_EVENT_Q_ID qId;
UGL_LOCAL volatile UGL_BOOL stopWex;
UGL_LOCAL UGL_MESA_CONTEXT umc;

UGL_LOCAL GLfloat view_rotx, view_roty, view_rotz;
UGL_LOCAL GLint gear1, gear2, gear3;
UGL_LOCAL GLfloat angle;

UGL_LOCAL GLuint limit;
UGL_LOCAL GLuint count;
UGL_LOCAL GLuint tickStart, tickStop, tickBySec;


/*
* Draw a gear wheel.  You'll probably want to call this function when
* building a display list since we do a lot of trig here.
*
* Input:  inner_radius - radius of hole at center
*         outer_radius - radius at center of teeth
*         width - width of gear
*         teeth - number of teeth
*         tooth_depth - depth of tooth
*/

UGL_LOCAL void gear
    (
    GLfloat inner_radius,
    GLfloat outer_radius,
    GLfloat width,
    GLint teeth,
    GLfloat tooth_depth
    )
    {
    GLint i;
    GLfloat r0, r1, r2;
    GLfloat angle, da;
    GLfloat u, v, len;

    r0 = inner_radius;
    r1 = outer_radius - tooth_depth/2.0;
    r2 = outer_radius + tooth_depth/2.0;
    
    da = 2.0*M_PI / teeth / 4.0;
    
    glShadeModel (GL_FLAT);
    
    glNormal3f (0.0, 0.0, 1.0);
    
    /* draw front face */
    glBegin (GL_QUAD_STRIP);
    for (i=0;i<=teeth;i++)
        {
	angle = i * 2.0*M_PI / teeth;
	glVertex3f (r0*cos (angle), r0*sin (angle), width*0.5);
	glVertex3f (r1*cos (angle), r1*sin (angle), width*0.5);
	glVertex3f (r0*cos (angle), r0*sin (angle), width*0.5);
	glVertex3f (r1*cos (angle+3*da), r1*sin (angle+3*da), width*0.5);
	}
    glEnd ();

    /* draw front sides of teeth */
    glBegin (GL_QUADS);
    da = 2.0*M_PI / teeth / 4.0;
    for (i=0; i<teeth; i++)
        {
	angle = i * 2.0*M_PI / teeth;

	glVertex3f (r1*cos (angle),      r1*sin (angle),      width*0.5);
	glVertex3f (r2*cos (angle+da),   r2*sin (angle+da),   width*0.5);
	glVertex3f (r2*cos (angle+2*da), r2*sin (angle+2*da), width*0.5);
	glVertex3f (r1*cos (angle+3*da), r1*sin (angle+3*da), width*0.5);
	}
    glEnd ();


    glNormal3f (0.0, 0.0, -1.0);

    /* draw back face */
    glBegin (GL_QUAD_STRIP);
    for (i=0; i<=teeth ;i++)
        {
	angle = i * 2.0*M_PI / teeth;
	glVertex3f (r1*cos (angle), r1*sin (angle), -width*0.5);
	glVertex3f (r0*cos (angle), r0*sin (angle), -width*0.5);
	glVertex3f (r1*cos (angle+3*da), r1*sin (angle+3*da), -width*0.5);
	glVertex3f (r0*cos (angle), r0*sin (angle), -width*0.5);
	}
    glEnd ();

    /* draw back sides of teeth */
    glBegin (GL_QUADS);
    da = 2.0*M_PI / teeth / 4.0;
    for (i=0;i<teeth;i++)
        {
	angle = i * 2.0*M_PI / teeth;

	glVertex3f (r1*cos (angle+3*da), r1*sin (angle+3*da), -width*0.5);
	glVertex3f (r2*cos (angle+2*da), r2*sin (angle+2*da), -width*0.5);
	glVertex3f (r2*cos (angle+da),   r2*sin (angle+da),   -width*0.5);
	glVertex3f (r1*cos (angle),      r1*sin (angle),      -width*0.5);
	}
    glEnd ();


    /* draw outward faces of teeth */
    glBegin (GL_QUAD_STRIP);
    for (i=0;i<teeth;i++)
        {
	angle = i * 2.0*M_PI / teeth;

	glVertex3f (r1*cos (angle),      r1*sin (angle),       width*0.5);
	glVertex3f (r1*cos (angle),      r1*sin (angle),      -width*0.5);
	u = r2*cos (angle+da) - r1*cos (angle);
	v = r2*sin (angle+da) - r1*sin (angle);
	len = sqrt (u*u + v*v);
	u /= len;
	v /= len;
	glNormal3f (v, -u, 0.0);
	glVertex3f (r2*cos (angle+da),   r2*sin (angle+da),    width*0.5);
	glVertex3f (r2*cos (angle+da),   r2*sin (angle+da),   -width*0.5);
	glNormal3f (cos (angle), sin (angle), 0.0);
	glVertex3f (r2*cos (angle+2*da), r2*sin (angle+2*da),  width*0.5);
	glVertex3f (r2*cos (angle+2*da), r2*sin (angle+2*da), -width*0.5);
	u = r1*cos (angle+3*da) - r2*cos (angle+2*da);
	v = r1*sin (angle+3*da) - r2*sin (angle+2*da);
	glNormal3f (v, -u, 0.0);
	glVertex3f (r1*cos (angle+3*da), r1*sin (angle+3*da),  width*0.5);
	glVertex3f (r1*cos (angle+3*da), r1*sin (angle+3*da), -width*0.5);
	glNormal3f (cos (angle), sin (angle), 0.0);
        }

    glVertex3f (r1*cos (0), r1*sin (0), width*0.5);
    glVertex3f (r1*cos (0), r1*sin (0), -width*0.5);
    
    glEnd ();
    
    glShadeModel (GL_SMOOTH);
    
    /* draw inside radius cylinder */
    glBegin (GL_QUAD_STRIP);
    for (i=0;i<=teeth;i++)
        {
	angle = i * 2.0*M_PI / teeth;
	glNormal3f (-cos (angle), -sin (angle), 0.0);
	glVertex3f (r0*cos (angle), r0*sin (angle), -width*0.5);
	glVertex3f (r0*cos (angle), r0*sin (angle), width*0.5);
	} 
    glEnd ();
      
}

UGL_LOCAL void drawGL (void)
    {
#ifdef COUNT_FRAMES
    int time;
#endif
    
    angle += 2.0;

    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix ();
    glRotatef (view_rotx, 1.0, 0.0, 0.0);
    glRotatef (view_roty, 0.0, 1.0, 0.0);
    glRotatef (view_rotz, 0.0, 0.0, 1.0);

    glPushMatrix ();
    glTranslatef (-3.0, -2.0, 0.0);
    glRotatef (angle, 0.0, 0.0, 1.0);
    glCallList (gear1);
    glPopMatrix ();

    glPushMatrix ();
    glTranslatef (3.1, -2.0, 0.0);
    glRotatef (-2.0*angle-9.0, 0.0, 0.0, 1.0);
    glCallList (gear2);
    glPopMatrix ();

    glPushMatrix ();
    glTranslatef (-3.1, 4.2, 0.0);
    glRotatef (-2.0*angle-25.0, 0.0, 0.0, 1.0);
    glCallList (gear3);
    glPopMatrix ();

    glPopMatrix ();

    glFlush();
    
    uglMesaSwapBuffers ();

#ifdef COUNT_FRAMES
    if (count > limit)
        {
	tickStop = tickGet ();
	time = (tickStop-tickStart)/tickBySec;
	printf (" %i fps\n", count/time);
	tickStart = tickStop;
	count = 0;
        }
    else 
	count++;
#endif
}


UGL_LOCAL void initGL (GLsizei width, GLsizei height)
    {
    UGL_LOCAL GLfloat pos[4] = {5.0, 5.0, 10.0, 1.0 };
    UGL_LOCAL GLfloat red[4] = {0.8, 0.1, 0.0, 1.0 };
    UGL_LOCAL GLfloat green[4] = {0.0, 0.8, 0.2, 1.0 };
    UGL_LOCAL GLfloat blue[4] = {0.2, 0.2, 1.0, 1.0 };

    glLightfv (GL_LIGHT0, GL_POSITION, pos);
    glEnable (GL_CULL_FACE);
    glEnable (GL_LIGHTING);
    glEnable (GL_LIGHT0);
    glEnable (GL_DEPTH_TEST);

    /* make the gears */
    gear1 = glGenLists (1);
    glNewList (gear1, GL_COMPILE);
    glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
    gear (1.0, 4.0, 1.0, 20, 0.7);
    glEndList ();

    gear2 = glGenLists (1);
    glNewList (gear2, GL_COMPILE);
    glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
    gear (0.5, 2.0, 2.0, 10, 0.7);
    glEndList ();

    gear3 = glGenLists (1);
    glNewList (gear3, GL_COMPILE);
    glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
    gear (1.3, 2.0, 0.5, 10, 0.7);
    glEndList ();

    glEnable (GL_NORMALIZE);

    glViewport (0, 0, width, height);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    if (width>height)
        {
	GLfloat w = (GLfloat) width / (GLfloat) height;
	glFrustum (-w, w, -1.0, 1.0, 5.0, 60.0);
        }
    else
        {
	GLfloat h = (GLfloat) height / (GLfloat) width;
	glFrustum (-1.0, 1.0, -h, h, 5.0, 60.0);
        }

    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();
    glTranslatef (0.0, 0.0, -40.0);

#ifdef COUNT_FRAMES
    tickStart = tickGet ();
    tickBySec = sysClkRateGet ();
#endif
}

UGL_LOCAL void echoUse(void)
    {
    printf("tGears keys:\n");
    printf("           z  Counter clockwise rotation (z-axis)\n");
    printf("           Z  Clockwise rotation (z-axis)\n");
    printf("          Up  Counter clockwise rotation (x-axis)\n");
    printf("        Down  Clockwise rotation (x-axis)\n");
    printf("        Left  Counter clockwise rotation (y-axis)\n");
    printf("       Right  Clockwise rotation (y-axis)\n");
    printf("         ESC  Exit\n");
    }


UGL_LOCAL void readKey (UGL_WCHAR key)
    {
    
    switch(key)
	{
	case 'z':
	    view_rotz += 5.0;
	    break;
	case 'Z':
	    view_rotz -= 5.0;
	    break;
	case UGL_UNI_UP_ARROW:
	    view_rotx += 5.0;
	    break;
	case UGL_UNI_DOWN_ARROW:
	    view_rotx -= 5.0;
	    break;
	case UGL_UNI_LEFT_ARROW:
	    view_roty += 5.0;
	    break;
	case UGL_UNI_RIGHT_ARROW:
	    view_roty -= 5.0;
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

void windMLGears (UGL_BOOL windMLMode);

void uglgears (void)
    {
    taskSpawn ("tGears", 210, VX_FP_TASK, 100000, (FUNCPTR)windMLGears,
	       UGL_FALSE,1,2,3,4,5,6,7,8,9);
    }

void windMLGears (UGL_BOOL windMLMode)
    {
    GLsizei width, height;
    UGL_INPUT_DEVICE_ID keyboardDevId;
    
    view_rotx=20.0;
    view_roty=30.0;
    view_rotz=0.0;
    angle = 0.0;
    limit = 100;
    count = 1;

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

    /* Fullscreen */

    uglMesaMakeCurrentContext (umc, 0, 0, UGL_MESA_FULLSCREEN_WIDTH,
			       UGL_MESA_FULLSCREEN_HEIGHT);

    uglMesaGetIntegerv(UGL_MESA_WIDTH, &width);
    uglMesaGetIntegerv(UGL_MESA_HEIGHT, &height);
    
    initGL (width, height);

    echoUse();

    stopWex = UGL_FALSE;
    loopEvent();
        
    uglEventQDestroy (eventServiceId, qId);

    uglMesaDestroyContext();
    uglDeinitialize ();

    return;
    }
