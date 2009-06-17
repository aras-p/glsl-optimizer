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


"""Base classes for tests.

Loosely inspired on Python's unittest module.
"""


import os.path
import sys

from gallium import *


# Enumerate all pixel formats
formats = {}
for name, value in globals().items():
    if name.startswith("PIPE_FORMAT_") and isinstance(value, int):
        formats[value] = name

def is_depth_stencil_format(format):
    # FIXME: make and use binding to pf_is_depth_stencil
    return format in (
        PIPE_FORMAT_Z32_UNORM,
        PIPE_FORMAT_Z24S8_UNORM,
        PIPE_FORMAT_Z24X8_UNORM,
        PIPE_FORMAT_Z16_UNORM,
    )

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
            r, g, b, a = [int(min(max(rgba[offset + ch], 0.0), 1.0)*255) for ch in range(4)]
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


class TestFailure(Exception):

    pass

class TestSkip(Exception):
    
    pass


class Test:

    def __init__(self):
        pass

    def _run(self, result):
        raise NotImplementedError
    
    def run(self):
        result = TestResult()
        self._run(result)
        result.summary()

    def assert_rgba(self, surface, x, y, w, h, expected_rgba, pixel_tol=4.0/256, surface_tol=0.85):
        total = h*w
        different = surface.compare_tile_rgba(x, y, w, h, expected_rgba, tol=pixel_tol)
        if different:
            sys.stderr.write("%u out of %u pixels differ\n" % (different, total))

        if float(total - different)/float(total) < surface_tol:
            if 0:
                rgba = FloatArray(h*w*4)
                surface.get_tile_rgba(x, y, w, h, rgba)
                show_image(w, h, Result=rgba, Expected=expected_rgba)
                save_image(w, h, rgba, "result.png")
                save_image(w, h, expected_rgba, "expected.png")
            #sys.exit(0)
            
            raise TestFailure


class TestCase(Test):
    
    tags = ()

    def __init__(self, dev, **kargs):
        Test.__init__(self)
        self.dev = dev
        self.__dict__.update(kargs)

    def description(self):
        descriptions = []
        for tag in self.tags:
            value = self.get(tag)
            if value is not None and value != '':
                descriptions.append(tag + '=' + str(value))
        return ' '.join(descriptions)

    def get(self, tag):
        try:
            method = getattr(self, '_get_' + tag)
        except AttributeError:
            return getattr(self, tag, None)
        else:
            return method()

    def _get_target(self):
        return {
            PIPE_TEXTURE_1D: "1d", 
            PIPE_TEXTURE_2D: "2d", 
            PIPE_TEXTURE_3D: "3d", 
            PIPE_TEXTURE_CUBE: "cube",
        }[self.target]

    def _get_format(self):
        name = formats[self.format]
        if name.startswith('PIPE_FORMAT_'):
            name  = name[12:]
        name = name.lower()
        return name

    def _get_face(self):
        if self.target == PIPE_TEXTURE_CUBE:
            return {
                PIPE_TEX_FACE_POS_X: "+x",
                PIPE_TEX_FACE_NEG_X: "-x",
                PIPE_TEX_FACE_POS_Y: "+y",
                PIPE_TEX_FACE_NEG_Y: "-y", 
                PIPE_TEX_FACE_POS_Z: "+z", 
                PIPE_TEX_FACE_NEG_Z: "-z",
            }[self.face]
        else:
            return ''

    def test(self):
        raise NotImplementedError
    
    def _run(self, result):
        result.test_start(self)
        try:
            self.test()
        except KeyboardInterrupt:
            raise
        except TestSkip:
            result.test_skipped(self)
        except TestFailure:
            result.test_failed(self)
        else:
            result.test_passed(self)


class TestSuite(Test):
    
    def __init__(self, tests = None):
        Test.__init__(self)
        if tests is None:
            self.tests = []
        else:
            self.tests = tests

    def add_test(self, test):
        self.tests.append(test) 
    
    def _run(self, result):
        for test in self.tests:
            test._run(result)


class TestResult:
    
    def __init__(self):
        self.tests = 0
        self.passed = 0
        self.skipped = 0
        self.failed = 0

        self.names = ['result']
        self.types = ['pass skip fail']
        self.rows = []
    
    def test_start(self, test):
        sys.stdout.write("Running %s...\n" % test.description())
        sys.stdout.flush()
        self.tests += 1
    
    def test_passed(self, test):
        sys.stdout.write("PASS\n")
        sys.stdout.flush()
        self.passed += 1
        self.log_result(test, 'pass')
            
    def test_skipped(self, test):
        sys.stdout.write("SKIP\n")
        sys.stdout.flush()
        self.skipped += 1
        #self.log_result(test, 'skip')
        
    def test_failed(self, test):
        sys.stdout.write("FAIL\n")
        sys.stdout.flush()
        self.failed += 1
        self.log_result(test, 'fail')

    def log_result(self, test, result):
        row = ['']*len(self.names)

        # add result
        assert self.names[0] == 'result'
        assert result in ('pass', 'skip', 'fail')
        row[0] = result

        # add tags
        for tag in test.tags:
            value = test.get(tag)

            # infer type
            if value is None:
                continue
            elif isinstance(value, (int, float)):
                value = str(value)
                type = 'c' # continous
            elif isinstance(value, basestring):
                type = 'd' # discrete
            else:
                assert False
                value = str(value)
                type = 'd' # discrete

            # insert value
            try:
                col = self.names.index(tag, 1)
            except ValueError:
                self.names.append(tag)
                self.types.append(type)
                row.append(value)
            else:
                row[col] = value
                assert self.types[col] == type
        
        self.rows.append(row)

    def summary(self):
        sys.stdout.write("%u tests, %u passed, %u skipped, %u failed\n\n" % (self.tests, self.passed, self.skipped, self.failed))
        sys.stdout.flush()

        name, ext = os.path.splitext(os.path.basename(sys.argv[0]))
        filename = name + '.tsv'
        stream = file(filename, 'wt')

        # header
        stream.write('\t'.join(self.names) + '\n')
        stream.write('\t'.join(self.types) + '\n')
        stream.write('class\n')

        # rows
        for row in self.rows:
            row += ['']*(len(self.names) - len(row))
            stream.write('\t'.join(row) + '\n')

        stream.close()

        # See http://www.ailab.si/orange/doc/ofb/c_otherclass.htm
        try:
            import orange
            import orngTree
        except ImportError:
            sys.stderr.write('Install Orange from http://www.ailab.si/orange/ for a classification tree.\n')
            return

        data = orange.ExampleTable(filename)

        tree = orngTree.TreeLearner(data, sameMajorityPruning=1, mForPruning=2)

        orngTree.printTxt(tree, maxDepth=4)

        file(name+'.txt', 'wt').write(orngTree.dumpTree(tree))

        orngTree.printDot(tree, fileName=name+'.dot', nodeShape='ellipse', leafShape='box')
