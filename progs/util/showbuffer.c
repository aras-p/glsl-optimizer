/* showbuffer.c */


/*
 * Copy the depth buffer to the color buffer as a grayscale image.
 * Useful for inspecting the depth buffer values.
 *
 * This program is in the public domain.
 *
 * Brian Paul   November 4, 1998
 */


#include <assert.h>
#include <stdlib.h>
#include <GL/gl.h>
#include "showbuffer.h"



/*
 * Copy the depth buffer values into the current color buffer as a
 * grayscale image.
 * Input:  winWidth, winHeight - size of the window
 *         zBlack - the Z value which should map to black (usually 1)
 *         zWhite - the Z value which should map to white (usually 0)
 */
void
ShowDepthBuffer( GLsizei winWidth, GLsizei winHeight,
                 GLfloat zBlack, GLfloat zWhite )
{
   GLfloat *depthValues;

   assert(zBlack >= 0.0);
   assert(zBlack <= 1.0);
   assert(zWhite >= 0.0);
   assert(zWhite <= 1.0);
   assert(zBlack != zWhite);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glPixelStorei(GL_PACK_ALIGNMENT, 1);

   /* Read depth values */
   depthValues = (GLfloat *) malloc(winWidth * winHeight * sizeof(GLfloat));
   assert(depthValues);
   glReadPixels(0, 0, winWidth, winHeight, GL_DEPTH_COMPONENT,
                GL_FLOAT, depthValues);

   /* Map Z values from [zBlack, zWhite] to gray levels in [0, 1] */
   /* Not using glPixelTransfer() because it's broke on some systems! */
   if (zBlack != 0.0 || zWhite != 1.0) {
      GLfloat scale = 1.0 / (zWhite - zBlack);
      GLfloat bias = -zBlack * scale;
      int n = winWidth * winHeight;
      int i;
      for (i = 0; i < n; i++)
         depthValues[i] = depthValues[i] * scale + bias;
   }

   /* save GL state */
   glPushAttrib(GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT |
                GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

   /* setup raster pos for glDrawPixels */
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();

   glOrtho(0.0, (GLdouble) winWidth, 0.0, (GLdouble) winHeight, -1.0, 1.0);
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   glDisable(GL_STENCIL_TEST);
   glDisable(GL_DEPTH_TEST);
   glRasterPos2f(0, 0);

   glDrawPixels(winWidth, winHeight, GL_LUMINANCE, GL_FLOAT, depthValues);

   glPopMatrix();
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   free(depthValues);

   glPopAttrib();
}




/*
 * Copy the alpha channel values into the current color buffer as a
 * grayscale image.
 * Input:  winWidth, winHeight - size of the window
 */
void
ShowAlphaBuffer( GLsizei winWidth, GLsizei winHeight )
{
   GLubyte *alphaValues;

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glPixelStorei(GL_PACK_ALIGNMENT, 1);

   /* Read alpha values */
   alphaValues = (GLubyte *) malloc(winWidth * winHeight * sizeof(GLubyte));
   assert(alphaValues);
   glReadPixels(0, 0, winWidth, winHeight, GL_ALPHA, GL_UNSIGNED_BYTE, alphaValues);

   /* save GL state */
   glPushAttrib(GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL |
                GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

   /* setup raster pos for glDrawPixels */
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();

   glOrtho(0.0, (GLdouble) winWidth, 0.0, (GLdouble) winHeight, -1.0, 1.0);
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   glDisable(GL_STENCIL_TEST);
   glDisable(GL_DEPTH_TEST);
   glRasterPos2f(0, 0);

   glDrawPixels(winWidth, winHeight, GL_LUMINANCE, GL_UNSIGNED_BYTE, alphaValues);

   glPopMatrix();
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   free(alphaValues);

   glPopAttrib();
}



/*
 * Copy the stencil buffer values into the current color buffer as a
 * grayscale image.
 * Input:  winWidth, winHeight - size of the window
 *         scale, bias - scale and bias to apply to stencil values for display
 */
void
ShowStencilBuffer( GLsizei winWidth, GLsizei winHeight,
                   GLfloat scale, GLfloat bias )
{
   GLubyte *stencilValues;

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glPixelStorei(GL_PACK_ALIGNMENT, 1);

   /* Read stencil values */
   stencilValues = (GLubyte *) malloc(winWidth * winHeight * sizeof(GLubyte));
   assert(stencilValues);
   glReadPixels(0, 0, winWidth, winHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilValues);

   /* save GL state */
   glPushAttrib(GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT |
                GL_PIXEL_MODE_BIT | GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

   /* setup raster pos for glDrawPixels */
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();

   glOrtho(0.0, (GLdouble) winWidth, 0.0, (GLdouble) winHeight, -1.0, 1.0);
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   glDisable(GL_STENCIL_TEST);
   glDisable(GL_DEPTH_TEST);
   glRasterPos2f(0, 0);

   glPixelTransferf(GL_RED_SCALE, scale);
   glPixelTransferf(GL_RED_BIAS, bias);
   glPixelTransferf(GL_GREEN_SCALE, scale);
   glPixelTransferf(GL_GREEN_BIAS, bias);
   glPixelTransferf(GL_BLUE_SCALE, scale);
   glPixelTransferf(GL_BLUE_BIAS, bias);

   glDrawPixels(winWidth, winHeight, GL_LUMINANCE, GL_UNSIGNED_BYTE, stencilValues);

   glPopMatrix();
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   free(stencilValues);

   glPopAttrib();
}
