#!/usr/bin/env python

# $Id: getprocaddress.py,v 1.5 2004/10/29 19:31:52 brianp Exp $

# Helper for the getprocaddress.c test.

from xml.sax import saxutils
from xml.sax import make_parser
from xml.sax.handler import feature_namespaces

import sys, getopt, re
sys.path.append("../../src/mesa/glapi/" )
import gl_XML
import license


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
			functions.append(func)
	f.close
	return functions


class PrintExports(gl_XML.FilterGLAPISpecBase):
	name = "gl_exports.py (from Mesa)"

	def __init__(self):
		gl_XML.FilterGLAPISpecBase.__init__(self)
		self.license = license.bsd_license_template % ( \
"""Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
(C) Copyright IBM Corporation 2004""", "BRIAN PAUL, IBM")
		self.tests = FindTestFunctions()
		self.prevCategory = ""

	def printRealHeader(self):
		print """
struct name_test_pair {
   const char *name;
   GLboolean (*test)(void *);
};
   
static struct name_test_pair functions[] = {"""

	def printRealFooter(self):
		print"""
   { NULL, NULL }
};
"""

	def printFunction(self, f):
		if f.category != self.prevCategory:
			print '   { "-%s", NULL},' % f.category
			self.prevCategory = f.category
			
		if f.name in self.tests:
			test = "test_%s" % f.name
		else:
			test = "NULL"
		print '   { "gl%s", %s },' % (f.name, test)
		return


if __name__ == '__main__':
	file_name = "../../src/mesa/glapi/gl_API.xml"
    
	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:")
	except Exception,e:
		show_usage()

	for (arg,val) in args:
		if arg == "-f":
			file_name = val

	dh = PrintExports()

	parser = make_parser()
	parser.setFeature(feature_namespaces, 0)
	parser.setContentHandler(dh)

	f = open(file_name)

	parser.parse(f)
	dh.printHeader()
	dh.printFunctions()
	dh.printFooter()
