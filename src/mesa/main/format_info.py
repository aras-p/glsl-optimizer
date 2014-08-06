#!/usr/bin/env python
#
# Copyright 2014 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sub license, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice (including the
# next paragraph) shall be included in all copies or substantial portions
# of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
# IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import format_parser as parser
import sys

def get_gl_base_format(fmat):
   if fmat.name == 'MESA_FORMAT_NONE':
      return 'GL_NONE'
   elif fmat.name in ['MESA_FORMAT_YCBCR', 'MESA_FORMAT_YCBCR_REV']:
      return 'GL_YCBCR_MESA'
   elif fmat.has_channel('r'):
      if fmat.has_channel('g'):
         if fmat.has_channel('b'):
            if fmat.has_channel('a'):
               return 'GL_RGBA'
            else:
               return 'GL_RGB'
         else:
            return 'GL_RG'
      else:
         return 'GL_RED'
   elif fmat.has_channel('l'):
      if fmat.has_channel('a'):
         return 'GL_LUMINANCE_ALPHA'
      else:
         return 'GL_LUMINANCE'
   elif fmat.has_channel('a') and fmat.num_channels() == 1:
      return 'GL_ALPHA'
   elif fmat.has_channel('z'):
      if fmat.has_channel('s'):
         return 'GL_DEPTH_STENCIL'
      else:
         return 'GL_DEPTH_COMPONENT'
   elif fmat.has_channel('s'):
      return 'GL_STENCIL_INDEX'
   elif fmat.has_channel('i') and fmat.num_channels() == 1:
      return 'GL_INTENSITY'
   else:
      assert False

def get_gl_data_type(fmat):
   if fmat.is_compressed():
      if 'FLOAT' in fmat.name:
         return 'GL_FLOAT'
      elif 'SIGNED' in fmat.name or 'SNORM' in fmat.name:
         return 'GL_SIGNED_NORMALIZED'
      else:
         return 'GL_UNSIGNED_NORMALIZED'
   elif fmat.name in ['MESA_FORMAT_YCBCR', 'MESA_FORMAT_YCBCR_REV']:
      return 'GL_UNSIGNED_NORMALIZED'

   channel = None
   for chan in fmat.channels:
      if chan.type == 'x' and len(fmat.channels) > 1:
         continue # We can do better
      elif chan.name == 's' and fmat.has_channel('z'):
         continue # We'll use the type from the depth instead

      channel = chan
      break;

   if channel.type == parser.UNSIGNED:
      if channel.norm:
         return 'GL_UNSIGNED_NORMALIZED'
      else:
         return 'GL_UNSIGNED_INT'
   elif channel.type == parser.SIGNED:
      if channel.norm:
         return 'GL_SIGNED_NORMALIZED'
      else:
         return 'GL_INT'
   elif channel.type == parser.FLOAT:
      return 'GL_FLOAT'
   elif channel.type == parser.VOID:
      return 'GL_NONE'
   else:
      assert False

def get_mesa_layout(fmat):
   if fmat.layout == 'array':
      return 'MESA_FORMAT_LAYOUT_ARRAY'
   elif fmat.layout == 'packed':
      return 'MESA_FORMAT_LAYOUT_PACKED'
   else:
      return 'MESA_FORMAT_LAYOUT_OTHER'

def get_channel_bits(fmat, chan_name):
   if fmat.is_compressed():
      # These values are pretty-much bogus, but OpenGL requires that we
      # return an "approximate" number of bits.
      if fmat.layout == 's3tc':
         return 4 if fmat.has_channel(chan_name) else 0
      elif fmat.layout == 'fxt1':
         if chan_name in 'rgb':
            return 4
         elif chan_name == 'a':
            return 1 if fmat.has_channel('a') else 0
         else:
            return 0
      elif fmat.layout == 'rgtc':
         return 8 if fmat.has_channel(chan_name) else 0
      elif fmat.layout in ('etc1', 'etc2'):
         if fmat.name.endswith('_ALPHA1') and chan_name == 'a':
            return 1

         bits = 11 if fmat.name.endswith('11_EAC') else 8
         return bits if fmat.has_channel(chan_name) else 0
      elif fmat.layout == 'bptc':
         bits = 16 if fmat.name.endswith('_FLOAT') else 8
         return bits if fmat.has_channel(chan_name) else 0
      else:
         assert False
   else:
      # Uncompressed textures
      for chan in fmat.channels:
         if chan.name == chan_name:
            return chan.size
      return 0

formats = parser.parse(sys.argv[1])

print '''
/*
 * Mesa 3-D graphics library
 *
 * Copyright (c) 2014 Intel Corporation
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

 /*
  * This file is AUTOGENERATED by format_info.py.  Do not edit it
  * manually or commit it into version control.
  */

static struct gl_format_info format_info[MESA_FORMAT_COUNT] =
{
'''

for fmat in formats:
   print '   {'
   print '      {0},'.format(fmat.name)
   print '      "{0}",'.format(fmat.name)
   print '      {0},'.format(get_mesa_layout(fmat))
   print '      {0},'.format(get_gl_base_format(fmat))
   print '      {0},'.format(get_gl_data_type(fmat))

   bits = [ get_channel_bits(fmat, name) for name in ['r', 'g', 'b', 'a']]
   print '      {0},'.format(', '.join(map(str, bits)))
   bits = [ get_channel_bits(fmat, name) for name in ['l', 'i', 'z', 's']]
   print '      {0},'.format(', '.join(map(str, bits)))

   print '      {0}, {1}, {2},'.format(fmat.block_width, fmat.block_height,
                                       int(fmat.block_size() / 8))

   print '      {{ {0} }},'.format(', '.join(map(str, fmat.swizzle)))
   print '   },'

print '};'
