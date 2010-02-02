/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright (c) 2008 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "pipe/p_config.h" 

#include <stdarg.h>


#ifdef PIPE_SUBSYSTEM_WINDOWS_DISPLAY

#include <windows.h>
#include <winddi.h>

#elif defined(PIPE_SUBSYSTEM_WINDOWS_CE)

#include <stdio.h> 
#include <stdlib.h> 
#include <windows.h> 
#include <types.h> 

#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN      // Exclude rarely-used stuff from Windows headers
#endif
#include <windows.h>
#include <stdio.h>

#else

#include <stdio.h>
#include <stdlib.h>

#endif

#include "pipe/p_compiler.h" 
#include "util/u_debug.h" 
#include "pipe/p_format.h" 
#include "pipe/p_state.h" 
#include "util/u_inlines.h" 
#include "util/u_format.h"
#include "util/u_memory.h" 
#include "util/u_string.h" 
#include "util/u_stream.h" 
#include "util/u_math.h" 
#include "util/u_tile.h" 
#include "util/u_prim.h" 


#ifdef PIPE_SUBSYSTEM_WINDOWS_DISPLAY
static INLINE void 
_EngDebugPrint(const char *format, ...)
{
   va_list ap;
   va_start(ap, format);
   EngDebugPrint("", (PCHAR)format, ap);
   va_end(ap);
}
#endif


void _debug_vprintf(const char *format, va_list ap)
{
#if defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)
   /* EngDebugPrint does not handle float point arguments, so we need to use
    * our own vsnprintf implementation. It is also very slow, so buffer until
    * we find a newline. */
   static char buf[512] = {'\0'};
   size_t len = strlen(buf);
   int ret = util_vsnprintf(buf + len, sizeof(buf) - len, format, ap);
   if(ret > (int)(sizeof(buf) - len - 1) || util_strchr(buf + len, '\n')) {
      _EngDebugPrint("%s", buf);
      buf[0] = '\0';
   }
#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   /* OutputDebugStringA can be very slow, so buffer until we find a newline. */
   static char buf[4096] = {'\0'};
   size_t len = strlen(buf);
   int ret = util_vsnprintf(buf + len, sizeof(buf) - len, format, ap);
   if(ret > (int)(sizeof(buf) - len - 1) || util_strchr(buf + len, '\n')) {
      OutputDebugStringA(buf);
      buf[0] = '\0';
   }
   
   if(GetConsoleWindow() && !IsDebuggerPresent()) {
      fflush(stdout);
      vfprintf(stderr, format, ap);
      fflush(stderr);
   }
   
#elif defined(PIPE_SUBSYSTEM_WINDOWS_CE)
   wchar_t *wide_format;
   long wide_str_len;   
   char buf[512];   
   int ret;   
#if (_WIN32_WCE < 600)
   ret = vsprintf(buf, format, ap);   
   if(ret < 0){   
       sprintf(buf, "Cant handle debug print!");   
       ret = 25;
   }
#else
   ret = vsprintf_s(buf, 512, format, ap);   
   if(ret < 0){   
       sprintf_s(buf, 512, "Cant handle debug print!");   
       ret = 25;
   }
#endif
   buf[ret] = '\0';   
   /* Format is ascii - needs to be converted to wchar_t for printing */   
   wide_str_len = MultiByteToWideChar(CP_ACP, 0, (const char *) buf, -1, NULL, 0);   
   wide_format = (wchar_t *) malloc((wide_str_len+1) * sizeof(wchar_t));   
   if (wide_format) {   
      MultiByteToWideChar(CP_ACP, 0, (const char *) buf, -1,   
            wide_format, wide_str_len);   
      NKDbgPrintfW(wide_format, wide_format);   
      free(wide_format);   
   } 
#elif defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT)
   /* TODO */
#else /* !PIPE_SUBSYSTEM_WINDOWS */
   fflush(stdout);
   vfprintf(stderr, format, ap);
#endif
}


#ifdef DEBUG
void debug_print_blob( const char *name,
                       const void *blob,
                       unsigned size )
{
   const unsigned *ublob = (const unsigned *)blob;
   unsigned i;

   debug_printf("%s (%d dwords%s)\n", name, size/4,
                size%4 ? "... plus a few bytes" : "");

   for (i = 0; i < size/4; i++) {
      debug_printf("%d:\t%08x\n", i, ublob[i]);
   }
}
#endif


