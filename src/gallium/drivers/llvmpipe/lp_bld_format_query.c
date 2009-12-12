/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * Utility functions to make assertions about formats.
 *
 * This module centralizes most of logic used when determining what algorithm
 * is most suitable (i.e., most efficient yet correct) for a given format.
 *
 * It might be possible to move some of these functions to u_format module,
 * but since tiny differences in the format my render it more/less
 * appropriate to a given algorithm it is impossible to make any long term
 * guarantee about the semantics of these functions.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_format.h"

#include "lp_bld_format.h"


/**
 * Whether this format is a 4 rgba8 variant
 */
boolean
lp_format_is_rgba8(const struct util_format_description *desc)
{
   unsigned chan;

   if(desc->block.width != 1 ||
      desc->block.height != 1 ||
      desc->block.bits != 32)
      return FALSE;

   for(chan = 0; chan < 4; ++chan) {
      if(desc->channel[chan].type != UTIL_FORMAT_TYPE_UNSIGNED &&
         desc->channel[chan].type != UTIL_FORMAT_TYPE_SIGNED &&
         desc->channel[chan].type != UTIL_FORMAT_TYPE_VOID)
         return FALSE;
      if(desc->channel[chan].size != 8)
         return FALSE;
   }

   return TRUE;
}
