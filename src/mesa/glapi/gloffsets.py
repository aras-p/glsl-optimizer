#!/usr/bin/env python

# $Id: gloffsets.py,v 1.1 2000/02/22 22:45:20 brianp Exp $

# Mesa 3-D graphics library
# Version:  3.3
# 
# Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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


# Generate the glapioffsets.h file.
#
# Usage:
#    gloffsets.py >glapioffsets.h
#
# Dependencies:
#    The gl.spec file from the SI must be in the current directory.
#
# Brian Paul  3 February 2000


import string
import re


def PrintHead():
	print '/* DO NOT EDIT - This file generated automatically */'
	print '#ifndef _GLAPI_OFFSETS_H_'
	print '#define _GLAPI_OFFSETS_H_'
	print ''
	return
#endif


def PrintTail():
	print ''
	print '#endif'
#endif


def GenerateDefine(name, offset):
	s = '#define _gloffset_' + name + ' ' + offset
	return s;
#enddef


def PrintDefines():
	functionPattern = re.compile('^[a-zA-Z0-9]+\(')
	functionNamePattern = re.compile('^[a-zA-Z0-9]+')

	funcName = ''

	f = open('gl.spec')
	for line in f.readlines():

		m = functionPattern.match(line)
		if m:
			# extract funcName
			n = functionNamePattern.findall(line)
			funcName = n[0]

		m = string.split(line)
		if len(m) > 1:
			if m[0] == 'param':
				paramName = m[1]
			if m[0] == 'offset':
				funcOffset = m[1]
				s = GenerateDefine(funcName, funcOffset)
				print s
		#endif
	#endfor
#enddef



PrintHead()
PrintDefines()
PrintTail()
