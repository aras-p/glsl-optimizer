#!/usr/bin/python

from os import path
import sys

def vec_type(g, size):
    if size == 1:
        if g == "i":
            return "int"
        elif g == "u":
            return "uint"
        return "float"
    return g + "vec" + str(size)

# Get the base dimension - i.e. sampler3D gives 3
# Array samplers also get +1 here since the layer is really an extra coordinate
def get_coord_dim(sampler_type):
    if sampler_type[0].isdigit():
        coord_dim = int(sampler_type[0])
    elif sampler_type.startswith("Cube"):
        coord_dim = 3
    else:
        assert False ("coord_dim: invalid sampler_type: " + sampler_type)

    if sampler_type.find("Array") != -1:
        coord_dim += 1
    return coord_dim

# Get the number of extra vector components (i.e. shadow comparitor)
def get_extra_dim(sampler_type, use_proj, unused_fields):
    extra_dim = unused_fields
    if sampler_type.find("Shadow") != -1:
        extra_dim += 1
    if use_proj:
        extra_dim += 1
    return extra_dim

def generate_sigs(g, tex_inst, sampler_type, use_proj = False, unused_fields = 0):
    coord_dim = get_coord_dim(sampler_type)
    extra_dim = get_extra_dim(sampler_type, use_proj, unused_fields)

    # Print parameters
    print "   (signature " + g + "vec4"
    print "     (parameters"
    print "       (declare (in) " + g + "sampler" + sampler_type + " sampler)"
    print "       (declare (in) " + vec_type("i" if tex_inst == "txf" else "", coord_dim + extra_dim) + " P)",
    if tex_inst == "txb":
        print "\n       (declare (in) float bias)",
    elif tex_inst == "txl":
        print "\n       (declare (in) float lod)",
    elif tex_inst == "txf":
        print "\n       (declare (in) int lod)",
    elif tex_inst == "txd":
        grad_type = vec_type("", coord_dim)
        print "\n       (declare (in) " + grad_type + " dPdx)",
        print "\n       (declare (in) " + grad_type + " dPdy)",

    print ")\n     ((return (" + tex_inst + " (var_ref sampler)",

    # Coordinate
    if extra_dim > 0:
        print "(swiz " + "xyzw"[:coord_dim] + " (var_ref P))",
    else:
        print "(var_ref P)",

    # Offset
    print "(0 0 0)",

    if tex_inst != "txf":
        # Projective divisor
        if use_proj:
            print "(swiz " + "xyzw"[coord_dim + extra_dim-1] + " (var_ref P))",
        else:
            print "1",

        # Shadow comparitor
        if sampler_type == "2DArrayShadow": # a special case:
            print "(swiz w (var_ref P))",   # ...array layer is z; shadow is w
        elif sampler_type.endswith("Shadow"):
            print "(swiz z (var_ref P))",
        else:
            print "()",

    # Bias/explicit LOD/gradient:
    if tex_inst == "txb":
        print "(var_ref bias)",
    elif tex_inst == "txl" or tex_inst == "txf":
        print "(var_ref lod)",
    elif tex_inst == "txd":
        print "((var_ref dPdx) (var_ref dPdy))",
    print "))))\n"

def generate_fiu_sigs(tex_inst, sampler_type, use_proj = False, unused_fields = 0):
    generate_sigs("",  tex_inst, sampler_type, use_proj, unused_fields)
    generate_sigs("i", tex_inst, sampler_type, use_proj, unused_fields)
    generate_sigs("u", tex_inst, sampler_type, use_proj, unused_fields)

builtins_dir = path.join(path.dirname(path.abspath(__file__)), "..")

with open(path.join(builtins_dir, "130", "texture"), 'w') as sys.stdout:
    print "((function texture"
    generate_fiu_sigs("tex", "1D")
    generate_fiu_sigs("tex", "2D")
    generate_fiu_sigs("tex", "3D")
    generate_fiu_sigs("tex", "Cube")
    generate_fiu_sigs("tex", "1DArray")
    generate_fiu_sigs("tex", "2DArray")
    print "))"

