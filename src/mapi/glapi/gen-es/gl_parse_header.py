#!/usr/bin/python
#
# Copyright (C) 2009 Chia-I Wu <olv@0xlab.org>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# on the rights to use, copy, modify, merge, publish, distribute, sub
# license, and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
# IBM AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

import sys
import os.path
import getopt
import re

GLAPI = "../../glapi/gen"
sys.path.append(GLAPI)

class HeaderParser(object):
    """Parser for GL header files."""

    def __init__(self, verbose=0):
        # match #if and #ifdef
        self.IFDEF = re.compile('#\s*if(n?def\s+(?P<ifdef>\w+)|\s+(?P<if>.+))')
        # match #endif
        self.ENDIF = re.compile('#\s*endif')
        # match typedef abc def;
        self.TYPEDEF = re.compile('typedef\s+(?P<from>[\w ]+)\s+(?P<to>\w+);')
        # match #define XYZ VAL
        self.DEFINE = re.compile('#\s*define\s+(?P<key>\w+)(?P<value>\s+[\w"]*)?')
        # match GLAPI
        self.GLAPI = re.compile('^GL_?API(CALL)?\s+(?P<return>[\w\s*]+[\w*])\s+(GL)?_?APIENTRY\s+(?P<name>\w+)\s*\((?P<params>[\w\s(,*\[\])]+)\)\s*;')

        self.split_params = re.compile('\s*,\s*')
        self.split_ctype = re.compile('(\W)')
        # ignore GL_VERSION_X_Y
        self.ignore_enum = re.compile('GL(_ES)?_VERSION(_ES_C[ML])?_\d_\d')

        self.verbose = verbose
        self._reset()

    def _reset(self):
        """Reset to initial state."""
        self.ifdef_levels = []
        self.need_char = False

    # use typeexpr?
    def _format_ctype(self, ctype, fix=True):
        """Format a ctype string, optionally fix it."""
        # split the type string
        tmp = self.split_ctype.split(ctype)
        tmp = [s for s in tmp if s and s != " "]

        pretty = ""
        for i in xrange(len(tmp)):
            # add missing GL prefix
            if (fix and tmp[i] != "const" and tmp[i] != "*" and
                not tmp[i].startswith("GL")):
                tmp[i] = "GL" + tmp[i]

            if i == 0:
                pretty = tmp[i]
            else:
                sep = " "
                if tmp[i - 1] == "*":
                    sep = ""
                pretty += sep + tmp[i]
        return pretty

    # use typeexpr?
    def _get_ctype_attrs(self, ctype):
        """Get the attributes of a ctype."""
        is_float = (ctype.find("float") != -1 or ctype.find("double") != -1)
        is_signed = not (ctype.find("unsigned")  != -1)

        size = 0
        if ctype.find("char") != -1:
            size = 1
        elif ctype.find("short") != -1:
            size = 2
        elif ctype.find("int") != -1:
            size = 4
        elif is_float:
            if ctype.find("float") != -1:
                size = 4
            else:
                size = 8

        return (size, is_float, is_signed)

    def _parse_define(self, line):
        """Parse a #define line for an <enum>."""
        m = self.DEFINE.search(line)
        if not m:
            if self.verbose and line.find("#define") >= 0:
                print "ignore %s" % (line)
            return None

        key = m.group("key").strip()
        val = m.group("value").strip()

        # enum must begin with GL_ and be all uppercase
        if ((not (key.startswith("GL_") and key.isupper())) or
            (self.ignore_enum.match(key) and val == "1")):
            if self.verbose:
                print "ignore enum %s" % (key)
            return None

        return (key, val)

    def _parse_typedef(self, line):
        """Parse a typedef line for a <type>."""
        m = self.TYPEDEF.search(line)
        if not m:
            if self.verbose and line.find("typedef") >= 0:
                print "ignore %s" % (line)
            return None

        f = m.group("from").strip()
        t = m.group("to").strip()
        if not t.startswith("GL"):
            if self.verbose:
                print "ignore type %s" % (t)
            return None
        attrs = self._get_ctype_attrs(f)

        return (f, t, attrs)

    def _parse_gl_api(self, line):
        """Parse a GLAPI line for a <function>."""
        m = self.GLAPI.search(line)
        if not m:
            if self.verbose and line.find("APIENTRY") >= 0:
                print "ignore %s" % (line)
            return None

        rettype = m.group("return")
        rettype = self._format_ctype(rettype)
        if rettype == "GLvoid":
            rettype = ""

        name = m.group("name")

        param_str = m.group("params")
        chunks = self.split_params.split(param_str)
        chunks = [s.strip() for s in chunks]
        if len(chunks) == 1 and (chunks[0] == "void" or chunks[0] == "GLvoid"):
            chunks = []

        params = []
        for c in chunks:
            # split type and variable name
            idx = c.rfind("*")
            if idx < 0:
                idx = c.rfind(" ")
            if idx >= 0:
                idx += 1
                ctype = c[:idx]
                var = c[idx:]
            else:
                ctype = c
                var = "unnamed"

            # convert array to pointer
            idx = var.find("[")
            if idx >= 0:
                var = var[:idx]
                ctype += "*"

            ctype = self._format_ctype(ctype)
            var = var.strip()

            if not self.need_char and ctype.find("GLchar") >= 0:
                self.need_char = True

            params.append((ctype, var))

        return (rettype, name, params)

    def _change_level(self, line):
        """Parse a #ifdef line and change level."""
        m = self.IFDEF.search(line)
        if m:
            ifdef = m.group("ifdef")
            if not ifdef:
                ifdef = m.group("if")
            self.ifdef_levels.append(ifdef)
            return True
        m = self.ENDIF.search(line)
        if m:
            self.ifdef_levels.pop()
            return True
        return False

    def _read_header(self, header):
        """Open a header file and read its contents."""
        lines = []
        try:
            fp = open(header, "rb")
            lines = fp.readlines()
            fp.close()
        except IOError, e:
            print "failed to read %s: %s" % (header, e)
        return lines

    def _cmp_enum(self, enum1, enum2):
        """Compare two enums."""
        # sort by length of the values as strings
        val1 = enum1[1]
        val2 = enum2[1]
        ret = len(val1) - len(val2)
        # sort by the values
        if not ret:
            val1 = int(val1, 16)
            val2 = int(val2, 16)
            ret = val1 - val2
            # in case int cannot hold the result
            if ret > 0:
                ret = 1
            elif ret < 0:
                ret = -1
        # sort by the names
        if not ret:
            if enum1[0] < enum2[0]:
                ret = -1
            elif enum1[0] > enum2[0]:
                ret = 1
        return ret

    def _cmp_type(self, type1, type2):
        """Compare two types."""
        attrs1 = type1[2]
        attrs2 = type2[2]
        # sort by type size
        ret = attrs1[0] - attrs2[0]
        # float is larger
        if not ret:
            ret = attrs1[1] - attrs2[1]
        # signed is larger
        if not ret:
            ret = attrs1[2] - attrs2[2]
        # reverse
        ret = -ret
        return ret

    def _cmp_function(self, func1, func2):
        """Compare two functions."""
        name1 = func1[1]
        name2 = func2[1]
        ret = 0
        # sort by the names
        if name1 < name2:
            ret = -1
        elif name1 > name2:
            ret = 1
        return ret

    def _postprocess_dict(self, hdict):
        """Post-process a header dict and return an ordered list."""
        hlist = []
        largest = 0
        for key, cat in hdict.iteritems():
            size = len(cat["enums"]) + len(cat["types"]) + len(cat["functions"])
            # ignore empty category
            if not size:
                continue

            cat["enums"].sort(self._cmp_enum)
            # remove duplicates
            dup = []
            for i in xrange(1, len(cat["enums"])):
                if cat["enums"][i] == cat["enums"][i - 1]:
                    dup.insert(0, i)
            for i in dup:
                e = cat["enums"].pop(i)
                if self.verbose:
                    print "remove duplicate enum %s" % e[0]

            cat["types"].sort(self._cmp_type)
            cat["functions"].sort(self._cmp_function)

            # largest category comes first
            if size > largest:
                hlist.insert(0, (key, cat))
                largest = size
            else:
                hlist.append((key, cat))
        return hlist

    def parse(self, header):
        """Parse a header file."""
        self._reset()

        if self.verbose:
            print "Parsing %s" % (header)

        hdict = {}
        lines = self._read_header(header)
        for line in lines:
            if self._change_level(line):
                continue

            # skip until the first ifdef (i.e. __gl_h_)
            if not self.ifdef_levels:
                continue

            cat_name = os.path.basename(header)
            # check if we are in an extension
            if (len(self.ifdef_levels) > 1 and
                self.ifdef_levels[-1].startswith("GL_")):
                cat_name = self.ifdef_levels[-1]

            try:
                cat = hdict[cat_name]
            except KeyError:
                cat = {
                        "enums": [],
                        "types": [],
                        "functions": []
                }
                hdict[cat_name] = cat

            key = "enums"
            elem = self._parse_define(line)
            if not elem:
                key = "types"
                elem = self._parse_typedef(line)
            if not elem:
                key = "functions"
                elem = self._parse_gl_api(line)

            if elem:
                cat[key].append(elem)

        if self.need_char:
            if self.verbose:
                print "define GLchar"
            elem = self._parse_typedef("typedef char GLchar;")
            cat["types"].append(elem)
        return self._postprocess_dict(hdict)

