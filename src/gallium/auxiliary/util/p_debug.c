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


#include <stdarg.h>

#ifdef WIN32
#include <windows.h>
#include <winddi.h>
#else
#include <stdio.h>
#include <stdlib.h>
#endif

#include "pipe/p_compiler.h" 
#include "pipe/p_util.h" 
#include "pipe/p_debug.h" 


#ifdef WIN32
static INLINE void 
rpl_EngDebugPrint(const char *format, ...)
{
   va_list ap;
   va_start(ap, format);
   EngDebugPrint("", (PCHAR)format, ap);
   va_end(ap);
}

int rpl_vsnprintf(char *, size_t, const char *, va_list);
int rpl_snprintf(char *str, size_t size, const char *format, ...);
#define vsnprintf rpl_vsnprintf
#define snprintf rpl_snprintf
#endif


void _debug_vprintf(const char *format, va_list ap)
{
#ifdef WIN32
#ifndef WINCE
   /* EngDebugPrint does not handle float point arguments, so we need to use
    * our own vsnprintf implementation */
   char buf[512 + 1];
   rpl_vsnprintf(buf, sizeof(buf), format, ap);
   rpl_EngDebugPrint("%s", buf);
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
#if (defined(__i386__) || defined(__386__)) && defined(__GNUC__)
   __asm("int3");
#elif (defined(__i386__) || defined(__386__)) && defined(__MSC__)
   _asm {int 3};
#elif defined(WIN32) && !defined(WINCE)
   EngDebugBreak();
#else
   abort();
#endif
}


#ifdef WIN32
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
#ifdef WIN32
   ULONG_PTR iFile = 0;
   const void *pMap = NULL;
   const char *sol, *eol, *sep;
   static char output[1024];
   
   result = dfault;
   /* XXX: this creates the file if it does not exists, so it must either be
    * disabled on release versions, or put in a less conspicuous place.
    */
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
   else if(!strcmp(str, "no"))
      result = FALSE;
   else if(!strcmp(str, "0"))
      result = FALSE;
   else if(!strcmp(str, "f"))
      result = FALSE;
   else if(!strcmp(str, "false"))
      result = FALSE;
   else
      result = TRUE;

   debug_printf("%s: %s = %s\n", __FUNCTION__, name, result ? "TRUE" : "FALSE");
   
   return result;
}


long
debug_get_num_option(const char *name, long dfault)
{
   /* FIXME */
   return dfault;
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
	 if (!strcmp(str, "all") || strstr(str, flags->name ))
	    result |= flags->value;
	 ++flags;
      }
   }

   debug_printf("%s: %s = 0x%lx\n", __FUNCTION__, name, result);

   return result;
}


#if defined(WIN32)
ULONG_PTR debug_config_file = 0;
void *mapped_config_file = 0;

enum {
	eAssertAbortEn = 0x1,
};

/* Check for aborts enabled. */
static unsigned abort_en(void)
{
   if (!mapped_config_file)
   {
      /* Open an 8 byte file for configuration data. */
      mapped_config_file = EngMapFile(L"\\??\\c:\\gaDebug.cfg", 8, &debug_config_file);
   }

   /* A value of "0" (ascii) in the configuration file will clear the
    * first 8 bits in the test byte. 
    *
    * A value of "1" (ascii) in the configuration file will set the
    * first bit in the test byte. 
    *
    * A value of "2" (ascii) in the configuration file will set the
    * second bit in the test byte. 
    *
    * Currently the only interesting values are 0 and 1, which clear
    * and set abort-on-assert behaviour respectively.
    */
   return ((((char *)mapped_config_file)[0]) - 0x30) & eAssertAbortEn;
}
#else /* WIN32 */
static unsigned abort_en(void)
{
   return !GETENV("GALLIUM_ABORT_ON_ASSERT");
}
#endif

void _debug_assert_fail(const char *expr, 
                        const char *file, 
                        unsigned line, 
                        const char *function) 
{
   _debug_printf("%s:%u:%s: Assertion `%s' failed.\n", file, line, function, expr);
   if (abort_en())
   {
      debug_break();
   } else
   {
      _debug_printf("continuing...\n");
   }
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

   snprintf(rest, sizeof(rest), "0x%08lx", value);
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
	    strncat(output, "|", sizeof(output));
	 else
	    first = 0;
	 strncat(output, names->name, sizeof(output));
	 value &= ~names->value;
      }
      ++names;
   }
   
   if (value) {
      if (!first)
	 strncat(output, "|", sizeof(output));
      else
	 first = 0;
      
      snprintf(rest, sizeof(rest), "0x%08lx", value);
      strncat(output, rest, sizeof(output));
   }
   
   if(first)
      return "0";
   
   return output;
}


