/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

struct builtin_variable {
   enum ir_variable_mode mode;
   const char *type;
   const char *name;
};

static const builtin_variable builtin_core_vs_variables[] = {
   { ir_var_out, "vec4",  "gl_Position" },
   { ir_var_out, "float", "gl_PointSize" },
};

static const builtin_variable builtin_110_deprecated_vs_variables[] = {
   { ir_var_in,  "vec4",  "gl_Vertex" },
   { ir_var_in,  "vec4",  "gl_Normal" },
   { ir_var_in,  "vec4",  "gl_Color" },
   { ir_var_in,  "vec4",  "gl_SecondaryColor" },
   { ir_var_in,  "vec4",  "gl_MultiTexCoord0" },
   { ir_var_in,  "vec4",  "gl_MultiTexCoord1" },
   { ir_var_in,  "vec4",  "gl_MultiTexCoord2" },
   { ir_var_in,  "vec4",  "gl_MultiTexCoord3" },
   { ir_var_in,  "vec4",  "gl_MultiTexCoord4" },
   { ir_var_in,  "vec4",  "gl_MultiTexCoord5" },
   { ir_var_in,  "vec4",  "gl_MultiTexCoord6" },
   { ir_var_in,  "vec4",  "gl_MultiTexCoord7" },
   { ir_var_in,  "float", "gl_FogCoord" },
   { ir_var_out, "vec4",  "gl_ClipVertex" },
   { ir_var_out, "vec4",  "gl_FrontColor" },
   { ir_var_out, "vec4",  "gl_BackColor" },
   { ir_var_out, "vec4",  "gl_FrontSecondaryColor" },
   { ir_var_out, "vec4",  "gl_BackSecondaryColor" },
   { ir_var_out, "vec4",  "gl_FogFragCoord" },
};

static const builtin_variable builtin_130_vs_variables[] = {
   { ir_var_in,  "int",   "gl_VertexID" },
};
