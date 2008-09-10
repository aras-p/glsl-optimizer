#!/usr/bin/env python
#############################################################################
#
# Copyright 2008 Tungsten Graphics, Inc.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#############################################################################


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
    

