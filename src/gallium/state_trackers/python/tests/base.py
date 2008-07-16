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


def make_image(width, height, rgba):
    import Image
    outimage = Image.new(
        mode='RGB',
        size=(width, height),
        color=(0,0,0))
    outpixels = outimage.load()
    for y in range(0, height):
        for x in range(0, width):
            offset = (y*width + x)*4
            r, g, b, a = [int(rgba[offset + ch]*255) for ch in range(4)]
            outpixels[x, y] = r, g, b
    return outimage

def save_image(width, height, rgba, filename):
    outimage = make_image(width, height, rgba)
    outimage.save(filename, "PNG")

def show_image(width, height, **rgbas):
    import Tkinter as tk
    from PIL import Image, ImageTk

    root = tk.Tk()
    
    x = 64
    y = 64
    
    labels = rgbas.keys()
    labels.sort() 
    for i in range(len(labels)):
        label = labels[i]
        outimage = make_image(width, height, rgbas[label])
        
        if i:
            window = tk.Toplevel(root)
        else:
            window = root    
        window.title(label)
        image1 = ImageTk.PhotoImage(outimage)
        w = image1.width()
        h = image1.height()
        window.geometry("%dx%d+%d+%d" % (w, h, x, y))
        panel1 = tk.Label(window, image=image1)
        panel1.pack(side='top', fill='both', expand='yes')
        panel1.image = image1
        x += w + 2
    
    root.mainloop()



class Test:
    
    def __init__(self):
        pass
    
    def description(self):
        raise NotImplementedError
        
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
            print "Running %s..." % test.description()
            test.run()


class TextureTemplate:
    
    def __init__(self, format=PIPE_FORMAT_R8G8B8A8_UNORM, width=1, height=1, depth=1, last_level=0, target=PIPE_TEXTURE_2D):
        self.format = format
        self.width = width
        self.height = height
        self.depth = depth
        self.last_level = last_level
        self.target = target



