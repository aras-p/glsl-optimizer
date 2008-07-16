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


def compare_rgba(width, height, rgba1, rgba2, tol=4.0/256, ratio=0.85):
    errors = 0
    for y in range(0, height):
        for x in range(0, width):
            differs = 0
            for ch in range(4):
                offset = (y*width + x)*4 + ch
                v1 = rgba1[offset]
                v2 = rgba2[offset]
                if abs(v1 - v2) > tol:
                    if errors == 0:
                        sys.stderr.write("x=%u, y=%u, ch=%u differ: %f vs %f\n" % (x, y, ch, v1, v2))
                    if errors == 1 and ch == 0:
                        sys.stderr.write("...\n")
                    differs = 1
            errors += differs
    total = height*width
    if errors:
        sys.stderr.write("%u out of %u pixels differ\n" % (errors, total))
    return float(total - errors)/float(total) >= ratio 


def lods(*dims):
    size = max(dims)
    lods = 0
    while size:
        lods += 1
        size >>= 1
    return lods


def minify(dims, level = 1):
    return [max(dim>>level, 1) for dim in dims]


def tex_coords(texture, face, level, zslice):
    st = [ 
        [0.0, 0.0], 
        [1.0, 0.0], 
        [1.0, 1.0], 
        [0.0, 1.0],
    ] 
    
    if texture.target == PIPE_TEXTURE_2D:
        return [[s, t, 0.0] for s, t in st]
    elif texture.target == PIPE_TEXTURE_3D:
        assert 0
    elif texture.target == PIPE_TEXTURE_CUBE:
        result = []
        for s, t in st:
            # See http://developer.nvidia.com/object/cube_map_ogl_tutorial.html
            sc = 2.0*s - 1.0
            tc = 2.0*t - 1.0
            if face == PIPE_TEX_FACE_POS_X:
                rx = 1.0
                ry = -tc
                rz = -sc
            if face == PIPE_TEX_FACE_NEG_X:
                rx = -1.0
                ry = -tc
                rz = sc
            if face == PIPE_TEX_FACE_POS_Y:
                rx = sc
                ry = 1.0
                rz = tc
            if face == PIPE_TEX_FACE_NEG_Y:
                rx = sc
                ry = -1.0
                rz = -tc
            if face == PIPE_TEX_FACE_POS_Z:
                rx = sc
                ry = -tc
                rz = 1.0
            if face == PIPE_TEX_FACE_NEG_Z:
                rx = -sc
                ry = -tc
                rz = -1.0
            result.append([rx, ry, rz])
        return result
                    
                
