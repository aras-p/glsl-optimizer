#!/usr/bin/env python
'''
Generates dumpers for the i965 state strucutures using pygccxml.

Run as 

  PYTHONPATH=/path/to/pygccxml-1.0.0 python brw_structs_dump.py

Jose Fonseca <jfonseca@vmware.com>
'''

copyright = '''
/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/
 '''

import os
import sys
import re

from pygccxml import parser
from pygccxml import declarations

from pygccxml.declarations import algorithm
from pygccxml.declarations import decl_visitor
from pygccxml.declarations import type_traits
from pygccxml.declarations import type_visitor


enums = True


def vars_filter(variable):
    name = variable.name
    return not re.match('^pad\d*', name) and name != 'dword' 


class decl_dumper_t(decl_visitor.decl_visitor_t):

    def __init__(self, stream, instance = '', decl = None):
        decl_visitor.decl_visitor_t.__init__(self)
        self.stream = stream
        self._instance = instance
        self.decl = decl

    def clone(self):
        return decl_dumper_t(self.stream, self._instance, self.decl)

    def visit_class(self):
        class_ = self.decl
        assert self.decl.class_type in ('struct', 'union')

        for variable in class_.variables(recursive = False):
            if vars_filter(variable):
                dump_type(self.stream, self._instance + '.' + variable.name, variable.type)

    def visit_enumeration(self):
        if enums:
            self.stream.write('   switch(%s) {\n' % ("(*ptr)" + self._instance,))
            for name, value in self.decl.values:
                self.stream.write('   case %s:\n' % (name,))
                self.stream.write('      debug_printf("\\t\\t%s = %s\\n");\n' % (self._instance, name))
                self.stream.write('      break;\n')
            self.stream.write('   default:\n')
            self.stream.write('      debug_printf("\\t\\t%s = %%i\\n", %s);\n' % (self._instance, "(*ptr)" + self._instance))
            self.stream.write('      break;\n')
            self.stream.write('   }\n')
        else:
            self.stream.write('   debug_printf("\\t\\t%s = %%i\\n", %s);\n' % (self._instance, "(*ptr)" + self._instance))


def dump_decl(stream, instance, decl):
    dumper = decl_dumper_t(stream, instance, decl)
    algorithm.apply_visitor(dumper, decl)


class type_dumper_t(type_visitor.type_visitor_t):

    def __init__(self, stream, instance, type_):
        type_visitor.type_visitor_t.__init__(self)
        self.stream = stream
        self.instance = instance
        self.type = type_

    def clone(self):
        return type_dumper_t(self.instance, self.type)

    def visit_bool(self):
        self.print_instance('%i')
        
    def visit_char(self):
        #self.print_instance('%i')
        self.print_instance('0x%x')
        
    def visit_unsigned_char(self):
        #self.print_instance('%u')
        self.print_instance('0x%x')

    def visit_signed_char(self):
        #self.print_instance('%i')
        self.print_instance('0x%x')
    
    def visit_wchar(self):
        self.print_instance('0x%x')
        
    def visit_short_int(self):
        #self.print_instance('%i')
        self.print_instance('0x%x')
        
    def visit_short_unsigned_int(self):
        #self.print_instance('%u')
        self.print_instance('0x%x')
        
    def visit_int(self):
        #self.print_instance('%i')
        self.print_instance('0x%x')
        
    def visit_unsigned_int(self):
        #self.print_instance('%u')
        self.print_instance('0x%x')
        
    def visit_long_int(self):
        #self.print_instance('%li')
        self.print_instance('0x%lx')
        
    def visit_long_unsigned_int(self):
        #self.print_instance('%lu')
        self.print_instance('%0xlx')
        
    def visit_long_long_int(self):
        #self.print_instance('%lli')
        self.print_instance('%0xllx')
        
    def visit_long_long_unsigned_int(self):
        #self.print_instance('%llu')
        self.print_instance('0x%llx')
        
    def visit_float(self):
        self.print_instance('%f')
        
    def visit_double(self):
        self.print_instance('%f')
        
    def visit_array(self):
        for i in range(type_traits.array_size(self.type)):
            dump_type(self.stream, self.instance + '[%i]' % i, type_traits.base_type(self.type))

    def visit_pointer(self):
        self.print_instance('%p')

    def visit_declarated(self):
        #stream.write('decl = %r\n' % self.type.decl_string)
        decl = type_traits.remove_declarated(self.type)
        dump_decl(self.stream, self.instance, decl)

    def print_instance(self, format):
        self.stream.write('   debug_printf("\\t\\t%s = %s\\n", %s);\n' % (self.instance, format, "(*ptr)" + self.instance))



