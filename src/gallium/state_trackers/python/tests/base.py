#!/usr/bin/env python
##########################################################################
# 
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
# IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
##########################################################################


from gallium import *


# Enumerate all pixel formats
formats = {}
for name, value in globals().items():
    if name.startswith("PIPE_FORMAT_") and isinstance(value, int):
        formats[value] = name


def save_image(filename, surface):
    pixels = FloatArray(surface.height*surface.width*4)
    surface.get_tile_rgba(0, 0, surface.width, surface.height, pixels)

    import Image
    outimage = Image.new(
        mode='RGB',
        size=(surface.width, surface.height),
        color=(0,0,0))
    outpixels = outimage.load()
    for y in range(0, surface.height):
        for x in range(0, surface.width):
            offset = (y*surface.width + x)*4
            r, g, b, a = [int(pixels[offset + ch]*255) for ch in range(4)]
            outpixels[x, y] = r, g, b
    outimage.save(filename, "PNG")



class Test:
    
    def __init__(self):
        pass
    
    def run(self):
        raise NotImplementedError


class TestSuite(Test):
    
    def __init__(self, tests = None):
        Test.__init__(self)
        if tests is None:
            self.tests = []
        else:
            self.tests = tests

    def add_test(self, test):
        self.tests.append(test) 
    
    def run(self):
        for test in self.tests:
            self.test.run()


class TextureTemplate:
    
    def __init__(self, format=PIPE_FORMAT_R8G8B8A8_UNORM, width=1, height=1, depth=1, last_level=0, target=PIPE_TEXTURE_2D):
        self.format = format
        self.width = width
        self.height = height
        self.depth = depth
        self.last_level = last_level
        self.target = target



