#!/usr/bin/env python

# $Id: glx86asm.py,v 1.2 2000/09/06 17:33:40 brianp Exp $

# Mesa 3-D graphics library
# Version:  3.4
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


# Generate the glapi_x86.S assembly language file.
#
# Usage:
#    glx86asm.py >glapi_x86.S
#
# Dependencies:
#    The gl.spec file from the SI must be in the current directory.
#
# Brian Paul  11 May 2000


import string
import re


def PrintHead():
	print '/* DO NOT EDIT - This file generated automatically with glx86asm.py script */'
	print '#include "assyntax.h"'
	print '#include "glapioffsets.h"'
	print ''
	print '#ifndef __WIN32__'
	print ''
	print '#if defined(USE_MGL_NAMESPACE)'
	print '#define GL_PREFIX(n) GLNAME(CONCAT(mgl,n))'
	print '#else'
	print '#define GL_PREFIX(n) GLNAME(CONCAT(gl,n))'
	print '#endif'
	print ''
	print '#define GL_OFFSET(x) CODEPTR(REGOFF(4 * x, EAX))'
	print ''
	print '#ifdef GNU_ASSEMBLER'
	print '#define GLOBL_FN(x) GLOBL x ; .type x,@function'
	print '#else'
	print '#define GLOBL_FN(x) GLOBL x'
	print '#endif'
	print ''
	print ''
	return
#endif


def PrintTail():
	print ''
	print '#endif  /* __WIN32__ */'
#endif


def GenerateDispatchCode(name, offset):
	print 'ALIGNTEXT16'
	print "GLOBL_FN(GL_PREFIX(%s))" % (name)
	print "GL_PREFIX(%s):" % (name)
	print '\tMOV_L(GLNAME(_glapi_Dispatch), EAX)'
	print '\tTEST_L(EAX, EAX)'
	print "\tJZ(GLNAME(_glapi_fallback_%s))" % (name)
	print "\tJMP(GL_OFFSET(_gloffset_%s))" % (offset)
	print ''
#enddef


def FindAlias(list, funcName):
	for i in range(0, len(list)):
		entry = list[i]
		if entry[0] == funcName:
			return entry[1]
		#endif
	#endfor
	return ''
#enddef



def PrintDefines():
	functionPattern = re.compile('^[a-zA-Z0-9]+\(')
	functionNamePattern = re.compile('^[a-zA-Z0-9]+')

	funcName = ''
	functions = [ ]

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
				if m[1] == '?':
					#print 'WARNING skipping', funcName
					noop = 0
				else:
					entry = [ funcName, funcName ]
					functions.append(entry)
				#endif
			elif m[0] == 'alias':
				aliasedName = FindAlias(functions, m[1])
				if aliasedName:
					entry = [ funcName, aliasedName ]
					functions.append(entry)
				else:
					print 'WARNING: alias to unknown function:', aliasedName
				#endif
			#endif
		#endif
	#endfor

	# Now generate the assembly dispatch code
	for i in range(0, len(functions)):
		entry = functions[i]
		GenerateDispatchCode( entry[0], entry[1] )

#enddef



PrintHead()
PrintDefines()
PrintTail()
