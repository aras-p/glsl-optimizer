/* File: mesa_extend.c for wmesa-2.3
   Written by Li Wei (liwei@aiar.xjtu.edu.cn)
*/

/*******************************************************************
 Users can use the following keys to control the view

 The following four key combinations can shift the view correspondingly,
 function in both stereo and normal mode.
 Ctrl+left arrow
 Ctrl+right arrow
 Ctrl+up arrow
 Ctrl+down arrow

 F (captital letter) shift the camera far from objects
 N (captital letter) shift the camera near from objects
 S (captital letter) toggle between normal and stereo mode
 I (captital letter) increase the distance between two views
 D (captital letter) decrease the distance between two views

 if the Key function defined by user maps any key appearing above, it will be
 masked by the program. Hence, user should either modify his own code or
 modify function defaultKeyProc at the end of this file 
*******************************************************************/

/* Log 6/14, 1997
 * revision 1.01
 * struct DisplayOptions defined for tk_ddmesa.c to read the initial file
 */

#include "mesa_extend.h"
#include "gltk.h"
#include <stdio.h>
#ifndef NO_STEREO
	#include "stereo.h"
#endif
#ifndef NO_PARALLEL
//	#include "parallel.h"
#endif

GLenum (*userKeyProc) (int, GLenum) = NULL;

GLfloat viewDistance = 1.0;
GLfloat deltaView = 0.1;
GLfloat deltaShift = 0.1;

GLuint viewShift = SHIFT_NONE;
GLuint viewTag = 0 ;

GLenum imageRendered = GL_FALSE;

GLenum glImageRendered()
{
	return imageRendered; 
}

//Code added by Li Wei to enable stereo display
GLenum defaultKeyProc(int key, GLenum mask )
{
	GLenum flag = GL_FALSE ;
	if(mask & TK_CONTROL){
	flag = GL_TRUE ;
	switch(key){
		case TK_LEFT:
		viewShift = SHIFT_LEFT;
		break;
		case TK_RIGHT:
		viewShift = SHIFT_RIGHT;
		break;
		case TK_UP:
		viewShift = SHIFT_UP;
		break;
		case TK_DOWN:
		viewShift = SHIFT_DOWN;
		break;
		default:
			flag = GL_FALSE ;
		}
	}
	if(flag == GL_FALSE){
	flag = GL_TRUE ;
	switch(key){
		case TK_F:
		viewShift = SHIFT_FAR;
		break;
		case TK_N:
		viewShift = SHIFT_NEAR;
		break;

#if !defined(NO_STEREO)
		case TK_D:
		viewDistance-= deltaView;
		break;
		case TK_I:
		viewDistance+= deltaView;
		break;
		case TK_S:
		toggleStereoMode();
		break;
#endif

#if !defined(NO_PARALLEL)
		case TK_P:
		if(machineType == MASTER)
			toggleParallelMode();
		break;
#endif
		default:
			flag = GL_FALSE;
		}
		}

	if(userKeyProc)
		flag=flag||(*userKeyProc)(key, mask);

#if !defined(NO_PARALLEL)
	if(parallelFlag&&key!=TK_P&&machineType == MASTER){
		PRKeyDown(key,mask);
	}
#endif

	return flag;
}

/* The following function implemented key board control of the display,
	availabe even in normal mode so long the driver is linked into exe file.
*/
void shiftView()
{
	GLfloat cm[16];
	if(viewShift != SHIFT_NONE){
/*	glGetFloatv(GL_MODELVIEW_MATRIX,cm);
	glMatrixMode(GL_MODELVIEW);
*/
    GLint matrix_mode;
    glGetIntegerv(GL_MATRIX_MODE,&matrix_mode);
/*	if(matrix_mode!=GL_PROJECTION)
        glMatrixMode(GL_PROJECTION);
    glGetFloatv(GL_PROJECTION_MATRIX,cm);
    glLoadIdentity();
	switch(viewShift){
		case SHIFT_LEFT:
			glTranslatef(-deltaShift,0,0);
			break;
		case SHIFT_RIGHT:
			glTranslatef(deltaShift,0,0);
			break;
		case SHIFT_UP:
			glTranslatef(0,deltaShift,0);
			break;
		case SHIFT_DOWN:
			glTranslatef(0,-deltaShift,0);
			break;
		case SHIFT_FAR:
			glTranslatef(0,0,-deltaShift);
			break;
		case SHIFT_NEAR:
			glTranslatef(0,0,deltaShift);
			break;
		}

		viewShift = SHIFT_NONE;
		glMultMatrixf( cm );
        if(matrix_mode!=GL_PROJECTION)
            glMatrixMode(matrix_mode);

	}
*/
	if(matrix_mode!=GL_MODELVIEW)
        glMatrixMode(GL_MODELVIEW);
    glGetFloatv(GL_MODELVIEW_MATRIX,cm);
    glLoadIdentity();
	switch(viewShift){
		case SHIFT_LEFT:
			glTranslatef(-deltaShift,0,0);
			break;
		case SHIFT_RIGHT:
			glTranslatef(deltaShift,0,0);
			break;
		case SHIFT_UP:
			glTranslatef(0,deltaShift,0);
			break;
		case SHIFT_DOWN:
			glTranslatef(0,-deltaShift,0);
			break;
		case SHIFT_FAR:
			glTranslatef(0,0,-deltaShift);
			break;
		case SHIFT_NEAR:
			glTranslatef(0,0,deltaShift);
			break;
		}

		viewShift = SHIFT_NONE;
		glMultMatrixf( cm );
        if(matrix_mode!=GL_MODELVIEW)
            glMatrixMode(matrix_mode);

	}
}


void getDisplayOptions( void)
{
	displayOptions.stereo = GetPrivateProfileInt("DISPLAY", "STEREO",1,"ddmesa.ini" );
	displayOptions.fullScreen = GetPrivateProfileInt("DISPLAY", "FULLSCREEN",0,"ddmesa.ini" );
	displayOptions.mode = GetPrivateProfileInt("DISPLAY", "MODE",1, "ddmesa.ini");
	displayOptions.bpp = GetPrivateProfileInt("DISPLAY", "BPP", 32, "ddmesa.ini");

}
//end modification
