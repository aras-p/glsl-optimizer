/* $Id: extensions.c,v 1.39 2000/10/30 13:32:00 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
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
   GLint enabled;
   GLboolean *flag;			/* optional flag stored elsewhere */
   char name[MAX_EXT_NAMELEN+1];
   void (*notify)( GLcontext *, GLboolean ); 
};

#define F(x) (int)&(((struct gl_extensions *)0)->x)
#define ON GL_TRUE
#define OFF GL_FALSE

static struct { 
   GLboolean enabled; 
   const char *name; 
   int flag_offset; 
} default_extensions[] = {
   { ON,  "GL_ARB_imaging",                   F(ARB_imaging) },
   { ON,  "GL_ARB_multitexture",              F(ARB_multitexture) },
   { OFF, "GL_ARB_texture_compression",       F(ARB_texture_compression) },
   { OFF, "GL_ARB_texture_cube_map",          F(ARB_texture_cube_map) },
   { ON,  "GL_ARB_texture_env_add",           F(ARB_texture_env_add) },
   { ON,  "GL_ARB_tranpose_matrix",           0 },
   { ON,  "GL_EXT_abgr",                      0 },
   { OFF, "GL_EXT_bgra",                      0 },
   { ON,  "GL_EXT_blend_color",               F(EXT_blend_color) },
   { ON,  "GL_EXT_blend_func_separate",       F(EXT_blend_func_separate) },
   { ON,  "GL_EXT_blend_logic_op",            F(EXT_blend_logic_op) },
   { ON,  "GL_EXT_blend_minmax",              F(EXT_blend_minmax) },
   { ON,  "GL_EXT_blend_subtract",            F(EXT_blend_subtract) },
   { ON,  "GL_EXT_clip_volume_hint",          F(EXT_clip_volume_hint) },
   { OFF, "GL_EXT_cull_vertex",               0 },
   { ON,  "GL_EXT_convolution",               F(EXT_convolution) },
   { ON,  "GL_EXT_compiled_vertex_array",     F(EXT_compiled_vertex_array) },
   { ON,  "GL_EXT_fog_coord",                 F(EXT_fog_coord) },
   { ON,  "GL_EXT_histogram",                 F(EXT_histogram) },
   { ON,  "GL_EXT_packed_pixels",             F(EXT_packed_pixels) },
   { ON,  "GL_EXT_paletted_texture",          F(EXT_paletted_texture) },
   { ON,  "GL_EXT_point_parameters",          F(EXT_point_parameters) },
   { ON,  "GL_EXT_polygon_offset",            F(EXT_polygon_offset) },
   { ON,  "GL_EXT_rescale_normal",            F(EXT_rescale_normal) },
   { ON,  "GL_EXT_secondary_color",           F(EXT_secondary_color) }, 
   { ON,  "GL_EXT_shared_texture_palette",    F(EXT_shared_texture_palette) },
   { ON,  "GL_EXT_stencil_wrap",              F(EXT_stencil_wrap) },
   { ON,  "GL_EXT_texture3D",                 F(EXT_texture3D) },
   { OFF, "GL_EXT_texture_compression_s3tc",  F(EXT_texture_compression_s3tc) },
   { ON,  "GL_EXT_texture_env_add",           F(EXT_texture_env_add) },
   { OFF, "GL_EXT_texture_env_combine",       F(EXT_texture_env_combine) },
   { ON,  "GL_EXT_texture_object",            F(EXT_texture_object) },
   { ON,  "GL_EXT_texture_lod_bias",          F(EXT_texture_lod_bias) },
   { ON,  "GL_EXT_vertex_array",              0 },
   { OFF, "GL_EXT_vertex_array_set",          F(EXT_vertex_array_set) },
   { OFF, "GL_HP_occlusion_test",             F(HP_occlusion_test) },
   { ON,  "GL_INGR_blend_func_separate",      F(INGR_blend_func_separate) },
   { ON,  "GL_MESA_window_pos",               F(MESA_window_pos) },
   { ON,  "GL_MESA_resize_buffers",           F(MESA_resize_buffers) },
   { OFF, "GL_NV_blend_square",               F(NV_blend_square) },
   { ON,  "GL_NV_texgen_reflection",          F(NV_texgen_reflection) },
   { ON,  "GL_PGI_misc_hints",                F(PGI_misc_hints) },
   { ON,  "GL_SGI_color_matrix",              F(SGI_color_matrix) },
   { ON,  "GL_SGI_color_table",               F(SGI_color_table) },
   { ON,  "GL_SGIS_pixel_texture",            F(SGIS_pixel_texture) },
   { ON,  "GL_SGIS_texture_edge_clamp",       F(SGIS_texture_edge_clamp) },
   { ON,  "GL_SGIX_pixel_texture",            F(SGIX_pixel_texture) },
   { OFF, "GL_3DFX_texture_compression_FXT1", F(_3DFX_texture_compression_FXT1) }
};





int gl_extensions_add( GLcontext *ctx, 
		       GLboolean enabled, 
		       const char *name, 
		       GLboolean *flag_ptr )
{
   if (ctx->Extensions.ext_string == 0) 
   {
      struct extension *t = MALLOC_STRUCT(extension);
      t->enabled = enabled;
      strncpy(t->name, name, MAX_EXT_NAMELEN);
      t->name[MAX_EXT_NAMELEN] = 0;
      t->flag = flag_ptr;
      insert_at_tail( ctx->Extensions.ext_list, t );
      return 0;
   }
   return 1;
}


/*
 * Either enable or disable the named extension.
 */
static int set_extension( GLcontext *ctx, const char *name, GLint state )
{
   struct extension *i;
   foreach( i, ctx->Extensions.ext_list ) 
      if (strncmp(i->name, name, MAX_EXT_NAMELEN) == 0) 
	 break;

   if (i == ctx->Extensions.ext_list)
      return 1;

   if (i->flag) *(i->flag) = state;      
   i->enabled = state;
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
   GLboolean *base = (GLboolean *)&ctx->Extensions;

   ctx->Extensions.ext_string = 0;
   ctx->Extensions.ext_list = MALLOC_STRUCT(extension);
   make_empty_list( ctx->Extensions.ext_list );

   for (i = 0 ; i < Elements(default_extensions) ; i++) {
      GLboolean *ptr = 0;

      if (default_extensions[i].flag_offset)
	 ptr = base + default_extensions[i].flag_offset;

      gl_extensions_add( ctx, 
			 default_extensions[i].enabled,
			 default_extensions[i].name,
			 ptr);
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