# txb variants are only allowed within a fragment shader (GLSL 1.30 p. 86)
with open(path.join(builtins_dir, "130_fs", "texture"), 'w') as sys.stdout:
    print "((function texture"
    generate_fiu_sigs("txb", "1D")
    generate_fiu_sigs("txb", "2D")
    generate_fiu_sigs("txb", "3D")
    generate_fiu_sigs("txb", "Cube")
    generate_fiu_sigs("txb", "1DArray")
    generate_fiu_sigs("txb", "2DArray")
    print "))"

with open(path.join(builtins_dir, "130", "textureProj"), 'w') as sys.stdout:
    print "((function textureProj"
    generate_fiu_sigs("tex", "1D", True)
    generate_fiu_sigs("tex", "1D", True, 2)
    generate_fiu_sigs("tex", "2D", True)
    generate_fiu_sigs("tex", "2D", True, 1)
    generate_fiu_sigs("tex", "3D", True)
    print "))"

with open(path.join(builtins_dir, "130_fs", "textureProj"), 'w') as sys.stdout:
    print "((function textureProj"
    generate_fiu_sigs("txb", "1D", True)
    generate_fiu_sigs("txb", "1D", True, 2)
    generate_fiu_sigs("txb", "2D", True)
    generate_fiu_sigs("txb", "2D", True, 1)
    generate_fiu_sigs("txb", "3D", True)
    print "))"

with open(path.join(builtins_dir, "130", "textureLod"), 'w') as sys.stdout:
    print "((function textureLod"
    generate_fiu_sigs("txl", "1D")
    generate_fiu_sigs("txl", "2D")
    generate_fiu_sigs("txl", "3D")
    generate_fiu_sigs("txl", "Cube")
    generate_fiu_sigs("txl", "1DArray")
    generate_fiu_sigs("txl", "2DArray")
    print "))"

with open(path.join(builtins_dir, "130", "texelFetch"), 'w') as sys.stdout:
    print "((function texelFetch"
    generate_fiu_sigs("txf", "1D")
    generate_fiu_sigs("txf", "2D")
    generate_fiu_sigs("txf", "3D")
    generate_fiu_sigs("txf", "1DArray")
    generate_fiu_sigs("txf", "2DArray")
    print "))"

with open(path.join(builtins_dir, "130", "textureProjLod"), 'w') as sys.stdout:
    print "((function textureProjLod"
    generate_fiu_sigs("txl", "1D", True)
    generate_fiu_sigs("txl", "1D", True, 2)
    generate_fiu_sigs("txl", "2D", True)
    generate_fiu_sigs("txl", "2D", True, 1)
    generate_fiu_sigs("txl", "3D", True)
    print "))"

with open(path.join(builtins_dir, "130", "textureGrad"), 'w') as sys.stdout:
    print "((function textureGrad"
    generate_fiu_sigs("txd", "1D")
    generate_fiu_sigs("txd", "2D")
    generate_fiu_sigs("txd", "3D")
    generate_fiu_sigs("txd", "Cube")
    generate_fiu_sigs("txd", "1DArray")
    generate_fiu_sigs("txd", "2DArray")
    print ")\n)"

with open(path.join(builtins_dir, "130", "textureProjGrad"), 'w') as sys.stdout:
    print "((function textureProjGrad"
    generate_fiu_sigs("txd", "1D", True)
    generate_fiu_sigs("txd", "1D", True, 2)
    generate_fiu_sigs("txd", "2D", True)
    generate_fiu_sigs("txd", "2D", True, 1)
    generate_fiu_sigs("txd", "3D", True)
    print "))"

# ARB_texture_rectangle extension
with open(path.join(builtins_dir, "ARB_texture_rectangle", "textures"), 'w') as sys.stdout:
    print "((function texture2DRect"
    generate_sigs("", "tex", "2DRect")
    print ")\n (function shadow2DRect"
    generate_sigs("", "tex", "2DRectShadow")
    print "))"

# EXT_texture_array extension
with open(path.join(builtins_dir, "EXT_texture_array", "textures"), 'w') as sys.stdout:
    print "((function texture1DArray"
    generate_sigs("", "tex", "1DArray")
    print ")\n (function texture1DArrayLod"
    generate_sigs("", "txl", "1DArray")
    print ")\n (function texture2DArray"
    generate_sigs("", "tex", "2DArray")
    print ")\n (function texture2DArrayLod"
    generate_sigs("", "txl", "2DArray")
    print ")\n (function shadow1DArray"
    generate_sigs("", "tex", "1DArrayShadow")
    print ")\n (function shadow1DArrayLod"
    generate_sigs("", "txl", "1DArrayShadow")
    print ")\n (function shadow2DArray"
    generate_sigs("", "tex", "2DArrayShadow")
    print "))"

