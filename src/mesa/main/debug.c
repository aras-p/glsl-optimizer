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
#include "attrib.h"
#include "colormac.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "debug.h"
#include "get.h"
#include "pixelstore.h"
#include "readpix.h"
#include "texgetimage.h"
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
          int comps, int rcomp, int gcomp, int bcomp, GLboolean invert)
{
   FILE *f = fopen( filename, "w" );
   if (f) {
      int x, y;
      const GLubyte *ptr = buffer;
      fprintf(f,"P6\n");
      fprintf(f,"# ppm-file created by osdemo.c\n");
      fprintf(f,"%i %i\n", width,height);
      fprintf(f,"255\n");
      fclose(f);
      f = fopen( filename, "ab" );  /* reopen in binary append mode */
      for (y=0; y < height; y++) {
         for (x = 0; x < width; x++) {
            int yy = invert ? (height - 1 - y) : y;
            int i = (yy * width + x) * comps;
            fputc(ptr[i+rcomp], f); /* write red */
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
write_texture_image(struct gl_texture_object *texObj, GLuint face, GLuint level)
{
   struct gl_texture_image *img = texObj->Image[face][level];
   if (img) {
      GET_CURRENT_CONTEXT(ctx);
      struct gl_pixelstore_attrib store;
      GLubyte *buffer;
      char s[100];

      buffer = (GLubyte *) _mesa_malloc(img->Width * img->Height
                                        * img->Depth * 4);

      store = ctx->Pack; /* save */
      ctx->Pack = ctx->DefaultPacking;

      ctx->Driver.GetTexImage(ctx, texObj->Target, level,
                              GL_RGBA, GL_UNSIGNED_BYTE,
                              buffer, texObj, img);

      /* make filename */
      _mesa_sprintf(s, "/tmp/teximage%u.ppm", texObj->Name);

      _mesa_printf("  Writing image level %u to %s\n", level, s);
      write_ppm(s, buffer, img->Width, img->Height, 4, 0, 1, 2, GL_FALSE);

      ctx->Pack = store; /* restore */

      _mesa_free(buffer);
   }
}


static GLboolean DumpImages;


static void
dump_texture_cb(GLuint id, void *data, void *userData)
{
   struct gl_texture_object *texObj = (struct gl_texture_object *) data;
   int i;
   GLboolean written = GL_FALSE;
   (void) userData;

   _mesa_printf("Texture %u\n", texObj->Name);
   _mesa_printf("  Target 0x%x\n", texObj->Target);
   for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
      struct gl_texture_image *texImg = texObj->Image[0][i];
      if (texImg) {
         _mesa_printf("  Image %u: %d x %d x %d, format %u at %p\n", i,
                      texImg->Width, texImg->Height, texImg->Depth,
                      texImg->TexFormat->MesaFormat, texImg->Data);
         if (DumpImages && !written) {
            GLuint face = 0;
            write_texture_image(texObj, face, i);
            written = GL_TRUE;
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


void
_mesa_dump_color_buffer(const char *filename)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLuint w = ctx->DrawBuffer->Width;
   const GLuint h = ctx->DrawBuffer->Height;
   GLubyte *buf;

   buf = (GLubyte *) _mesa_malloc(w * h * 4);

   _mesa_PushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
   _mesa_PixelStorei(GL_PACK_ALIGNMENT, 1);
   _mesa_PixelStorei(GL_PACK_INVERT_MESA, GL_TRUE);

   _mesa_ReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf);

   _mesa_printf("ReadBuffer %p 0x%x  DrawBuffer %p 0x%x\n",
                ctx->ReadBuffer->_ColorReadBuffer,
                ctx->ReadBuffer->ColorReadBuffer,
                ctx->DrawBuffer->_ColorDrawBuffers[0],
                ctx->DrawBuffer->ColorDrawBuffer[0]);
   _mesa_printf("Writing %d x %d color buffer to %s\n", w, h, filename);
   write_ppm(filename, buf, w, h, 4, 0, 1, 2, GL_TRUE);

   _mesa_PopClientAttrib();

   _mesa_free(buf);
}


void
_mesa_dump_depth_buffer(const char *filename)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLuint w = ctx->DrawBuffer->Width;
   const GLuint h = ctx->DrawBuffer->Height;
   GLuint *buf;
   GLubyte *buf2;
   GLuint i;

   buf = (GLuint *) _mesa_malloc(w * h * 4);  /* 4 bpp */
   buf2 = (GLubyte *) _mesa_malloc(w * h * 3); /* 3 bpp */

   _mesa_PushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
   _mesa_PixelStorei(GL_PACK_ALIGNMENT, 1);
   _mesa_PixelStorei(GL_PACK_INVERT_MESA, GL_TRUE);

   _mesa_ReadPixels(0, 0, w, h, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, buf);

   /* spread 24 bits of Z across R, G, B */
   for (i = 0; i < w * h; i++) {
      buf2[i*3+0] = (buf[i] >> 24) & 0xff;
      buf2[i*3+1] = (buf[i] >> 16) & 0xff;
      buf2[i*3+2] = (buf[i] >>  8) & 0xff;
   }

   _mesa_printf("Writing %d x %d depth buffer to %s\n", w, h, filename);
   write_ppm(filename, buf2, w, h, 3, 0, 1, 2, GL_TRUE);

   _mesa_PopClientAttrib();

   _mesa_free(buf);
   _mesa_free(buf2);
}


void
_mesa_dump_stencil_buffer(const char *filename)
{
   GET_CURRENT_CONTEXT(ctx);
   const GLuint w = ctx->DrawBuffer->Width;
   const GLuint h = ctx->DrawBuffer->Height;
   GLubyte *buf;
   GLubyte *buf2;
   GLuint i;

   buf = (GLubyte *) _mesa_malloc(w * h);  /* 1 bpp */
   buf2 = (GLubyte *) _mesa_malloc(w * h * 3); /* 3 bpp */

   _mesa_PushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
   _mesa_PixelStorei(GL_PACK_ALIGNMENT, 1);
   _mesa_PixelStorei(GL_PACK_INVERT_MESA, GL_TRUE);

   _mesa_ReadPixels(0, 0, w, h, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, buf);

   for (i = 0; i < w * h; i++) {
      buf2[i*3+0] = buf[i];
      buf2[i*3+1] = (buf[i] & 127) * 2;
      buf2[i*3+2] = (buf[i] - 128) * 2;
   }

   _mesa_printf("Writing %d x %d stencil buffer to %s\n", w, h, filename);
   write_ppm(filename, buf2, w, h, 3, 0, 1, 2, GL_TRUE);

   _mesa_PopClientAttrib();

   _mesa_free(buf);
   _mesa_free(buf2);
}
