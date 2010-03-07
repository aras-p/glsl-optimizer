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


from gallium import *
from base import *


def lods(*dims):
    size = max(dims)
    lods = 0
    while size:
        lods += 1
        size >>= 1
    return lods


class TextureTest(TestCase):
    
    tags = (
        'target',
        'format',
        'width',
        'height',
        'depth',
        'last_level',
        'face',
        'level',
        'zslice',
    )

    def test(self):
        dev = self.dev
        
        target = self.target
        format = self.format
        width = self.width
        height = self.height
        depth = self.depth
        last_level = self.last_level
        face = self.face
        level = self.level
        zslice = self.zslice
        
        #  textures
        dst_texture = dev.texture_create(
            target = target,
            format = format, 
            width = width, 
            height = height,
            depth = depth, 
            last_level = last_level,
            tex_usage = PIPE_TEXTURE_USAGE_RENDER_TARGET,
        )
        if dst_texture is None:
            raise TestSkip

        dst_surface = dst_texture.get_surface(face = face, level = level, zslice = zslice)
        
        ref_texture = dev.texture_create(
            target = target,
            format = format, 
            width = dst_surface.width, 
            height = dst_surface.height,
            depth = 1, 
            last_level = 0,
            tex_usage = PIPE_TEXTURE_USAGE_SAMPLER,
        )

        ref_surface = ref_texture.get_surface()
        
        src_texture = dev.texture_create(
            target = target,
            format = PIPE_FORMAT_B8G8R8A8_UNORM, 
            width = dst_surface.width, 
            height = dst_surface.height,
            depth = 1, 
            last_level = 0,
            tex_usage = PIPE_TEXTURE_USAGE_SAMPLER,
        )

        src_surface = src_texture.get_surface()
        
        expected_rgba = FloatArray(height*width*4) 
        ref_surface.sample_rgba(expected_rgba)

        src_surface.put_tile_rgba(0, 0, src_surface.width, src_surface.height, expected_rgba)
        
        ctx = self.dev.context_create()
    
        # disabled blending/masking
        blend = Blend()
        blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE
        blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE
        blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO
        blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO
        blend.rt[0].colormask = PIPE_MASK_RGBA
        ctx.set_blend(blend)
    
        # no-op depth/stencil/alpha
        depth_stencil_alpha = DepthStencilAlpha()
        ctx.set_depth_stencil_alpha(depth_stencil_alpha)
    
        # rasterizer
        rasterizer = Rasterizer()
        rasterizer.front_winding = PIPE_WINDING_CW
        rasterizer.cull_mode = PIPE_WINDING_NONE
        rasterizer.bypass_vs_clip_and_viewport = 1
        ctx.set_rasterizer(rasterizer)
    
        # samplers
        sampler = Sampler()
        sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE
        sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE
        sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE
        sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST
        sampler.min_img_filter = PIPE_TEX_MIPFILTER_NEAREST
        sampler.mag_img_filter = PIPE_TEX_MIPFILTER_NEAREST
        sampler.normalized_coords = 1
        sampler.min_lod = 0
        sampler.max_lod = PIPE_MAX_TEXTURE_LEVELS - 1
        ctx.set_fragment_sampler(0, sampler)
        ctx.set_fragment_sampler_texture(0, src_texture)

        #  framebuffer 
        cbuf_tex = dev.texture_create(
            PIPE_FORMAT_B8G8R8A8_UNORM, 
            width, 
            height,
            tex_usage = PIPE_TEXTURE_USAGE_RENDER_TARGET,
        )

        fb = Framebuffer()
        fb.width = dst_surface.width
        fb.height = dst_surface.height
        fb.nr_cbufs = 1
        fb.set_cbuf(0, dst_surface)
        ctx.set_framebuffer(fb)
        rgba = FloatArray(4);
        rgba[0] = 0.0
        rgba[1] = 0.0
        rgba[2] = 0.0
        rgba[3] = 0.0
        ctx.clear(PIPE_CLEAR_COLOR, rgba, 0.0, 0)
        del fb
    
        # vertex shader
        vs = Shader('''
            VERT
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
            FRAG
            DCL IN[0], GENERIC[0], LINEAR
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
        w = dst_surface.width
        h = dst_surface.height
    
        pos = [
            [x, y],
            [x+w, y],
            [x+w, y+h],
            [x, y+h],
        ]
    
        tex = [
            [0.0, 0.0], 
            [1.0, 0.0], 
            [1.0, 1.0], 
            [0.0, 1.0],
        ]
    
        for i in range(0, 4):
            j = 8*i
            verts[j + 0] = pos[i][0] # x
            verts[j + 1] = pos[i][1] # y
            verts[j + 2] = 0.0 # z
            verts[j + 3] = 1.0 # w
            verts[j + 4] = tex[i][0] # s
            verts[j + 5] = tex[i][1] # r
            verts[j + 6] = 0.0
            verts[j + 7] = 1.0
    
        ctx.draw_vertices(PIPE_PRIM_TRIANGLE_FAN,
                          nverts, 
                          nattrs, 
                          verts)
    
        ctx.flush()
    
        self.assert_rgba(dst_surface, x, y, w, h, expected_rgba, 4.0/256, 0.85)
        


def main():
    dev = Device()
    suite = TestSuite()
    
    targets = [
        PIPE_TEXTURE_2D,
        PIPE_TEXTURE_CUBE,
        #PIPE_TEXTURE_3D,
    ]
    
    formats = [
        PIPE_FORMAT_B8G8R8A8_UNORM,
        PIPE_FORMAT_B8G8R8X8_UNORM,
        #PIPE_FORMAT_B8G8R8A8_SRGB,
        PIPE_FORMAT_B5G6R5_UNORM,
        PIPE_FORMAT_B5G5R5A1_UNORM,
        PIPE_FORMAT_B4G4R4A4_UNORM,
        #PIPE_FORMAT_Z32_UNORM,
        #PIPE_FORMAT_S8Z24_UNORM,
        #PIPE_FORMAT_X8Z24_UNORM,
        #PIPE_FORMAT_Z16_UNORM,
        #PIPE_FORMAT_S8_UNORM,
        PIPE_FORMAT_A8_UNORM,
        PIPE_FORMAT_L8_UNORM,
        #PIPE_FORMAT_DXT1_RGB,
        #PIPE_FORMAT_DXT1_RGBA,
        #PIPE_FORMAT_DXT3_RGBA,
        #PIPE_FORMAT_DXT5_RGBA,
    ]
    
    sizes = [64, 32, 16, 8, 4, 2, 1]
    #sizes = [1020, 508, 252, 62, 30, 14, 6, 3]
    #sizes = [64]
    #sizes = [63]
    
    faces = [
        PIPE_TEX_FACE_POS_X,
        PIPE_TEX_FACE_NEG_X,
        PIPE_TEX_FACE_POS_Y,
        PIPE_TEX_FACE_NEG_Y, 
        PIPE_TEX_FACE_POS_Z, 
        PIPE_TEX_FACE_NEG_Z,
    ]

    for target in targets:
        for format in formats:
            for size in sizes:
                if target == PIPE_TEXTURE_3D:
                    depth = size
                else:
                    depth = 1
                for face in faces:
                    if target != PIPE_TEXTURE_CUBE and face:
                        continue
                    levels = lods(size)
                    for last_level in range(levels):
                        for level in range(0, last_level + 1):
                            zslice = 0
                            while zslice < depth >> level:
                                test = TextureTest(
                                    dev = dev,
                                    target = target,
                                    format = format, 
                                    width = size,
                                    height = size,
                                    depth = depth,
                                    last_level = last_level,
                                    face = face,
                                    level = level,
                                    zslice = zslice,
                                )
                                suite.add_test(test)
                                zslice = (zslice + 1)*2 - 1
    suite.run()


if __name__ == '__main__':
    main()
