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


# Generate the src/SPARC/glapi_sparc.S file.
#
# Usage:
#    gloffsets.py >glapi_sparc.S
#
# Dependencies:
#    The apispec file must be in the current directory.


import apiparser;


def PrintHead():
	print '/* DO NOT EDIT - This file generated automatically with glsparcasm.py script */'
	print '#include "glapioffsets.h"'
	print ''
	print '/* The _glapi_Dispatch symbol addresses get relocated into the'
	print ' * sethi/or instruction sequences below at library init time.'
	print ' */'
	print ''
	print ''
	print '.text'
	print '.align 32'
	print '.globl __glapi_sparc_icache_flush'
	print '__glapi_sparc_icache_flush: /* %o0 = insn_addr */'
	print '\tflush\t%o0'
	print '\tretl'
	print '\tnop'
	print ''
	print '.data'
	print '.align 64'
	print ''
	print '.globl _mesa_sparc_glapi_begin'
	print '.type _mesa_sparc_glapi_begin,#function'
	print '_mesa_sparc_glapi_begin:'
	return
#endif

def PrintTail():
	print '\t nop'
	print ''
	print '.globl _mesa_sparc_glapi_end'
	print '.type _mesa_sparc_glapi_end,#function'
	print '_mesa_sparc_glapi_end:'
	print ''
#endif



records = []

def FindOffset(funcName):
	for (name, alias, offset) in records:
		if name == funcName:
			return offset
		#endif
	#endfor
	return -1
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

	# print the assembly code
	print ''
	print '.globl gl%s' % (name)
	print '.type gl%s,#function' % (name)
	print 'gl%s:' % (name)
	print '#if defined(__sparc_v9__) && !defined(__linux__)'
	print '\tsethi\t%hi(0x00000000), %g2'
	print '\tsethi\t%hi(0x00000000), %g1'
	print '\tor\t%g2, %lo(0x00000000), %g2'
	print '\tor\t%g1, %lo(0x00000000), %g1'
	print '\tsllx\t%g2, 32, %g2'
	print '\tldx\t[%g1 + %g2], %g1'
	print "\tsethi\t%%hi(8 * _gloffset_%s), %%g2" % (dispatchName)
	print "\tor\t%%g2, %%lo(8 * _gloffset_%s), %%g2" % (dispatchName)
	print '\tldx\t[%g1 + %g2], %g3'
	print '#else'
	print '\tsethi\t%hi(0x00000000), %g1'
	print '\tld\t[%g1 + %lo(0x00000000)], %g1'
	print "\tld\t[%%g1 + (4 * _gloffset_%s)], %%g3" % (dispatchName)
	print '#endif'
	print '\tjmpl\t%g3, %g0'
	print '\tnop'
#enddef


PrintHead()
apiparser.ProcessSpecFile("APIspec", EmitFunction)
PrintTail()
