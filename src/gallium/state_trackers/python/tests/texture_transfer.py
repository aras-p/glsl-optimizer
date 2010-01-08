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
        
        target = self.target
        format = self.format
        width = self.width
        height = self.height
        depth = self.depth
        last_level = self.last_level
        face = self.face
        level = self.level
        zslice = self.zslice
        
        tex_usage = 0
        
        texture = dev.texture_create(
            target = target,
            format = format, 
            width = width, 
            height = height,
            depth = depth, 
            last_level = last_level,
            tex_usage = tex_usage,
        )
        if texture is None:
            raise TestSkip
        
        surface = texture.get_surface(face, level, zslice)
        
        # ???
        stride = pf_get_stride(texture->format, w)
        size = pf_get_nblocksy(texture->format) * stride

        in_raw = os.urandom(size)

        surface.put_tile_raw(0, 0, surface.width, surface.height, in_raw, stride)

        out_raw = surface.get_tile_raw(0, 0, surface.width, surface.height)

        if in_raw != out_raw:
            raise TestFailure


def main():
    dev = Device()
    suite = TestSuite()
    
    targets = [
        PIPE_TEXTURE_2D,
        PIPE_TEXTURE_CUBE,
        PIPE_TEXTURE_3D,
    ]
    
    formats = [
        PIPE_FORMAT_A8R8G8B8_UNORM,
        PIPE_FORMAT_X8R8G8B8_UNORM,
        PIPE_FORMAT_A8R8G8B8_SRGB,
        PIPE_FORMAT_R5G6B5_UNORM,
        PIPE_FORMAT_A1R5G5B5_UNORM,
        PIPE_FORMAT_A4R4G4B4_UNORM,
        PIPE_FORMAT_Z32_UNORM,
        PIPE_FORMAT_Z24S8_UNORM,
        PIPE_FORMAT_Z24X8_UNORM,
        PIPE_FORMAT_Z16_UNORM,
        PIPE_FORMAT_S8_UNORM,
        PIPE_FORMAT_A8_UNORM,
        PIPE_FORMAT_L8_UNORM,
        PIPE_FORMAT_DXT1_RGB,
        PIPE_FORMAT_DXT1_RGBA,
        PIPE_FORMAT_DXT3_RGBA,
        PIPE_FORMAT_DXT5_RGBA,
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

    for target in targets:
        for format in formats:
            for size in sizes:
                if target == PIPE_TEXTURE_3D:
                    depth = size
                else:
                    depth = 1
                for face in faces:
                    if target != PIPE_TEXTURE_CUBE and face:
                        continue
                    levels = lods(size)
                    for last_level in range(levels):
                        for level in range(0, last_level + 1):
                            zslice = 0
                            while zslice < depth >> level:
                                test = TextureTest(
                                    dev = dev,
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
                                zslice = (zslice + 1)*2 - 1
    suite.run()


if __name__ == '__main__':
    main()
