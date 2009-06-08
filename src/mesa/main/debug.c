/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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

#include "mtypes.h"
#include "colormac.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "debug.h"
#include "get.h"
#include "texobj.h"
#include "texformat.h"


/**
 * Primitive names
 */
const char *_mesa_prim_name[GL_POLYGON+4] = {
   "GL_POINTS",
   "GL_LINES",
   "GL_LINE_LOOP",
   "GL_LINE_STRIP",
   "GL_TRIANGLES",
   "GL_TRIANGLE_STRIP",
   "GL_TRIANGLE_FAN",
   "GL_QUADS",
   "GL_QUAD_STRIP",
   "GL_POLYGON",
   "outside begin/end",
   "inside unkown primitive",
   "unknown state"
};

void
_mesa_print_state( const char *msg, GLuint state )
{
   _mesa_debug(NULL,
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	   msg,
	   state,
	   (state & _NEW_MODELVIEW)       ? "ctx->ModelView, " : "",
	   (state & _NEW_PROJECTION)      ? "ctx->Projection, " : "",
	   (state & _NEW_TEXTURE_MATRIX)  ? "ctx->TextureMatrix, " : "",
	   (state & _NEW_COLOR_MATRIX)    ? "ctx->ColorMatrix, " : "",
	   (state & _NEW_ACCUM)           ? "ctx->Accum, " : "",
	   (state & _NEW_COLOR)           ? "ctx->Color, " : "",
	   (state & _NEW_DEPTH)           ? "ctx->Depth, " : "",
	   (state & _NEW_EVAL)            ? "ctx->Eval/EvalMap, " : "",
	   (state & _NEW_FOG)             ? "ctx->Fog, " : "",
	   (state & _NEW_HINT)            ? "ctx->Hint, " : "",
	   (state & _NEW_LIGHT)           ? "ctx->Light, " : "",
	   (state & _NEW_LINE)            ? "ctx->Line, " : "",
	   (state & _NEW_PIXEL)           ? "ctx->Pixel, " : "",
	   (state & _NEW_POINT)           ? "ctx->Point, " : "",
	   (state & _NEW_POLYGON)         ? "ctx->Polygon, " : "",
	   (state & _NEW_POLYGONSTIPPLE)  ? "ctx->PolygonStipple, " : "",
	   (state & _NEW_SCISSOR)         ? "ctx->Scissor, " : "",
	   (state & _NEW_TEXTURE)         ? "ctx->Texture, " : "",
	   (state & _NEW_TRANSFORM)       ? "ctx->Transform, " : "",
	   (state & _NEW_VIEWPORT)        ? "ctx->Viewport, " : "",
	   (state & _NEW_PACKUNPACK)      ? "ctx->Pack/Unpack, " : "",
	   (state & _NEW_ARRAY)           ? "ctx->Array, " : "",
	   (state & _NEW_RENDERMODE)      ? "ctx->RenderMode, " : "",
	   (state & _NEW_BUFFERS)         ? "ctx->Visual, ctx->DrawBuffer,, " : "");
}



void
_mesa_print_tri_caps( const char *name, GLuint flags )
{
   _mesa_debug(NULL,
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	   name,
	   flags,
	   (flags & DD_FLATSHADE)           ? "flat-shade, " : "",
	   (flags & DD_SEPARATE_SPECULAR)   ? "separate-specular, " : "",
	   (flags & DD_TRI_LIGHT_TWOSIDE)   ? "tri-light-twoside, " : "",
	   (flags & DD_TRI_TWOSTENCIL)      ? "tri-twostencil, " : "",
	   (flags & DD_TRI_UNFILLED)        ? "tri-unfilled, " : "",
	   (flags & DD_TRI_STIPPLE)         ? "tri-stipple, " : "",
	   (flags & DD_TRI_OFFSET)          ? "tri-offset, " : "",
	   (flags & DD_TRI_SMOOTH)          ? "tri-smooth, " : "",
	   (flags & DD_LINE_SMOOTH)         ? "line-smooth, " : "",
	   (flags & DD_LINE_STIPPLE)        ? "line-stipple, " : "",
	   (flags & DD_LINE_WIDTH)          ? "line-wide, " : "",
	   (flags & DD_POINT_SMOOTH)        ? "point-smooth, " : "",
	   (flags & DD_POINT_SIZE)          ? "point-size, " : "",
	   (flags & DD_POINT_ATTEN)         ? "point-atten, " : "",
	   (flags & DD_TRI_CULL_FRONT_BACK) ? "cull-all, " : ""
      );
}


