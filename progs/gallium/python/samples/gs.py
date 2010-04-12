#!/usr/bin/env python
##########################################################################
#
# Copyright 2009 VMware
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
    data = surface.get_tile_rgba8(0, 0, surface.width, surface.height)

    import Image
    outimage = Image.fromstring('RGBA', (surface.width, surface.height), data, "raw", 'RGBA', 0, 1)
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
    minz = 0.0
    maxz = 1.0

    # disabled blending/masking
    blend = Blend()
    blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE
    blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE
    blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO
    blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO
    blend.rt[0].colormask = PIPE_MASK_RGBA
    ctx.set_blend(blend)

    # depth/stencil/alpha
    depth_stencil_alpha = DepthStencilAlpha()
    depth_stencil_alpha.depth.enabled = 1
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
    cbuf = dev.resource_create(
        PIPE_FORMAT_B8G8R8X8_UNORM,
        width, height,
        bind=PIPE_BIND_RENDER_TARGET,
    ).get_surface()
    zbuf = dev.resource_create(
        PIPE_FORMAT_Z32_UNORM,
        width, height,
        bind=PIPE_BIND_DEPTH_STENCIL,
    ).get_surface()
    fb = Framebuffer()
    fb.width = width
    fb.height = height
    fb.nr_cbufs = 1
    fb.set_cbuf(0, cbuf)
    fb.set_zsbuf(zbuf)
    ctx.set_framebuffer(fb)
    rgba = FloatArray(4);
    rgba[0] = 0.0
    rgba[1] = 0.0
    rgba[2] = 0.0
    rgba[3] = 0.0
    ctx.clear(PIPE_CLEAR_COLOR | PIPE_CLEAR_DEPTHSTENCIL, rgba, 1.0, 0xff)

    # vertex shader
    vs = Shader('''
        VERT
        DCL IN[0], POSITION, CONSTANT
        DCL IN[1], COLOR, CONSTANT
        DCL OUT[0], POSITION, CONSTANT
        DCL OUT[1], COLOR, CONSTANT
        0:MOV OUT[0], IN[0]
        1:MOV OUT[1], IN[1]
        2:END
    ''')
    ctx.set_vertex_shader(vs)

    gs = Shader('''
        GEOM
        PROPERTY GS_INPUT_PRIMITIVE TRIANGLES
        PROPERTY GS_OUTPUT_PRIMITIVE TRIANGLE_STRIP
        DCL IN[][0], POSITION, CONSTANT
        DCL IN[][1], COLOR, CONSTANT
        DCL OUT[0], POSITION, CONSTANT
        DCL OUT[1], COLOR, CONSTANT
        0:MOV OUT[0], IN[0][0]
        1:MOV OUT[1], IN[0][1]
        2:EMIT
        3:MOV OUT[0], IN[1][0]
        4:MOV OUT[1], IN[1][1]
        5:EMIT
        6:MOV OUT[0], IN[2][0]
        7:MOV OUT[1], IN[2][1]
        8:EMIT
        9:ENDPRIM
        10:END
    ''')
    ctx.set_geometry_shader(gs)

    # fragment shader
    fs = Shader('''
        FRAG
        DCL IN[0], COLOR, LINEAR
        DCL OUT[0], COLOR, CONSTANT
        0:MOV OUT[0], IN[0]
        1:END
    ''')
    ctx.set_fragment_shader(fs)

    nverts = 3
    nattrs = 2
    verts = FloatArray(nverts * nattrs * 4)

    verts[ 0] =   0.0 # x1
    verts[ 1] =   0.8 # y1
    verts[ 2] =   0.2 # z1
    verts[ 3] =   1.0 # w1
    verts[ 4] =   1.0 # r1
    verts[ 5] =   0.0 # g1
    verts[ 6] =   0.0 # b1
    verts[ 7] =   1.0 # a1
    verts[ 8] =  -0.8 # x2
    verts[ 9] =  -0.8 # y2
    verts[10] =   0.5 # z2
    verts[11] =   1.0 # w2
    verts[12] =   0.0 # r2
    verts[13] =   1.0 # g2
    verts[14] =   0.0 # b2
    verts[15] =   1.0 # a2
    verts[16] =   0.8 # x3
    verts[17] =  -0.8 # y3
    verts[18] =   0.8 # z3
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

    show_image(cbuf)
    #show_image(zbuf)
    #save_image('cbuf.png', cbuf)
    #save_image('zbuf.png', zbuf)



def main():
    dev = Device()
    test(dev)


if __name__ == '__main__':
    main()
