/* $Id: extensions.c,v 1.62 2001/06/15 14:18:46 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
#include "mtypes.h"
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
   { OFF, "GL_ARB_imaging",                    F(ARB_imaging) },
   { OFF, "GL_ARB_multisample",                F(ARB_multisample) },
   { OFF, "GL_ARB_multitexture",               F(ARB_multitexture) },
   { OFF, "GL_ARB_texture_border_clamp",       F(ARB_texture_border_clamp) },
   { OFF, "GL_ARB_texture_compression",        F(ARB_texture_compression) },
   { OFF, "GL_ARB_texture_cube_map",           F(ARB_texture_cube_map) },
   { OFF, "GL_ARB_texture_env_add",            F(EXT_texture_env_add) },
   { OFF, "GL_ARB_texture_env_combine",        F(ARB_texture_env_combine) },
   { OFF, "GL_ARB_texture_env_dot3",           F(ARB_texture_env_dot3) },
   { ON,  "GL_ARB_transpose_matrix",           0 },
   { ON,  "GL_EXT_abgr",                       0 },
   { ON,  "GL_EXT_bgra",                       0 },
   { OFF, "GL_EXT_blend_color",                F(EXT_blend_color) },
   { OFF, "GL_EXT_blend_func_separate",        F(EXT_blend_func_separate) },
   { OFF, "GL_EXT_blend_logic_op",             F(EXT_blend_logic_op) },
   { OFF, "GL_EXT_blend_minmax",               F(EXT_blend_minmax) },
   { OFF, "GL_EXT_blend_subtract",             F(EXT_blend_subtract) },
   { ON,  "GL_EXT_clip_volume_hint",           F(EXT_clip_volume_hint) },
   { OFF, "GL_EXT_cull_vertex",                0 },
   { OFF, "GL_EXT_convolution",                F(EXT_convolution) },
   { ON,  "GL_EXT_compiled_vertex_array",      F(EXT_compiled_vertex_array) },
   { OFF, "GL_EXT_fog_coord",                  F(EXT_fog_coord) },
   { OFF, "GL_EXT_histogram",                  F(EXT_histogram) },
   { ON,  "GL_EXT_packed_pixels",              F(EXT_packed_pixels) },
   { OFF, "GL_EXT_paletted_texture",           F(EXT_paletted_texture) },
   { OFF, "GL_EXT_point_parameters",           F(EXT_point_parameters) },
   { ON,  "GL_EXT_polygon_offset",             F(EXT_polygon_offset) },
   { ON,  "GL_EXT_rescale_normal",             F(EXT_rescale_normal) },
   { OFF, "GL_EXT_secondary_color",            F(EXT_secondary_color) },
   { OFF, "GL_EXT_shared_texture_palette",     F(EXT_shared_texture_palette) },
   { OFF, "GL_EXT_stencil_wrap",               F(EXT_stencil_wrap) },
   { ON,  "GL_EXT_texture3D",                  F(EXT_texture3D) },
   { OFF, "GL_EXT_texture_compression_s3tc",   F(EXT_texture_compression_s3tc) },
   { OFF, "GL_EXT_texture_env_add",            F(EXT_texture_env_add) },
   { OFF, "GL_EXT_texture_env_combine",        F(EXT_texture_env_combine) },
   { OFF, "GL_EXT_texture_env_dot3",           F(EXT_texture_env_dot3) },
   { OFF, "GL_EXT_texture_filter_anisotropic", F(EXT_texture_filter_anisotropic) },
   { ON,  "GL_EXT_texture_object",             F(EXT_texture_object) },
   { OFF, "GL_EXT_texture_lod_bias",           F(EXT_texture_lod_bias) },
   { ON,  "GL_EXT_vertex_array",               0 },
   { OFF, "GL_EXT_vertex_array_set",           F(EXT_vertex_array_set) },
   { OFF, "GL_HP_occlusion_test",              F(HP_occlusion_test) },
   { OFF, "GL_INGR_blend_func_separate",       F(INGR_blend_func_separate) },
   { OFF, "GL_MESA_packed_depth_stencil",      0 },
   { OFF, "GL_MESA_resize_buffers",            F(MESA_resize_buffers) },
   { OFF, "GL_MESA_sprite_point",              F(MESA_sprite_point) },
   { ON,  "GL_MESA_window_pos",                F(MESA_window_pos) },
   { OFF, "GL_NV_blend_square",                F(NV_blend_square) },
   { ON,  "GL_NV_texgen_reflection",           F(NV_texgen_reflection) },
   { OFF, "GL_SGI_color_matrix",               F(SGI_color_matrix) },
   { OFF, "GL_SGI_color_table",                F(SGI_color_table) },
   { OFF, "GL_SGIS_generate_mipmap",           F(SGIS_generate_mipmap) },
   { OFF, "GL_SGIS_pixel_texture",             F(SGIS_pixel_texture) },
   { OFF, "GL_SGIS_texture_border_clamp",      F(ARB_texture_border_clamp) },
   { OFF, "GL_SGIS_texture_edge_clamp",        F(SGIS_texture_edge_clamp) },
   { OFF, "GL_SGIX_depth_texture",             F(SGIX_depth_texture) },
   { OFF, "GL_SGIX_pixel_texture",             F(SGIX_pixel_texture) },
   { OFF, "GL_SGIX_shadow",                    F(SGIX_shadow) },
   { OFF, "GL_SGIX_shadow_ambient",            F(SGIX_shadow_ambient) },
   { OFF, "GL_3DFX_texture_compression_FXT1",  F(_3DFX_texture_compression_FXT1) }
};




/*
 * Enable all extensions suitable for a software-only renderer.
 * This is a convenience function used by the XMesa, OSMesa, GGI drivers, etc.
 */
