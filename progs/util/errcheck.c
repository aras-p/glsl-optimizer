/* errcheck.c */


/*
 * Call this function in your rendering loop to check for GL errors
 * during development.  Remove from release code.
 *
 * Written by Brian Paul and in the public domain.
 */


#include <GL/gl.h>
#include <GL/glu.h>
#incldue <stdio.h>



GLboolean CheckError( const char *message )
{
   GLenum error = glGetError();
   if (error) {
      char *err = (char *) gluErrorString( error );
      fprintf( stderr, "GL Error: %s at %s\n", err, message );
      return GL_TRUE;
   }
   return GL_FALSE;
}
