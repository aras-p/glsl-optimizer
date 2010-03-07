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

GLAPI = "../../glapi/gen"
sys.path.append(GLAPI)

import gl_XML
import glX_XML

class ApiSet(object):
    def __init__(self, api, elts=["enum", "type", "function"]):
        self.api = api
        self.elts = elts

    def _check_enum(self, e1, e2, strict=True):
        if e1.name != e2.name:
            raise ValueError("%s: name mismatch" % e1.name)
        if e1.value != e2.value:
            raise ValueError("%s: value 0x%04x != 0x%04x"
                    % (e1.name, e1.value, e2.value))

    def _check_type(self, t1, t2, strict=True):
        if t1.name != t2.name:
            raise ValueError("%s: name mismatch" % t1.name)
        if t1.type_expr.string() != t2.type_expr.string():
            raise ValueError("%s: type %s != %s"
                    % (t1.name, t1.type_expr.string(), t2.type_expr.string()))

    def _check_function(self, f1, f2, strict=True):
        if f1.name != f2.name:
            raise ValueError("%s: name mismatch" % f1.name)
        if f1.return_type != f2.return_type:
            raise ValueError("%s: return type %s != %s"
                    % (f1.name, f1.return_type, f2.return_type))
        # there might be padded parameters
        if strict and len(f1.parameters) != len(f2.parameters):
            raise ValueError("%s: parameter length %d != %d"
                    % (f1.name, len(f1.parameters), len(f2.parameters)))
        if f1.assign_offset != f2.assign_offset:
            if ((f1.assign_offset and f2.offset < 0) or
                (f2.assign_offset and f1.offset < 0)):
                raise ValueError("%s: assign offset %d != %d"
                        % (f1.name, f1.assign_offset, f2.assign_offset))
        elif not f1.assign_offset:
            if f1.offset != f2.offset:
                raise ValueError("%s: offset %d != %d"
                        % (f1.name, f1.offset, f2.offset))

        if strict:
            l1 = f1.entry_points
            l2 = f2.entry_points
            l1.sort()
            l2.sort()
            if l1 != l2:
                raise ValueError("%s: entry points %s != %s"
                        % (f1.name, l1, l2))

            l1 = f1.static_entry_points
            l2 = f2.static_entry_points
            l1.sort()
            l2.sort()
            if l1 != l2:
                raise ValueError("%s: static entry points %s != %s"
                        % (f1.name, l1, l2))

        pad = 0
        for i in xrange(len(f1.parameters)):
            p1 = f1.parameters[i]
            p2 = f2.parameters[i + pad]

            if not strict and p1.is_padding != p2.is_padding:
                if p1.is_padding:
                    pad -= 1
                    continue
                else:
                    pad += 1
                    p2 = f2.parameters[i + pad]

            if strict and p1.name != p2.name:
                raise ValueError("%s: parameter %d name %s != %s"
                        % (f1.name, i, p1.name, p2.name))
            if p1.type_expr.string() != p2.type_expr.string():
                if (strict or
                    # special case
                    f1.name == "TexImage2D" and p1.name != "internalformat"):
                    raise ValueError("%s: parameter %s type %s != %s"
                            % (f1.name, p1.name, p1.type_expr.string(),
                               p2.type_expr.string()))

    def union(self, other):
        union = gl_XML.gl_api(None)

        if "enum" in self.elts:
            union.enums_by_name = other.enums_by_name.copy()
            for key, val in self.api.enums_by_name.iteritems():
                if key not in union.enums_by_name:
                    union.enums_by_name[key] = val
                else:
                    self._check_enum(val, other.enums_by_name[key])

        if "type" in self.elts:
            union.types_by_name = other.types_by_name.copy()
            for key, val in self.api.types_by_name.iteritems():
                if key not in union.types_by_name:
                    union.types_by_name[key] = val
                else:
                    self._check_type(val, other.types_by_name[key])

        if "function" in self.elts:
            union.functions_by_name = other.functions_by_name.copy()
            for key, val in self.api.functions_by_name.iteritems():
                if key not in union.functions_by_name:
                    union.functions_by_name[key] = val
                else:
                    self._check_function(val, other.functions_by_name[key])

        return union

    def intersection(self, other):
        intersection = gl_XML.gl_api(None)

        if "enum" in self.elts:
            for key, val in self.api.enums_by_name.iteritems():
                if key in other.enums_by_name:
                    self._check_enum(val, other.enums_by_name[key])
                    intersection.enums_by_name[key] = val

        if "type" in self.elts:
            for key, val in self.api.types_by_name.iteritems():
                if key in other.types_by_name:
                    self._check_type(val, other.types_by_name[key])
                    intersection.types_by_name[key] = val

        if "function" in self.elts:
            for key, val in self.api.functions_by_name.iteritems():
                if key in other.functions_by_name:
                    self._check_function(val, other.functions_by_name[key])
                    intersection.functions_by_name[key] = val

        return intersection

    def difference(self, other):
        difference = gl_XML.gl_api(None)

        if "enum" in self.elts:
            for key, val in self.api.enums_by_name.iteritems():
                if key not in other.enums_by_name:
                    difference.enums_by_name[key] = val
                else:
                    self._check_enum(val, other.enums_by_name[key])

        if "type" in self.elts:
            for key, val in self.api.types_by_name.iteritems():
                if key not in other.types_by_name:
                    difference.types_by_name[key] = val
                else:
                    self._check_type(val, other.types_by_name[key])

        if "function" in self.elts:
            for key, val in self.api.functions_by_name.iteritems():
                if key not in other.functions_by_name:
                    difference.functions_by_name[key] = val
                else:
                    self._check_function(val, other.functions_by_name[key], False)

        return difference