#ifndef debug_break
void debug_break(void) 
{
#if defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   DebugBreak();
#elif defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)
   EngDebugBreak();
#else
   abort();
#endif
}
#endif


#ifdef PIPE_SUBSYSTEM_WINDOWS_DISPLAY
static const char *
find(const char *start, const char *end, char c) 
{
   const char *p;
   for(p = start; !end || p != end; ++p) {
      if(*p == c)
	 return p;
      if(*p < 32)
	 break;
   }
   return NULL;
}

static int 
compare(const char *start, const char *end, const char *s)
{
   const char *p, *q;
   for(p = start, q = s; p != end && *q != '\0'; ++p, ++q) {
      if(*p != *q)
	 return 0;
   }
   return p == end && *q == '\0';
}

static void 
copy(char *dst, const char *start, const char *end, size_t n) 
{
   const char *p;
   char *q;
   for(p = start, q = dst, n = n - 1; p != end && n; ++p, ++q, --n)
      *q = *p;
   *q = '\0';
}
#endif


static INLINE const char *
_debug_get_option(const char *name)
{
#if defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)
   /* EngMapFile creates the file if it does not exists, so it must either be
    * disabled on release versions (or put in a less conspicuous place). */
#ifdef DEBUG
   const char *result = NULL;
   ULONG_PTR iFile = 0;
   const void *pMap = NULL;
   const char *sol, *eol, *sep;
   static char output[1024];
   
   pMap = EngMapFile(L"\\??\\c:\\gallium.cfg", 0, &iFile);
   if(pMap) {
      sol = (const char *)pMap;
      while(1) {
	 /* TODO: handle LF line endings */
	 eol = find(sol, NULL, '\r');
	 if(!eol || eol == sol)
	    break;
	 sep = find(sol, eol, '=');
	 if(!sep)
	    break;
	 if(compare(sol, sep, name)) {
	    copy(output, sep + 1, eol, sizeof(output));
	    result = output;
	    break;
	 }
	 sol = eol + 2;
      }
      EngUnmapFile(iFile);
   }
   return result;
#else
   return NULL;
#endif
#elif defined(PIPE_SUBSYSTEM_WINDOWS_CE) || defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT) 
   /* TODO: implement */
   return NULL;
#else
   return getenv(name);
#endif
}

const char *
debug_get_option(const char *name, const char *dfault)
{
   const char *result;

   result = _debug_get_option(name);
   if(!result)
      result = dfault;
      
   debug_printf("%s: %s = %s\n", __FUNCTION__, name, result ? result : "(null)");
   
   return result;
}

boolean
debug_get_bool_option(const char *name, boolean dfault)
{
   const char *str = _debug_get_option(name);
   boolean result;
   
   if(str == NULL)
      result = dfault;
   else if(!util_strcmp(str, "n"))
      result = FALSE;
   else if(!util_strcmp(str, "no"))
      result = FALSE;
   else if(!util_strcmp(str, "0"))
      result = FALSE;
   else if(!util_strcmp(str, "f"))
      result = FALSE;
   else if(!util_strcmp(str, "false"))
      result = FALSE;
   else
      result = TRUE;

   debug_printf("%s: %s = %s\n", __FUNCTION__, name, result ? "TRUE" : "FALSE");
   
   return result;
}


long
debug_get_num_option(const char *name, long dfault)
{
   long result;
   const char *str;
   
   str = _debug_get_option(name);
   if(!str)
      result = dfault;
   else {
      long sign;
      char c;
      c = *str++;
      if(c == '-') {
	 sign = -1;
	 c = *str++;
      } 
      else {
	 sign = 1;
      }
      result = 0;
      while('0' <= c && c <= '9') {
	 result = result*10 + (c - '0');
	 c = *str++;
      }
      result *= sign;
   }
   
   debug_printf("%s: %s = %li\n", __FUNCTION__, name, result);

   return result;
}


unsigned long
debug_get_flags_option(const char *name, 
                       const struct debug_named_value *flags,
                       unsigned long dfault)
{
   unsigned long result;
   const char *str;
   
   str = _debug_get_option(name);
   if(!str)
      result = dfault;
   else if (!util_strcmp(str, "help")) {
      result = dfault;
      while (flags->name) {
         debug_printf("%s: help for %s: %s [0x%lx]\n", __FUNCTION__, name, flags->name, flags->value);
         flags++;
      }
   }
   else {
      result = 0;
      while( flags->name ) {
	 if (!util_strcmp(str, "all") || util_strstr(str, flags->name ))
	    result |= flags->value;
	 ++flags;
      }
   }

   if (str) {
      debug_printf("%s: %s = 0x%lx (%s)\n", __FUNCTION__, name, result, str);
   }
   else {
      debug_printf("%s: %s = 0x%lx\n", __FUNCTION__, name, result);
   }

   return result;
}


