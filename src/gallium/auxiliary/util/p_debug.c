/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
#else
#include <stdio.h>
#include <stdlib.h>
#endif

#include "pipe/p_compiler.h" 
#include "pipe/p_util.h" 
#include "pipe/p_debug.h" 
#include "pipe/p_format.h" 
#include "util/u_string.h" 


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
#ifdef PIPE_SUBSYSTEM_WINDOWS_DISPLAY
#ifndef WINCE
   /* EngDebugPrint does not handle float point arguments, so we need to use
    * our own vsnprintf implementation. It is also very slow, so buffer until
    * we find a newline. */
   static char buf[512 + 1] = {'\0'};
   size_t len = strlen(buf);
   int ret = util_vsnprintf(buf + len, sizeof(buf) - len, format, ap);
   if(ret > (int)(sizeof(buf) - len - 1) || util_strchr(buf + len, '\n')) {
      _EngDebugPrint("%s", buf);
      buf[0] = '\0';
   }
#else
   /* TODO: Implement debug print for WINCE */
#endif
#else
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


void _debug_break(void) 
{
#if defined(PIPE_ARCH_X86) && defined(PIPE_CC_GCC)
   __asm("int3");
#elif defined(PIPE_ARCH_X86) && defined(PIPE_CC_MSVC)
   _asm {int 3};
#elif defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)
   EngDebugBreak();
#else
   abort();
#endif
}


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


const char *
debug_get_option(const char *name, const char *dfault)
{
   const char *result;
#if defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)
   /* EngMapFile creates the file if it does not exists, so it must either be
    * disabled on release versions (or put in a less conspicuous place). */
#ifdef DEBUG
   ULONG_PTR iFile = 0;
   const void *pMap = NULL;
   const char *sol, *eol, *sep;
   static char output[1024];
   
   result = dfault;
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
#else
   result = dfault;
#endif
#elif defined(PIPE_SUBSYSTEM_WINDOWS_CE)
   /* TODO: implement */
   result = dfault;
#else
   result = getenv(name);
   if(!result)
      result = dfault;
#endif

   debug_printf("%s: %s = %s\n", __FUNCTION__, name, result ? result : "(null)");
   
   return result;
}

boolean
debug_get_bool_option(const char *name, boolean dfault)
{
   const char *str = debug_get_option(name, NULL);
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
   
   str = debug_get_option(name, NULL);
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
   
   str = debug_get_option(name, NULL);
   if(!str)
      result = dfault;
   else {
      result = 0;
      while( flags->name ) {
	 if (!util_strcmp(str, "all") || util_strstr(str, flags->name ))
	    result |= flags->value;
	 ++flags;
      }
   }

   debug_printf("%s: %s = 0x%lx\n", __FUNCTION__, name, result);

   return result;
}


void _debug_assert_fail(const char *expr, 
                        const char *file, 
                        unsigned line, 
                        const char *function) 
{
   _debug_printf("%s:%u:%s: Assertion `%s' failed.\n", file, line, function, expr);
   if (debug_get_bool_option("GALLIUM_ABORT_ON_ASSERT", TRUE))
      debug_break();
   else
      _debug_printf("continuing...\n");
}


const char *
debug_dump_enum(const struct debug_named_value *names, 
                unsigned long value)
{
   static char rest[256];
   
   while(names->name) {
      if(names->value == value)
	 return names->name;
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
	 util_strncat(output, names->name, sizeof(output));
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
      util_strncat(output, rest, sizeof(output));
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
   DEBUG_NAMED_VALUE(PIPE_FORMAT_A8_L8_SRGB),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8_SRGB),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8A8_SRGB),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_R8G8B8X8_SRGB),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_X8UB8UG8SR8S_NORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_B6UG5SR5S_NORM),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_DXT1_RGB),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_DXT1_RGBA),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_DXT3_RGBA),
   DEBUG_NAMED_VALUE(PIPE_FORMAT_DXT5_RGBA),
#endif
   DEBUG_NAMED_VALUE_END
};

#ifdef DEBUG
void debug_print_format(const char *msg, unsigned fmt )
{
   debug_printf("%s: %s\n", msg, debug_dump_enum(pipe_format_names, fmt)); 
}
#endif

char *pf_sprint_name( char *str, enum pipe_format format )
{
   strcpy( str, debug_dump_enum(pipe_format_names, format) );
   return str;
}


#ifdef DEBUG
void debug_dump_image(const char *prefix,
                      unsigned format, unsigned cpp,
                      unsigned width, unsigned height,
                      unsigned pitch,
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
   
   pMap = (unsigned char *)EngMapFile(wfilename, sizeof(header) + cpp*width*height, &iFile);
   if(!pMap)
      return;
   
   header.format = format;
   header.cpp = cpp;
   header.width = width;
   header.height = height;
   memcpy(pMap, &header, sizeof(header));
   pMap += sizeof(header);
   
   for(i = 0; i < height; ++i) {
      memcpy(pMap, (unsigned char *)data + cpp*pitch*i, cpp*width);
      pMap += cpp*width;
   }
      
   EngUnmapFile(iFile);
#endif
}
#endif
