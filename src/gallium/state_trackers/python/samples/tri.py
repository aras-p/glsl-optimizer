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


from gallium import *


def make_image(surface):
    pixels = FloatArray(surface.height*surface.width*4)
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


def test(dev):
    ctx = dev.context_create()

    width = 255
    height = 255

    # disabled blending/masking
    blend = Blend()
    blend.rgb_src_factor = PIPE_BLENDFACTOR_ONE
    blend.alpha_src_factor = PIPE_BLENDFACTOR_ONE
    blend.rgb_dst_factor = PIPE_BLENDFACTOR_ZERO
    blend.alpha_dst_factor = PIPE_BLENDFACTOR_ZERO
    blend.colormask = PIPE_MASK_RGBA
    ctx.set_blend(blend)

    # no-op depth/stencil/alpha
    depth_stencil_alpha = DepthStencilAlpha()
    ctx.set_depth_stencil_alpha(depth_stencil_alpha)

    # rasterizer
    rasterizer = Rasterizer()
    rasterizer.front_winding = PIPE_WINDING_CW
    rasterizer.cull_mode = PIPE_WINDING_NONE
    rasterizer.bypass_clipping = 1
    rasterizer.scissor = 1
    #rasterizer.bypass_vs = 1
    ctx.set_rasterizer(rasterizer)

    # viewport (identity, we setup vertices in wincoords)
    viewport = Viewport()
    scale = FloatArray(4)
    scale[0] = 1.0
    scale[1] = 1.0
    scale[2] = 1.0
    scale[3] = 1.0
    viewport.scale = scale
    translate = FloatArray(4)
    translate[0] = 0.0
    translate[1] = 0.0
    translate[2] = 0.0
    translate[3] = 0.0
    viewport.translate = translate
    ctx.set_viewport(viewport)

    # samplers
    sampler = Sampler()
    sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE
    sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE
    sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE
    sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE
    sampler.min_img_filter = PIPE_TEX_MIPFILTER_NEAREST
    sampler.mag_img_filter = PIPE_TEX_MIPFILTER_NEAREST
    sampler.normalized_coords = 1
    ctx.set_sampler(0, sampler)

    # scissor
    scissor = Scissor()
    scissor.minx = 0
    scissor.miny = 0
    scissor.maxx = width
    scissor.maxy = height
    ctx.set_scissor(scissor)

    clip = Clip()
    clip.nr = 0
    ctx.set_clip(clip)

    # framebuffer
    cbuf = dev.texture_create(
        PIPE_FORMAT_X8R8G8B8_UNORM, 
        width, height,
        tex_usage=PIPE_TEXTURE_USAGE_DISPLAY_TARGET,
    )
    _cbuf = cbuf.get_surface(usage = PIPE_BUFFER_USAGE_GPU_READ|PIPE_BUFFER_USAGE_GPU_WRITE)
    fb = Framebuffer()
    fb.width = width
    fb.height = height
    fb.num_cbufs = 1
    fb.set_cbuf(0, _cbuf)
    ctx.set_framebuffer(fb)
    _cbuf.clear_value = 0x00000000
    ctx.surface_clear(_cbuf, _cbuf.clear_value)
    del _cbuf
    
    # vertex shader
    vs = Shader('''
        VERT1.1
        DCL IN[0], POSITION, CONSTANT
        DCL IN[1], COLOR, CONSTANT
        DCL OUT[0], POSITION, CONSTANT
        DCL OUT[1], COLOR, CONSTANT
        0:MOV OUT[0], IN[0]
        1:MOV OUT[1], IN[1]
        2:END
    ''')
    ctx.set_vertex_shader(vs)

    # fragment shader
    fs = Shader('''
        FRAG1.1
        DCL IN[0], COLOR, LINEAR
        DCL OUT[0], COLOR, CONSTANT
        0:MOV OUT[0], IN[0]
        1:END
    ''')
    ctx.set_fragment_shader(fs)

    nverts = 3
    nattrs = 2
    verts = FloatArray(nverts * nattrs * 4)

    verts[ 0] = 128.0 # x1
    verts[ 1] =  32.0 # y1
    verts[ 2] =   0.0 # z1
    verts[ 3] =   1.0 # w1
    verts[ 4] =   1.0 # r1
    verts[ 5] =   0.0 # g1
    verts[ 6] =   0.0 # b1
    verts[ 7] =   1.0 # a1
    verts[ 8] =  32.0 # x2
    verts[ 9] = 224.0 # y2
    verts[10] =   0.0 # z2
    verts[11] =   1.0 # w2
    verts[12] =   0.0 # r2
    verts[13] =   1.0 # g2
    verts[14] =   0.0 # b2
    verts[15] =   1.0 # a2
    verts[16] = 224.0 # x3
    verts[17] = 224.0 # y3
    verts[18] =   0.0 # z3
    verts[19] =   1.0 # w3
    verts[20] =   0.0 # r3
    verts[21] =   0.0 # g3
    verts[22] =   1.0 # b3
    verts[23] =   1.0 # a3

    ctx.draw_vertices(PIPE_PRIM_TRIANGLES,
                      nverts, 
                      nattrs, 
                      verts)

    ctx.flush()
    
    show_image(cbuf.get_surface(usage = PIPE_BUFFER_USAGE_CPU_READ|PIPE_BUFFER_USAGE_CPU_WRITE))
    #save_image('tri.png', cbuf.get_surface(usage = PIPE_BUFFER_USAGE_CPU_READ|PIPE_BUFFER_USAGE_CPU_WRITE))



def main():
    dev = Device()
    test(dev)


if __name__ == '__main__':
    main()
