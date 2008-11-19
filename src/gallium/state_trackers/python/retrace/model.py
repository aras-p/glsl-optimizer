#!/usr/bin/env python
##########################################################################
# 
# Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
# All Rights Reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sub license, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice (including the
# next paragraph) shall be included in all copies or substantial portions
# of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
# IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
##########################################################################


'''Trace data model.'''


class Node:
    
    def visit(self, visitor):
        raise NotImplementedError


class Literal(Node):
    
    def __init__(self, value):
        self.value = value

    def visit(self, visitor):
        visitor.visit_literal(self)
        
    def __str__(self):
        if isinstance(self.value, str) and len(self.value) > 32:
            return '...'
        else: 
            return repr(self.value)


class NamedConstant(Node):
    
    def __init__(self, name):
        self.name = name

    def visit(self, visitor):
        visitor.visit_named_constant(self)
        
    def __str__(self):
        return self.name
    

class Array(Node):
    
    def __init__(self, elements):
        self.elements = elements

    def visit(self, visitor):
        visitor.visit_array(self)
        
    def __str__(self):
        return '{' + ', '.join([str(value) for value in self.elements]) + '}'


class Struct(Node):
    
    def __init__(self, name, members):
        self.name = name
        self.members = members        

    def visit(self, visitor):
        visitor.visit_struct(self)
        
    def __str__(self):
        return '{' + ', '.join([name + ' = ' + str(value) for name, value in self.members]) + '}'

        
class Pointer(Node):
    
    def __init__(self, address):
        self.address = address

    def visit(self, visitor):
        visitor.visit_pointer(self)
        
    def __str__(self):
        return self.address


class Call:
    
    def __init__(self, klass, method, args, ret):
        self.klass = klass
        self.method = method
        self.args = args
        self.ret = ret
        
    def visit(self, visitor):
        visitor.visit_call(self)
        
    def __str__(self):
        s = self.method
        if self.klass:
            s = self.klass + '::' + s
        s += '(' + ', '.join([name + ' = ' + str(value) for name, value in self.args]) + ')'
        if self.ret is not None:
            s += ' = ' + str(self.ret)
        return s


class Trace:
    
    def __init__(self, calls):
        self.calls = calls
        
    def visit(self, visitor):
        visitor.visit_trace(self)
        
    def __str__(self):
        return '\n'.join([str(call) for call in self.calls])
    
    
class Visitor:
    
    def visit_literal(self, node):
        raise NotImplementedError
    
    def visit_named_constant(self, node):
        raise NotImplementedError
    
    def visit_array(self, node):
        raise NotImplementedError
    
    def visit_struct(self, node):
        raise NotImplementedError
    
    def visit_pointer(self, node):
        raise NotImplementedError
    
    def visit_call(self, node):
        raise NotImplementedError
    
    def visit_trace(self, node):
        raise NotImplementedError
    