with open(path.join(builtins_dir, "EXT_texture_array_fs", "textures"), 'w') as sys.stdout:
    print "((function texture1DArray"
    generate_sigs("", "txb", "1DArray")
    print ")\n (function texture2DArray"
    generate_sigs("", "txb", "2DArray")
    print ")\n (function shadow1DArray"
    generate_sigs("", "txb", "1DArrayShadow")
    print "))"

# Deprecated (110/120 style) functions with silly names:
with open(path.join(builtins_dir, "110", "textures"), 'w') as sys.stdout:
    print "((function texture1D"
    generate_sigs("", "tex", "1D")
    print ")\n (function texture1DLod"
    generate_sigs("", "txl", "1D")
    print ")\n (function texture1DProj"
    generate_sigs("", "tex", "1D", True)
    generate_sigs("", "tex", "1D", True, 2)
    print ")\n (function texture1DProjLod"
    generate_sigs("", "txl", "1D", True)
    generate_sigs("", "txl", "1D", True, 2)
    print ")\n (function texture2D"
    generate_sigs("", "tex", "2D")
    print ")\n(function texture2DLod"
    generate_sigs("", "txl", "2D")
    print ")\n (function texture2DProj"
    generate_sigs("", "tex", "2D", True)
    generate_sigs("", "tex", "2D", True, 1)
    print ")\n (function texture2DProjLod"
    generate_sigs("", "txl", "2D", True)
    generate_sigs("", "txl", "2D", True, 1)
    print ")\n (function texture3D"
    generate_sigs("", "tex", "3D")
    print ")\n (function texture3DLod"
    generate_sigs("", "txl", "3D")
    print ")\n (function texture3DProj"
    generate_sigs("", "tex", "3D", True)
    print ")\n (function texture3DProjLod"
    generate_sigs("", "txl", "3D", True)
    print ")\n (function textureCube"
    generate_sigs("", "tex", "Cube")
    print ")\n (function textureCubeLod"
    generate_sigs("", "txl", "Cube")
    print ")\n (function shadow1D"
    generate_sigs("", "tex", "1DShadow", False, 1)
    print ")\n (function shadow1DLod"
    generate_sigs("", "txl", "1DShadow", False, 1)
    print ")\n (function shadow1DProj"
    generate_sigs("", "tex", "1DShadow", True, 1)
    print ")\n (function shadow1DProjLod"
    generate_sigs("", "txl", "1DShadow", True, 1)
    print ")\n (function shadow2D"
    generate_sigs("", "tex", "2DShadow")
    print ")\n (function shadow2DLod"
    generate_sigs("", "txl", "2DShadow")
    print ")\n (function shadow2DProj"
    generate_sigs("", "tex", "2DShadow", True)
    print ")\n (function shadow2DProjLod"
    generate_sigs("", "txl", "2DShadow", True)
    print "))"

with open(path.join(builtins_dir, "110_fs", "textures"), 'w') as sys.stdout:
    print "((function texture1D"
    generate_sigs("", "txb", "1D")
    print ")\n (function texture1DProj"
    generate_sigs("", "txb", "1D", True)
    generate_sigs("", "txb", "1D", True, 2)
    print ")\n (function texture2D"
    generate_sigs("", "txb", "2D")
    print ")\n (function texture2DProj"
    generate_sigs("", "txb", "2D", True)
    generate_sigs("", "txb", "2D", True, 1)
    print ")\n (function texture3D"
    generate_sigs("", "txb", "3D")
    print ")\n (function texture3DProj"
    generate_sigs("", "txb", "3D", True)
    print ")\n (function textureCube"
    generate_sigs("", "txb", "Cube")
    print ")\n (function shadow1D"
    generate_sigs("", "txb", "1DShadow", False, 1)
    print ")\n (function shadow1DProj"
    generate_sigs("", "txb", "1DShadow", True, 1)
    print ")\n (function shadow2D"
    generate_sigs("", "txb", "2DShadow")
    print ")\n (function shadow2DProj"
    generate_sigs("", "txb", "2DShadow", True)
    print "))"
