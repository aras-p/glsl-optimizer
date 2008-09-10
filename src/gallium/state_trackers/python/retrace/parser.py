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


import sys
import xml.parsers.expat
import binascii

from model import *


ELEMENT_START, ELEMENT_END, CHARACTER_DATA, EOF = range(4)


class XmlToken:

    def __init__(self, type, name_or_data, attrs = None, line = None, column = None):
        assert type in (ELEMENT_START, ELEMENT_END, CHARACTER_DATA, EOF)
        self.type = type
        self.name_or_data = name_or_data
        self.attrs = attrs
        self.line = line
        self.column = column

    def __str__(self):
        if self.type == ELEMENT_START:
            return '<' + self.name_or_data + ' ...>'
        if self.type == ELEMENT_END:
            return '</' + self.name_or_data + '>'
        if self.type == CHARACTER_DATA:
            return self.name_or_data
        if self.type == EOF:
            return 'end of file'
        assert 0


class XmlTokenizer:
    """Expat based XML tokenizer."""

    def __init__(self, fp, skip_ws = True):
        self.fp = fp
        self.tokens = []
        self.index = 0
        self.final = False
        self.skip_ws = skip_ws
        
        self.character_pos = 0, 0
        self.character_data = ''
        
        self.parser = xml.parsers.expat.ParserCreate()
        self.parser.StartElementHandler  = self.handle_element_start
        self.parser.EndElementHandler    = self.handle_element_end
        self.parser.CharacterDataHandler = self.handle_character_data
    
    def handle_element_start(self, name, attributes):
        self.finish_character_data()
        line, column = self.pos()
        token = XmlToken(ELEMENT_START, name, attributes, line, column)
        self.tokens.append(token)
    
    def handle_element_end(self, name):
        self.finish_character_data()
        line, column = self.pos()
        token = XmlToken(ELEMENT_END, name, None, line, column)
        self.tokens.append(token)

    def handle_character_data(self, data):
        if not self.character_data:
            self.character_pos = self.pos()
        self.character_data += data
    
    def finish_character_data(self):
        if self.character_data:
            if not self.skip_ws or not self.character_data.isspace(): 
                line, column = self.character_pos
                token = XmlToken(CHARACTER_DATA, self.character_data, None, line, column)
                self.tokens.append(token)
            self.character_data = ''
    
    def next(self):
        size = 16*1024
        while self.index >= len(self.tokens) and not self.final:
            self.tokens = []
            self.index = 0
            data = self.fp.read(size)
            self.final = len(data) < size
            data = data.rstrip('\0')
            try:
                self.parser.Parse(data, self.final)
            except xml.parsers.expat.ExpatError, e:
                #if e.code == xml.parsers.expat.errors.XML_ERROR_NO_ELEMENTS:
                if e.code == 3:
                    pass
                else:
                    raise e
        if self.index >= len(self.tokens):
            line, column = self.pos()
            token = XmlToken(EOF, None, None, line, column)
        else:
            token = self.tokens[self.index]
            self.index += 1
        return token

    def pos(self):
        return self.parser.CurrentLineNumber, self.parser.CurrentColumnNumber


class TokenMismatch(Exception):

    def __init__(self, expected, found):
        self.expected = expected
        self.found = found

    def __str__(self):
        return '%u:%u: %s expected, %s found' % (self.found.line, self.found.column, str(self.expected), str(self.found))



class XmlParser:
    """Base XML document parser."""

    def __init__(self, fp):
        self.tokenizer = XmlTokenizer(fp)
        self.consume()
    
    def consume(self):
        self.token = self.tokenizer.next()

    def match_element_start(self, name):
        return self.token.type == ELEMENT_START and self.token.name_or_data == name
    
    def match_element_end(self, name):
        return self.token.type == ELEMENT_END and self.token.name_or_data == name

    def element_start(self, name):
        while self.token.type == CHARACTER_DATA:
            self.consume()
        if self.token.type != ELEMENT_START:
            raise TokenMismatch(XmlToken(ELEMENT_START, name), self.token)
        if self.token.name_or_data != name:
            raise TokenMismatch(XmlToken(ELEMENT_START, name), self.token)
        attrs = self.token.attrs
        self.consume()
        return attrs
    
    def element_end(self, name):
        while self.token.type == CHARACTER_DATA:
            self.consume()
        if self.token.type != ELEMENT_END:
            raise TokenMismatch(XmlToken(ELEMENT_END, name), self.token)
        if self.token.name_or_data != name:
            raise TokenMismatch(XmlToken(ELEMENT_END, name), self.token)
        self.consume()

    def character_data(self, strip = True):
        data = ''
        while self.token.type == CHARACTER_DATA:
            data += self.token.name_or_data
            self.consume()
        if strip:
            data = data.strip()
        return data


