/* $Id: extensions.c,v 1.17 2000/02/23 22:31:35 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "context.h"
#include "extensions.h"
#include "mem.h"
#include "simple_list.h"
#include "types.h"
#endif


#define MAX_EXT_NAMELEN 80

struct extension {
   struct extension *next, *prev;
   int enabled;
   char name[MAX_EXT_NAMELEN+1];
   void (*notify)( GLcontext *, GLboolean ); 
};



static struct { int enabled; const char *name; } default_extensions[] = {
   { DEFAULT_ON,     "GL_EXT_blend_color" },
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
   { DEFAULT_ON,     "GL_EXT_texture_env_add" },
   { ALWAYS_ENABLED, "GL_ARB_tranpose_matrix" },
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
      

/*
 * Test if the named extension is enabled in this context.
 */
GLboolean gl_extension_is_enabled( GLcontext *ctx, const char *name)
{
   struct extension *i;
   foreach( i, ctx->Extensions.ext_list )
      if (strncmp(i->name, name, MAX_EXT_NAMELEN) == 0) {
         if (i->enabled)
            return GL_TRUE;
         else
            return GL_FALSE;
      }

   return GL_FALSE;
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
