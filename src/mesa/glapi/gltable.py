#!/usr/bin/env python

# $Id: gltable.py,v 1.2 2000/05/11 17:44:42 brianp Exp $

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


# Generate the glapitable.h file.
#
# Usage:
#    gltable.py >glapitable.h
#
# Dependencies:
#    The gl.spec file from the SI must be in the current directory.
#
# Brian Paul  3 February 2000


import string
import re


#
# This table maps types from the gl.spec file to the OpenGL C types.
#
TypeTable = {
	'AttribMask' : 'GLbitfield',
	'Boolean' : 'GLboolean',
	'CheckedFloat32' : 'GLfloat',
	'CheckedInt32' : 'GLint',
	'ClampedColorF' : 'GLclampf',
	'ClampedFloat32' : 'GLclampf',
	'ClampedFloat64' : 'GLclampd',
	'ClampedStencilValue' : 'GLint',
	'ClearBufferMask' : 'GLbitfield',
	'ClientAttribMask' : 'GLbitfield',
	'ColorB' : 'GLbyte',
	'ColorD' : 'GLdouble',
	'ColorF' : 'GLfloat',
	'ColorI' : 'GLint',
	'ColorIndexValueD' : 'GLdouble',
	'ColorIndexValueF' : 'GLfloat',
	'ColorIndexValueI' : 'GLint',
	'ColorIndexValueS' : 'GLshort',
	'ColorIndexValueUB' : 'GLubyte',
	'ColorS' : 'GLshort',
	'ColorUB' : 'GLubyte',
	'ColorUI' : 'GLuint',
	'ColorUS' : 'GLushort',
	'CoordF' : 'GLfloat',
	'CoordD' : 'GLdouble',
	'CoordI' : 'GLint',
	'CoordS' : 'GLshort',
	'FeedbackElement' : 'GLfloat',
	'Float32' : 'GLfloat',
	'Float64' : 'GLdouble',
	'Float32Pointer' : 'GLfloat',
	'Float64Pointer' : 'GLdouble',
	'Int8' : 'GLbyte',
	'Int16' : 'GLshort',
	'Int32' : 'GLint',
	'LineStipple' : 'GLushort',
	'List' : 'GLuint',
	'MaskedColorIndexValueF' : 'GLfloat',
	'MaskedColorIndexValueI' : 'GLuint',
	'MaskedStencilValue' : 'GLuint',
	'PixelInternalFormat' : 'GLenum',
	'SelectName' : 'GLuint',
	'SizeI' : 'GLsizei',
	'StencilValue' : 'GLint',
	'String' : 'const GLubyte *',
	'TexelInternalFormat' : 'GLint',
	'TextureComponentCount' : 'GLint',
	'WinCoord' : 'GLint',
	'UInt8' : 'GLubyte',
	'UInt16' : 'GLushort',
	'UInt32' : 'GLuint',
	'Void' : 'GLvoid',
	'VoidPointer' : 'GLvoid *',
	'void' : 'void',
}



#
# Return C-style argument type string.
# Input:  t = a type like ListMode, Int16, CoordF, etc.
#         pointerQual = '' or '*'
#         constQual = '' or 'const '
# Return:  a string like "const GLubyte *'
#
def ActualType(t, pointerQual, constQual):
	if TypeTable.has_key(t):
		type = TypeTable[t]
	else:
		type = 'GLenum'
	if pointerQual == '':
		s = constQual + type
	else:
		s = constQual + type + ' ' + pointerQual
	return s
#enddef



#
# Convert a Python list of arguments into a string.
#
def ArgListToString(argList):
	result = ''
	i = 1
	n = len(argList)
	for pair in argList:
		result = result + pair[0] + ' ' + pair[1]
		if i < n:
			result = result + ', '
		i = i + 1

	if result == '':
		result = 'void'
	return result
#enddef


#
# Return a dispatch table entry, like "void (*Enable)(GLenum cap);"
#
def MakeTableEntry(retType, funcName, argList, offset):
	s = '   '
	s = s + ActualType(retType, '', '')
	s = s + ' (*'
	s = s + funcName
	s = s + ')('
	s = s + ArgListToString(argList)
	s = s + '); /* '
	s = s + str(offset)
	s = s + ' */'
	return s
#enddef



def GroupFromCategory(category):
	baseCats = [
		'display-list',
		'drawing',
		'drawing-control',
		'feedback',
		'framebuf',
		'misc',
		'modeling',
		'pixel-op',
		'pixel-rw',
		'state-req',
		'xform'
	]

	if baseCats.count(category) > 0:
		return 'GL_1_0'
	else:
		return 'GL_' + category
	#endif
#endif


def PrintGroup(group):
	s = '   /* '
	s = s + group
	s = s + ' */'
	print s
#enddef



#
# Parse gl.spec to generate all the function pointers in the dispatch struct.
#
def PrintTableEntries():
	functionPattern = re.compile('^[a-zA-Z0-9]+\(')
	functionNamePattern = re.compile('^[a-zA-Z0-9]+')

	prevGroup = ''
	funcName = ''
	returnType = ''
	argList = [ ]
	maxOffset = 0
	table = { }

	f = open('gl.spec')
	for line in f.readlines():

		m = functionPattern.match(line)
		if m:
			# extract funcName
			n = functionNamePattern.findall(line)
			funcName = n[0]
			argList = [ ]
		#endif

		m = string.split(line)
		if len(m) > 1:
			# return datatype
			if m[0] == 'return':
				returnType = m[1]
			#endif

			# function parameter
			if m[0] == 'param':
				constQual = ''
				pointerQual = ''
				if len(m) >= 5 and m[4] == 'array':
					pointerQual = '*'
					if m[3] == 'in':
						constQual = 'const '
				paramName = m[1]
				paramType = ActualType(m[2], pointerQual, constQual)

				argList.append( (paramType, paramName) )
			#endif

#			# category
			if m[0] == 'category':
				category = m[1]
				group = GroupFromCategory(category)
				if group != prevGroup:
#					PrintGroup(group)
					prevGroup = group
			#endif

			# end of function spec
			if m[0] == 'offset':
				if m[1] == '?':
					#print 'WARNING: skipping', funcName
					noop = 0
				else:
					funcOffset = int(m[1])
					if funcOffset > maxOffset:
						maxOffset = funcOffset
					#PrintProto(returnType, funcName, argList)
					s = MakeTableEntry(returnType, funcName, argList, funcOffset)
#					print s
					table[funcOffset] = s;
				#endif
			#endif
		#endif
	#endfor

	# Now dump the table, this effectively does the sort by offset number
	for i in range(0, maxOffset + 1):
		if table.has_key(i):
			print table[i]

#enddef



def PrintHead():
	print '/* DO NOT EDIT - This file generated automatically with gltable.py script */'
	print '#ifndef _GLAPI_TABLE_H_'
	print '#define _GLAPI_TABLE_H_'
	print ''
	print '#include <GL/gl.h>'
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



PrintHead()	
PrintTableEntries()
PrintTail()