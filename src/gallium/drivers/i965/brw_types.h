#ifndef BRW_TYPES_H
#define BRW_TYPES_H

#include "pipe/p_compiler.h"

typedef uint32_t GLuint;
typedef uint8_t GLubyte;
typedef uint16_t GLushort;
typedef int32_t GLint;
typedef int8_t GLbyte;
typedef int16_t GLshort;
typedef float GLfloat;

/* no GLenum, translate all away */

typedef uint8_t GLboolean;

#define GL_FALSE FALSE
#define GL_TRUE TRUE

#endif
