/* showbuffer. h*/

/*
 * Copy the depth buffer to the color buffer as a grayscale image.
 * Useful for inspecting the depth buffer values.
 *
 * This program is in the public domain.
 *
 * Brian Paul   November 4, 1998
 */


#ifndef SHOWBUFFER_H
#define SHOWBUFFER_H


#include <GL/gl.h>



extern void
ShowDepthBuffer( GLsizei winWidth, GLsizei winHeight,
                 GLfloat zBlack, GLfloat zWhite );


extern void
ShowAlphaBuffer( GLsizei winWidth, GLsizei winHeight );


extern void
ShowStencilBuffer( GLsizei winWidth, GLsizei winHeight,
                   GLfloat scale, GLfloat bias );



#endif
