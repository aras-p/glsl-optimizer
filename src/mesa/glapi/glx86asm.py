#!/usr/bin/env python


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


# Generate the src/X86/glapi_x86.S file.
#
# Usage:
#    gloffsets.py >glapi_x86.S
#
# Dependencies:
#    The apispec file must be in the current directory.


import apiparser


def PrintHead():
	print '/* DO NOT EDIT - This file generated automatically with glx86asm.py script */'
	print '#include "assyntax.h"'
	print '#include "glapioffsets.h"'
	print ''
	print '#ifndef __WIN32__'
	print ''	
	print '#if defined(STDCALL_API)'
	print '#define GL_PREFIX(n,n2) GLNAME(CONCAT(gl,n2))'
	print '#elif defined(USE_MGL_NAMESPACE)'
	print '#define GL_PREFIX(n,n2) GLNAME(CONCAT(mgl,n))'
	print '#else'
	print '#define GL_PREFIX(n,n2) GLNAME(CONCAT(gl,n))'
	print '#endif'
	print ''
	print '#define GL_OFFSET(x) CODEPTR(REGOFF(4 * x, EAX))'
	print ''
	print '#if defined(GNU_ASSEMBLER) && !defined(__DJGPP__) && !defined(__MINGW32__)'
	print '#define GLOBL_FN(x) GLOBL x ; .type x,@function'
	print '#else'
	print '#define GLOBL_FN(x) GLOBL x'
	print '#endif'
	print ''
	print 'SEG_TEXT'
	print ''
	print 'EXTERN GLNAME(_glapi_Dispatch)'
	print ''
	return
#enddef


def PrintTail():
	print ''
	print '#endif  /* __WIN32__ */'
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

# Find the size of the arguments on the stack for _stdcall name mangling
def FindStackSize(typeList):
	result = 0
	for typ in typeList:
		if typ == 'GLdouble' or typ == 'GLclampd':
			result += 8;
		else:
			result += 4;
		#endif
	#endfor
	return result
#enddef

def EmitFunction(name, returnType, argTypeList, argNameList, alias, offset):
	argList = apiparser.MakeArgList(argTypeList, argNameList)
	if alias != '':
		dispatchName = alias
	else:
		dispatchName = name
	#endif
	
	if offset < 0:
		# try to find offset from alias name
		assert dispatchName != ''
		offset = FindOffset(dispatchName)
		if offset == -1:
			#print 'Cannot dispatch %s' % name
			return
		#endif
	#endif

	# save this info in case we need to look up an alias later
	records.append((name, dispatchName, offset))

	# Find the argument stack size for _stdcall name mangling
	stackSize = FindStackSize(argTypeList)

	# print the assembly code
	print 'ALIGNTEXT16'
	print "GLOBL_FN(GL_PREFIX(%s,%s@%s))" % (name, name, stackSize)
	print "GL_PREFIX(%s,%s@%s):" % (name, name, stackSize)
	print '\tMOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX)'
	print "\tJMP(GL_OFFSET(_gloffset_%s))" % (dispatchName)
	print ''
#enddef

PrintHead()
apiparser.ProcessSpecFile("APIspec", EmitFunction)
PrintTail()
