/* $Id: extensions.c,v 1.4 1999/09/16 16:47:35 brianp Exp $ */

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


#include <stdlib.h>
#include "context.h"
#include "extensions.h"
#include "simple_list.h"
#include "types.h"


#define MAX_EXT_NAMELEN 80
#define MALLOC_STRUCT(T)  (struct T *) malloc( sizeof(struct T) )

struct extension {
   struct extension *next, *prev;
   int enabled;
   char name[MAX_EXT_NAMELEN+1];
   void (*notify)( GLcontext *, GLboolean ); 
};



static struct { int enabled; const char *name; } default_extensions[] = {
   { ALWAYS_ENABLED, "GL_EXT_blend_color" },
   { ALWAYS_ENABLED, "GL_EXT_blend_minmax" },
   { ALWAYS_ENABLED, "GL_EXT_blend_logic_op" },
   { ALWAYS_ENABLED, "GL_EXT_blend_subtract" },
   { ALWAYS_ENABLED, "GL_EXT_paletted_texture" },
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
   { ALWAYS_ENABLED, "GL_INGR_blend_func_separate" },
   { DEFAULT_ON,     "GL_ARB_multitexture" },
   { ALWAYS_ENABLED, "GL_NV_texgen_reflection" },
   { DEFAULT_ON,     "GL_PGI_misc_hints" },
   { DEFAULT_ON,     "GL_EXT_compiled_vertex_array" },
   { DEFAULT_OFF,    "GL_EXT_vertex_array_set" },
   { DEFAULT_ON,     "GL_EXT_clip_volume_hint" },
   { ALWAYS_ENABLED, "GL_EXT_get_proc_address" }
};


