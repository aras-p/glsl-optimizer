/* readtex.h */

#ifndef READTEX_H
#define READTEX_H


#include <GL/gl.h>


extern GLboolean
LoadRGBMipmaps( const char *imageFile, GLint intFormat );


extern GLboolean
LoadRGBMipmaps2( const char *imageFile, GLenum target,
                 GLint intFormat, GLint *width, GLint *height );


extern GLubyte *
LoadRGBImage( const char *imageFile,
              GLint *width, GLint *height, GLenum *format );

extern GLushort *
LoadYUVImage( const char *imageFile, GLint *width, GLint *height );

#endif