/**
 * Print information about this Mesa version and build options.
 */
void _mesa_print_info( void )
{
   _mesa_debug(NULL, "Mesa GL_VERSION = %s\n",
	   (char *) _mesa_GetString(GL_VERSION));
   _mesa_debug(NULL, "Mesa GL_RENDERER = %s\n",
	   (char *) _mesa_GetString(GL_RENDERER));
   _mesa_debug(NULL, "Mesa GL_VENDOR = %s\n",
	   (char *) _mesa_GetString(GL_VENDOR));
   _mesa_debug(NULL, "Mesa GL_EXTENSIONS = %s\n",
	   (char *) _mesa_GetString(GL_EXTENSIONS));
#if defined(THREADS)
   _mesa_debug(NULL, "Mesa thread-safe: YES\n");
#else
   _mesa_debug(NULL, "Mesa thread-safe: NO\n");
#endif
#if defined(USE_X86_ASM)
   _mesa_debug(NULL, "Mesa x86-optimized: YES\n");
#else
   _mesa_debug(NULL, "Mesa x86-optimized: NO\n");
#endif
#if defined(USE_SPARC_ASM)
   _mesa_debug(NULL, "Mesa sparc-optimized: YES\n");
#else
   _mesa_debug(NULL, "Mesa sparc-optimized: NO\n");
#endif
}


/**
 * Set the debugging flags.
 *
 * \param debug debug string
 *
 * If compiled with debugging support then search for keywords in \p debug and
 * enables the verbose debug output of the respective feature.
 */
static void add_debug_flags( const char *debug )
{
#ifdef DEBUG
   struct debug_option {
      const char *name;
      GLbitfield flag;
   };
   static const struct debug_option debug_opt[] = {
      { "varray",    VERBOSE_VARRAY },
      { "tex",       VERBOSE_TEXTURE },
      { "imm",       VERBOSE_IMMEDIATE },
      { "pipe",      VERBOSE_PIPELINE },
      { "driver",    VERBOSE_DRIVER },
      { "state",     VERBOSE_STATE },
      { "api",       VERBOSE_API },
      { "list",      VERBOSE_DISPLAY_LIST },
      { "lighting",  VERBOSE_LIGHTING },
      { "disassem",  VERBOSE_DISASSEM }
   };
   GLuint i;

   MESA_VERBOSE = 0x0;
   for (i = 0; i < Elements(debug_opt); i++) {
      if (_mesa_strstr(debug, debug_opt[i].name))
         MESA_VERBOSE |= debug_opt[i].flag;
   }

   /* Debug flag:
    */
   if (_mesa_strstr(debug, "flush")) 
      MESA_DEBUG_FLAGS |= DEBUG_ALWAYS_FLUSH;

#if defined(_FPU_GETCW) && defined(_FPU_SETCW)
   if (_mesa_strstr(debug, "fpexceptions")) {
      /* raise FP exceptions */
      fpu_control_t mask;
      _FPU_GETCW(mask);
      mask &= ~(_FPU_MASK_IM | _FPU_MASK_DM | _FPU_MASK_ZM
                | _FPU_MASK_OM | _FPU_MASK_UM);
      _FPU_SETCW(mask);
   }
#endif

#else
   (void) debug;
#endif
}


void 
_mesa_init_debug( GLcontext *ctx )
{
   char *c;

   /* Dither disable */
   ctx->NoDither = _mesa_getenv("MESA_NO_DITHER") ? GL_TRUE : GL_FALSE;
   if (ctx->NoDither) {
      if (_mesa_getenv("MESA_DEBUG")) {
         _mesa_debug(ctx, "MESA_NO_DITHER set - dithering disabled\n");
      }
      ctx->Color.DitherFlag = GL_FALSE;
   }

   c = _mesa_getenv("MESA_DEBUG");
   if (c)
      add_debug_flags(c);

   c = _mesa_getenv("MESA_VERBOSE");
   if (c)
      add_debug_flags(c);
}