def spaces(n, str=""):
    spaces = n - len(str)
    if spaces < 1:
        spaces = 1
    return " " * spaces

def output_xml(name, hlist):
    """Output a parsed header in OpenGLAPI XML."""

    for i in xrange(len(hlist)):
        cat_name, cat = hlist[i]

        print '<category name="%s">' % (cat_name)
        indent = 4

        for enum in cat["enums"]:
            name = enum[0][3:]
            value = enum[1]
            tab = spaces(41, name)
            attrs = 'name="%s"%svalue="%s"' % (name, tab, value)
            print '%s<enum %s/>' % (spaces(indent), attrs)

        if cat["enums"] and cat["types"]:
            print

        for type in cat["types"]:
            ctype = type[0]
            size, is_float, is_signed = type[2]

            attrs = 'name="%s"' % (type[1][2:])
            attrs += spaces(16, attrs) + 'size="%d"' % (size)
            if is_float:
                attrs += ' float="true"'
            elif not is_signed:
                attrs += ' unsigned="true"'

            print '%s<type %s/>' % (spaces(indent), attrs)

        for func in cat["functions"]:
            print
            ret = func[0]
            name = func[1][2:]
            params = func[2]

            attrs = 'name="%s" offset="assign"' % name
            print '%s<function %s>' % (spaces(indent), attrs)

            for param in params:
                attrs = 'name="%s" type="%s"' % (param[1], param[0])
                print '%s<param %s/>' % (spaces(indent * 2), attrs)
            if ret:
                attrs = 'type="%s"' % ret
                print '%s<return %s/>' % (spaces(indent * 2), attrs)

            print '%s</function>' % spaces(indent)

        print '</category>'
        print

def show_usage():
    print "Usage: %s [-v] <header> ..." % sys.argv[0]
    sys.exit(1)

def main():
    try:
        args, headers = getopt.getopt(sys.argv[1:], "v")
    except Exception, e:
        show_usage()
    if not headers:
        show_usage()

    verbose = 0
    for arg in args:
        if arg[0] == "-v":
            verbose += 1

    need_xml_header = True
    parser = HeaderParser(verbose)
    for h in headers:
        h = os.path.abspath(h)
        hlist = parser.parse(h)

        if need_xml_header:
            print '<?xml version="1.0"?>'
            print '<!DOCTYPE OpenGLAPI SYSTEM "%s/gl_API.dtd">' % GLAPI
            need_xml_header = False

        print
        print '<!-- %s -->' % (h)
        print '<OpenGLAPI>'
        print
        output_xml(h, hlist)
        print '</OpenGLAPI>'

if __name__ == '__main__':
    main()
