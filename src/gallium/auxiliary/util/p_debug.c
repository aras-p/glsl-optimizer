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


void debug_vprintf(const char *format, va_list ap)
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


void debug_printf(const char *format, ...)
{
   va_list ap;
   va_start(ap, format);
   debug_vprintf(format, ap);
   va_end(ap);
}


/* TODO: implement a debug_abort that calls EngBugCheckEx on WIN32 */


static INLINE void debug_break(void) 
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

#if defined(WIN32)
ULONG_PTR debug_config_file = 0;
void *mapped_config_file = 0;

enum {
	eAssertAbortEn = 0x1,
};

/* Check for aborts enabled. */
static unsigned abort_en()
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
static unsigned abort_en()
{
   return !GETENV("GALLIUM_ABORT_ON_ASSERT");
}
#endif

void debug_assert_fail(const char *expr, const char *file, unsigned line) 
{
   debug_printf("%s:%i: Assertion `%s' failed.\n", file, line, expr);
   if (abort_en())
   {
      debug_break();
   } else
   {
      debug_printf("continuing...\n");
   }
}


#define DEBUG_MASK_TABLE_SIZE 256


/**
 * Mask hash table.
 * 
 * For now we just take the lower bits of the key, and do no attempt to solve
 * collisions. Use a proper hash table when we have dozens of drivers. 
 */
static uint32_t debug_mask_table[DEBUG_MASK_TABLE_SIZE];


void debug_mask_set(uint32_t uuid, uint32_t mask) 
{
   unsigned hash = uuid & (DEBUG_MASK_TABLE_SIZE - 1);
   debug_mask_table[hash] = mask;
}


uint32_t debug_mask_get(uint32_t uuid)
{
   unsigned hash = uuid & (DEBUG_MASK_TABLE_SIZE - 1);
   return debug_mask_table[hash];
}


void debug_mask_vprintf(uint32_t uuid, uint32_t what, const char *format, va_list ap)
{
   uint32_t mask = debug_mask_get(uuid);
   if(mask & what)
      debug_vprintf(format, ap);
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

   snprintf(rest, sizeof(rest), "0x%08x", value);
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
      
      snprintf(rest, sizeof(rest), "0x%08x", value);
      strncat(output, rest, sizeof(output));
   }
   
   if(first)
      return "0";
   
   return output;
}