/*
 * Write ppm file
 */
static void
write_ppm(const char *filename, const GLubyte *buffer, int width, int height,
          int comps, int rcomp, int gcomp, int bcomp)
{
   FILE *f = fopen( filename, "w" );
   if (f) {
      int i, x, y;
      const GLubyte *ptr = buffer;
      fprintf(f,"P6\n");
      fprintf(f,"# ppm-file created by osdemo.c\n");
      fprintf(f,"%i %i\n", width,height);
      fprintf(f,"255\n");
      fclose(f);
      f = fopen( filename, "ab" );  /* reopen in binary append mode */
      for (y=height-1; y>=0; y--) {
         for (x=0; x<width; x++) {
            i = (y*width + x) * comps;
            fputc(ptr[i+rcomp], f);   /* write red */
            fputc(ptr[i+gcomp], f); /* write green */
            fputc(ptr[i+bcomp], f); /* write blue */
         }
      }
      fclose(f);
   }
}


/**
 * Write level[0] image to a ppm file.
 */
static void
write_texture_image(struct gl_texture_object *texObj)
{
   const struct gl_texture_image *img = texObj->Image[0][0];
   if (img && img->Data) {
      char s[100];

      /* make filename */
      sprintf(s, "/tmp/teximage%u.ppm", texObj->Name);

      switch (img->TexFormat->MesaFormat) {
      case MESA_FORMAT_RGBA8888:
         write_ppm(s, img->Data, img->Width, img->Height, 4, 3, 2, 1);
         break;
      case MESA_FORMAT_ARGB8888:
         write_ppm(s, img->Data, img->Width, img->Height, 4, 2, 1, 0);
         break;
      case MESA_FORMAT_RGB888:
         write_ppm(s, img->Data, img->Width, img->Height, 3, 2, 1, 0);
         break;
      case MESA_FORMAT_RGB565:
         {
            GLubyte *buf2 = (GLubyte *) _mesa_malloc(img->Width * img->Height * 3);
            GLuint i;
            for (i = 0; i < img->Width * img->Height; i++) {
               GLint r, g, b;
               GLushort s = ((GLushort *) img->Data)[i];
               r = UBYTE_TO_CHAN( ((s >> 8) & 0xf8) | ((s >> 13) & 0x7) );
               g = UBYTE_TO_CHAN( ((s >> 3) & 0xfc) | ((s >>  9) & 0x3) );
               b = UBYTE_TO_CHAN( ((s << 3) & 0xf8) | ((s >>  2) & 0x7) );
               buf2[i*3+1] = r;
               buf2[i*3+2] = g;
               buf2[i*3+3] = b;
            }
            write_ppm(s, buf2, img->Width, img->Height, 3, 2, 1, 0);
            _mesa_free(buf2);
         }
         break;
      default:
         printf("XXXX unsupported mesa tex format %d in %s\n",
                img->TexFormat->MesaFormat, __FUNCTION__);
      }
   }
}


static GLboolean DumpImages;


static void
dump_texture_cb(GLuint id, void *data, void *userData)
{
   struct gl_texture_object *texObj = (struct gl_texture_object *) data;
   int i;
   (void) userData;

   printf("Texture %u\n", texObj->Name);
   printf("  Target 0x%x\n", texObj->Target);
   for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
      struct gl_texture_image *texImg = texObj->Image[0][i];
      if (texImg) {
         printf("  Image %u: %d x %d x %d at %p\n", i,
                texImg->Width, texImg->Height, texImg->Depth, texImg->Data);
         if (DumpImages && i == 0) {
            write_texture_image(texObj);
         }
      }
   }
}


/**
 * Print basic info about all texture objext to stdout.
 * If dumpImages is true, write PPM of level[0] image to a file.
 */
void
_mesa_dump_textures(GLboolean dumpImages)
{
   GET_CURRENT_CONTEXT(ctx);
   DumpImages = dumpImages;
   _mesa_HashWalk(ctx->Shared->TexObjects, dump_texture_cb, ctx);
}
