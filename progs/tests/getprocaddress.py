#!/usr/bin/env python

# $Id: getprocaddress.py,v 1.1 2002/11/08 15:35:47 brianp Exp $

# Helper for the getprocaddress.c test.


import re, string

def PrintHead():
	print """
struct name_test_pair {
   const char *name;
   GLboolean (*test)(void *);
};
   
static struct name_test_pair functions[] = {"""


def PrintTail():
	print"""
   { NULL, NULL }
};
"""


def HaveTest(function):
	testFuncs = [
		"glActiveTextureARB",
		"glSampleCoverageARB"
	]
	if function in testFuncs:
		return 1
	else:
		return 0


def FindTestFunctions():
	"""Scan getprocaddress.c for lines that start with "test_" to find
	extension function tests.  Return a list of names found."""
	functions = []
	f = open("getprocaddress.c")
	if not f:
		return functions
	for line in f.readlines():
		v = re.search("^test_([a-zA-Z0-9]+)", line)
		if v:
			func = v.group(1)
			#print "Found -%s-" % func
			functions.append(func)
	f.close
	return functions


def PrintFunctions(specFile, tests):

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
						print '   { "-%s", NULL},' % category
						prevCategory = category

#					if HaveTest("gl" + funcName):
					if funcName in tests:
						test = "test_%s" % funcName
					else:
						test = "NULL"
					print '   { "gl%s", %s },' % (funcName, test)
				funcName = tokens[1]

			elif tokens[0] == 'category':
				category = tokens[1]

			#endif
		#endif
	#endfor
#enddef


tests = FindTestFunctions()
PrintHead()
PrintFunctions("../bin/APIspec", tests)
PrintTail()


