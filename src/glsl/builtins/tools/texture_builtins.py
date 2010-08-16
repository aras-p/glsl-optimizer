#!/usr/bin/python

import sys
import StringIO

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

def start_function(name):
    sys.stdout = StringIO.StringIO()
    print "((function " + name

def end_function(fs, name):
    print "))"
    fs[name] = sys.stdout.getvalue();
    sys.stdout.close()

# Generate all the functions and store them in the supplied dictionary.
# This is better than writing them to actual files since they should never be
# edited; it'd also be easy to confuse them with the many hand-generated files.
#
# Takes a dictionary as an argument.
def generate_texture_functions(fs):
    start_function("texture")
    generate_fiu_sigs("tex", "1D")
    generate_fiu_sigs("tex", "2D")
    generate_fiu_sigs("tex", "3D")
    generate_fiu_sigs("tex", "Cube")
    generate_fiu_sigs("tex", "1DArray")
    generate_fiu_sigs("tex", "2DArray")

    generate_fiu_sigs("txb", "1D")
    generate_fiu_sigs("txb", "2D")
    generate_fiu_sigs("txb", "3D")
    generate_fiu_sigs("txb", "Cube")
    generate_fiu_sigs("txb", "1DArray")
    generate_fiu_sigs("txb", "2DArray")
    end_function(fs, "texture")

    start_function("textureProj")
    generate_fiu_sigs("tex", "1D", True)
    generate_fiu_sigs("tex", "1D", True, 2)
    generate_fiu_sigs("tex", "2D", True)
    generate_fiu_sigs("tex", "2D", True, 1)
    generate_fiu_sigs("tex", "3D", True)

    generate_fiu_sigs("txb", "1D", True)
    generate_fiu_sigs("txb", "1D", True, 2)
    generate_fiu_sigs("txb", "2D", True)
    generate_fiu_sigs("txb", "2D", True, 1)
    generate_fiu_sigs("txb", "3D", True)
    end_function(fs, "textureProj")

    start_function("textureLod")
    generate_fiu_sigs("txl", "1D")
    generate_fiu_sigs("txl", "2D")
    generate_fiu_sigs("txl", "3D")
    generate_fiu_sigs("txl", "Cube")
    generate_fiu_sigs("txl", "1DArray")
    generate_fiu_sigs("txl", "2DArray")
    end_function(fs, "textureLod")

    start_function("texelFetch")
    generate_fiu_sigs("txf", "1D")
    generate_fiu_sigs("txf", "2D")
    generate_fiu_sigs("txf", "3D")
    generate_fiu_sigs("txf", "1DArray")
    generate_fiu_sigs("txf", "2DArray")
    end_function(fs, "texelFetch")

    start_function("textureProjLod")
    generate_fiu_sigs("txl", "1D", True)
    generate_fiu_sigs("txl", "1D", True, 2)
    generate_fiu_sigs("txl", "2D", True)
    generate_fiu_sigs("txl", "2D", True, 1)
    generate_fiu_sigs("txl", "3D", True)
    end_function(fs, "textureProjLod")

    start_function("textureGrad")
    generate_fiu_sigs("txd", "1D")
    generate_fiu_sigs("txd", "2D")
    generate_fiu_sigs("txd", "3D")
    generate_fiu_sigs("txd", "Cube")
    generate_fiu_sigs("txd", "1DArray")
    generate_fiu_sigs("txd", "2DArray")
    end_function(fs, "textureGrad")

    start_function("textureProjGrad")
    generate_fiu_sigs("txd", "1D", True)
    generate_fiu_sigs("txd", "1D", True, 2)
    generate_fiu_sigs("txd", "2D", True)
    generate_fiu_sigs("txd", "2D", True, 1)
    generate_fiu_sigs("txd", "3D", True)
    end_function(fs, "textureProjGrad")

    # ARB_texture_rectangle extension
    start_function("texture2DRect")
    generate_sigs("", "tex", "2DRect")
    end_function(fs, "texture2DRect")

    start_function("texture2DRectProj")
    generate_sigs("", "tex", "2DRect", True)
    generate_sigs("", "tex", "2DRect", True, 1)
    end_function(fs, "texture2DRectProj")

    start_function("shadow2DRect")
    generate_sigs("", "tex", "2DRectShadow")
    end_function(fs, "shadow2DRect")

    start_function("shadow2DRectProj")
    generate_sigs("", "tex", "2DRectShadow", True)
    end_function(fs, "shadow2DRectProj")

    # EXT_texture_array extension
    start_function("texture1DArray")
    generate_sigs("", "tex", "1DArray")
    generate_sigs("", "txb", "1DArray")
    end_function(fs, "texture1DArray")

    start_function("texture1DArrayLod")
    generate_sigs("", "txl", "1DArray")
    end_function(fs, "texture1DArrayLod")

    start_function("texture2DArray")
    generate_sigs("", "tex", "2DArray")
    generate_sigs("", "txb", "2DArray")
    end_function(fs, "texture2DArray")

    start_function("texture2DArrayLod")
    generate_sigs("", "txl", "2DArray")
    end_function(fs, "texture2DArrayLod")

    start_function("shadow1DArray")
    generate_sigs("", "tex", "1DArrayShadow")
    generate_sigs("", "txb", "1DArrayShadow")
    end_function(fs, "shadow1DArray")

    start_function("shadow1DArrayLod")
    generate_sigs("", "txl", "1DArrayShadow")
    end_function(fs, "shadow1DArrayLod")

    start_function("shadow2DArray")
    generate_sigs("", "tex", "2DArrayShadow")
    end_function(fs, "shadow2DArray")

    # Deprecated (110/120 style) functions with silly names:
    start_function("texture1D")
    generate_sigs("", "tex", "1D")
    generate_sigs("", "txb", "1D")
    end_function(fs, "texture1D")

    start_function("texture1DLod")
    generate_sigs("", "txl", "1D")
    end_function(fs, "texture1DLod")

    start_function("texture1DProj")
    generate_sigs("", "tex", "1D", True)
    generate_sigs("", "tex", "1D", True, 2)
    generate_sigs("", "txb", "1D", True)
    generate_sigs("", "txb", "1D", True, 2)
    end_function(fs, "texture1DProj")

    start_function("texture1DProjLod")
    generate_sigs("", "txl", "1D", True)
    generate_sigs("", "txl", "1D", True, 2)
    end_function(fs, "texture1DProjLod")

    start_function("texture2D")
    generate_sigs("", "tex", "2D")
    generate_sigs("", "txb", "2D")
    end_function(fs, "texture2D")

    start_function("texture2DLod")
    generate_sigs("", "txl", "2D")
    end_function(fs, "texture2DLod")

    start_function("texture2DProj")
    generate_sigs("", "tex", "2D", True)
    generate_sigs("", "tex", "2D", True, 1)
    generate_sigs("", "txb", "2D", True)
    generate_sigs("", "txb", "2D", True, 1)
    end_function(fs, "texture2DProj")

    start_function("texture2DProjLod")
    generate_sigs("", "txl", "2D", True)
    generate_sigs("", "txl", "2D", True, 1)
    end_function(fs, "texture2DProjLod")

    start_function("texture3D")
    generate_sigs("", "tex", "3D")
    generate_sigs("", "txb", "3D")
    end_function(fs, "texture3D")

    start_function("texture3DLod")
    generate_sigs("", "txl", "3D")
    end_function(fs, "texture3DLod")

    start_function("texture3DProj")
    generate_sigs("", "tex", "3D", True)
    generate_sigs("", "txb", "3D", True)
    end_function(fs, "texture3DProj")

    start_function("texture3DProjLod")
    generate_sigs("", "txl", "3D", True)
    end_function(fs, "texture3DProjLod")

    start_function("textureCube")
    generate_sigs("", "tex", "Cube")
    generate_sigs("", "txb", "Cube")
    end_function(fs, "textureCube")

    start_function("textureCubeLod")
    generate_sigs("", "txl", "Cube")
    end_function(fs, "textureCubeLod")

    start_function("shadow1D")
    generate_sigs("", "tex", "1DShadow", False, 1)
    generate_sigs("", "txb", "1DShadow", False, 1)
    end_function(fs, "shadow1D")

    start_function("shadow1DLod")
    generate_sigs("", "txl", "1DShadow", False, 1)
    end_function(fs, "shadow1DLod")

    start_function("shadow1DProj")
    generate_sigs("", "tex", "1DShadow", True, 1)
    generate_sigs("", "txb", "1DShadow", True, 1)
    end_function(fs, "shadow1DProj")

    start_function("shadow1DProjLod")
    generate_sigs("", "txl", "1DShadow", True, 1)
    end_function(fs, "shadow1DProjLod")

    start_function("shadow2D")
    generate_sigs("", "tex", "2DShadow")
    generate_sigs("", "txb", "2DShadow")
    end_function(fs, "shadow2D")

    start_function("shadow2DLod")
    generate_sigs("", "txl", "2DShadow")
    end_function(fs, "shadow2DLod")

    start_function("shadow2DProj")
    generate_sigs("", "tex", "2DShadow", True)
    generate_sigs("", "txb", "2DShadow", True)
    end_function(fs, "shadow2DProj")

    start_function("shadow2DProjLod")
    generate_sigs("", "txl", "2DShadow", True)
    end_function(fs, "shadow2DProjLod")

    sys.stdout = sys.__stdout__
    return fs

# If you actually run this script, it'll print out all the functions.
if __name__ == "__main__":
    fs = {}
    generate_texture_functions(fs);
    for k, v in fs.iteritems():
	print v
