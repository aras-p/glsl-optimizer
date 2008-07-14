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


def save_image(filename, surface):
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
    outimage.save(filename, "PNG")


def test(dev):
    ctx = dev.context_create()

    width = 256
    height = 256

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

    #  texture 
    texture = dev.texture_create(PIPE_FORMAT_A8R8G8B8_UNORM, 
                                 width, height, 
                                 usage=PIPE_TEXTURE_USAGE_RENDER_TARGET)
    ctx.set_sampler_texture(0, texture)

    #  drawing dest 
    surface = texture.get_surface(usage = PIPE_BUFFER_USAGE_GPU_WRITE)
    fb = Framebuffer()
    fb.width = surface.width
    fb.height = surface.height
    fb.num_cbufs = 1
    fb.set_cbuf(0, surface)
    ctx.set_framebuffer(fb)

    # vertex shader
    vs = Shader('''
        VERT1.1
        DCL IN[0], POSITION, CONSTANT
        DCL IN[1], GENERIC[0], CONSTANT
        DCL OUT[0], POSITION, CONSTANT
        DCL OUT[1], GENERIC[0], CONSTANT
        0:MOV OUT[0], IN[0]
        1:MOV OUT[1], IN[1]
        2:END
    ''')
    #vs.dump()
    ctx.set_vertex_shader(vs)

    # fragment shader
    fs = Shader('''
        FRAG1.1
        DCL IN[0], COLOR, CONSTANT
        DCL OUT[0], COLOR, CONSTANT
        0:MOV OUT[0], IN[0]
        1:END
    ''')
    #fs.dump()
    ctx.set_fragment_shader(fs)

    if 0:
        nverts = 4
        nattrs = 1
        vertices = FloatArray(n_verts * nattrs * 4)

        # init vertex data that doesn't change
        for i in range(nverts):
            for j in range(nattrs):
                vertices[(i*nattrs +j)*4 + 0] = 0.0
                vertices[(i*nattrs +j)*4 + 1] = 0.0
                vertices[(i*nattrs +j)*4 + 2] = 0.0
                vertices[(i*nattrs +j)*4 + 3] = 0.0

        ctx.draw_vertices(PIPE_PRIM_TRIANGLE_FAN,
                          4,  #  verts 
                          2, #  attribs/vert 
                          vertices)
    else:
        ctx.draw_quad(32.0, 32.0, 224.0, 224.0)

    ctx.flush()

    save_image("simple.png", surface)



def main():
    dev = Device(0)
    test(dev)


if __name__ == '__main__':
    main()
