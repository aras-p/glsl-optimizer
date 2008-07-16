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
from gallium import *
from base import *
from data import generate_data


def compare_rgba(width, height, rgba1, rgba2, tol=4.0/256):
    errors = 0
    for y in range(0, height):
        for x in range(0, width):
            for ch in range(4):
                offset = (y*width + x)*4 + ch
                v1 = rgba1[offset]
                v2 = rgba2[offset]
                if abs(v1 - v2) > tol:
                    if errors == 0:
                        sys.stderr.write("x=%u, y=%u, ch=%u differ: %f vs %f\n" % (x, y, ch, v1, v2))
                    if errors == 1:
                        sys.stderr.write("...\n")
                    errors += 1
    if errors:
        sys.stderr.write("%u out of %u pixels differ\n" % (errors/4, height*width))
    return errors == 0


class TextureTest(Test):
    
    def __init__(self, **kargs):
        Test.__init__(self)
        self.__dict__.update(kargs)

    def description(self):
        return "%s %ux%u" % (formats[self.format], self.width, self.height)
        
    def run(self):
        dev = self.dev
        
        format = self.format
        width = self.width
        height = self.height
        
        if not dev.is_format_supported(format, PIPE_TEXTURE):
            print "SKIP"
            return
        if not dev.is_format_supported(format, PIPE_SURFACE):
            print "SKIP"
            return
        
        ctx = dev.context_create()
    
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
        texture = dev.texture_create(format, 
                                     width, 
                                     height)
        ctx.set_sampler_texture(0, texture)
        
        expected_rgba = generate_data(texture.get_surface(usage = PIPE_BUFFER_USAGE_CPU_READ|PIPE_BUFFER_USAGE_CPU_WRITE))
        
        #  framebuffer 
        cbuf_tex = dev.texture_create(PIPE_FORMAT_A8R8G8B8_UNORM, 
                                      width, 
                                      height,
                                      usage = PIPE_TEXTURE_USAGE_RENDER_TARGET)

        cbuf = cbuf_tex.get_surface(usage = PIPE_BUFFER_USAGE_GPU_WRITE|PIPE_BUFFER_USAGE_GPU_READ)
        fb = Framebuffer()
        fb.width = width
        fb.height = height
        fb.num_cbufs = 1
        fb.set_cbuf(0, cbuf)
        ctx.set_framebuffer(fb)
        ctx.surface_clear(cbuf, 0x00000000)
        del fb
    
        # vertex shader
        vs = Shader('''
            VERT1.1
            DCL IN[0], POSITION, CONSTANT
            DCL IN[1], GENERIC, CONSTANT
            DCL OUT[0], POSITION, CONSTANT
            DCL OUT[1], GENERIC, CONSTANT
            0:MOV OUT[0], IN[0]
            1:MOV OUT[1], IN[1]
            2:END
        ''')
        #vs.dump()
        ctx.set_vertex_shader(vs)
    
        # fragment shader
        fs = Shader('''
            FRAG1.1
            DCL IN[0], GENERIC[0], PERSPECTIVE
            DCL OUT[0], COLOR, CONSTANT
            DCL SAMP[0], CONSTANT
            0:TEX OUT[0], IN[0], SAMP[0], 2D
            1:END
        ''')
        #fs.dump()
        ctx.set_fragment_shader(fs)

        nverts = 4
        nattrs = 2
        verts = FloatArray(nverts * nattrs * 4)
    
        x = 0
        y = 0
        w = width
        h = height
    
        verts[ 0] =     x # x1
        verts[ 1] =     y # y1
        verts[ 2] =   0.0 # z1
        verts[ 3] =   1.0 # w1
        verts[ 4] =   0.0 # s1
        verts[ 5] =   0.0 # t
        verts[ 6] =   0.0
        verts[ 7] =   0.0
        verts[ 8] = x + w # x2
        verts[ 9] =     y # y2
        verts[10] =   0.0 # z2
        verts[11] =   1.0 # w2
        verts[12] =   1.0 # s2
        verts[13] =   0.0 # t2
        verts[14] =   0.0
        verts[15] =   0.0
        verts[16] = x + w # x3
        verts[17] = y + h # y3
        verts[18] =   0.0 # z3
        verts[19] =   1.0 # w3
        verts[20] =   1.0 # s3
        verts[21] =   1.0 # t3
        verts[22] =   0.0
        verts[23] =   0.0
        verts[24] =     x # x4
        verts[25] = y + h # y4
        verts[26] =   0.0 # z4
        verts[27] =   1.0 # w4
        verts[28] =   0.0 # s4
        verts[29] =   1.0 # t4
        verts[30] =   0.0
        verts[31] =   0.0
    
        ctx.draw_vertices(PIPE_PRIM_TRIANGLE_FAN,
                          nverts, 
                          nattrs, 
                          verts)
    
        ctx.flush()

        del ctx
        
        rgba = FloatArray(height*width*4)

        cbuf_tex.get_surface(usage = PIPE_BUFFER_USAGE_CPU_READ).get_tile_rgba(x, y, w, h, rgba)

        if compare_rgba(width, height, rgba, expected_rgba):
            print "OK"
        else:
            print "FAIL"
        
            show_image(width, height, Result=rgba, Expected=expected_rgba)
            #save_image(width, height, rgba, "result.png")
            #save_image(width, height, expected_rgba, "expected.png")
            sys.exit(0)


def main():
    dev = Device()
    suite = TestSuite()
    formats = [PIPE_FORMAT_A8R8G8B8_UNORM, PIPE_FORMAT_YCBCR, PIPE_FORMAT_DXT1_RGB]
    sizes = [64, 32, 16, 8, 4, 2]
    for format in formats:
        for size in sizes:
            suite.add_test(TextureTest(dev=dev, format=format, width=size, height=size))
    suite.run()


if __name__ == '__main__':
    main()