void _debug_assert_fail(const char *expr, 
                        const char *file, 
                        unsigned line, 
                        const char *function) 
{
   _debug_printf("%s:%u:%s: Assertion `%s' failed.\n", file, line, function, expr);
#if defined(PIPE_OS_WINDOWS) && !defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   if (debug_get_bool_option("GALLIUM_ABORT_ON_ASSERT", FALSE))
#else
   if (debug_get_bool_option("GALLIUM_ABORT_ON_ASSERT", TRUE))
#endif
      debug_break();
   else
      _debug_printf("continuing...\n");
}


const char *
debug_dump_enum(const struct debug_named_value *names, 
                unsigned long value)
{
   static char rest[64];
   
   while(names->name) {
      if(names->value == value)
	 return names->name;
      ++names;
   }

   util_snprintf(rest, sizeof(rest), "0x%08lx", value);
   return rest;
}


const char *
debug_dump_enum_noprefix(const struct debug_named_value *names, 
                         const char *prefix,
                         unsigned long value)
{
   static char rest[64];
   
   while(names->name) {
      if(names->value == value) {
         const char *name = names->name;
         while (*name == *prefix) {
            name++;
            prefix++;
         }
         return name;
      }
      ++names;
   }

   

   util_snprintf(rest, sizeof(rest), "0x%08lx", value);
   return rest;
}


const char *
debug_dump_flags(const struct debug_named_value *names, 
                 unsigned long value)
{
   static char output[4096];
   static char rest[256];
   int first = 1;

   output[0] = '\0';

   while(names->name) {
      if((names->value & value) == names->value) {
	 if (!first)
	    util_strncat(output, "|", sizeof(output));
	 else
	    first = 0;
	 util_strncat(output, names->name, sizeof(output) - 1);
	 output[sizeof(output) - 1] = '\0';
	 value &= ~names->value;
      }
      ++names;
   }
   
   if (value) {
      if (!first)
	 util_strncat(output, "|", sizeof(output));
      else
	 first = 0;
      
      util_snprintf(rest, sizeof(rest), "0x%08lx", value);
      util_strncat(output, rest, sizeof(output) - 1);
      output[sizeof(output) - 1] = '\0';
   }
   
   if(first)
      return "0";
   
   return output;
}


static const struct debug_named_value pipe_format_names[] = {
#ifdef DEBUG
   DEBUG_NAMED_VALUE(PIPE_FORMAT_NONE),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_A8R8G8B8_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_X8R8G8B8_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_B8G8R8A8_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_B8G8R8X8_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_A1R5G5B5_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_A4R4G4B4_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R5G6B5_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_A2B10G10R10_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_L8_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_A8_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_I8_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_A8L8_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_L16_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_YCBCR),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_YCBCR_REV),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_Z16_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_Z32_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_Z32_FLOAT),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_S8Z24_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_Z24S8_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_X8Z24_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_Z24X8_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_S8_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R64_FLOAT),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R64G64_FLOAT),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R64G64B64_FLOAT),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R64G64B64A64_FLOAT),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32_FLOAT),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32G32_FLOAT),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32G32B32_FLOAT),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32G32B32A32_FLOAT),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32G32_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32G32B32_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32G32B32A32_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32_USCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32G32_USCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32G32B32_USCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32G32B32A32_USCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32_SNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32G32_SNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32G32B32_SNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32G32B32A32_SNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32_SSCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32G32_SSCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32G32B32_SSCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R32G32B32A32_SSCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R16_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R16G16_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R16G16B16_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R16G16B16A16_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R16_USCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R16G16_USCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R16G16B16_USCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R16G16B16A16_USCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R16_SNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R16G16_SNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R16G16B16_SNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R16G16B16A16_SNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R16_SSCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R16G16_SSCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R16G16B16_SSCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R16G16B16A16_SSCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8A8_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8X8_UNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8_USCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8_USCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8_USCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8A8_USCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8X8_USCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8_SNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8_SNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8_SNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8A8_SNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8X8_SNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_B6G5R5_SNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_A8B8G8R8_SNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_X8B8G8R8_SNORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8_SSCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8_SSCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8_SSCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8A8_SSCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8X8_SSCALED),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_L8_SRGB),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_A8L8_SRGB),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8_SRGB),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8A8_SRGB),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8X8_SRGB),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_A8R8G8B8_SRGB),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_X8R8G8B8_SRGB),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_B8G8R8A8_SRGB),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_B8G8R8X8_SRGB),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_X8UB8UG8SR8S_NORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_B6UG5SR5S_NORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_DXT1_RGB),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_DXT1_RGBA),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_DXT3_RGBA),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_DXT5_RGBA),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_DXT1_SRGB),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_DXT1_SRGBA),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_DXT3_SRGBA),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_DXT5_SRGBA),