def cmp_enum(e1, e2):
    if e1.value < e2.value:
        return -1
    elif e1.value > e2.value:
        return 1
    else:
        return 0

def cmp_type(t1, t2):
    return t1.size - t2.size

def cmp_function(f1, f2):
    if f1.name > f2.name:
        return 1
    elif f1.name < f2.name:
        return -1
    else:
        return 0

def spaces(n, str=""):
    spaces = n - len(str)
    if spaces < 1:
        spaces = 1
    return " " * spaces

def output_enum(e, indent=0):
    attrs = 'name="%s"' % e.name
    if e.default_count > 0:
        tab = spaces(37, attrs)
        attrs += '%scount="%d"' % (tab, e.default_count)
    tab = spaces(48, attrs)
    val = "%04x" % e.value
    val = "0x" + val.upper()
    attrs += '%svalue="%s"' % (tab, val)

    # no child
    if not e.functions:
        print '%s<enum %s/>' % (spaces(indent), attrs)
        return

    print '%s<enum %s>' % (spaces(indent), attrs)
    for key, val in e.functions.iteritems():
        attrs = 'name="%s"' % key
        if val[0] != e.default_count:
            attrs += ' count="%d"' % val[0]
        if not val[1]:
            attrs += ' mode="get"'

        print '%s<size %s/>' % (spaces(indent * 2), attrs)

    print '%s</enum>' % spaces(indent)

def output_type(t, indent=0):
    tab = spaces(16, t.name)
    attrs = 'name="%s"%ssize="%d"' % (t.name, tab, t.size)
    ctype = t.type_expr.string()
    if ctype.find("unsigned") != -1:
        attrs += ' unsigned="true"'
    elif ctype.find("signed") == -1:
        attrs += ' float="true"'
    print '%s<type %s/>' % (spaces(indent), attrs)

def output_function(f, indent=0):
    attrs = 'name="%s"' % f.name
    if f.offset > 0:
        if f.assign_offset:
            attrs += ' offset="assign"'
        else:
            attrs += ' offset="%d"' % f.offset
    print '%s<function %s>' % (spaces(indent), attrs)

    for p in f.parameters:
        attrs = 'name="%s" type="%s"' \
                % (p.name, p.type_expr.original_string)
        print '%s<param %s/>' % (spaces(indent * 2), attrs)
    if f.return_type != "void":
        attrs = 'type="%s"' % f.return_type
        print '%s<return %s/>' % (spaces(indent * 2), attrs)

    print '%s</function>' % spaces(indent)

def output_category(api, indent=0):
    enums = api.enums_by_name.values()
    enums.sort(cmp_enum)
    types = api.types_by_name.values()
    types.sort(cmp_type)
    functions = api.functions_by_name.values()
    functions.sort(cmp_function)

    for e in enums:
        output_enum(e, indent)
    if enums and types:
        print
    for t in types:
        output_type(t, indent)
    if enums or types:
        print
    for f in functions:
        output_function(f, indent)
        if f != functions[-1]:
            print

def is_api_empty(api):
    return bool(not api.enums_by_name and
                not api.types_by_name and
                not api.functions_by_name)

def show_usage(ops):
    print "Usage: %s [-k elts] <%s> <file1> <file2>" % (sys.argv[0], "|".join(ops))
    print "    -k elts   A comma separated string of types of elements to"
    print "              skip.  Possible types are enum, type, and function."
    sys.exit(1)

def main():
    ops = ["union", "intersection", "difference"]
    elts = ["enum", "type", "function"]

    try:
        options, args = getopt.getopt(sys.argv[1:], "k:")
    except Exception, e:
        show_usage(ops)

    if len(args) != 3:
        show_usage(ops)
    op, file1, file2 = args
    if op not in ops:
        show_usage(ops)

    skips = []
    for opt, val in options:
        if opt == "-k":
            skips = val.split(",")

    for elt in skips:
        try:
            elts.remove(elt)
        except ValueError:
            show_usage(ops)

    api1 = gl_XML.parse_GL_API(file1, glX_XML.glx_item_factory())
    api2 = gl_XML.parse_GL_API(file2, glX_XML.glx_item_factory())

    set = ApiSet(api1, elts)
    func = getattr(set, op)
    result = func(api2)

    if not is_api_empty(result):
        cat_name = "%s_of_%s_and_%s" \
                % (op, os.path.basename(file1), os.path.basename(file2))

        print '<?xml version="1.0"?>'
        print '<!DOCTYPE OpenGLAPI SYSTEM "%s/gl_API.dtd">' % GLAPI
        print
        print '<OpenGLAPI>'
        print
        print '<category name="%s">' % (cat_name)
        output_category(result, 4)
        print '</category>'
        print
        print '</OpenGLAPI>'

if __name__ == "__main__":
    main()
