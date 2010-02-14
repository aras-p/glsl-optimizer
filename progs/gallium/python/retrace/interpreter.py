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


import sys
import struct

import gallium
import model
import parse as parser


try:
    from struct import unpack_from
except ImportError:
    def unpack_from(fmt, buf, offset=0):
        size = struct.calcsize(fmt)
        return struct.unpack(fmt, buf[offset:offset + size])


def make_image(surface, x=None, y=None, w=None, h=None):
    if x is None:
        x = 0
    if y is None:
        y = 0
    if w is None:
        w = surface.width - x
    if h is None:
        h = surface.height - y
    data = surface.get_tile_rgba8(x, y, surface.width, surface.height)

    import Image
    outimage = Image.fromstring('RGBA', (w, h), data, "raw", 'RGBA', 0, 1)
    return outimage

def save_image(filename, surface, x=None, y=None, w=None, h=None):
    outimage = make_image(surface, x, y, w, h)
    outimage.save(filename, "PNG")

def show_image(surface, title, x=None, y=None, w=None, h=None):
    outimage = make_image(surface, x, y, w, h)
    
    import Tkinter as tk
    from PIL import Image, ImageTk
    root = tk.Tk()
    
    root.title(title)
    
    image1 = ImageTk.PhotoImage(outimage)
    w = image1.width()
    h = image1.height()
    x = 100
    y = 100
    root.geometry("%dx%d+%d+%d" % (w, h, x, y))
    panel1 = tk.Label(root, image=image1)
    panel1.pack(side='top', fill='both', expand='yes')
    panel1.image = image1
    root.mainloop()


class Struct:
    """C-like struct"""

    # A basic Python class can pass as a C-like structure
    pass


struct_factories = {
    "pipe_blend_color": gallium.BlendColor,
    "pipe_blend_state": gallium.Blend,
    #"pipe_clip_state": gallium.Clip,
    #"pipe_buffer": gallium.Buffer,
    "pipe_depth_state": gallium.Depth,
    "pipe_stencil_state": gallium.Stencil,
    "pipe_alpha_state": gallium.Alpha,
    "pipe_depth_stencil_alpha_state": gallium.DepthStencilAlpha,
    #"pipe_framebuffer_state": gallium.Framebuffer,
    "pipe_poly_stipple": gallium.PolyStipple,
    "pipe_rasterizer_state": gallium.Rasterizer,
    "pipe_sampler_state": gallium.Sampler,
    "pipe_scissor_state": gallium.Scissor,
    #"pipe_shader_state": gallium.Shader,
    #"pipe_vertex_buffer": gallium.VertexBuffer,
    "pipe_vertex_element": gallium.VertexElement,
    "pipe_viewport_state": gallium.Viewport,
    #"pipe_texture": gallium.Texture,
}


member_array_factories = {
    #"pipe_rasterizer_state": {"sprite_coord_mode": gallium.ByteArray},                          
    "pipe_poly_stipple": {"stipple": gallium.UnsignedArray},                          
    "pipe_viewport_state": {"scale": gallium.FloatArray, "translate": gallium.FloatArray},                          
    #"pipe_clip_state": {"ucp": gallium.FloatArray},
    "pipe_depth_stencil_alpha_state": {"stencil": gallium.StencilArray},
    "pipe_blend_color": {"color": gallium.FloatArray},
    "pipe_sampler_state": {"border_color": gallium.FloatArray},              
}


class Translator(model.Visitor):
    """Translate model arguments into regular Python objects"""

    def __init__(self, interpreter):
        self.interpreter = interpreter
        self.result = None

    def visit(self, node):
        self.result = None
        node.visit(self)
        return self.result
        
    def visit_literal(self, node):
        self.result = node.value
    
    def visit_named_constant(self, node):
        # lookup the named constant in the gallium module
        self.result = getattr(gallium, node.name)
    
    def visit_array(self, node):
        array = []
        for element in node.elements:
            array.append(self.visit(element))
        self.result = array
    
    def visit_struct(self, node):
        struct_factory = struct_factories.get(node.name, Struct)
        struct = struct_factory()
        for member_name, member_node in node.members:
            member_value = self.visit(member_node)
            try:
                array_factory = member_array_factories[node.name][member_name]
            except KeyError:
                pass
            else:
                assert isinstance(member_value, list)
                array = array_factory(len(member_value))
                for i in range(len(member_value)):
                    array[i] = member_value[i]
                member_value = array
            #print node.name, member_name, member_value
            assert isinstance(struct, Struct) or hasattr(struct, member_name)
            setattr(struct, member_name, member_value)
        self.result = struct
    
    def visit_pointer(self, node):
        self.result = self.interpreter.lookup_object(node.address)


