#!/usr/bin/env python


# Mesa 3-D graphics library
# Version:  5.1
# 
# Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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


# Generate the glapitable.h file.
#
# Usage:
#    python gloffsets.py >glapitable.h
#
# Dependencies:
#    The apispec file must be in the current directory.


import apiparser;


def PrintHead():
        print '/* DO NOT EDIT - This file generated automatically with gltable.py script */'
        print '#ifndef _GLAPI_TABLE_H_'
        print '#define _GLAPI_TABLE_H_'
        print ''
        print '#ifndef GLAPIENTRYP'
        print '#define GLAPIENTRYP'
        print '#endif'
        print ''
        print 'struct _glapi_table'
        print '{'
        return
#endif


def PrintTail():
        print '};'
        print ''
        print '#endif'
#endif


records = {}

def DoRecord(name, returnType, argTypeList, argNameList, alias, offset):
	argList = apiparser.MakeArgList(argTypeList, argNameList)
	if offset >= 0 and not records.has_key(offset):
		records[offset] = (name, returnType, argList)
		#print '#define _gloffset_%s %d' % (name, offset)
#endif


def PrintRecords():
	keys = records.keys()
	keys.sort()
	prevk = -1
	for k in keys:
		if k != prevk + 1:
			#print 'Missing offset %d' % (prevk)
			pass
		prevk = int(k)
		(name, returnType, argList) = records[k]
		print '   %s (GLAPIENTRYP %s)(%s); /* %d */' % (returnType, name, argList, k)
#endef


PrintHead()
apiparser.ProcessSpecFile("APIspec", DoRecord)
PrintRecords()
PrintTail()

