/* winpos.c */


/*
 * Set the current raster position to a specific window
 * coordinate.  Also see the GL_MESA_window_pos extension.
 *
 * Written by Brian Paul and in the public domain.
 */


void WindowPos( GLfloat x, GLfloat y, GLfloat z )
{
   GLfloat fx, fy;

   /* Push current matrix mode and viewport attributes */
   glPushAttrib( GL_TRANSFORM_BIT | GL_VIEWPORT_BIT );

   /* Setup projection parameters */
   glMatrixMode( GL_PROJECTION );
   glPushMatrix();
   glLoadIdentity();
   glMatrixMode( GL_MODELVIEW );
   glPushMatrix();
   glLoadIdentity();

   glDepthRange( z, z );
   glViewport( (int) x - 1, (int) y - 1, 2, 2 );

   /* set the raster (window) position */
   fx = x - (int) x;
   fy = y - (int) y;
   glRasterPos3f( fx, fy, 0.0 );

   /* restore matrices, viewport and matrix mode */
   glPopMatrix();
   glMatrixMode( GL_PROJECTION );
   glPopMatrix();

   glPopAttrib();
}