class Object:
    
    def __init__(self, interpreter, real):
        self.interpreter = interpreter
        self.real = real
        

class Global(Object):

    def __init__(self, interpreter, real):
        self.interpreter = interpreter
        self.real = real
        
    def pipe_winsys_create(self):
        return Winsys(self.interpreter, gallium.Device())

    def pipe_screen_create(self, winsys=None):
        if winsys is None:
            real = gallium.Device()
        else:
            real = winsys.real
        return Screen(self.interpreter, real)
    
    def pipe_context_create(self, screen):
        context = screen.real.context_create()
        return Context(self.interpreter, context)

    
class Winsys(Object):
    
    def __init__(self, interpreter, real):
        self.interpreter = interpreter
        self.real = real

    def get_name(self):
        pass
    
    def user_buffer_create(self, data, size):
        # We don't really care to distinguish between user and regular buffers
        buffer = self.real.buffer_create(size, 
                                         4, 
                                         gallium.PIPE_BUFFER_USAGE_CPU_READ |
                                         gallium.PIPE_BUFFER_USAGE_CPU_WRITE )
        assert size == len(data)
        buffer.write(data)
        return buffer
    
    def buffer_create(self, alignment, usage, size):
        return self.real.buffer_create(size, alignment, usage)
    
    def buffer_destroy(self, buffer):
        pass
    
    def buffer_write(self, buffer, data, size):
        assert size == len(data)
        buffer.write(data)
        
    def fence_finish(self, fence, flags):
        pass
    
    def fence_reference(self, dst, src):
        pass
    
    def flush_frontbuffer(self, surface):
        pass

    def surface_alloc(self):
        return None
    
    def surface_release(self, surface):
        pass


class Transfer:

    def __init__(self, surface, x, y, w, h):
        self.surface = surface
        self.x = x
        self.y = y
        self.w = w
        self.h = h


class Screen(Object):
    
    def destroy(self):
        pass

    def get_name(self):
        pass
    
    def get_vendor(self):
        pass
    
    def get_param(self, param):
        pass
    
    def get_paramf(self, param):
        pass
    
    def is_format_supported(self, format, target, tex_usage, geom_flags):
        return self.real.is_format_supported(format, target, tex_usage, geom_flags)
    
    def texture_create(self, templat):
        return self.real.texture_create(
            format = templat.format,
            width = templat.width,
            height = templat.height,
            depth = templat.depth,
            last_level = templat.last_level,
            target = templat.target,
            tex_usage = templat.tex_usage,
        )

    def texture_destroy(self, texture):
        self.interpreter.unregister_object(texture)

    def texture_release(self, surface):
        pass

    def get_tex_surface(self, texture, face, level, zslice, usage):
        if texture is None:
            return None
        return texture.get_surface(face, level, zslice)
    
    def tex_surface_destroy(self, surface):
        self.interpreter.unregister_object(surface)

    def tex_surface_release(self, surface):
        pass

    def surface_write(self, surface, data, stride, size):
        if surface is None:
            return
#        assert surface.nblocksy * stride == size 
        surface.put_tile_raw(0, 0, surface.width, surface.height, data, stride)

    def get_tex_transfer(self, texture, face, level, zslice, usage, x, y, w, h):
        if texture is None:
            return None
        transfer = Transfer(texture.get_surface(face, level, zslice), x, y, w, h)
        if transfer and usage & gallium.PIPE_TRANSFER_READ:
            if self.interpreter.options.all:
                self.interpreter.present(transfer.surface, 'transf_read', x, y, w, h)
        return transfer
    
    def tex_transfer_destroy(self, transfer):
        self.interpreter.unregister_object(transfer)

    def transfer_write(self, transfer, stride, data, size):
        if transfer is None:
            return
        transfer.surface.put_tile_raw(transfer.x, transfer.y, transfer.w, transfer.h, data, stride)
        if self.interpreter.options.all:
            self.interpreter.present(transfer.surface, 'transf_write', transfer.x, transfer.y, transfer.w, transfer.h)

    def user_buffer_create(self, data, size):
        # We don't really care to distinguish between user and regular buffers
        buffer = self.real.buffer_create(size, 
                                         4, 
                                         gallium.PIPE_BUFFER_USAGE_CPU_READ |
                                         gallium.PIPE_BUFFER_USAGE_CPU_WRITE )
        assert size == len(data)
        buffer.write(data)
        return buffer
    
    def buffer_create(self, alignment, usage, size):
        return self.real.buffer_create(size, alignment, usage)
    
    def buffer_destroy(self, buffer):
        pass
    
    def buffer_write(self, buffer, data, size, offset=0):
        assert size == len(data)
        buffer.write(data)
        
    def fence_finish(self, fence, flags):
        pass
    
    def fence_reference(self, dst, src):
        pass
    
    def flush_frontbuffer(self, surface):
        pass


