#!/usr/bin/env python

# $Id: gloffsets.py,v 1.5 2001/11/18 22:42:57 brianp Exp $

# Mesa 3-D graphics library
# Version:  4.1
# 
# Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
#    The apispec file must be in the current directory.



import apiparser;


def PrintHead():
	print '/* DO NOT EDIT - This file generated automatically by gloffsets.py script */'
	print '#ifndef _GLAPI_OFFSETS_H_'
	print '#define _GLAPI_OFFSETS_H_'
	print ''
	return
#enddef


def PrintTail():
	print ''
	print '#endif'
#enddef


records = {}

def AddOffset(name, returnType, argTypeList, argNameList, alias, offset):
	argList = apiparser.MakeArgList(argTypeList, argNameList)
	if offset >= 0 and not records.has_key(offset):
		records[offset] = name
		#print '#define _gloffset_%s %d' % (name, offset)
#enddef


def PrintRecords():
	keys = records.keys()
	keys.sort()
	prevk = -1
	for k in keys:
		if k != prevk + 1:
			#print 'Missing offset %d' % (prevk)
			pass
		prevk = int(k)
		name = records[k]
		print '#define _gloffset_%s %d' % (name, k)
#endef




PrintHead()
apiparser.ProcessSpecFile("APIspec", AddOffset)
PrintRecords()
PrintTail()
