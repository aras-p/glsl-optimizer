#!/usr/bin/env python

# $Id: functions.py,v 1.1 2001/11/18 23:16:56 brianp Exp $

# Helper for the getprocaddress.c test.


import string

def PrintHead():
	print """
static const char *functions[] = {"""


def PrintTail():
	print"""
   NULL
};
"""


def PrintFunctions(specFile):

	# init some vars
	prevCategory = ''
	funcName = ''

	f = open(specFile)
	for line in f.readlines():

		# split line into tokens
		tokens = string.split(line)

		if len(tokens) > 0 and line[0] != '#':

			if tokens[0] == 'name':
				if funcName != '':
					if category != prevCategory:
						print '   "-%s",' % category
						prevCategory = category

					print '   "gl%s",' % funcName
				funcName = tokens[1]

			elif tokens[0] == 'category':
				category = tokens[1]

			#endif
		#endif
	#endfor
#enddef


PrintHead()
PrintFunctions("../bin/APIspec")
PrintTail()