int gl_extensions_add( GLcontext *ctx, 
		       int state, 
		       const char *name, 
		       void (*notify)() )
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
      free( ctx->Extensions.ext_string );
      ctx->Extensions.ext_string = 0;
   }

   if (ctx->Extensions.ext_list) {
      foreach_s( i, nexti, ctx->Extensions.ext_list ) {
	 free( i );
      }
   
      free(ctx->Extensions.ext_list);
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

      str = (char *)malloc(len * sizeof(char));
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
 * NOTE: this function could be optimized to binary search a sorted
 * list of function names.
 * Also, this function does not yet do per-context function searches.
 * Not applicable to Mesa at this time.
 */
GLfunction gl_GetProcAddress( GLcontext *ctx, const GLubyte *procName )
{
   struct proc {
      const char *name;
      GLfunction address;
   };
   static struct proc procTable[] = {
#ifdef GL_EXT_get_proc_address
      { "glGetProcAddressEXT", (GLfunction) glGetProcAddressEXT },  /* me! */
#endif
      /* OpenGL 1.1 functions */
      { "glEnableClientState", (GLfunction) glEnableClientState },
      { "glDisableClientState", (GLfunction) glDisableClientState },
      { "glPushClientAttrib", (GLfunction) glPushClientAttrib },
      { "glPopClientAttrib", (GLfunction) glPopClientAttrib },
      { "glIndexub", (GLfunction) glIndexub },
      { "glIndexubv", (GLfunction) glIndexubv },
      { "glVertexPointer", (GLfunction) glVertexPointer },
      { "glNormalPointer", (GLfunction) glNormalPointer },
      { "glColorPointer", (GLfunction) glColorPointer },
      { "glIndexPointer", (GLfunction) glIndexPointer },
      { "glTexCoordPointer", (GLfunction) glTexCoordPointer },
      { "glEdgeFlagPointer", (GLfunction) glEdgeFlagPointer },
      { "glGetPointerv", (GLfunction) glGetPointerv },
      { "glArrayElement", (GLfunction) glArrayElement },
      { "glDrawArrays", (GLfunction) glDrawArrays },
      { "glDrawElements", (GLfunction) glDrawElements },
      { "glInterleavedArrays", (GLfunction) glInterleavedArrays },
      { "glGenTextures", (GLfunction) glGenTextures },
      { "glDeleteTextures", (GLfunction) glDeleteTextures },
      { "glBindTexture", (GLfunction) glBindTexture },
      { "glPrioritizeTextures", (GLfunction) glPrioritizeTextures },
      { "glAreTexturesResident", (GLfunction) glAreTexturesResident },
      { "glIsTexture", (GLfunction) glIsTexture },
      { "glTexSubImage1D", (GLfunction) glTexSubImage1D },
      { "glTexSubImage2D", (GLfunction) glTexSubImage2D },
      { "glCopyTexImage1D", (GLfunction) glCopyTexImage1D },
      { "glCopyTexImage2D", (GLfunction) glCopyTexImage2D },
      { "glCopyTexSubImage1D", (GLfunction) glCopyTexSubImage1D },
      { "glCopyTexSubImage2D", (GLfunction) glCopyTexSubImage2D },

      /* OpenGL 1.2 functions */
      { "glDrawRangeElements", (GLfunction) glDrawRangeElements },
      { "glTexImage3D", (GLfunction) glTexImage3D },
      { "glTexSubImage3D", (GLfunction) glTexSubImage3D },
      { "glCopyTexSubImage3D", (GLfunction) glCopyTexSubImage3D },
      /* NOTE: 1.2 imaging subset functions not implemented in Mesa */

      /* GL_EXT_blend_minmax */
      { "glBlendEquationEXT", (GLfunction) glBlendEquationEXT },

      /* GL_EXT_blend_color */
      { "glBlendColorEXT", (GLfunction) glBlendColorEXT },

      /* GL_EXT_polygon_offset */
      { "glPolygonOffsetEXT", (GLfunction) glPolygonOffsetEXT },

      /* GL_EXT_vertex_arrays */
      { "glVertexPointerEXT", (GLfunction) glVertexPointerEXT },
      { "glNormalPointerEXT", (GLfunction) glNormalPointerEXT },
      { "glColorPointerEXT", (GLfunction) glColorPointerEXT },
      { "glIndexPointerEXT", (GLfunction) glIndexPointerEXT },
      { "glTexCoordPointerEXT", (GLfunction) glTexCoordPointerEXT },
      { "glEdgeFlagPointerEXT", (GLfunction) glEdgeFlagPointerEXT },
      { "glGetPointervEXT", (GLfunction) glGetPointervEXT },
      { "glArrayElementEXT", (GLfunction) glArrayElementEXT },
      { "glDrawArraysEXT", (GLfunction) glDrawArraysEXT },

      /* GL_EXT_texture_object */
      { "glGenTexturesEXT", (GLfunction) glGenTexturesEXT },
      { "glDeleteTexturesEXT", (GLfunction) glDeleteTexturesEXT },
      { "glBindTextureEXT", (GLfunction) glBindTextureEXT },
      { "glPrioritizeTexturesEXT", (GLfunction) glPrioritizeTexturesEXT },
      { "glAreTexturesResidentEXT", (GLfunction) glAreTexturesResidentEXT },
      { "glIsTextureEXT", (GLfunction) glIsTextureEXT },

      /* GL_EXT_texture3D */
      { "glTexImage3DEXT", (GLfunction) glTexImage3DEXT },
      { "glTexSubImage3DEXT", (GLfunction) glTexSubImage3DEXT },
      { "glCopyTexSubImage3DEXT", (GLfunction) glCopyTexSubImage3DEXT },

      /* GL_EXT_color_table */
      { "glColorTableEXT", (GLfunction) glColorTableEXT },
      { "glColorSubTableEXT", (GLfunction) glColorSubTableEXT },
      { "glGetColorTableEXT", (GLfunction) glGetColorTableEXT },
      { "glGetColorTableParameterfvEXT", (GLfunction) glGetColorTableParameterfvEXT },
      { "glGetColorTableParameterivEXT", (GLfunction) glGetColorTableParameterivEXT },

      /* GL_ARB_multitexture */
      { "glActiveTextureARB", (GLfunction) glActiveTextureARB },
      { "glClientActiveTextureARB", (GLfunction) glClientActiveTextureARB },
      { "glMultiTexCoord1dARB", (GLfunction) glMultiTexCoord1dARB },
      { "glMultiTexCoord1dvARB", (GLfunction) glMultiTexCoord1dvARB },
      { "glMultiTexCoord1fARB", (GLfunction) glMultiTexCoord1fARB },
      { "glMultiTexCoord1fvARB", (GLfunction) glMultiTexCoord1fvARB },
      { "glMultiTexCoord1iARB", (GLfunction) glMultiTexCoord1iARB },
      { "glMultiTexCoord1ivARB", (GLfunction) glMultiTexCoord1ivARB },
      { "glMultiTexCoord1sARB", (GLfunction) glMultiTexCoord1sARB },
      { "glMultiTexCoord1svARB", (GLfunction) glMultiTexCoord1svARB },
      { "glMultiTexCoord2dARB", (GLfunction) glMultiTexCoord2dARB },
      { "glMultiTexCoord2dvARB", (GLfunction) glMultiTexCoord2dvARB },
      { "glMultiTexCoord2fARB", (GLfunction) glMultiTexCoord2fARB },
      { "glMultiTexCoord2fvARB", (GLfunction) glMultiTexCoord2fvARB },
      { "glMultiTexCoord2iARB", (GLfunction) glMultiTexCoord2iARB },
      { "glMultiTexCoord2ivARB", (GLfunction) glMultiTexCoord2ivARB },
      { "glMultiTexCoord2sARB", (GLfunction) glMultiTexCoord2sARB },
      { "glMultiTexCoord2svARB", (GLfunction) glMultiTexCoord2svARB },
      { "glMultiTexCoord3dARB", (GLfunction) glMultiTexCoord3dARB },
      { "glMultiTexCoord3dvARB", (GLfunction) glMultiTexCoord3dvARB },
      { "glMultiTexCoord3fARB", (GLfunction) glMultiTexCoord3fARB },
      { "glMultiTexCoord3fvARB", (GLfunction) glMultiTexCoord3fvARB },
      { "glMultiTexCoord3iARB", (GLfunction) glMultiTexCoord3iARB },
      { "glMultiTexCoord3ivARB", (GLfunction) glMultiTexCoord3ivARB },
      { "glMultiTexCoord3sARB", (GLfunction) glMultiTexCoord3sARB },
      { "glMultiTexCoord3svARB", (GLfunction) glMultiTexCoord3svARB },
      { "glMultiTexCoord4dARB", (GLfunction) glMultiTexCoord4dARB },
      { "glMultiTexCoord4dvARB", (GLfunction) glMultiTexCoord4dvARB },
      { "glMultiTexCoord4fARB", (GLfunction) glMultiTexCoord4fARB },
      { "glMultiTexCoord4fvARB", (GLfunction) glMultiTexCoord4fvARB },
      { "glMultiTexCoord4iARB", (GLfunction) glMultiTexCoord4iARB },
      { "glMultiTexCoord4ivARB", (GLfunction) glMultiTexCoord4ivARB },
      { "glMultiTexCoord4sARB", (GLfunction) glMultiTexCoord4sARB },
      { "glMultiTexCoord4svARB", (GLfunction) glMultiTexCoord4svARB },

      /* GL_EXT_point_parameters */
      { "glPointParameterfEXT", (GLfunction) glPointParameterfEXT },
      { "glPointParameterfvEXT", (GLfunction) glPointParameterfvEXT },

      /* GL_INGR_blend_func_separate */
      { "glBlendFuncSeparateINGR", (GLfunction) glBlendFuncSeparateINGR },

      /* GL_MESA_window_pos */
      { "glWindowPos2iMESA", (GLfunction) glWindowPos2iMESA },
      { "glWindowPos2sMESA", (GLfunction) glWindowPos2sMESA },
      { "glWindowPos2fMESA", (GLfunction) glWindowPos2fMESA },
      { "glWindowPos2dMESA", (GLfunction) glWindowPos2dMESA },
      { "glWindowPos2ivMESA", (GLfunction) glWindowPos2ivMESA },
      { "glWindowPos2svMESA", (GLfunction) glWindowPos2svMESA },
      { "glWindowPos2fvMESA", (GLfunction) glWindowPos2fvMESA },
      { "glWindowPos2dvMESA", (GLfunction) glWindowPos2dvMESA },
      { "glWindowPos3iMESA", (GLfunction) glWindowPos3iMESA },
      { "glWindowPos3sMESA", (GLfunction) glWindowPos3sMESA },
      { "glWindowPos3fMESA", (GLfunction) glWindowPos3fMESA },
      { "glWindowPos3dMESA", (GLfunction) glWindowPos3dMESA },
      { "glWindowPos3ivMESA", (GLfunction) glWindowPos3ivMESA },
      { "glWindowPos3svMESA", (GLfunction) glWindowPos3svMESA },
      { "glWindowPos3fvMESA", (GLfunction) glWindowPos3fvMESA },
      { "glWindowPos3dvMESA", (GLfunction) glWindowPos3dvMESA },
      { "glWindowPos4iMESA", (GLfunction) glWindowPos4iMESA },
      { "glWindowPos4sMESA", (GLfunction) glWindowPos4sMESA },
      { "glWindowPos4fMESA", (GLfunction) glWindowPos4fMESA },
      { "glWindowPos4dMESA", (GLfunction) glWindowPos4dMESA },
      { "glWindowPos4ivMESA", (GLfunction) glWindowPos4ivMESA },
      { "glWindowPos4svMESA", (GLfunction) glWindowPos4svMESA },
      { "glWindowPos4fvMESA", (GLfunction) glWindowPos4fvMESA },
      { "glWindowPos4dvMESA", (GLfunction) glWindowPos4dvMESA },

      /* GL_MESA_resize_buffers */
      { "glResizeBuffersMESA", (GLfunction) glResizeBuffersMESA },

      /* GL_EXT_compiled_vertex_array */
      { "glLockArraysEXT", (GLfunction) glLockArraysEXT },
      { "glUnlockArraysEXT", (GLfunction) glUnlockArraysEXT },

      { NULL, NULL } /* end of list token */
   };
   GLuint i;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, "glGetProcAddressEXT", NULL);

   for (i = 0; procTable[i].address; i++) {
      if (strcmp((const char *) procName, procTable[i].name) == 0)
         return procTable[i].address;
   }

   return NULL;
}
