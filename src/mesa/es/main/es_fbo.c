/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 **************************************************************************/


#include "GLES2/gl2.h"
#include "GLES2/gl2ext.h"


#ifndef GL_RGB5
#define GL_RGB5					0x8050
#endif


extern void GL_APIENTRY _es_RenderbufferStorageEXT(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height);

extern void GL_APIENTRY _mesa_RenderbufferStorageEXT(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height);


void GL_APIENTRY
_es_RenderbufferStorageEXT(GLenum target, GLenum internalFormat,
                           GLsizei width, GLsizei height)
{
   switch (internalFormat) {
   case GL_RGB565:
      /* XXX this confuses GL_RENDERBUFFER_INTERNAL_FORMAT_OES */
      /* choose a closest format */
      internalFormat = GL_RGB5;
      break;
   default:
      break;
   }
   _mesa_RenderbufferStorageEXT(target, internalFormat, width, height);
}