void
_mesa_enable_sw_extensions(GLcontext *ctx)
{
   const char *extensions[] = {
      "GL_ARB_imaging",
      "GL_ARB_multitexture",
      "GL_ARB_texture_border_clamp",
      "GL_ARB_texture_cube_map",
      "GL_ARB_texture_env_add",
      "GL_ARB_texture_env_combine",
      "GL_ARB_texture_env_dot3",
      "GL_EXT_blend_color",
      "GL_EXT_blend_func_separate",
      "GL_EXT_blend_logic_op",
      "GL_EXT_blend_minmax",
      "GL_EXT_blend_subtract",
      "GL_EXT_convolution",
      "GL_EXT_fog_coord",
      "GL_EXT_histogram",
      "GL_EXT_paletted_texture",
      "GL_EXT_point_parameters",
      "GL_EXT_secondary_color",
      "GL_EXT_shared_texture_palette",
      "GL_EXT_stencil_wrap",
      "GL_EXT_texture_env_add",
      "GL_EXT_texture_env_combine",
      "GL_EXT_texture_env_dot3",
      "GL_EXT_texture_lod_bias",
      "GL_HP_occlusion_test",
      "GL_INGR_blend_func_separate",
      "GL_MESA_resize_buffers",
      "GL_NV_blend_square",
      "GL_NV_texgen_reflection",
      "GL_SGI_color_matrix",
      "GL_SGI_color_table",
      "GL_SGIS_generate_mipmap",
      "GL_SGIS_pixel_texture",
      "GL_SGIS_texture_edge_clamp",
      "GL_SGIS_texture_border_clamp",
      "GL_SGIX_depth_texture",
      "GL_SGIX_pixel_texture",
      "GL_SGIX_shadow",
      "GL_SGIX_shadow_ambient",
      NULL
   };
   GLuint i;

   for (i = 0; extensions[i]; i++) {
      _mesa_enable_extension(ctx, extensions[i]);
   }
}


/*
 * Enable GL_ARB_imaging and all the EXT extensions that are subsets of it.
 */
