#!/usr/bin/env python

# $Id: apiparser.py,v 1.2 2003/08/19 01:06:24 brianp Exp $

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


# These helper functions are used by the other Mesa Python scripts.
# The main function is ProcessSpecFile(spedFile, function) which parses
# the named spec file and calls function() for each entry in the spec file.


import string


# Given parallel arrays of types and names, make a C-style parameter string
def MakeArgList(typeList, nameList):
	result = ''
	i = 1
	n = len(typeList)
	for typ in typeList:
		result = result + typ + ' ' + nameList[i - 1]
		if i < n:
			result = result + ', '
		i = i + 1
	#endfor
	if result == '':
		result = 'void'
	#endif
	return result
#enddef


prevCatagory = ''

#
# Example callback function for ProcessSpecFile()
#
def PrintRecord(name, returnType, argTypeList, argNameList, alias, offset):
	argList = MakeArgList(argTypeList, argNameList)
	if category != prevCategory or prevCategory == '':
		print '\n/* %s */' % category
		prevCategory = category
	#endif
	print '%s gl%s(%s); /* %d */' % (returnType, name, argList, offset)
#endfor


#
# Process the api spec file
#
def ProcessSpecFile(specFile, userFunc):

	NO_OFFSET = -2
	
	# init some vars
	prevCategory = ''
	funcName = ''
	returnType = ''
	argTypeList = [ ]
	argNameList = [ ]
	maxOffset = 0
	table = { }
	offset = -1
	alias = ''

	f = open(specFile)
	for line in f.readlines():

		# split line into tokens
		tokens = string.split(line)

		if len(tokens) > 0 and line[0] != '#':

			if tokens[0] == 'name':
				if funcName != '':
					# Verify entry has offset or alias
					pnts = 0
					if offset == NO_OFFSET:
						pnts = pnts + 1
					if offset >= 0:
						pnts = pnts + 1
					if alias != '':
						pnts = pnts + 1
					if pnts != 1:
						print 'XXXXXXXXXX bad entry for %s' % funcName
						
					# process the function now
					userFunc (funcName, returnType, argTypeList, argNameList, alias, offset)
					# reset the lists
					argTypeList = [ ]
					argNameList = [ ]
					returnType = ''
					offset = -1
					alias = ''

				funcName = tokens[1]

			elif tokens[0] == 'return':
				returnType = string.join(tokens[1:], ' ')
			
			elif tokens[0] == 'param':
				argNameList.append(tokens[1])
				argTypeList.append(string.join(tokens[2:], ' '))

			elif tokens[0] == 'category':
				category = tokens[1]

			elif tokens[0] == 'offset':
				if tokens[1] == '?':
					offset = NO_OFFSET
				else:
					offset = int(tokens[1])
					if offset > maxOffset:
						maxOffset = offset
#				else:
#					print 'Unassigned offset for %s' % funcName

			elif tokens[0] == 'alias':
				alias = tokens[1]

			else:
				print 'Invalid token %s after function %s' % (tokens[0], funcName)
			#endif
		#endif
	#endfor
#enddef