class TraceParser(XmlParser):

    def parse(self):
        self.element_start('trace')
        while self.token.type not in (ELEMENT_END, EOF):
            call = self.parse_call()
            self.handle_call(call)
        if self.token.type != EOF:
            self.element_end('trace')

    def parse_call(self):
        attrs = self.element_start('call')
        klass = attrs['class']
        method = attrs['method']
        args = []
        ret = None
        while self.token.type == ELEMENT_START:
            if self.token.name_or_data == 'arg':
                arg = self.parse_arg()
                args.append(arg)
            elif self.token.name_or_data == 'ret':
                ret = self.parse_ret()
            elif self.token.name_or_data == 'call':
                # ignore nested function calls
                self.parse_call()
            else:
                raise TokenMismatch("<arg ...> or <ret ...>", self.token)
        self.element_end('call')
        
        return Call(klass, method, args, ret)

    def parse_arg(self):
        attrs = self.element_start('arg')
        name = attrs['name']
        value = self.parse_value()
        self.element_end('arg')

        return name, value

    def parse_ret(self):
        attrs = self.element_start('ret')
        value = self.parse_value()
        self.element_end('ret')

        return value

    def parse_value(self):
        expected_tokens = ('null', 'bool', 'int', 'uint', 'float', 'string', 'enum', 'array', 'struct', 'ptr', 'bytes')
        if self.token.type == ELEMENT_START:
            if self.token.name_or_data in expected_tokens:
                method = getattr(self, 'parse_' +  self.token.name_or_data)
                return method()
        raise TokenMismatch(" or " .join(expected_tokens), self.token)

    def parse_null(self):
        self.element_start('null')
        self.element_end('null')
        return Literal(None)
        
    def parse_bool(self):
        self.element_start('bool')
        value = int(self.character_data())
        self.element_end('bool')
        return Literal(value)
        
    def parse_int(self):
        self.element_start('int')
        value = int(self.character_data())
        self.element_end('int')
        return Literal(value)
        
    def parse_uint(self):
        self.element_start('uint')
        value = int(self.character_data())
        self.element_end('uint')
        return Literal(value)
        
    def parse_float(self):
        self.element_start('float')
        value = float(self.character_data())
        self.element_end('float')
        return Literal(value)
        
    def parse_enum(self):
        self.element_start('enum')
        name = self.character_data()
        self.element_end('enum')
        return NamedConstant(name)
        
    def parse_string(self):
        self.element_start('string')
        value = self.character_data()
        self.element_end('string')
        return Literal(value)
        
    def parse_bytes(self):
        self.element_start('bytes')
        value = binascii.a2b_hex(self.character_data())
        self.element_end('bytes')
        return Literal(value)
        
    def parse_array(self):
        self.element_start('array')
        elems = []
        while self.token.type != ELEMENT_END:
            elems.append(self.parse_elem())
        self.element_end('array')
        return Array(elems)

    def parse_elem(self):
        self.element_start('elem')
        value = self.parse_value()
        self.element_end('elem')
        return value

    def parse_struct(self):
        attrs = self.element_start('struct')
        name = attrs['name']
        members = []
        while self.token.type != ELEMENT_END:
            members.append(self.parse_member())
        self.element_end('struct')
        return Struct(name, members)

    def parse_member(self):
        attrs = self.element_start('member')
        name = attrs['name']
        value = self.parse_value()
        self.element_end('member')

        return name, value

    def parse_ptr(self):
        self.element_start('ptr')
        address = self.character_data()
        self.element_end('ptr')

        return Pointer(address)

    def handle_call(self, call):
        
        pass
    
    
class TraceDumper(TraceParser):
    

    def handle_call(self, call):
        print call
        

def main(ParserFactory):
    for arg in sys.argv[1:]:
        if arg.endswith('.gz'):
            import gzip
            stream = gzip.GzipFile(arg, 'rt')
        else:
            stream = open(arg, 'rt')
        parser = ParserFactory(stream)
        parser.parse()


if __name__ == '__main__':
    main(TraceDumper)
