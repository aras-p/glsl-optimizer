/* $Id: extensions.c,v 1.10 1999/11/08 07:36:44 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef XFree86Server
#include <stdlib.h>
#else
#include "GL/xf86glx.h"
#endif
#include "context.h"
#include "extensions.h"
#include "simple_list.h"
#include "types.h"


#define MAX_EXT_NAMELEN 80

struct extension {
   struct extension *next, *prev;
   int enabled;
   char name[MAX_EXT_NAMELEN+1];
   void (*notify)( GLcontext *, GLboolean ); 
};



static struct { int enabled; const char *name; } default_extensions[] = {
   { ALWAYS_ENABLED, "GL_EXT_blend_color" },
   { DEFAULT_OFF,    "ARB_imaging" },
   { DEFAULT_ON,     "GL_EXT_blend_minmax" },
   { DEFAULT_ON,     "GL_EXT_blend_logic_op" },
   { DEFAULT_ON,     "GL_EXT_blend_subtract" },
   { DEFAULT_ON,     "GL_EXT_paletted_texture" },
   { DEFAULT_ON,     "GL_EXT_point_parameters" },
   { ALWAYS_ENABLED, "GL_EXT_polygon_offset" },
   { ALWAYS_ENABLED, "GL_EXT_vertex_array" },
   { ALWAYS_ENABLED, "GL_EXT_texture_object" },
   { DEFAULT_ON,     "GL_EXT_texture3D" },
   { ALWAYS_ENABLED, "GL_MESA_window_pos" },
   { ALWAYS_ENABLED, "GL_MESA_resize_buffers" },
   { ALWAYS_ENABLED, "GL_EXT_shared_texture_palette" },
   { ALWAYS_ENABLED, "GL_EXT_rescale_normal" },
   { ALWAYS_ENABLED, "GL_EXT_abgr" },
   { ALWAYS_ENABLED, "GL_SGIS_texture_edge_clamp" },
   { ALWAYS_ENABLED, "GL_EXT_stencil_wrap" },
   { DEFAULT_ON,     "GL_INGR_blend_func_separate" },
   { DEFAULT_ON,     "GL_ARB_multitexture" },
   { ALWAYS_ENABLED, "GL_NV_texgen_reflection" },
   { DEFAULT_ON,     "GL_PGI_misc_hints" },
   { DEFAULT_ON,     "GL_EXT_compiled_vertex_array" },
   { DEFAULT_OFF,    "GL_EXT_vertex_array_set" },
   { DEFAULT_ON,     "GL_EXT_clip_volume_hint" },
};


int gl_extensions_add( GLcontext *ctx, 
		       int state, 
		       const char *name, 
		       void (*notify)(void) )
{
   (void) notify;

   if (ctx->Extensions.ext_string == 0) 
   {
      struct extension *t = MALLOC_STRUCT(extension);
      t->enabled = state;
      strncpy(t->name, name, MAX_EXT_NAMELEN);
      t->name[MAX_EXT_NAMELEN] = 0;
      t->notify = (void (*)(GLcontext *, GLboolean)) notify;
      insert_at_tail( ctx->Extensions.ext_list, t );
      return 0;
   }
   return 1;
}


static int set_extension( GLcontext *ctx, const char *name, GLuint state )
{
   struct extension *i;
   foreach( i, ctx->Extensions.ext_list ) 
      if (strncmp(i->name, name, MAX_EXT_NAMELEN) == 0) 
	 break;

   if (i == ctx->Extensions.ext_list) return 1;

   if (i->enabled && !(i->enabled & ALWAYS_ENABLED))
   {
      if (i->notify) i->notify( ctx, state );      
      i->enabled = state;
   }

   return 0;
}   


int gl_extensions_enable( GLcontext *ctx, const char *name )
{
   if (ctx->Extensions.ext_string == 0) 
      return set_extension( ctx, name, 1 );
   return 1;
}


int gl_extensions_disable( GLcontext *ctx, const char *name )
{
   if (ctx->Extensions.ext_string == 0) 
      return set_extension( ctx, name, 0 );
   return 1;
}
      

void gl_extensions_dtr( GLcontext *ctx )
{
   struct extension *i, *nexti;

   if (ctx->Extensions.ext_string) {
      FREE( ctx->Extensions.ext_string );
      ctx->Extensions.ext_string = 0;
   }

   if (ctx->Extensions.ext_list) {
      foreach_s( i, nexti, ctx->Extensions.ext_list ) {
	 remove_from_list( i );
	 FREE( i );
      }
   
      FREE(ctx->Extensions.ext_list);
      ctx->Extensions.ext_list = 0;
   }      
}


void gl_extensions_ctr( GLcontext *ctx )
{
   GLuint i;

   ctx->Extensions.ext_string = 0;
   ctx->Extensions.ext_list = MALLOC_STRUCT(extension);
   make_empty_list( ctx->Extensions.ext_list );

   for (i = 0 ; i < Elements(default_extensions) ; i++) {
      gl_extensions_add( ctx, 
			 default_extensions[i].enabled,
			 default_extensions[i].name,
			 0 );
   }
}


const char *gl_extensions_get_string( GLcontext *ctx )
{
   if (ctx->Extensions.ext_string == 0) 
   {
      struct extension *i;
      char *str;
      GLuint len = 0;
      foreach (i, ctx->Extensions.ext_list) 
	 if (i->enabled)
	    len += strlen(i->name) + 1;
      
      if (len == 0) 
	 return "";

      str = (char *)MALLOC(len * sizeof(char));
      ctx->Extensions.ext_string = str;

      foreach (i, ctx->Extensions.ext_list) 
	 if (i->enabled) {
	    strcpy(str, i->name);
	    str += strlen(str);
	    *str++ = ' ';
	 }

      *(str-1) = 0;
   }
      
   return ctx->Extensions.ext_string;
}



/*
 * Return the address of an extension function.
 * This is meant to be called by glXGetProcAddress(), wglGetProcAddress(),
 * or similar function.
 * NOTE: this function could be optimized to binary search a sorted
 * list of function names.
 */
