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
import gallium
import model
import parser


def make_image(surface):
    pixels = gallium.FloatArray(surface.height*surface.width*4)
    surface.get_tile_rgba(0, 0, surface.width, surface.height, pixels)

    import Image
    outimage = Image.new(
        mode='RGB',
        size=(surface.width, surface.height),
        color=(0,0,0))
    outpixels = outimage.load()
    for y in range(0, surface.height):
        for x in range(0, surface.width):
            offset = (y*surface.width + x)*4
            r, g, b, a = [int(pixels[offset + ch]*255) for ch in range(4)]
            outpixels[x, y] = r, g, b
    return outimage

def save_image(filename, surface):
    outimage = make_image(surface)
    outimage.save(filename, "PNG")

def show_image(surface):
    outimage = make_image(surface)
    
    import Tkinter as tk
    from PIL import Image, ImageTk
    root = tk.Tk()
    
    root.title('background image')
    
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
    #"pipe_constant_buffer": gallium.ConstantBuffer,
    "pipe_depth_state": gallium.Depth,
    "pipe_stencil_state": gallium.Stencil,
    "pipe_alpha_state": gallium.Alpha,
    "pipe_depth_stencil_alpha_state": gallium.DepthStencilAlpha,
    "pipe_format_block": gallium.FormatBlock,
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
    "pipe_rasterizer_state": {"sprite_coord_mode": gallium.ByteArray},                          
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

    def pipe_screen_create(self, winsys):
        return Screen(self.interpreter, winsys.real)
    
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
        buffer.write(data, size)
        return buffer
    
    def buffer_create(self, alignment, usage, size):
        return self.real.buffer_create(size, alignment, usage)
    
    def buffer_destroy(self, buffer):
        pass
    
    def buffer_write(self, buffer, data, size):
        buffer.write(data, size)
        
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


class Screen(Object):
    
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
    
    def texture_create(self, template):
        return self.real.texture_create(
            format = template.format,
            width = template.width[0],
            height = template.height[0],
            depth = template.depth[0],
            last_level = template.last_level,
            target = template.target,
            tex_usage = template.tex_usage,
        )

    def texture_destroy(self, texture):
        self.interpreter.unregister_object(texture)

    def texture_release(self, surface):
        pass

    def get_tex_surface(self, texture, face, level, zslice, usage):
        return texture.get_surface(face, level, zslice, usage)
    
    def tex_surface_destroy(self, surface):
        self.interpreter.unregister_object(surface)

    def tex_surface_release(self, surface):
        pass

    def surface_write(self, surface, data, stride, size):
        assert surface.nblocksy * stride == size 
        surface.put_tile_raw(0, 0, surface.width, surface.height, data, stride)


class Context(Object):
    
    def __init__(self, interpreter, real):
        Object.__init__(self, interpreter, real)
        self.cbufs = []
        self.zsbuf = None

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

    def bind_sampler_states(self, n, states):
        for i in range(n):
            self.real.set_sampler(i, states[i])
        
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

    def set_constant_buffer(self, shader, index, state):
        if state is not None:
            self.real.set_constant_buffer(shader, index, state.buffer)

    def set_framebuffer_state(self, state):
        _state = gallium.Framebuffer()
        _state.width = state.width
        _state.height = state.height
        _state.num_cbufs = state.num_cbufs
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

    def set_sampler_textures(self, n, textures):
        for i in range(n):
            self.real.set_sampler_texture(i, textures[i])

    def set_vertex_buffers(self, n, vbufs):
        for i in range(n):
            vbuf = vbufs[i]
            self.real.set_vertex_buffer(
                i,
                pitch = vbuf.pitch,
                max_index = vbuf.max_index,
                buffer_offset = vbuf.buffer_offset,
                buffer = vbuf.buffer,
            )

    def set_vertex_elements(self, n, elements):
        for i in range(n):
            self.real.set_vertex_element(i, elements[i])
        self.real.set_vertex_elements(n)

    def set_edgeflags(self, bitfield):
        # FIXME
        pass
    
    def draw_arrays(self, mode, start, count):
        self.real.draw_arrays(mode, start, count)
    
    def draw_elements(self, indexBuffer, indexSize, mode, start, count):
        self.real.draw_elements(indexBuffer, indexSize, mode, start, count)
        
    def draw_range_elements(self, indexBuffer, indexSize, minIndex, maxIndex, mode, start, count):
        self.real.draw_range_elements(indexBuffer, indexSize, minIndex, maxIndex, mode, start, count)
        
    def flush(self, flags):
        self.real.flush(flags)
        if flags & gallium.PIPE_FLUSH_FRAME:
            self._update()
        return None

    def clear(self, surface, value):
        self.real.surface_clear(surface, value)
        
    def _update(self):
        self.real.flush()
    
        if self.cbufs and self.cbufs[0]:
            show_image(self.cbufs[0])
    

class Interpreter(parser.TraceParser):
    
    def __init__(self, stream):
        parser.TraceParser.__init__(self, stream)
        self.objects = {}
        self.result = None
        self.globl = Global(self, None)

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
        sys.stderr.write("%s\n" % call)
        
        args = [self.interpret_arg(arg) for name, arg in call.args] 
        
        if call.klass:
            obj = args[0]
            args = args[1:]
        else:
            obj = self.globl
            
        method = getattr(obj, call.method)
        ret = method(*args)
        
        if call.ret and isinstance(call.ret, model.Pointer):
            self.register_object(call.ret.address, ret)

    def interpret_arg(self, node):
        translator = Translator(self)
        return translator.visit(node)
    

if __name__ == '__main__':
    parser.main(Interpreter)
