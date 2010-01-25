#!/usr/bin/env python
##########################################################################
# 
# Copyright 2009 VMware, Inc.
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
# IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
##########################################################################


import struct

from gallium import *

def make_image(surface):
    data = surface.get_tile_rgba8(0, 0, surface.width, surface.height)

    import Image
    outimage = Image.fromstring('RGBA', (surface.width, surface.height), data, "raw", 'RGBA', 0, 1)
    return outimage

def save_image(filename, surface):
    outimage = make_image(surface)
    outimage.save(filename, "PNG")

def test(dev, name):
    ctx = dev.context_create()

    width = 320
    height = 320
    minz = 0.0
    maxz = 1.0

    # disabled blending/masking
    blend = Blend()
    blend.rgb_src_factor = PIPE_BLENDFACTOR_ONE
    blend.alpha_src_factor = PIPE_BLENDFACTOR_ONE
    blend.rgb_dst_factor = PIPE_BLENDFACTOR_ZERO
    blend.alpha_dst_factor = PIPE_BLENDFACTOR_ZERO
    blend.colormask = PIPE_MASK_RGBA
    ctx.set_blend(blend)

    # depth/stencil/alpha
    depth_stencil_alpha = DepthStencilAlpha()
    depth_stencil_alpha.depth.enabled = 0
    depth_stencil_alpha.depth.writemask = 1
    depth_stencil_alpha.depth.func = PIPE_FUNC_LESS
    ctx.set_depth_stencil_alpha(depth_stencil_alpha)

    # rasterizer
    rasterizer = Rasterizer()
    rasterizer.front_winding = PIPE_WINDING_CW
    rasterizer.cull_mode = PIPE_WINDING_NONE
    rasterizer.scissor = 1
    ctx.set_rasterizer(rasterizer)

    # viewport
    viewport = Viewport()
    scale = FloatArray(4)
    scale[0] = width / 2.0
    scale[1] = -height / 2.0
    scale[2] = (maxz - minz) / 2.0
    scale[3] = 1.0
    viewport.scale = scale
    translate = FloatArray(4)
    translate[0] = width / 2.0
    translate[1] = height / 2.0
    translate[2] = (maxz - minz) / 2.0
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
    ctx.set_fragment_sampler(0, sampler)

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
        tex_usage=PIPE_TEXTURE_USAGE_RENDER_TARGET,
    ).get_surface()
    fb = Framebuffer()
    fb.width = width
    fb.height = height
    fb.nr_cbufs = 1
    fb.set_cbuf(0, cbuf)
    ctx.set_framebuffer(fb)
    rgba = FloatArray(4);
    rgba[0] = 0.5
    rgba[1] = 0.5
    rgba[2] = 0.5
    rgba[3] = 0.5
    ctx.clear(PIPE_CLEAR_COLOR, rgba, 0.0, 0)

    # vertex shader
    vs = Shader(file('vert-' + name + '.sh', 'rt').read())
    ctx.set_vertex_shader(vs)

    # fragment shader
    fs = Shader('''
        FRAG
        DCL IN[0], COLOR, LINEAR
        DCL OUT[0], COLOR, CONSTANT
        0:MOV OUT[0], IN[0]
        1:END
    ''')
    ctx.set_fragment_shader(fs)

    constbuf0 = dev.buffer_create(64,
                                  (PIPE_BUFFER_USAGE_CONSTANT |
                                   PIPE_BUFFER_USAGE_GPU_READ |
                                   PIPE_BUFFER_USAGE_CPU_WRITE),
                                  4 * 4 * 4)

    cbdata = ''
    cbdata += struct.pack('4f', 0.4, 0.0, 0.0, 1.0)
    cbdata += struct.pack('4f', 1.0, 1.0, 1.0, 1.0)
    cbdata += struct.pack('4f', 2.0, 2.0, 2.0, 2.0)
    cbdata += struct.pack('4f', 4.0, 8.0, 16.0, 32.0)

    constbuf0.write(cbdata, 0)

    ctx.set_constant_buffer(PIPE_SHADER_VERTEX,
                            0,
                            constbuf0)

    constbuf1 = dev.buffer_create(64,
                                  (PIPE_BUFFER_USAGE_CONSTANT |
                                   PIPE_BUFFER_USAGE_GPU_READ |
                                   PIPE_BUFFER_USAGE_CPU_WRITE),
                                  4 * 4 * 4)

    cbdata = ''
    cbdata += struct.pack('4f', 0.1, 0.1, 0.1, 0.1)
    cbdata += struct.pack('4f', 0.25, 0.25, 0.25, 0.25)
    cbdata += struct.pack('4f', 0.5, 0.5, 0.5, 0.5)
    cbdata += struct.pack('4f', 0.75, 0.75, 0.75, 0.75)

    constbuf1.write(cbdata, 0)

    ctx.set_constant_buffer(PIPE_SHADER_VERTEX,
                            1,
                            constbuf1)

    xy = [
         0.0,  0.8,
        -0.2,  0.4,
         0.2,  0.4,
        -0.4,  0.0,
         0.0,  0.0,
         0.4,  0.0,
        -0.6, -0.4,
        -0.2, -0.4,
         0.2, -0.4,
         0.6, -0.4,
        -0.8, -0.8,
        -0.4, -0.8,
         0.0, -0.8,
         0.4, -0.8,
         0.8, -0.8,
    ]
    color = [
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
    ]
    tri = [
         1,  2,  0,
         3,  4,  1,
         4,  2,  1,
         4,  5,  2,
         6,  7,  3,
         7,  4,  3,
         7,  8,  4,
         8,  5,  4,
         8,  9,  5,
        10, 11,  6,
        11,  7,  6,
        11, 12,  7,
        12,  8,  7,
        12, 13,  8,
        13,  9,  8,
        13, 14,  9,
    ]

    nverts = 16 * 3
    nattrs = 2
    verts = FloatArray(nverts * nattrs * 4)

    for i in range(0, nverts):
        verts[i * nattrs * 4 + 0] = xy[tri[i] * 2 + 0] # x
        verts[i * nattrs * 4 + 1] = xy[tri[i] * 2 + 1] # y
        verts[i * nattrs * 4 + 2] = 0.5 # z
        verts[i * nattrs * 4 + 3] = 1.0 # w
        verts[i * nattrs * 4 + 4] = color[(i % 3) * 3 + 0] # r
        verts[i * nattrs * 4 + 5] = color[(i % 3) * 3 + 1] # g
        verts[i * nattrs * 4 + 6] = color[(i % 3) * 3 + 2] # b
        verts[i * nattrs * 4 + 7] = 1.0 # a

    ctx.draw_vertices(PIPE_PRIM_TRIANGLES,
                      nverts,
                      nattrs,
                      verts)

    ctx.flush()

    save_image('vert-' + name + '.png', cbuf)

def main():
    tests = [
        'abs',
        'add',
        'arl',
        'arr',
        'cb-1d',
        'cb-2d',
        'dp3',
        'dp4',
        'dst',
        'ex2',
        'flr',
        'frc',
        'lg2',
        'lit',
        'lrp',
        'mad',
        'max',
        'min',
        'mov',
        'mul',
        'rcp',
        'rsq',
        'sge',
        'slt',
        'srcmod-abs',
        'srcmod-absneg',
        'srcmod-neg',
        'srcmod-swz',
        'sub',
        'xpd',
    ]

    dev = Device()
    for t in tests:
        test(dev, t)

if __name__ == '__main__':
    main()
