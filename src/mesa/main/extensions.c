/* $Id: extensions.c,v 1.1 1999/08/19 00:55:41 jtg Exp $ */

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