void
_mesa_enable_imaging_extensions(GLcontext *ctx)
{
   const char *extensions[] = {
      "GL_ARB_imaging",
      "GL_EXT_blend_color",
      "GL_EXT_blend_minmax",
      "GL_EXT_blend_subtract",
      "GL_EXT_convolution",
      "GL_EXT_histogram",
      "GL_SGI_color_matrix",
      "GL_SGI_color_table",
      NULL
   };
   GLuint i;

   for (i = 0; extensions[i]; i++) {
      _mesa_enable_extension(ctx, extensions[i]);
   }
}



/*
 * Enable all OpenGL 1.3 features and extensions.
 */
void
_mesa_enable_1_3_extensions(GLcontext *ctx)
{
   const char *extensions[] = {
      "GL_ARB_multisample",
      "GL_ARB_multitexture",
      "GL_ARB_texture_border_clamp",
      "GL_ARB_texture_compression",
      "GL_ARB_texture_cube_map",
      "GL_ARB_texture_env_add",
      "GL_ARB_texture_env_combine",
      "GL_ARB_texture_env_dot3",
      "GL_ARB_transpose_matrix",
      NULL
   };
   GLuint i;

   for (i = 0; extensions[i]; i++) {
      _mesa_enable_extension(ctx, extensions[i]);
   }
}



/*
 * Add a new extenstion.  This would be called from a Mesa driver.
 */
void
_mesa_add_extension( GLcontext *ctx,
                     GLboolean enabled,
                     const char *name,
                     GLboolean *flag_ptr )
{
   /* We should never try to add an extension after
    * _mesa_extensions_get_string() has been called!
    */
   assert(ctx->Extensions.ext_string == 0);

   {
      struct extension *t = MALLOC_STRUCT(extension);
      t->enabled = enabled;
      strncpy(t->name, name, MAX_EXT_NAMELEN);
      t->name[MAX_EXT_NAMELEN] = 0;
      t->flag = flag_ptr;
      if (t->flag)
         *t->flag = enabled;
      insert_at_tail( ctx->Extensions.ext_list, t );
   }
}


/*
 * Either enable or disable the named extension.
 */
static void
set_extension( GLcontext *ctx, const char *name, GLint state )
{
   /* XXX we should assert that ext_string is null.  We should never be
    * enabling/disabling extensions after _mesa_extensions_get_string()
    * has been called!
    */
   struct extension *i;
   foreach( i, ctx->Extensions.ext_list )
      if (strncmp(i->name, name, MAX_EXT_NAMELEN) == 0)
	 break;

   if (i == ctx->Extensions.ext_list) {
      /* this is an error.  Drivers should never try to enable/disable
       * an extension which is unknown to Mesa or wasn't added by calling
       * _mesa_add_extension().
       */
      return;
   }

   if (i->flag)
      *(i->flag) = state;
   i->enabled = state;
}



void
_mesa_enable_extension( GLcontext *ctx, const char *name )
{
   if (ctx->Extensions.ext_string == 0)
      set_extension( ctx, name, 1 );
}


void
_mesa_disable_extension( GLcontext *ctx, const char *name )
{
   if (ctx->Extensions.ext_string == 0)
      set_extension( ctx, name, 0 );
}


/*
 * Test if the named extension is enabled in this context.
 */
GLboolean
_mesa_extension_is_enabled( GLcontext *ctx, const char *name)
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


void
_mesa_extensions_dtr( GLcontext *ctx )
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


void
_mesa_extensions_ctr( GLcontext *ctx )
{
   GLuint i;
   GLboolean *base = (GLboolean *)&ctx->Extensions;

   ctx->Extensions.ext_string = NULL;
   ctx->Extensions.ext_list = MALLOC_STRUCT(extension);
   make_empty_list( ctx->Extensions.ext_list );

   for (i = 0 ; i < Elements(default_extensions) ; i++) {
      GLboolean *ptr = NULL;

      if (default_extensions[i].flag_offset)
	 ptr = base + default_extensions[i].flag_offset;

      (void) _mesa_add_extension( ctx,
                                  default_extensions[i].enabled,
                                  default_extensions[i].name,
                                  ptr);
   }
}


const char *
_mesa_extensions_get_string( GLcontext *ctx )
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
