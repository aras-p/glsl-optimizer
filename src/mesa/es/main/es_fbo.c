/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 **************************************************************************/


#include "GLES2/gl2.h"
#include "GLES2/gl2ext.h"


extern void GL_APIENTRY _es_RenderbufferStorageEXT(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height);

extern void GL_APIENTRY _mesa_RenderbufferStorageEXT(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height);


void GL_APIENTRY
_es_RenderbufferStorageEXT(GLenum target, GLenum internalFormat,
                           GLsizei width, GLsizei height)
{
   switch (internalFormat) {
   case GL_RGBA4:
   case GL_RGB5_A1:
   case GL_RGB565:
      internalFormat = GL_RGBA;
      break;
   case GL_STENCIL_INDEX1_OES:
   case GL_STENCIL_INDEX4_OES:
   case GL_STENCIL_INDEX8:
      internalFormat = GL_STENCIL_INDEX;
      break;
   default:
      ; /* no op */
   }
   _mesa_RenderbufferStorageEXT(target, internalFormat, width, height);
}
