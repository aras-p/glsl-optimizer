/* $Id: extensions.c,v 1.2 1999/09/11 11:31:34 brianp Exp $ */

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
 * But since the client must also call glGetString(GL_EXTENSIONS) to
 * test for the extension this isn't a big deal.
 */
void *gl_GetProcAddress( GLcontext *ctx, const GLubyte *procName )
{
   struct proc {
      const char *name;
      void *address;
   };
   static struct proc procTable[] = {
      { "glGetProcAddressEXT", glGetProcAddressEXT },  /* myself! */

      /* OpenGL 1.1 functions */
      { "glEnableClientState", glEnableClientState },
      { "glDisableClientState", glDisableClientState },
      { "glPushClientAttrib", glPushClientAttrib },
      { "glPopClientAttrib", glPopClientAttrib },
      { "glIndexub", glIndexub },
      { "glIndexubv", glIndexubv },
      { "glVertexPointer", glVertexPointer },
      { "glNormalPointer", glNormalPointer },
      { "glColorPointer", glColorPointer },
      { "glIndexPointer", glIndexPointer },
      { "glTexCoordPointer", glTexCoordPointer },
      { "glEdgeFlagPointer", glEdgeFlagPointer },
      { "glGetPointerv", glGetPointerv },
      { "glArrayElement", glArrayElement },
      { "glDrawArrays", glDrawArrays },
      { "glDrawElements", glDrawElements },
      { "glInterleavedArrays", glInterleavedArrays },
      { "glGenTextures", glGenTextures },
      { "glDeleteTextures", glDeleteTextures },
      { "glBindTexture", glBindTexture },
      { "glPrioritizeTextures", glPrioritizeTextures },
      { "glAreTexturesResident", glAreTexturesResident },
      { "glIsTexture", glIsTexture },
      { "glTexSubImage1D", glTexSubImage1D },
      { "glTexSubImage2D", glTexSubImage2D },
      { "glCopyTexImage1D", glCopyTexImage1D },
      { "glCopyTexImage2D", glCopyTexImage2D },
      { "glCopyTexSubImage1D", glCopyTexSubImage1D },
      { "glCopyTexSubImage2D", glCopyTexSubImage2D },

      /* OpenGL 1.2 functions */
      { "glDrawRangeElements", glDrawRangeElements },
      { "glTexImage3D", glTexImage3D },
      { "glTexSubImage3D", glTexSubImage3D },
      { "glCopyTexSubImage3D", glCopyTexSubImage3D },
      /* NOTE: 1.2 imaging subset functions not implemented in Mesa */

      /* GL_EXT_blend_minmax */
      { "glBlendEquationEXT", glBlendEquationEXT },

      /* GL_EXT_blend_color */
      { "glBlendColorEXT", glBlendColorEXT },

      /* GL_EXT_polygon_offset */
      { "glPolygonOffsetEXT", glPolygonOffsetEXT },

      /* GL_EXT_vertex_arrays */
      { "glVertexPointerEXT", glVertexPointerEXT },
      { "glNormalPointerEXT", glNormalPointerEXT },
      { "glColorPointerEXT", glColorPointerEXT },
      { "glIndexPointerEXT", glIndexPointerEXT },
      { "glTexCoordPointerEXT", glTexCoordPointerEXT },
      { "glEdgeFlagPointerEXT", glEdgeFlagPointerEXT },
      { "glGetPointervEXT", glGetPointervEXT },
      { "glArrayElementEXT", glArrayElementEXT },
      { "glDrawArraysEXT", glDrawArraysEXT },

      /* GL_EXT_texture_object */
      { "glGenTexturesEXT", glGenTexturesEXT },
      { "glDeleteTexturesEXT", glDeleteTexturesEXT },
      { "glBindTextureEXT", glBindTextureEXT },
      { "glPrioritizeTexturesEXT", glPrioritizeTexturesEXT },
      { "glAreTexturesResidentEXT", glAreTexturesResidentEXT },
      { "glIsTextureEXT", glIsTextureEXT },

      /* GL_EXT_texture3D */
      { "glTexImage3DEXT", glTexImage3DEXT },
      { "glTexSubImage3DEXT", glTexSubImage3DEXT },
      { "glCopyTexSubImage3DEXT", glCopyTexSubImage3DEXT },

      /* GL_EXT_color_table */
      { "glColorTableEXT", glColorTableEXT },
      { "glColorSubTableEXT", glColorSubTableEXT },
      { "glGetColorTableEXT", glGetColorTableEXT },
      { "glGetColorTableParameterfvEXT", glGetColorTableParameterfvEXT },
      { "glGetColorTableParameterivEXT", glGetColorTableParameterivEXT },

      /* GL_ARB_multitexture */
      { "glActiveTextureARB", glActiveTextureARB },
      { "glClientActiveTextureARB", glClientActiveTextureARB },
      { "glMultiTexCoord1dARB", glMultiTexCoord1dARB },
      { "glMultiTexCoord1dvARB", glMultiTexCoord1dvARB },
      { "glMultiTexCoord1fARB", glMultiTexCoord1fARB },
      { "glMultiTexCoord1fvARB", glMultiTexCoord1fvARB },
      { "glMultiTexCoord1iARB", glMultiTexCoord1iARB },
      { "glMultiTexCoord1ivARB", glMultiTexCoord1ivARB },
      { "glMultiTexCoord1sARB", glMultiTexCoord1sARB },
      { "glMultiTexCoord1svARB", glMultiTexCoord1svARB },
      { "glMultiTexCoord2dARB", glMultiTexCoord2dARB },
      { "glMultiTexCoord2dvARB", glMultiTexCoord2dvARB },
      { "glMultiTexCoord2fARB", glMultiTexCoord2fARB },
      { "glMultiTexCoord2fvARB", glMultiTexCoord2fvARB },
      { "glMultiTexCoord2iARB", glMultiTexCoord2iARB },
      { "glMultiTexCoord2ivARB", glMultiTexCoord2ivARB },
      { "glMultiTexCoord2sARB", glMultiTexCoord2sARB },
      { "glMultiTexCoord2svARB", glMultiTexCoord2svARB },
      { "glMultiTexCoord3dARB", glMultiTexCoord3dARB },
      { "glMultiTexCoord3dvARB", glMultiTexCoord3dvARB },
      { "glMultiTexCoord3fARB", glMultiTexCoord3fARB },
      { "glMultiTexCoord3fvARB", glMultiTexCoord3fvARB },
      { "glMultiTexCoord3iARB", glMultiTexCoord3iARB },
      { "glMultiTexCoord3ivARB", glMultiTexCoord3ivARB },
      { "glMultiTexCoord3sARB", glMultiTexCoord3sARB },
      { "glMultiTexCoord3svARB", glMultiTexCoord3svARB },
      { "glMultiTexCoord4dARB", glMultiTexCoord4dARB },
      { "glMultiTexCoord4dvARB", glMultiTexCoord4dvARB },
      { "glMultiTexCoord4fARB", glMultiTexCoord4fARB },
      { "glMultiTexCoord4fvARB", glMultiTexCoord4fvARB },
      { "glMultiTexCoord4iARB", glMultiTexCoord4iARB },
      { "glMultiTexCoord4ivARB", glMultiTexCoord4ivARB },
      { "glMultiTexCoord4sARB", glMultiTexCoord4sARB },
      { "glMultiTexCoord4svARB", glMultiTexCoord4svARB },

      /* GL_EXT_point_parameters */
      { "glPointParameterfEXT", glPointParameterfEXT },
      { "glPointParameterfvEXT", glPointParameterfvEXT },

      /* GL_INGR_blend_func_separate */
      { "glBlendFuncSeparateINGR", glBlendFuncSeparateINGR },

      /* GL_MESA_window_pos */
      { "glWindowPos2iMESA", glWindowPos2iMESA },
      { "glWindowPos2sMESA", glWindowPos2sMESA },
      { "glWindowPos2fMESA", glWindowPos2fMESA },
      { "glWindowPos2dMESA", glWindowPos2dMESA },
      { "glWindowPos2ivMESA", glWindowPos2ivMESA },
      { "glWindowPos2svMESA", glWindowPos2svMESA },
      { "glWindowPos2fvMESA", glWindowPos2fvMESA },
      { "glWindowPos2dvMESA", glWindowPos2dvMESA },
      { "glWindowPos3iMESA", glWindowPos3iMESA },
      { "glWindowPos3sMESA", glWindowPos3sMESA },
      { "glWindowPos3fMESA", glWindowPos3fMESA },
      { "glWindowPos3dMESA", glWindowPos3dMESA },
      { "glWindowPos3ivMESA", glWindowPos3ivMESA },
      { "glWindowPos3svMESA", glWindowPos3svMESA },
      { "glWindowPos3fvMESA", glWindowPos3fvMESA },
      { "glWindowPos3dvMESA", glWindowPos3dvMESA },
      { "glWindowPos4iMESA", glWindowPos4iMESA },
      { "glWindowPos4sMESA", glWindowPos4sMESA },
      { "glWindowPos4fMESA", glWindowPos4fMESA },
      { "glWindowPos4dMESA", glWindowPos4dMESA },
      { "glWindowPos4ivMESA", glWindowPos4ivMESA },
      { "glWindowPos4svMESA", glWindowPos4svMESA },
      { "glWindowPos4fvMESA", glWindowPos4fvMESA },
      { "glWindowPos4dvMESA", glWindowPos4dvMESA },

      /* GL_MESA_resize_buffers */
      { "glResizeBuffersMESA", glResizeBuffersMESA },

      /* GL_EXT_compiled_vertex_array */
      { "glLockArraysEXT", glLockArraysEXT },
      { "glUnlockArraysEXT", glUnlockArraysEXT },

      { NULL, NULL } /* end of list token */
   };
   GLuint i;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, "glGetProcAddressEXT", NULL);

   /* First, look for core library functions */
   for (i = 0; procTable[i].address; i++) {
      if (strcmp((const char *) procName, procTable[i].name) == 0)
	 return procTable[i].address;
   }

   return NULL;
}
