/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Bismarck, ND., USA
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/

/*
 * Authors:
 *   Keith Whitwell
 */

#include "target-helpers/wrap_screen.h"
#include "trace/tr_public.h"
#include "rbug/rbug_public.h"
#include "identity/id_public.h"
#include "util/u_debug.h"


/* Centralized code to inject common wrapping layers:
 */
struct pipe_screen *
gallium_wrap_screen( struct pipe_screen *screen )
{
   /* Screen wrapping functions are required not to fail.  If it is
    * impossible to wrap a screen, the unwrapped screen should be
    * returned instead.  Any failure condition should be returned in
    * an OUT argument.
    *
    * Otherwise it is really messy trying to clean up in this code.
    */
   if (debug_get_bool_option("GALLIUM_WRAP", FALSE)) {
      screen = identity_screen_create(screen);
   }

   /* Trace does its own checking if it should run */
   screen = trace_screen_create(screen);

   /* Rbug does its own checking if it should run */
   screen = rbug_screen_create(screen);

   return screen;
}




