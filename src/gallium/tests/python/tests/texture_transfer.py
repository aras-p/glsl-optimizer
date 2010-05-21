#!/usr/bin/env python
##########################################################################
# 
# Copyright 2009 VMware, Inc.
# Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
# All Rights Reserved.
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
# 
##########################################################################


import os
import random

from gallium import *
from base import *


def lods(*dims):
    size = max(dims)
    lods = 0
    while size:
        lods += 1
        size >>= 1
    return lods


class TextureTest(TestCase):
    
    tags = (
        'target',
        'format',
        'width',
        'height',
        'depth',
        'last_level',
        'face',
        'level',
        'zslice',
    )

    def test(self):
        dev = self.dev
        ctx = self.ctx
        
        target = self.target
        format = self.format
        width = self.width
        height = self.height
        depth = self.depth
        last_level = self.last_level
        face = self.face
        level = self.level
        zslice = self.zslice

        bind = PIPE_BIND_SAMPLER_VIEW
        geom_flags = 0
        sample_count = 0
        if not dev.is_format_supported(format, target, sample_count, bind, geom_flags):
            raise TestSkip

        #  textures
        texture = dev.resource_create(
            target = target,
            format = format, 
            width = width, 
            height = height,
            depth = depth, 
            last_level = last_level,
            bind = bind,
        )
        
        surface = texture.get_surface(face, level, zslice)
        
        stride = util_format_get_stride(format, surface.width)
        size = util_format_get_nblocksy(format, surface.height) * stride

        in_raw = os.urandom(size)

        ctx.surface_write_raw(surface, 0, 0, surface.width, surface.height, in_raw, stride)

        out_raw = ctx.surface_read_raw(surface, 0, 0, surface.width, surface.height)

        if in_raw != out_raw:
            raise TestFailure


def main():
    dev = Device()
    ctx = dev.context_create()
    suite = TestSuite()
    
    targets = [
        PIPE_TEXTURE_2D,
        PIPE_TEXTURE_CUBE,
        PIPE_TEXTURE_3D,
    ]
    
    sizes = [64, 32, 16, 8, 4, 2, 1]
    #sizes = [1020, 508, 252, 62, 30, 14, 6, 3]
    #sizes = [64]
    #sizes = [63]
    
    faces = [
        PIPE_TEX_FACE_POS_X,
        PIPE_TEX_FACE_NEG_X,
        PIPE_TEX_FACE_POS_Y,
        PIPE_TEX_FACE_NEG_Y, 
        PIPE_TEX_FACE_POS_Z, 
        PIPE_TEX_FACE_NEG_Z,
    ]

    try:
        n = int(sys.argv[1])
    except:
        n = 10000
    
    for i in range(n):
        format = random.choice(formats.keys())
        if not util_format_is_depth_or_stencil(format):
            is_depth_or_stencil = util_format_is_depth_or_stencil(format)

            if is_depth_or_stencil:
                target = PIPE_TEXTURE_2D
            else:
                target = random.choice(targets)
            
            size = random.choice(sizes)

            if target == PIPE_TEXTURE_3D:
                depth = size
            else:
                depth = 1

            if target == PIPE_TEXTURE_CUBE:
                face = random.choice(faces)
            else:
                face = PIPE_TEX_FACE_POS_X

            levels = lods(size)
            last_level = random.randint(0, levels - 1)
            level = random.randint(0, last_level)
            zslice = random.randint(0, max(depth >> level, 1) - 1)

            test = TextureTest(
                dev = dev,
                ctx = ctx,
                target = target,
                format = format, 
                width = size,
                height = size,
                depth = depth,
                last_level = last_level,
                face = face,
                level = level,
                zslice = zslice,
            )
            suite.add_test(test)
    suite.run()


if __name__ == '__main__':
    main()
