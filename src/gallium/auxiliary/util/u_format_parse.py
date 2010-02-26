#!/usr/bin/env python

'''
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
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
'''


VOID, UNSIGNED, SIGNED, FIXED, FLOAT = range(5)

SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W, SWIZZLE_0, SWIZZLE_1, SWIZZLE_NONE, = range(7)

PLAIN = 'plain'


class Type:
    '''Describe the type of a color channel.'''
    
    def __init__(self, kind, norm, size):
        self.kind = kind
        self.norm = norm
        self.size = size
        self.sign = kind in (SIGNED, FIXED, FLOAT)

    def __str__(self):
        s = str(self.kind)
        if self.norm:
            s += 'n'
        s += str(self.size)
        return s

    def __eq__(self, other):
        return self.kind == other.kind and self.norm == other.norm and self.size == other.size


class Format:
    '''Describe a pixel format.'''

    def __init__(self, name, layout, block_width, block_height, in_types, out_swizzle, colorspace):
        self.name = name
        self.layout = layout
        self.block_width = block_width
        self.block_height = block_height
        self.in_types = in_types
        self.out_swizzle = out_swizzle
        self.name = name
        self.colorspace = colorspace

    def __str__(self):
        return self.name

    def short_name(self):
        '''Make up a short norm for a format, suitable to be used as suffix in
        function names.'''

        name = self.name
        if name.startswith('PIPE_FORMAT_'):
            name = name[len('PIPE_FORMAT_'):]
        name = name.lower()
        return name

    def block_size(self):
        size = 0
        for type in self.in_types:
            size += type.size
        return size

    def nr_channels(self):
        nr_channels = 0
        for type in self.in_types:
            if type.size:
                nr_channels += 1
        return nr_channels

    def is_array(self):
        ref_type = self.in_types[0]
        for type in self.in_types[1:]:
            if type.size and (type.size != ref_type.size or type.size % 8):
                return False
        return True

    def is_mixed(self):
        ref_type = self.in_types[0]
        for type in self.in_types[1:]:
            if type.kind != VOID:
                if type.kind != ref_type.kind:
                    return True
                if type.norm != ref_type.norm:
                    return True
        return False

    def stride(self):
        return self.block_size()/8


_kind_parse_map = {
    '':  VOID,
    'x': VOID,
    'u': UNSIGNED,
    's': SIGNED,
    'h': FIXED,
    'f': FLOAT,
}

_swizzle_parse_map = {
    'x': SWIZZLE_X,
    'y': SWIZZLE_Y,
    'z': SWIZZLE_Z,
    'w': SWIZZLE_W,
    '0': SWIZZLE_0,
    '1': SWIZZLE_1,
    '_': SWIZZLE_NONE,
}

def parse(filename):
    '''Parse the format descrition in CSV format in terms of the 
    Type and Format classes above.'''

    stream = open(filename)
    formats = []
    for line in stream:
        try:
            comment = line.index('#')
        except ValueError:
            pass
        else:
            line = line[:comment]
        line = line.strip()
        if not line:
            continue
        fields = [field.strip() for field in line.split(',')]
        name = fields[0]
        layout = fields[1]
        block_width, block_height = map(int, fields[2:4])
        in_types = []
        for field in fields[4:8]:
            if field:
                kind = _kind_parse_map[field[0]]
                if field[1] == 'n':
                    norm = True
                    size = int(field[2:])
                else:
                    norm = False
                    size = int(field[1:])
            else:
                kind = VOID
                norm = False
                size = 0
            in_type = Type(kind, norm, size)
            in_types.append(in_type)
        out_swizzle = [_swizzle_parse_map[swizzle] for swizzle in fields[8]]
        colorspace = fields[9]
        formats.append(Format(name, layout, block_width, block_height, in_types, out_swizzle, colorspace))
    return formats

