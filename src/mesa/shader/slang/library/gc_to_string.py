#!/usr/bin/env python

# Mesa 3-D graphics library
# Version:  6.3
# 
# Copyright (C) 2005  Brian Paul   All Rights Reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
# AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# author: Michal Krol

# converts slang source file into a C header containing one big string
# comments and trailing whitespaces are stripped
# C style comments are not supported, only C++ style ones are
# empty lines are kept to maintain line numbers correlation
# escape characters are not handled except for newlines

# example:
# -- source file
# // some comment
# 
# attribute vec4 myPosition;	// my vertex data
# -- output file
# "\n"
# "\n"
# "attribute vec4 myPosition;\n"

# usage: gc_to_string.py filename.gc > filename_gc.h

import sys

f = open (sys.argv[1], 'r')
s = f.readline ()
while s != '':
	s = s[0:s.find ('//')].rstrip ()
	# output empty lines, too, so line numbers can be tracked
	print '\"' + s + '\\n\"'
	s = f.readline ()
f.close ()