#endif
   DEBUG_NAMED_VALUE_END
};

#ifdef DEBUG
void debug_print_format(const char *msg, unsigned fmt )
{
   debug_printf("%s: %s\n", msg, debug_dump_enum(pipe_format_names, fmt)); 
}
#endif

const char *pf_name( enum pipe_format format )
{
   return debug_dump_enum(pipe_format_names, format);
}



static const struct debug_named_value pipe_prim_names[] = {
#ifdef DEBUG
   DEBUG_NAMED_VALUE(PIPE_PRIM_POINTS),
   DEBUG_NAMED_VALUE(PIPE_PRIM_LINES),
   DEBUG_NAMED_VALUE(PIPE_PRIM_LINE_LOOP),
   DEBUG_NAMED_VALUE(PIPE_PRIM_LINE_STRIP),
   DEBUG_NAMED_VALUE(PIPE_PRIM_TRIANGLES),
   DEBUG_NAMED_VALUE(PIPE_PRIM_TRIANGLE_STRIP),
   DEBUG_NAMED_VALUE(PIPE_PRIM_TRIANGLE_FAN),
   DEBUG_NAMED_VALUE(PIPE_PRIM_QUADS),
   DEBUG_NAMED_VALUE(PIPE_PRIM_QUAD_STRIP),
   DEBUG_NAMED_VALUE(PIPE_PRIM_POLYGON),
#endif
   DEBUG_NAMED_VALUE_END
};


const char *u_prim_name( unsigned prim )
{
   return debug_dump_enum(pipe_prim_names, prim);
}




#ifdef DEBUG
void debug_dump_image(const char *prefix,
                      unsigned format, unsigned cpp,
                      unsigned width, unsigned height,
                      unsigned stride,
                      const void *data)     
{
#ifdef PIPE_SUBSYSTEM_WINDOWS_DISPLAY
   static unsigned no = 0; 
   char filename[256];
   WCHAR wfilename[sizeof(filename)];
   ULONG_PTR iFile = 0;
   struct {
      unsigned format;
      unsigned cpp;
      unsigned width;
      unsigned height;
   } header;
   unsigned char *pMap = NULL;
   unsigned i;

   util_snprintf(filename, sizeof(filename), "\\??\\c:\\%03u%s.raw", ++no, prefix);
   for(i = 0; i < sizeof(filename); ++i)
      wfilename[i] = (WCHAR)filename[i];
   
   pMap = (unsigned char *)EngMapFile(wfilename, sizeof(header) + height*width*cpp, &iFile);
   if(!pMap)
      return;
   
   header.format = format;
   header.cpp = cpp;
   header.width = width;
   header.height = height;
   memcpy(pMap, &header, sizeof(header));
   pMap += sizeof(header);
   
   for(i = 0; i < height; ++i) {
      memcpy(pMap, (unsigned char *)data + stride*i, cpp*width);
      pMap += cpp*width;
   }
      
   EngUnmapFile(iFile);
#endif
}

void debug_dump_surface(const char *prefix,
                        struct pipe_surface *surface)     
{
   struct pipe_texture *texture;
   struct pipe_screen *screen;
   struct pipe_transfer *transfer;
   void *data;

   if (!surface)
      return;

   texture = surface->texture;
   screen = texture->screen;

   transfer = screen->get_tex_transfer(screen, texture, surface->face,
                                       surface->level, surface->zslice,
                                       PIPE_TRANSFER_READ, 0, 0, surface->width,
                                       surface->height);
   
   data = screen->transfer_map(screen, transfer);
   if(!data)
      goto error;
   