class TextureTest(TestCase):
    
    def description(self):
        target = {
            PIPE_TEXTURE_1D: "1d", 
            PIPE_TEXTURE_2D: "2d", 
            PIPE_TEXTURE_3D: "3d", 
            PIPE_TEXTURE_CUBE: "cube",
        }[self.target]
        format = formats[self.format]
        if self.target == PIPE_TEXTURE_CUBE:
            face = {
                PIPE_TEX_FACE_POS_X: "+x",
                PIPE_TEX_FACE_NEG_X: "-x",
                PIPE_TEX_FACE_POS_Y: "+y",
                PIPE_TEX_FACE_NEG_Y: "-y", 
                PIPE_TEX_FACE_POS_Z: "+z", 
                PIPE_TEX_FACE_NEG_Z: "-z",
            }[self.face]
        else:
            face = ""
        return "%s %s %ux%u last_level=%u %s level=%u" % (
            target, format, 
            self.width, self.height, 
            self.last_level, face, self.level,
        )
    
    def test(self):
        dev = self.dev
        
        target = self.target
        format = self.format
        width = self.width
        height = self.height
        last_level = self.last_level
        level = self.level
        face = self.face
        
        if not dev.is_format_supported(format, PIPE_TEXTURE):
            raise TestSkip
        
        ctx = self.dev.context_create()
    
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
        sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST
        sampler.min_img_filter = PIPE_TEX_MIPFILTER_NEAREST
        sampler.mag_img_filter = PIPE_TEX_MIPFILTER_NEAREST
        sampler.normalized_coords = 1
        sampler.min_lod = 0
        sampler.max_lod = PIPE_MAX_TEXTURE_LEVELS - 1
        ctx.set_sampler(0, sampler)
    
        #  texture 
        texture = dev.texture_create(target=target,
                                     format=format, 
                                     width=width, 
                                     height=height,
                                     last_level = last_level)
        
        expected_rgba = generate_data(texture.get_surface(
            usage = PIPE_BUFFER_USAGE_CPU_READ|PIPE_BUFFER_USAGE_CPU_WRITE,
            face = face,
            level = level))
        
        ctx.set_sampler_texture(0, texture)

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
        op = {
            PIPE_TEXTURE_1D: "1D", 
            PIPE_TEXTURE_2D: "2D", 
            PIPE_TEXTURE_3D: "3D", 
            PIPE_TEXTURE_CUBE: "CUBE",
        }[target]
        fs = Shader('''
            FRAG1.1
            DCL IN[0], GENERIC[0], LINEAR
            DCL OUT[0], COLOR, CONSTANT
            DCL SAMP[0], CONSTANT
            0:TEX OUT[0], IN[0], SAMP[0], %s
            1:END
        ''' % op)
        #fs.dump()
        ctx.set_fragment_shader(fs)

        nverts = 4
        nattrs = 2
        verts = FloatArray(nverts * nattrs * 4)
    
        x = 0
        y = 0
        w, h = minify((width, height), level)
    
        pos = [
            [x, y],
            [x+w, y],
            [x+w, y+h],
            [x, y+h],
        ]
    
        tex = tex_coords(texture, face, level, zslice=0)
    
        for i in range(0, 4):
            j = 8*i
            verts[j + 0] = pos[i][0] # x
            verts[j + 1] = pos[i][1] # y
            verts[j + 2] = 0.0 # z
            verts[j + 3] = 1.0 # w
            verts[j + 4] = tex[i][0] # s
            verts[j + 5] = tex[i][1] # r
            verts[j + 6] = tex[i][2] # q
            verts[j + 7] = 1.0
    
        ctx.draw_vertices(PIPE_PRIM_TRIANGLE_FAN,
                          nverts, 
                          nattrs, 
                          verts)
    
        ctx.flush()
    
        rgba = FloatArray(h*w*4)

        cbuf_tex.get_surface(usage = PIPE_BUFFER_USAGE_CPU_READ).get_tile_rgba(x, y, w, h, rgba)

        if not compare_rgba(w, h, rgba, expected_rgba):
        
            #show_image(w, h, Result=rgba, Expected=expected_rgba)
            #save_image(w, h, rgba, "result.png")
            #save_image(w, h, expected_rgba, "expected.png")
            #sys.exit(0)
            
            raise TestFailure

        del ctx
        


def main():
    dev = Device()
    suite = TestSuite()
    
    targets = []
    targets += [PIPE_TEXTURE_2D]
    targets += [PIPE_TEXTURE_CUBE]
    
    formats = []
    formats += [PIPE_FORMAT_A8R8G8B8_UNORM]
    formats += [PIPE_FORMAT_R5G6B5_UNORM]
    formats += [PIPE_FORMAT_L8_UNORM]
    formats += [PIPE_FORMAT_YCBCR]
    formats += [PIPE_FORMAT_DXT1_RGB]
    
    sizes = [64, 32, 16, 8, 4, 2, 1]
    #sizes = [64]
    
    for target in targets:
        for format in formats:
            for size in sizes:
                if target == PIPE_TEXTURE_CUBE:
                    faces = [
                        PIPE_TEX_FACE_POS_X,
                        PIPE_TEX_FACE_NEG_X,
                        PIPE_TEX_FACE_POS_Y,
                        PIPE_TEX_FACE_NEG_Y, 
                        PIPE_TEX_FACE_POS_Z, 
                        PIPE_TEX_FACE_NEG_Z,
                    ]
                    #faces = [PIPE_TEX_FACE_NEG_X]
                else:
                    faces = [0]
                for face in faces:
                    levels = lods(size)
                    for last_level in range(levels):
                        for level in range(0, last_level + 1):
                            test = TextureTest(
                                dev=dev,
                                target=target,
                                format=format, 
                                width=size,
                                height=size,
                                face=face,
                                last_level = last_level,
                                level=level,
                            )
                            suite.add_test(test)
    suite.run()


if __name__ == '__main__':
    main()