class Context(Object):
    
    def __init__(self, interpreter, real):
        Object.__init__(self, interpreter, real)
        self.cbufs = []
        self.zsbuf = None
        self.vbufs = []
        self.velems = []
        self.dirty = False

    def destroy(self):
        pass
    
    def create_blend_state(self, state):
        return state

    def bind_blend_state(self, state):
        if state is not None:
            self.real.set_blend(state)

    def delete_blend_state(self, state):
        pass
    
    def create_sampler_state(self, state):
        return state

    def delete_sampler_state(self, state):
        pass

    def bind_vertex_sampler_states(self, num_states, states):
        for i in range(num_states):
            self.real.set_vertex_sampler(i, states[i])
        
    def bind_fragment_sampler_states(self, num_states, states):
        for i in range(num_states):
            self.real.set_fragment_sampler(i, states[i])
        
    def create_rasterizer_state(self, state):
        return state

    def bind_rasterizer_state(self, state):
        if state is not None:
            self.real.set_rasterizer(state)
        
    def delete_rasterizer_state(self, state):
        pass
    
    def create_depth_stencil_alpha_state(self, state):
        return state

    def bind_depth_stencil_alpha_state(self, state):
        if state is not None:
            self.real.set_depth_stencil_alpha(state)
            
    def delete_depth_stencil_alpha_state(self, state):
        pass

    def create_fs_state(self, state):
        tokens = str(state.tokens)
        shader = gallium.Shader(tokens)
        return shader

    create_vs_state = create_fs_state
    
    def bind_fs_state(self, state):
        self.real.set_fragment_shader(state)
        
    def bind_vs_state(self, state):
        self.real.set_vertex_shader(state)

    def delete_fs_state(self, state):
        pass

    delete_vs_state = delete_fs_state

    def set_blend_color(self, state):
        self.real.set_blend_color(state)

    def set_stencil_ref(self, state):
        self.real.set_stencil_ref(state)

    def set_clip_state(self, state):
        _state = gallium.Clip()
        _state.nr = state.nr
        if state.nr:
            # FIXME
            ucp = gallium.FloatArray(gallium.PIPE_MAX_CLIP_PLANES*4)
            for i in range(len(state.ucp)):
                for j in range(len(state.ucp[i])):
                    ucp[i*4 + j] = state.ucp[i][j]
            _state.ucp = ucp
        self.real.set_clip(_state)

    def dump_constant_buffer(self, buffer):
        if not self.interpreter.verbosity(2):
            return

        data = buffer.read()
        format = '4f'
        index = 0
        for offset in range(0, len(data), struct.calcsize(format)):
            x, y, z, w = unpack_from(format, data, offset)
            sys.stdout.write('\tCONST[%2u] = {%10.4f, %10.4f, %10.4f, %10.4f}\n' % (index, x, y, z, w))
            index += 1
        sys.stdout.flush()

    def set_constant_buffer(self, shader, index, buffer):
        if buffer is not None:
            self.real.set_constant_buffer(shader, index, buffer)

            self.dump_constant_buffer(buffer)

    def set_framebuffer_state(self, state):
        _state = gallium.Framebuffer()
        _state.width = state.width
        _state.height = state.height
        _state.nr_cbufs = state.nr_cbufs
        for i in range(len(state.cbufs)):
            _state.set_cbuf(i, state.cbufs[i])
        _state.set_zsbuf(state.zsbuf)    
        self.real.set_framebuffer(_state)
        
        self.cbufs = state.cbufs
        self.zsbuf = state.zsbuf

    def set_polygon_stipple(self, state):
        self.real.set_polygon_stipple(state)

    def set_scissor_state(self, state):
        self.real.set_scissor(state)

    def set_viewport_state(self, state):
        self.real.set_viewport(state)

    def set_fragment_sampler_textures(self, num_textures, textures):
        for i in range(num_textures):
            self.real.set_fragment_sampler_texture(i, textures[i])

    def set_vertex_sampler_textures(self, num_textures, textures):
        for i in range(num_textures):
            self.real.set_vertex_sampler_texture(i, textures[i])

    def set_vertex_buffers(self, num_buffers, buffers):
        self.vbufs = buffers[0:num_buffers]
        for i in range(num_buffers):
            vbuf = buffers[i]
            self.real.set_vertex_buffer(
                i,
                stride = vbuf.stride,
                max_index = vbuf.max_index,
                buffer_offset = vbuf.buffer_offset,
                buffer = vbuf.buffer,
            )

    def set_vertex_elements(self, num_elements, elements):
        self.velems = elements[0:num_elements]
        for i in range(num_elements):
            self.real.set_vertex_element(i, elements[i])
        self.real.set_vertex_elements(num_elements)

    def dump_vertices(self, start, count):
        if not self.interpreter.verbosity(2):
            return

        for index in range(start, start + count):
            if index >= start + 16:
                sys.stdout.write('\t...\n')
                break
            sys.stdout.write('\t{\n')
            for velem in self.velems:
                vbuf = self.vbufs[velem.vertex_buffer_index]

                offset = vbuf.buffer_offset + velem.src_offset + vbuf.stride*index
                format = {
                    gallium.PIPE_FORMAT_R32_FLOAT: 'f',
                    gallium.PIPE_FORMAT_R32G32_FLOAT: '2f',
                    gallium.PIPE_FORMAT_R32G32B32_FLOAT: '3f',
                    gallium.PIPE_FORMAT_R32G32B32A32_FLOAT: '4f',
                    gallium.PIPE_FORMAT_B8G8R8A8_UNORM: '4B',
                    gallium.PIPE_FORMAT_R8G8B8A8_UNORM: '4B',
                    gallium.PIPE_FORMAT_R16G16B16_SNORM: '3h',
                }[velem.src_format]

                data = vbuf.buffer.read()
                values = unpack_from(format, data, offset)
                sys.stdout.write('\t\t{' + ', '.join(map(str, values)) + '},\n')
                assert len(values) == velem.nr_components
            sys.stdout.write('\t},\n')
        sys.stdout.flush()

    def dump_indices(self, ibuf, isize, start, count):
        if not self.interpreter.verbosity(2):
            return

        format = {
            1: 'B',
            2: 'H',
            4: 'I',
        }[isize]

        assert struct.calcsize(format) == isize

        data = ibuf.read()
        maxindex, minindex = 0, 0xffffffff

        sys.stdout.write('\t{\n')
        for i in range(start, start + count):
            if i >= start + 16:
                sys.stdout.write('\t...\n')
                break
            offset = i*isize
            index, = unpack_from(format, data, offset)
            sys.stdout.write('\t\t%u,\n' % index)
            minindex = min(minindex, index)
            maxindex = max(maxindex, index)
        sys.stdout.write('\t},\n')
        sys.stdout.flush()

        return minindex, maxindex

    def draw_arrays(self, mode, start, count):
        self.dump_vertices(start, count)
            
        self.real.draw_arrays(mode, start, count)
        self._set_dirty()
    
    def draw_elements(self, indexBuffer, indexSize, mode, start, count):
        if self.interpreter.verbosity(2):
            minindex, maxindex = self.dump_indices(indexBuffer, indexSize, start, count)
            self.dump_vertices(minindex, maxindex - minindex)

        self.real.draw_elements(indexBuffer, indexSize, mode, start, count)
        self._set_dirty()
        
    def draw_range_elements(self, indexBuffer, indexSize, minIndex, maxIndex, mode, start, count):
        if self.interpreter.verbosity(2):
            minindex, maxindex = self.dump_indices(indexBuffer, indexSize, start, count)
            minindex = min(minindex, minIndex)
            maxindex = min(maxindex, maxIndex)
            self.dump_vertices(minindex, maxindex - minindex)

        self.real.draw_range_elements(indexBuffer, indexSize, minIndex, maxIndex, mode, start, count)
        self._set_dirty()
        
    def surface_copy(self, dest, destx, desty, src, srcx, srcy, width, height):
        if dest is not None and src is not None:
            if self.interpreter.options.all:
                self.interpreter.present(src, 'surface_copy_src', srcx, srcy, width, height)
            self.real.surface_copy(dest, destx, desty, src, srcx, srcy, width, height)
            if dest in self.cbufs:
                self._set_dirty()
                flags = gallium.PIPE_FLUSH_FRAME
            else:
                flags = 0
            self.flush(flags)
            if self.interpreter.options.all:
                self.interpreter.present(dest, 'surface_copy_dest', destx, desty, width, height)

    def is_texture_referenced(self, texture, face, level):
        #return self.real.is_texture_referenced(format, texture, face, level)
        pass
    
    def is_buffer_referenced(self, buf):
        #return self.real.is_buffer_referenced(format, buf)
        pass
    
    def _set_dirty(self):
        if self.interpreter.options.step:
            self._present()
        else:
            self.dirty = True

    def flush(self, flags):
        self.real.flush(flags)
        if self.dirty:
            if flags & gallium.PIPE_FLUSH_FRAME:
                self._present()
            self.dirty = False
        return None

    def clear(self, buffers, rgba, depth, stencil):
        _rgba = gallium.FloatArray(4)
        for i in range(4):
            _rgba[i] = rgba[i]
        self.real.clear(buffers, _rgba, depth, stencil)
        
    def _present(self):
        self.real.flush()
    
        if self.cbufs and self.cbufs[0]:
            self.interpreter.present(self.cbufs[0], "cbuf")
        if self.zsbuf:
            if self.interpreter.options.all:
                self.interpreter.present(self.zsbuf, "zsbuf")
    