   debug_dump_image(prefix, 
                    texture->format,
                    util_format_get_blocksize(texture->format), 
                    util_format_get_nblocksx(texture->format, transfer->width),
                    util_format_get_nblocksy(texture->format, transfer->height),
                    transfer->stride,
                    data);
   
   screen->transfer_unmap(screen, transfer);
error:
   screen->tex_transfer_destroy(transfer);
}


#pragma pack(push,2)
struct bmp_file_header {
   uint16_t bfType;
   uint32_t bfSize;
   uint16_t bfReserved1;
   uint16_t bfReserved2;
   uint32_t bfOffBits;
};
#pragma pack(pop)

struct bmp_info_header {
   uint32_t biSize;
   int32_t biWidth;
   int32_t biHeight;
   uint16_t biPlanes;
   uint16_t biBitCount;
   uint32_t biCompression;
   uint32_t biSizeImage;
   int32_t biXPelsPerMeter;
   int32_t biYPelsPerMeter;
   uint32_t biClrUsed;
   uint32_t biClrImportant;
};

struct bmp_rgb_quad {
   uint8_t rgbBlue;
   uint8_t rgbGreen;
   uint8_t rgbRed;
   uint8_t rgbAlpha;
};

void
debug_dump_surface_bmp(const char *filename,
                       struct pipe_surface *surface)
{
#ifndef PIPE_SUBSYSTEM_WINDOWS_MINIPORT
   struct pipe_transfer *transfer;
   struct pipe_texture *texture = surface->texture;
   struct pipe_screen *screen = texture->screen;

   transfer = screen->get_tex_transfer(screen, texture, surface->face,
                                       surface->level, surface->zslice,
                                       PIPE_TRANSFER_READ, 0, 0, surface->width,
                                       surface->height);

   debug_dump_transfer_bmp(filename, transfer);

   screen->tex_transfer_destroy(transfer);
#endif
}

void
debug_dump_transfer_bmp(const char *filename,
                        struct pipe_transfer *transfer)
{
#ifndef PIPE_SUBSYSTEM_WINDOWS_MINIPORT
   float *rgba;

   if (!transfer)
      goto error1;

   rgba = MALLOC(transfer->width*transfer->height*4*sizeof(float));
   if(!rgba)
      goto error1;

   pipe_get_tile_rgba(transfer, 0, 0,
                      transfer->width, transfer->height,
                      rgba);

   debug_dump_float_rgba_bmp(filename,
                             transfer->width, transfer->height,
                             rgba, transfer->width);

   FREE(rgba);
error1:
   ;
#endif
}

void
debug_dump_float_rgba_bmp(const char *filename,
                          unsigned width, unsigned height,
                          float *rgba, unsigned stride)
{
#ifndef PIPE_SUBSYSTEM_WINDOWS_MINIPORT
   struct util_stream *stream;
   struct bmp_file_header bmfh;
   struct bmp_info_header bmih;
   unsigned x, y;

   if(!rgba)
      goto error1;

   bmfh.bfType = 0x4d42;
   bmfh.bfSize = 14 + 40 + height*width*4;
   bmfh.bfReserved1 = 0;
   bmfh.bfReserved2 = 0;
   bmfh.bfOffBits = 14 + 40;

   bmih.biSize = 40;
   bmih.biWidth = width;
   bmih.biHeight = height;
   bmih.biPlanes = 1;
   bmih.biBitCount = 32;
   bmih.biCompression = 0;
   bmih.biSizeImage = height*width*4;
   bmih.biXPelsPerMeter = 0;
   bmih.biYPelsPerMeter = 0;
   bmih.biClrUsed = 0;
   bmih.biClrImportant = 0;

   stream = util_stream_create(filename, bmfh.bfSize);
   if(!stream)
      goto error1;

   util_stream_write(stream, &bmfh, 14);
   util_stream_write(stream, &bmih, 40);

   y = height;
   while(y--) {
      float *ptr = rgba + (stride * y * 4);
      for(x = 0; x < width; ++x)
      {
         struct bmp_rgb_quad pixel;
         pixel.rgbRed   = float_to_ubyte(ptr[x*4 + 0]);
         pixel.rgbGreen = float_to_ubyte(ptr[x*4 + 1]);
         pixel.rgbBlue  = float_to_ubyte(ptr[x*4 + 2]);
         pixel.rgbAlpha = 255;
         util_stream_write(stream, &pixel, 4);
      }
   }

   util_stream_close(stream);
error1:
   ;
#endif
}

#endif