void (*gl_get_proc_address( const GLubyte *procName ))()
{
   typedef void (*gl_function)();
   struct proc {
      const char *name;
      gl_function address;
   };
   static struct proc procTable[] = {
      /* OpenGL 1.1 functions */
      { "glEnableClientState", (gl_function) glEnableClientState },
      { "glDisableClientState", (gl_function) glDisableClientState },
      { "glPushClientAttrib", (gl_function) glPushClientAttrib },
      { "glPopClientAttrib", (gl_function) glPopClientAttrib },
      { "glIndexub", (gl_function) glIndexub },
      { "glIndexubv", (gl_function) glIndexubv },
      { "glVertexPointer", (gl_function) glVertexPointer },
      { "glNormalPointer", (gl_function) glNormalPointer },
      { "glColorPointer", (gl_function) glColorPointer },
      { "glIndexPointer", (gl_function) glIndexPointer },
      { "glTexCoordPointer", (gl_function) glTexCoordPointer },
      { "glEdgeFlagPointer", (gl_function) glEdgeFlagPointer },
      { "glGetPointerv", (gl_function) glGetPointerv },
      { "glArrayElement", (gl_function) glArrayElement },
      { "glDrawArrays", (gl_function) glDrawArrays },
      { "glDrawElements", (gl_function) glDrawElements },
      { "glInterleavedArrays", (gl_function) glInterleavedArrays },
      { "glGenTextures", (gl_function) glGenTextures },
      { "glDeleteTextures", (gl_function) glDeleteTextures },
      { "glBindTexture", (gl_function) glBindTexture },
      { "glPrioritizeTextures", (gl_function) glPrioritizeTextures },
      { "glAreTexturesResident", (gl_function) glAreTexturesResident },
      { "glIsTexture", (gl_function) glIsTexture },
      { "glTexSubImage1D", (gl_function) glTexSubImage1D },
      { "glTexSubImage2D", (gl_function) glTexSubImage2D },
      { "glCopyTexImage1D", (gl_function) glCopyTexImage1D },
      { "glCopyTexImage2D", (gl_function) glCopyTexImage2D },
      { "glCopyTexSubImage1D", (gl_function) glCopyTexSubImage1D },
      { "glCopyTexSubImage2D", (gl_function) glCopyTexSubImage2D },

      /* OpenGL 1.2 functions */
      { "glDrawRangeElements", (gl_function) glDrawRangeElements },
      { "glTexImage3D", (gl_function) glTexImage3D },
      { "glTexSubImage3D", (gl_function) glTexSubImage3D },
      { "glCopyTexSubImage3D", (gl_function) glCopyTexSubImage3D },
      /* NOTE: 1.2 imaging subset functions not implemented in Mesa */

      /* GL_EXT_blend_minmax */
      { "glBlendEquationEXT", (gl_function) glBlendEquationEXT },

      /* GL_EXT_blend_color */
      { "glBlendColorEXT", (gl_function) glBlendColorEXT },

      /* GL_EXT_polygon_offset */
      { "glPolygonOffsetEXT", (gl_function) glPolygonOffsetEXT },

      /* GL_EXT_vertex_arrays */
      { "glVertexPointerEXT", (gl_function) glVertexPointerEXT },
      { "glNormalPointerEXT", (gl_function) glNormalPointerEXT },
      { "glColorPointerEXT", (gl_function) glColorPointerEXT },
      { "glIndexPointerEXT", (gl_function) glIndexPointerEXT },
      { "glTexCoordPointerEXT", (gl_function) glTexCoordPointerEXT },
      { "glEdgeFlagPointerEXT", (gl_function) glEdgeFlagPointerEXT },
      { "glGetPointervEXT", (gl_function) glGetPointervEXT },
      { "glArrayElementEXT", (gl_function) glArrayElementEXT },
      { "glDrawArraysEXT", (gl_function) glDrawArraysEXT },

      /* GL_EXT_texture_object */
      { "glGenTexturesEXT", (gl_function) glGenTexturesEXT },
      { "glDeleteTexturesEXT", (gl_function) glDeleteTexturesEXT },
      { "glBindTextureEXT", (gl_function) glBindTextureEXT },
      { "glPrioritizeTexturesEXT", (gl_function) glPrioritizeTexturesEXT },
      { "glAreTexturesResidentEXT", (gl_function) glAreTexturesResidentEXT },
      { "glIsTextureEXT", (gl_function) glIsTextureEXT },

      /* GL_EXT_texture3D */
      { "glTexImage3DEXT", (gl_function) glTexImage3DEXT },
      { "glTexSubImage3DEXT", (gl_function) glTexSubImage3DEXT },
      { "glCopyTexSubImage3DEXT", (gl_function) glCopyTexSubImage3DEXT },

      /* GL_EXT_color_table */
      { "glColorTableEXT", (gl_function) glColorTableEXT },
      { "glColorSubTableEXT", (gl_function) glColorSubTableEXT },
      { "glGetColorTableEXT", (gl_function) glGetColorTableEXT },
      { "glGetColorTableParameterfvEXT", (gl_function) glGetColorTableParameterfvEXT },
      { "glGetColorTableParameterivEXT", (gl_function) glGetColorTableParameterivEXT },

      /* GL_ARB_multitexture */
      { "glActiveTextureARB", (gl_function) glActiveTextureARB },
      { "glClientActiveTextureARB", (gl_function) glClientActiveTextureARB },
      { "glMultiTexCoord1dARB", (gl_function) glMultiTexCoord1dARB },
      { "glMultiTexCoord1dvARB", (gl_function) glMultiTexCoord1dvARB },
      { "glMultiTexCoord1fARB", (gl_function) glMultiTexCoord1fARB },
      { "glMultiTexCoord1fvARB", (gl_function) glMultiTexCoord1fvARB },
      { "glMultiTexCoord1iARB", (gl_function) glMultiTexCoord1iARB },
      { "glMultiTexCoord1ivARB", (gl_function) glMultiTexCoord1ivARB },
      { "glMultiTexCoord1sARB", (gl_function) glMultiTexCoord1sARB },
      { "glMultiTexCoord1svARB", (gl_function) glMultiTexCoord1svARB },
      { "glMultiTexCoord2dARB", (gl_function) glMultiTexCoord2dARB },
      { "glMultiTexCoord2dvARB", (gl_function) glMultiTexCoord2dvARB },
      { "glMultiTexCoord2fARB", (gl_function) glMultiTexCoord2fARB },
      { "glMultiTexCoord2fvARB", (gl_function) glMultiTexCoord2fvARB },
      { "glMultiTexCoord2iARB", (gl_function) glMultiTexCoord2iARB },
      { "glMultiTexCoord2ivARB", (gl_function) glMultiTexCoord2ivARB },
      { "glMultiTexCoord2sARB", (gl_function) glMultiTexCoord2sARB },
      { "glMultiTexCoord2svARB", (gl_function) glMultiTexCoord2svARB },
      { "glMultiTexCoord3dARB", (gl_function) glMultiTexCoord3dARB },
      { "glMultiTexCoord3dvARB", (gl_function) glMultiTexCoord3dvARB },
      { "glMultiTexCoord3fARB", (gl_function) glMultiTexCoord3fARB },
      { "glMultiTexCoord3fvARB", (gl_function) glMultiTexCoord3fvARB },
      { "glMultiTexCoord3iARB", (gl_function) glMultiTexCoord3iARB },
      { "glMultiTexCoord3ivARB", (gl_function) glMultiTexCoord3ivARB },
      { "glMultiTexCoord3sARB", (gl_function) glMultiTexCoord3sARB },
      { "glMultiTexCoord3svARB", (gl_function) glMultiTexCoord3svARB },
      { "glMultiTexCoord4dARB", (gl_function) glMultiTexCoord4dARB },
      { "glMultiTexCoord4dvARB", (gl_function) glMultiTexCoord4dvARB },
      { "glMultiTexCoord4fARB", (gl_function) glMultiTexCoord4fARB },
      { "glMultiTexCoord4fvARB", (gl_function) glMultiTexCoord4fvARB },
      { "glMultiTexCoord4iARB", (gl_function) glMultiTexCoord4iARB },
      { "glMultiTexCoord4ivARB", (gl_function) glMultiTexCoord4ivARB },
      { "glMultiTexCoord4sARB", (gl_function) glMultiTexCoord4sARB },
      { "glMultiTexCoord4svARB", (gl_function) glMultiTexCoord4svARB },

      /* GL_EXT_point_parameters */
      { "glPointParameterfEXT", (gl_function) glPointParameterfEXT },
      { "glPointParameterfvEXT", (gl_function) glPointParameterfvEXT },

      /* GL_INGR_blend_func_separate */
      { "glBlendFuncSeparateINGR", (gl_function) glBlendFuncSeparateINGR },

      /* GL_MESA_window_pos */
      { "glWindowPos2iMESA", (gl_function) glWindowPos2iMESA },
      { "glWindowPos2sMESA", (gl_function) glWindowPos2sMESA },
      { "glWindowPos2fMESA", (gl_function) glWindowPos2fMESA },
      { "glWindowPos2dMESA", (gl_function) glWindowPos2dMESA },
      { "glWindowPos2ivMESA", (gl_function) glWindowPos2ivMESA },
      { "glWindowPos2svMESA", (gl_function) glWindowPos2svMESA },
      { "glWindowPos2fvMESA", (gl_function) glWindowPos2fvMESA },
      { "glWindowPos2dvMESA", (gl_function) glWindowPos2dvMESA },
      { "glWindowPos3iMESA", (gl_function) glWindowPos3iMESA },
      { "glWindowPos3sMESA", (gl_function) glWindowPos3sMESA },
      { "glWindowPos3fMESA", (gl_function) glWindowPos3fMESA },
      { "glWindowPos3dMESA", (gl_function) glWindowPos3dMESA },
      { "glWindowPos3ivMESA", (gl_function) glWindowPos3ivMESA },
      { "glWindowPos3svMESA", (gl_function) glWindowPos3svMESA },
      { "glWindowPos3fvMESA", (gl_function) glWindowPos3fvMESA },
      { "glWindowPos3dvMESA", (gl_function) glWindowPos3dvMESA },
      { "glWindowPos4iMESA", (gl_function) glWindowPos4iMESA },
      { "glWindowPos4sMESA", (gl_function) glWindowPos4sMESA },
      { "glWindowPos4fMESA", (gl_function) glWindowPos4fMESA },
      { "glWindowPos4dMESA", (gl_function) glWindowPos4dMESA },
      { "glWindowPos4ivMESA", (gl_function) glWindowPos4ivMESA },
      { "glWindowPos4svMESA", (gl_function) glWindowPos4svMESA },
      { "glWindowPos4fvMESA", (gl_function) glWindowPos4fvMESA },
      { "glWindowPos4dvMESA", (gl_function) glWindowPos4dvMESA },

      /* GL_MESA_resize_buffers */
      { "glResizeBuffersMESA", (gl_function) glResizeBuffersMESA },

      /* GL_EXT_compiled_vertex_array */
      { "glLockArraysEXT", (gl_function) glLockArraysEXT },
      { "glUnlockArraysEXT", (gl_function) glUnlockArraysEXT },

      { NULL, NULL } /* end of list token */
   };
   GLuint i;

   for (i = 0; procTable[i].address; i++) {
      if (strcmp((const char *) procName, procTable[i].name) == 0)
         return procTable[i].address;
   }

   return NULL;
}
