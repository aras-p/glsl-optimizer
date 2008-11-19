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


class Formatter:
    '''Plain formatter'''

    def __init__(self, stream):
        self.stream = stream

    def text(self, text):
        self.stream.write(text)

    def newline(self):
        self.text('\n')

    def function(self, name):
        self.text(name)

    def variable(self, name):
        self.text(name)

    def literal(self, value):
        self.text(str(value))

    def address(self, addr):
        self.text(str(addr))


class AnsiFormatter(Formatter):
    '''Formatter for plain-text files which outputs ANSI escape codes. See
    http://en.wikipedia.org/wiki/ANSI_escape_code for more information
    concerning ANSI escape codes.
    '''

    _csi = '\33['

    _normal = '0m'
    _bold = '1m'
    _italic = '3m'
    _red = '31m'
    _green = '32m'
    _blue = '34m'

    def _escape(self, code):
        self.text(self._csi + code)

    def function(self, name):
        self._escape(self._bold)
        Formatter.function(self, name)
        self._escape(self._normal)

    def variable(self, name):
        self._escape(self._italic)
        Formatter.variable(self, name)
        self._escape(self._normal)

    def literal(self, value):
        self._escape(self._blue)
        Formatter.literal(self, value)
        self._escape(self._normal)

    def address(self, value):
        self._escape(self._green)
        Formatter.address(self, value)
        self._escape(self._normal)



def DefaultFormatter(stream):
    if stream.isatty():
        return AnsiFormatter(stream)
    else:
        return Formatter(stream)

