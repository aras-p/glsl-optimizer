#!/usr/bin/env python

# $Id: glprocs.py,v 1.1 2001/11/18 22:42:57 brianp Exp $

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


# Generate the glprocs.h file.
#
# Usage:
#    gloffsets.py >glprocs.h
#
# Dependencies:
#    The apispec file must be in the current directory.



import apiparser
import string


def PrintHead():
	print '/* DO NOT EDIT - This file generated automatically by glprocs.py script */'
	print ''
	print '/* This file is only included by glapi.c and is used for'
	print ' * the GetProcAddress() function'
	print ' */'
	print ''
	print 'static struct name_address_offset static_functions[] = {'
	return
#enddef


def PrintTail():
	print '   { NULL, NULL }  /* end of list marker */'
	print '};'
#enddef


records = []

def FindOffset(funcName):
	for (name, alias, offset) in records:
		if name == funcName:
			return offset
		#endif
	#endfor
	return -1
#enddef


def EmitEntry(name, returnType, argTypeList, argNameList, alias, offset):
	if alias == '':
		dispatchName = name
	else:
		dispatchName = alias
	if offset < 0:
		offset = FindOffset(dispatchName)
	if offset >= 0 and string.find(name, "unused") == -1:
		print '   { "gl%s", (GLvoid *) gl%s, _gloffset_%s },' % (name, name, dispatchName)
		# save this info in case we need to look up an alias later
		records.append((name, dispatchName, offset))

#enddef


PrintHead()
apiparser.ProcessSpecFile("APIspec", EmitEntry)
PrintTail()
