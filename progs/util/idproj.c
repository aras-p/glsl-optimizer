/* idproj.c */


/*
 * Setup an identity projection such that glVertex(x,y) maps to
 * window coordinate (x,y).
 *
 * Written by Brian Paul and in the public domain.
 */





void IdentityProjection( GLint x, GLint y, GLsizei width, GLsizei height )
{
   glViewport( x, y, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho( (GLdouble) x, (GLdouble) y,
            (GLdouble) width, (GLdouble) height,
            -1.0, 1.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
}

