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


#ifndef SPU_DEBUG_H
#define SPU_DEBUG_H


/* Set to 0 to disable all extraneous debugging code */
#define DEBUG 1

#if DEBUG
extern boolean Debug;
extern boolean force_fragment_ops_fallback;

/* These debug macros use the unusual construction ", ##__VA_ARGS__"
 * which expands to the expected comma + args if variadic arguments
 * are supplied, but swallows the comma if there are no variadic
 * arguments (which avoids syntax errors that would otherwise occur).
 */
#define DEBUG_PRINTF(format,...) \
   if (Debug) \
      printf("SPU %u: " format, spu.init.id, ##__VA_ARGS__)
#define D_PRINTF(flag, format,...) \
   if (spu.init.debug_flags & (flag)) \
      printf("SPU %u: " format, spu.init.id, ##__VA_ARGS__)

#else

#define DEBUG_PRINTF(...)
#define D_PRINTF(...)

#endif


#endif /* SPU_DEBUG_H */