def dump_type(stream, instance, type_):
    type_ = type_traits.remove_alias(type_)
    visitor = type_dumper_t(stream, instance, type_)
    algorithm.apply_visitor(visitor, type_)


def dump_struct_interface(stream, class_, suffix = ';'):
    name = class_.name
    assert name.startswith('brw_');
    name = name[:4] + 'dump_' + name[4:]
    stream.write('void\n')
    stream.write('%s(const struct %s *ptr)%s\n' % (name, class_.name, suffix))


def dump_struct_implementation(stream, decls, class_):
    dump_struct_interface(stream, class_, suffix = '')
    stream.write('{\n')
    dump_decl(stream, '', class_)
    stream.write('}\n')
    stream.write('\n')


def dump_header(stream):
    stream.write(copyright.strip() + '\n')
    stream.write('\n')
    stream.write('/**\n')
    stream.write(' * @file\n')
    stream.write(' * Dump i965 data structures.\n')
    stream.write(' *\n')
    stream.write(' * Generated automatically from brw_structs.h by brw_structs_dump.py.\n')
    stream.write(' */\n')
    stream.write('\n')


def dump_interfaces(decls, global_ns, names):
    stream = open('brw_structs_dump.h', 'wt')
    
    dump_header(stream)
    
    stream.write('#ifndef BRW_STRUCTS_DUMP_H\n')
    stream.write('#define BRW_STRUCTS_DUMP_H\n')
    stream.write('\n')
    
    for name in names:
        stream.write('struct %s;\n' % (name,))
    stream.write('\n')

    for name in names:
        (class_,) = global_ns.classes(name = name)
        dump_struct_interface(stream, class_)
        stream.write('\n')
    stream.write('\n')

    stream.write('#endif /* BRW_STRUCTS_DUMP_H */\n')


def dump_implementations(decls, global_ns, names):
    stream = open('brw_structs_dump.c', 'wt')
    
    dump_header(stream)

    stream.write('#include "util/u_debug.h"\n')
    stream.write('\n')
    stream.write('#include "brw_types.h"\n')
    stream.write('#include "brw_structs.h"\n')
    stream.write('#include "brw_structs_dump.h"\n')
    stream.write('\n')

    for name in names:
        (class_,) = global_ns.classes(name = name)
        dump_struct_implementation(stream, decls, class_)


def decl_filter(decl):
    '''Filter the declarations we're interested in'''
    name = decl.name
    return name.startswith('brw_') and name not in ('brw_instruction',) 


def main():

    config = parser.config_t(
        include_paths = [
            '../../include',
        ],
        compiler = 'gcc',
    )

    headers = [
        'brw_types.h', 
        'brw_structs.h', 
    ]

    decls = parser.parse(headers, config, parser.COMPILATION_MODE.ALL_AT_ONCE)
    global_ns = declarations.get_global_namespace(decls)

    names = []
    for class_ in global_ns.classes(decl_filter):
        names.append(class_.name)
    names.sort()

    dump_interfaces(decls, global_ns, names)
    dump_implementations(decls, global_ns, names)


if __name__ == '__main__':
    main()