class Interpreter(parser.TraceDumper):
    
    ignore_calls = set((
            ('pipe_screen', 'is_format_supported'),
            ('pipe_screen', 'get_param'),
            ('pipe_screen', 'get_paramf'),
    ))

    def __init__(self, stream, options):
        parser.TraceDumper.__init__(self, stream)
        self.options = options
        self.objects = {}
        self.result = None
        self.globl = Global(self, None)
        self.call_no = None

    def register_object(self, address, object):
        self.objects[address] = object
        
    def unregister_object(self, object):
        # FIXME:
        pass

    def lookup_object(self, address):
        return self.objects[address]
    
    def interpret(self, trace):
        for call in trace.calls:
            self.interpret_call(call)

    def handle_call(self, call):
        if self.options.stop and call.no > self.options.stop:
            sys.exit(0)

        if (call.klass, call.method) in self.ignore_calls:
            return

        self.call_no = call.no

        if self.verbosity(1):
            parser.TraceDumper.handle_call(self, call)
            sys.stdout.flush()
        
        args = [(str(name), self.interpret_arg(arg)) for name, arg in call.args] 
        
        if call.klass:
            name, obj = args[0]
            args = args[1:]
        else:
            obj = self.globl
            
        method = getattr(obj, call.method)
        ret = method(**dict(args))
        
        if call.ret and isinstance(call.ret, model.Pointer):
            if ret is None:
                sys.stderr.write('warning: NULL returned\n')
            self.register_object(call.ret.address, ret)

        self.call_no = None

    def interpret_arg(self, node):
        translator = Translator(self)
        return translator.visit(node)

    def verbosity(self, level):
        return self.options.verbosity >= level

    def present(self, surface, description, x=None, y=None, w=None, h=None):
        if self.call_no < self.options.start:
            return

        if self.options.images:
            filename = '%04u_%s.png' % (self.call_no, description)
            save_image(filename, surface, x, y, w, h)
        else:
            title = '%u. %s' % (self.call_no, description)
            show_image(surface, title, x, y, w, h)
    

class Main(parser.Main):

    def get_optparser(self):
        optparser = parser.Main.get_optparser(self)
        optparser.add_option("-q", "--quiet", action="store_const", const=0, dest="verbosity", help="no messages")
        optparser.add_option("-v", "--verbose", action="count", dest="verbosity", default=1, help="increase verbosity level")
        optparser.add_option("-i", "--images", action="store_true", dest="images", default=False, help="save images instead of showing them")
        optparser.add_option("-a", "--all", action="store_true", dest="all", default=False, help="show depth, stencil, and transfers")
        optparser.add_option("-s", "--step", action="store_true", dest="step", default=False, help="step trhough every draw")
        optparser.add_option("-f", "--from", action="store", type="int", dest="start", default=0, help="from call no")
        optparser.add_option("-t", "--to", action="store", type="int", dest="stop", default=0, help="until call no")
        return optparser

    def process_arg(self, stream, options):
        parser = Interpreter(stream, options)
        parser.parse()


if __name__ == '__main__':
    Main().main()
