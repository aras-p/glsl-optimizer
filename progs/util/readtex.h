/* readtex.h */

#ifndef READTEX_H
#define READTEX_H


#include <GL/gl.h>


extern GLboolean LoadRGBMipmaps( const char *imageFile, GLint intFormat );


extern GLubyte *LoadRGBImage( const char *imageFile,
                              GLint *width, GLint *height, GLenum *format );


#endif
