/*
 * Copyright (C) 2010  Brian Paul   All Rights Reserved.
 * Copyright (C) 2010  Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author: Kristian Høgsberg <krh@bitplanet.net>
 */

#include "glheader.h"
#include "context.h"
#include "blend.h"
#include "enable.h"
#include "enums.h"
#include "extensions.h"
#include "get.h"
#include "macros.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "state.h"
#include "texcompress.h"
#include "framebuffer.h"
#include "samplerobj.h"

/* This is a table driven implemetation of the glGet*v() functions.
 * The basic idea is that most getters just look up an int somewhere
 * in struct gl_context and then convert it to a bool or float according to
 * which of glGetIntegerv() glGetBooleanv() etc is being called.
 * Instead of generating code to do this, we can just record the enum
 * value and the offset into struct gl_context in an array of structs.  Then
 * in glGet*(), we lookup the struct for the enum in question, and use
 * the offset to get the int we need.
 *
 * Sometimes we need to look up a float, a boolean, a bit in a
 * bitfield, a matrix or other types instead, so we need to track the
 * type of the value in struct gl_context.  And sometimes the value isn't in
 * struct gl_context but in the drawbuffer, the array object, current texture
 * unit, or maybe it's a computed value.  So we need to also track
 * where or how to find the value.  Finally, we sometimes need to
 * check that one of a number of extensions are enabled, the GL
 * version or flush or call _mesa_update_state().  This is done by
 * attaching optional extra information to the value description
 * struct, it's sort of like an array of opcodes that describe extra
 * checks or actions.
 *
 * Putting all this together we end up with struct value_desc below,
 * and with a couple of macros to help, the table of struct value_desc
 * is about as concise as the specification in the old python script.
 */

#define FLOAT_TO_BOOLEAN(X)   ( (X) ? GL_TRUE : GL_FALSE )
#define FLOAT_TO_FIXED(F)     ( ((F) * 65536.0f > INT_MAX) ? INT_MAX : \
                                ((F) * 65536.0f < INT_MIN) ? INT_MIN : \
                                (GLint) ((F) * 65536.0f) )

#define INT_TO_BOOLEAN(I)     ( (I) ? GL_TRUE : GL_FALSE )
#define INT_TO_FIXED(I)       ( ((I) > SHRT_MAX) ? INT_MAX : \
                                ((I) < SHRT_MIN) ? INT_MIN : \
                                (GLint) ((I) * 65536) )

#define INT64_TO_BOOLEAN(I)   ( (I) ? GL_TRUE : GL_FALSE )
#define INT64_TO_INT(I)       ( (GLint)((I > INT_MAX) ? INT_MAX : ((I < INT_MIN) ? INT_MIN : (I))) )

#define BOOLEAN_TO_INT(B)     ( (GLint) (B) )
#define BOOLEAN_TO_INT64(B)   ( (GLint64) (B) )
#define BOOLEAN_TO_FLOAT(B)   ( (B) ? 1.0F : 0.0F )
#define BOOLEAN_TO_FIXED(B)   ( (GLint) ((B) ? 1 : 0) << 16 )

#define ENUM_TO_INT64(E)      ( (GLint64) (E) )
#define ENUM_TO_FIXED(E)      (E)

enum value_type {
   TYPE_INVALID,
   TYPE_INT,
   TYPE_INT_2,
   TYPE_INT_3,
   TYPE_INT_4,
   TYPE_INT_N,
   TYPE_INT64,
   TYPE_ENUM,
   TYPE_ENUM_2,
   TYPE_BOOLEAN,
   TYPE_BIT_0,
   TYPE_BIT_1,
   TYPE_BIT_2,
   TYPE_BIT_3,
   TYPE_BIT_4,
   TYPE_BIT_5,
   TYPE_BIT_6,
   TYPE_BIT_7,
   TYPE_FLOAT,
   TYPE_FLOAT_2,
   TYPE_FLOAT_3,
   TYPE_FLOAT_4,
   TYPE_FLOATN,
   TYPE_FLOATN_2,
   TYPE_FLOATN_3,
   TYPE_FLOATN_4,
   TYPE_DOUBLEN,
   TYPE_MATRIX,
   TYPE_MATRIX_T,
   TYPE_CONST
};

enum value_location {
   LOC_BUFFER,
   LOC_CONTEXT,
   LOC_ARRAY,
   LOC_TEXUNIT,
   LOC_CUSTOM
};

enum value_extra {
   EXTRA_END = 0x8000,
   EXTRA_VERSION_30,
   EXTRA_VERSION_31,
   EXTRA_VERSION_32,
   EXTRA_API_GL,
   EXTRA_API_GL_CORE,
   EXTRA_API_ES2,
   EXTRA_API_ES3,
   EXTRA_NEW_BUFFERS, 
   EXTRA_NEW_FRAG_CLAMP,
   EXTRA_VALID_DRAW_BUFFER,
   EXTRA_VALID_TEXTURE_UNIT,
   EXTRA_VALID_CLIP_DISTANCE,
   EXTRA_FLUSH_CURRENT,
   EXTRA_GLSL_130,
};

#define NO_EXTRA NULL
#define NO_OFFSET 0

struct value_desc {
   GLenum pname;
   GLubyte location;  /**< enum value_location */
   GLubyte type;      /**< enum value_type */
   int offset;
   const int *extra;
};

union value {
   GLfloat value_float;
   GLfloat value_float_4[4];
   GLmatrix *value_matrix;
   GLint value_int;
   GLint value_int_4[4];
   GLint64 value_int64;
   GLenum value_enum;

   /* Sigh, see GL_COMPRESSED_TEXTURE_FORMATS_ARB handling */
   struct {
      GLint n, ints[100];
   } value_int_n;
   GLboolean value_bool;
};

#define BUFFER_FIELD(field, type) \
   LOC_BUFFER, type, offsetof(struct gl_framebuffer, field)
#define CONTEXT_FIELD(field, type) \
   LOC_CONTEXT, type, offsetof(struct gl_context, field)
#define ARRAY_FIELD(field, type) \
   LOC_ARRAY, type, offsetof(struct gl_array_object, field)
#undef CONST /* already defined through windows.h */
#define CONST(value) \
   LOC_CONTEXT, TYPE_CONST, value

#define BUFFER_INT(field) BUFFER_FIELD(field, TYPE_INT)
#define BUFFER_ENUM(field) BUFFER_FIELD(field, TYPE_ENUM)
#define BUFFER_BOOL(field) BUFFER_FIELD(field, TYPE_BOOLEAN)

#define CONTEXT_INT(field) CONTEXT_FIELD(field, TYPE_INT)
#define CONTEXT_INT2(field) CONTEXT_FIELD(field, TYPE_INT_2)
#define CONTEXT_INT64(field) CONTEXT_FIELD(field, TYPE_INT64)
#define CONTEXT_ENUM(field) CONTEXT_FIELD(field, TYPE_ENUM)
#define CONTEXT_ENUM2(field) CONTEXT_FIELD(field, TYPE_ENUM_2)
#define CONTEXT_BOOL(field) CONTEXT_FIELD(field, TYPE_BOOLEAN)
#define CONTEXT_BIT0(field) CONTEXT_FIELD(field, TYPE_BIT_0)
#define CONTEXT_BIT1(field) CONTEXT_FIELD(field, TYPE_BIT_1)
#define CONTEXT_BIT2(field) CONTEXT_FIELD(field, TYPE_BIT_2)
#define CONTEXT_BIT3(field) CONTEXT_FIELD(field, TYPE_BIT_3)
#define CONTEXT_BIT4(field) CONTEXT_FIELD(field, TYPE_BIT_4)
#define CONTEXT_BIT5(field) CONTEXT_FIELD(field, TYPE_BIT_5)
#define CONTEXT_BIT6(field) CONTEXT_FIELD(field, TYPE_BIT_6)
#define CONTEXT_BIT7(field) CONTEXT_FIELD(field, TYPE_BIT_7)
#define CONTEXT_FLOAT(field) CONTEXT_FIELD(field, TYPE_FLOAT)
#define CONTEXT_FLOAT2(field) CONTEXT_FIELD(field, TYPE_FLOAT_2)
#define CONTEXT_FLOAT3(field) CONTEXT_FIELD(field, TYPE_FLOAT_3)
#define CONTEXT_FLOAT4(field) CONTEXT_FIELD(field, TYPE_FLOAT_4)
#define CONTEXT_MATRIX(field) CONTEXT_FIELD(field, TYPE_MATRIX)
#define CONTEXT_MATRIX_T(field) CONTEXT_FIELD(field, TYPE_MATRIX_T)

#define ARRAY_INT(field) ARRAY_FIELD(field, TYPE_INT)
#define ARRAY_ENUM(field) ARRAY_FIELD(field, TYPE_ENUM)
#define ARRAY_BOOL(field) ARRAY_FIELD(field, TYPE_BOOLEAN)

#define EXT(f)					\
   offsetof(struct gl_extensions, f)

#define EXTRA_EXT(e)				\
   static const int extra_##e[] = {		\
      EXT(e), EXTRA_END				\
   }

#define EXTRA_EXT2(e1, e2)			\
   static const int extra_##e1##_##e2[] = {	\
      EXT(e1), EXT(e2), EXTRA_END		\
   }

/* The 'extra' mechanism is a way to specify extra checks (such as
 * extensions or specific gl versions) or actions (flush current, new
 * buffers) that we need to do before looking up an enum.  We need to
 * declare them all up front so we can refer to them in the value_desc
 * structs below. */

static const int extra_new_buffers[] = {
   EXTRA_NEW_BUFFERS,
   EXTRA_END
};

static const int extra_new_frag_clamp[] = {
   EXTRA_NEW_FRAG_CLAMP,
   EXTRA_END
};

static const int extra_valid_draw_buffer[] = {
   EXTRA_VALID_DRAW_BUFFER,
   EXTRA_END
};

static const int extra_valid_texture_unit[] = {
   EXTRA_VALID_TEXTURE_UNIT,
   EXTRA_END
};

static const int extra_valid_clip_distance[] = {
   EXTRA_VALID_CLIP_DISTANCE,
   EXTRA_END
};

static const int extra_flush_current_valid_texture_unit[] = {
   EXTRA_FLUSH_CURRENT,
   EXTRA_VALID_TEXTURE_UNIT,
   EXTRA_END
};

static const int extra_flush_current[] = {
   EXTRA_FLUSH_CURRENT,
   EXTRA_END
};

static const int extra_EXT_secondary_color_flush_current[] = {
   EXT(EXT_secondary_color),
   EXTRA_FLUSH_CURRENT,
   EXTRA_END
};

static const int extra_EXT_fog_coord_flush_current[] = {
   EXT(EXT_fog_coord),
   EXTRA_FLUSH_CURRENT,
   EXTRA_END
};

static const int extra_EXT_texture_integer[] = {
   EXT(EXT_texture_integer),
   EXTRA_END
};

static const int extra_GLSL_130[] = {
   EXTRA_GLSL_130,
   EXTRA_END
};

static const int extra_texture_buffer_object[] = {
   EXTRA_API_GL_CORE,
   EXTRA_VERSION_31,
   EXT(ARB_texture_buffer_object),
   EXTRA_END
};

static const int extra_ARB_transform_feedback2_api_es3[] = {
   EXT(ARB_transform_feedback2),
   EXTRA_API_ES3,
   EXTRA_END
};

static const int extra_ARB_uniform_buffer_object_and_geometry_shader[] = {
   EXT(ARB_uniform_buffer_object),
   EXT(ARB_geometry_shader4),
   EXTRA_END
};

static const int extra_ARB_ES2_compatibility_api_es2[] = {
   EXT(ARB_ES2_compatibility),
   EXTRA_API_ES2,
   EXTRA_END
};

static const int extra_ARB_ES3_compatibility_api_es3[] = {
   EXT(ARB_ES3_compatibility),
   EXTRA_API_ES3,
   EXTRA_END
};

EXTRA_EXT(ARB_texture_cube_map);
EXTRA_EXT(MESA_texture_array);
EXTRA_EXT2(EXT_secondary_color, ARB_vertex_program);
EXTRA_EXT(EXT_secondary_color);
EXTRA_EXT(EXT_fog_coord);
EXTRA_EXT(NV_fog_distance);
EXTRA_EXT(EXT_texture_filter_anisotropic);
EXTRA_EXT(NV_point_sprite);
EXTRA_EXT(NV_texture_rectangle);
EXTRA_EXT(EXT_stencil_two_side);
EXTRA_EXT(EXT_depth_bounds_test);
EXTRA_EXT(ARB_depth_clamp);
EXTRA_EXT(ATI_fragment_shader);
EXTRA_EXT(EXT_framebuffer_blit);
EXTRA_EXT(ARB_shader_objects);
EXTRA_EXT(EXT_provoking_vertex);
EXTRA_EXT(ARB_fragment_shader);
EXTRA_EXT(ARB_fragment_program);
EXTRA_EXT2(ARB_framebuffer_object, EXT_framebuffer_multisample);
EXTRA_EXT(EXT_framebuffer_object);
EXTRA_EXT(ARB_seamless_cube_map);
EXTRA_EXT(ARB_sync);
EXTRA_EXT(ARB_vertex_shader);
EXTRA_EXT(EXT_transform_feedback);
EXTRA_EXT(ARB_transform_feedback3);
EXTRA_EXT(EXT_pixel_buffer_object);
EXTRA_EXT(ARB_vertex_program);
EXTRA_EXT2(NV_point_sprite, ARB_point_sprite);
EXTRA_EXT2(ARB_vertex_program, ARB_fragment_program);
EXTRA_EXT(ARB_geometry_shader4);
EXTRA_EXT(ARB_color_buffer_float);
EXTRA_EXT(EXT_framebuffer_sRGB);
EXTRA_EXT(OES_EGL_image_external);
EXTRA_EXT(ARB_blend_func_extended);
EXTRA_EXT(ARB_uniform_buffer_object);
EXTRA_EXT(ARB_timer_query);
EXTRA_EXT(ARB_map_buffer_alignment);
EXTRA_EXT(ARB_texture_cube_map_array);
EXTRA_EXT(ARB_texture_buffer_range);
EXTRA_EXT(ARB_texture_multisample);

static const int
extra_ARB_color_buffer_float_or_glcore[] = {
   EXT(ARB_color_buffer_float),
   EXTRA_API_GL_CORE,
   EXTRA_END
};

static const int
extra_NV_primitive_restart[] = {
   EXT(NV_primitive_restart),
   EXTRA_END
};

static const int extra_version_30[] = { EXTRA_VERSION_30, EXTRA_END };
static const int extra_version_31[] = { EXTRA_VERSION_31, EXTRA_END };
static const int extra_version_32[] = { EXTRA_VERSION_32, EXTRA_END };

static const int extra_gl30_es3[] = {
    EXTRA_VERSION_30,
    EXTRA_API_ES3,
    EXTRA_END,
};

static const int
extra_ARB_vertex_program_api_es2[] = {
   EXT(ARB_vertex_program),
   EXTRA_API_ES2,
   EXTRA_END
};

/* The ReadBuffer get token is valid under either full GL or under
 * GLES2 if the NV_read_buffer extension is available. */
static const int
extra_NV_read_buffer_api_gl[] = {
   EXTRA_API_ES2,
   EXTRA_API_GL,
   EXTRA_END
};

/* This is the big table describing all the enums we accept in
 * glGet*v().  The table is partitioned into six parts: enums
 * understood by all GL APIs (OpenGL, GLES and GLES2), enums shared
 * between OpenGL and GLES, enums exclusive to GLES, etc for the
 * remaining combinations. To look up the enums valid in a given API
 * we will use a hash table specific to that API. These tables are in
 * turn generated at build time and included through get_hash.h.
 * The different sections are guarded by #if FEATURE_GL etc to make
 * sure we only compile in the enums we may need. */

#include "get_hash.h"

/* All we need now is a way to look up the value struct from the enum.
 * The code generated by gcc for the old generated big switch
 * statement is a big, balanced, open coded if/else tree, essentially
 * an unrolled binary search.  It would be natural to sort the new
 * enum table and use bsearch(), but we will use a read-only hash
 * table instead.  bsearch() has a nice guaranteed worst case
 * performance, but we're also guaranteed to hit that worst case
 * (log2(n) iterations) for about half the enums.  Instead, using an
 * open addressing hash table, we can find the enum on the first try
 * for 80% of the enums, 1 collision for 10% and never more than 5
 * collisions for any enum (typical numbers).  And the code is very
 * simple, even though it feels a little magic. */

#ifdef GET_DEBUG
static void
print_table_stats(int api)
{
   int i, j, collisions[11], count, hash, mask;
   const struct value_desc *d;
   const char *api_names[] = {
      [API_OPENGL_COMPAT] = "GL",
      [API_OPENGL_CORE] = "GL_CORE",
      [API_OPENGLES] = "GLES",
      [API_OPENGLES2] = "GLES2",
   };
   const char *api_name;

   api_name = api < Elements(api_names) ? api_names[api] : "N/A";
   count = 0;
   mask = Elements(table(api)) - 1;
   memset(collisions, 0, sizeof collisions);

   for (i = 0; i < Elements(table(api)); i++) {
      if (!table(api)[i])
         continue;
      count++;
      d = &values[table(api)[i]];
      hash = (d->pname * prime_factor);
      j = 0;
      while (1) {
         if (values[table(api)[hash & mask]].pname == d->pname)
            break;
         hash += prime_step;
         j++;
      }

      if (j < 10)
         collisions[j]++;
      else
         collisions[10]++;
   }

   printf("number of enums for %s: %d (total %ld)\n",
         api_name, count, Elements(values));
   for (i = 0; i < Elements(collisions) - 1; i++)
      if (collisions[i] > 0)
         printf("  %d enums with %d %scollisions\n",
               collisions[i], i, i == 10 ? "or more " : "");
}
#endif

/**
 * Initialize the enum hash for a given API 
 *
 * This is called from one_time_init() to insert the enum values that
 * are valid for the API in question into the enum hash table.
 *
 * \param the current context, for determining the API in question
 */
void _mesa_init_get_hash(struct gl_context *ctx)
{
#ifdef GET_DEBUG
   print_table_stats();
#endif
}

/**
 * Handle irregular enums
 *
 * Some values don't conform to the "well-known type at context
 * pointer + offset" pattern, so we have this function to catch all
 * the corner cases.  Typically, it's a computed value or a one-off
 * pointer to a custom struct or something.
 *
 * In this case we can't return a pointer to the value, so we'll have
 * to use the temporary variable 'v' declared back in the calling
 * glGet*v() function to store the result.
 *
 * \param ctx the current context
 * \param d the struct value_desc that describes the enum
 * \param v pointer to the tmp declared in the calling glGet*v() function
 */
static void
find_custom_value(struct gl_context *ctx, const struct value_desc *d, union value *v)
{
   struct gl_buffer_object **buffer_obj;
   struct gl_client_array *array;
   GLuint unit, *p;

   switch (d->pname) {
   case GL_MAJOR_VERSION:
      v->value_int = ctx->Version / 10;
      break;
   case GL_MINOR_VERSION:
      v->value_int = ctx->Version % 10;
      break;

   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_1D_ARRAY_EXT:
   case GL_TEXTURE_2D_ARRAY_EXT:
   case GL_TEXTURE_CUBE_MAP_ARB:
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_TEXTURE_EXTERNAL_OES:
      v->value_bool = _mesa_IsEnabled(d->pname);
      break;

   case GL_LINE_STIPPLE_PATTERN:
      /* This is the only GLushort, special case it here by promoting
       * to an int rather than introducing a new type. */
      v->value_int = ctx->Line.StipplePattern;
      break;

   case GL_CURRENT_RASTER_TEXTURE_COORDS:
      unit = ctx->Texture.CurrentUnit;
      v->value_float_4[0] = ctx->Current.RasterTexCoords[unit][0];
      v->value_float_4[1] = ctx->Current.RasterTexCoords[unit][1];
      v->value_float_4[2] = ctx->Current.RasterTexCoords[unit][2];
      v->value_float_4[3] = ctx->Current.RasterTexCoords[unit][3];
      break;

   case GL_CURRENT_TEXTURE_COORDS:
      unit = ctx->Texture.CurrentUnit;
      v->value_float_4[0] = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][0];
      v->value_float_4[1] = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][1];
      v->value_float_4[2] = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][2];
      v->value_float_4[3] = ctx->Current.Attrib[VERT_ATTRIB_TEX0 + unit][3];
      break;

   case GL_COLOR_WRITEMASK:
      v->value_int_4[0] = ctx->Color.ColorMask[0][RCOMP] ? 1 : 0;
      v->value_int_4[1] = ctx->Color.ColorMask[0][GCOMP] ? 1 : 0;
      v->value_int_4[2] = ctx->Color.ColorMask[0][BCOMP] ? 1 : 0;
      v->value_int_4[3] = ctx->Color.ColorMask[0][ACOMP] ? 1 : 0;
      break;

   case GL_EDGE_FLAG:
      v->value_bool = ctx->Current.Attrib[VERT_ATTRIB_EDGEFLAG][0] == 1.0;
      break;

   case GL_READ_BUFFER:
      v->value_enum = ctx->ReadBuffer->ColorReadBuffer;
      break;

   case GL_MAP2_GRID_DOMAIN:
      v->value_float_4[0] = ctx->Eval.MapGrid2u1;
      v->value_float_4[1] = ctx->Eval.MapGrid2u2;
      v->value_float_4[2] = ctx->Eval.MapGrid2v1;
      v->value_float_4[3] = ctx->Eval.MapGrid2v2;
      break;

   case GL_TEXTURE_STACK_DEPTH:
      unit = ctx->Texture.CurrentUnit;
      v->value_int = ctx->TextureMatrixStack[unit].Depth + 1;
      break;
   case GL_TEXTURE_MATRIX:
      unit = ctx->Texture.CurrentUnit;
      v->value_matrix = ctx->TextureMatrixStack[unit].Top;
      break;

   case GL_TEXTURE_COORD_ARRAY:
   case GL_TEXTURE_COORD_ARRAY_SIZE:
   case GL_TEXTURE_COORD_ARRAY_TYPE:
   case GL_TEXTURE_COORD_ARRAY_STRIDE:
      array = &ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_TEX(ctx->Array.ActiveTexture)];
      v->value_int = *(GLuint *) ((char *) array + d->offset);
      break;

   case GL_ACTIVE_TEXTURE_ARB:
      v->value_int = GL_TEXTURE0_ARB + ctx->Texture.CurrentUnit;
      break;
   case GL_CLIENT_ACTIVE_TEXTURE_ARB:
      v->value_int = GL_TEXTURE0_ARB + ctx->Array.ActiveTexture;
      break;

   case GL_MODELVIEW_STACK_DEPTH:
   case GL_PROJECTION_STACK_DEPTH:
      v->value_int = *(GLint *) ((char *) ctx + d->offset) + 1;
      break;

   case GL_MAX_TEXTURE_SIZE:
   case GL_MAX_3D_TEXTURE_SIZE:
   case GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB:
      p = (GLuint *) ((char *) ctx + d->offset);
      v->value_int = 1 << (*p - 1);
      break;

   case GL_SCISSOR_BOX:
      v->value_int_4[0] = ctx->Scissor.X;
      v->value_int_4[1] = ctx->Scissor.Y;
      v->value_int_4[2] = ctx->Scissor.Width;
      v->value_int_4[3] = ctx->Scissor.Height;
      break;

   case GL_LIST_INDEX:
      v->value_int =
	 ctx->ListState.CurrentList ? ctx->ListState.CurrentList->Name : 0;
      break;
   case GL_LIST_MODE:
      if (!ctx->CompileFlag)
	 v->value_enum = 0;
      else if (ctx->ExecuteFlag)
	 v->value_enum = GL_COMPILE_AND_EXECUTE;
      else
	 v->value_enum = GL_COMPILE;
      break;

   case GL_VIEWPORT:
      v->value_int_4[0] = ctx->Viewport.X;
      v->value_int_4[1] = ctx->Viewport.Y;
      v->value_int_4[2] = ctx->Viewport.Width;
      v->value_int_4[3] = ctx->Viewport.Height;
      break;

   case GL_ACTIVE_STENCIL_FACE_EXT:
      v->value_enum = ctx->Stencil.ActiveFace ? GL_BACK : GL_FRONT;
      break;

   case GL_STENCIL_FAIL:
      v->value_enum = ctx->Stencil.FailFunc[ctx->Stencil.ActiveFace];
      break;
   case GL_STENCIL_FUNC:
      v->value_enum = ctx->Stencil.Function[ctx->Stencil.ActiveFace];
      break;
   case GL_STENCIL_PASS_DEPTH_FAIL:
      v->value_enum = ctx->Stencil.ZFailFunc[ctx->Stencil.ActiveFace];
      break;
   case GL_STENCIL_PASS_DEPTH_PASS:
      v->value_enum = ctx->Stencil.ZPassFunc[ctx->Stencil.ActiveFace];
      break;
   case GL_STENCIL_REF:
      v->value_int = ctx->Stencil.Ref[ctx->Stencil.ActiveFace];
      break;
   case GL_STENCIL_VALUE_MASK:
      v->value_int = ctx->Stencil.ValueMask[ctx->Stencil.ActiveFace];
      break;
   case GL_STENCIL_WRITEMASK:
      v->value_int = ctx->Stencil.WriteMask[ctx->Stencil.ActiveFace];
      break;

   case GL_NUM_EXTENSIONS:
      v->value_int = _mesa_get_extension_count(ctx);
      break;

   case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
      v->value_int = _mesa_get_color_read_type(ctx);
      break;
   case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
      v->value_int = _mesa_get_color_read_format(ctx);
      break;

   case GL_CURRENT_MATRIX_STACK_DEPTH_ARB:
      v->value_int = ctx->CurrentStack->Depth + 1;
      break;
   case GL_CURRENT_MATRIX_ARB:
   case GL_TRANSPOSE_CURRENT_MATRIX_ARB:
      v->value_matrix = ctx->CurrentStack->Top;
      break;

   case GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB:
      v->value_int = _mesa_get_compressed_formats(ctx, NULL);
      break;
   case GL_COMPRESSED_TEXTURE_FORMATS_ARB:
      v->value_int_n.n = 
	 _mesa_get_compressed_formats(ctx, v->value_int_n.ints);
      ASSERT(v->value_int_n.n <= 100);
      break;

   case GL_MAX_VARYING_FLOATS_ARB:
      v->value_int = ctx->Const.MaxVarying * 4;
      break;

   /* Various object names */

   case GL_TEXTURE_BINDING_1D:
   case GL_TEXTURE_BINDING_2D:
   case GL_TEXTURE_BINDING_3D:
   case GL_TEXTURE_BINDING_1D_ARRAY_EXT:
   case GL_TEXTURE_BINDING_2D_ARRAY_EXT:
   case GL_TEXTURE_BINDING_CUBE_MAP_ARB:
   case GL_TEXTURE_BINDING_RECTANGLE_NV:
   case GL_TEXTURE_BINDING_EXTERNAL_OES:
   case GL_TEXTURE_BINDING_CUBE_MAP_ARRAY:
   case GL_TEXTURE_BINDING_2D_MULTISAMPLE:
   case GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY:
      unit = ctx->Texture.CurrentUnit;
      v->value_int =
	 ctx->Texture.Unit[unit].CurrentTex[d->offset]->Name;
      break;

   /* GL_ARB_vertex_buffer_object */
   case GL_VERTEX_ARRAY_BUFFER_BINDING_ARB:
   case GL_NORMAL_ARRAY_BUFFER_BINDING_ARB:
   case GL_COLOR_ARRAY_BUFFER_BINDING_ARB:
   case GL_INDEX_ARRAY_BUFFER_BINDING_ARB:
   case GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB:
   case GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB:
   case GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB:
      buffer_obj = (struct gl_buffer_object **)
	 ((char *) ctx->Array.ArrayObj + d->offset);
      v->value_int = (*buffer_obj)->Name;
      break;
   case GL_ARRAY_BUFFER_BINDING_ARB:
      v->value_int = ctx->Array.ArrayBufferObj->Name;
      break;
   case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB:
      v->value_int =
	 ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_TEX(ctx->Array.ActiveTexture)].BufferObj->Name;
      break;
   case GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB:
      v->value_int = ctx->Array.ArrayObj->ElementArrayBufferObj->Name;
      break;

   /* ARB_copy_buffer */
   case GL_COPY_READ_BUFFER:
      v->value_int = ctx->CopyReadBuffer->Name;
      break;
   case GL_COPY_WRITE_BUFFER:
      v->value_int = ctx->CopyWriteBuffer->Name;
      break;

   case GL_PIXEL_PACK_BUFFER_BINDING_EXT:
      v->value_int = ctx->Pack.BufferObj->Name;
      break;
   case GL_PIXEL_UNPACK_BUFFER_BINDING_EXT:
      v->value_int = ctx->Unpack.BufferObj->Name;
      break;
   case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
      v->value_int = ctx->TransformFeedback.CurrentBuffer->Name;
      break;
   case GL_TRANSFORM_FEEDBACK_BUFFER_PAUSED:
      v->value_int = ctx->TransformFeedback.CurrentObject->Paused;
      break;
   case GL_TRANSFORM_FEEDBACK_BUFFER_ACTIVE:
      v->value_int = ctx->TransformFeedback.CurrentObject->Active;
      break;
   case GL_TRANSFORM_FEEDBACK_BINDING:
      v->value_int = ctx->TransformFeedback.CurrentObject->Name;
      break;
   case GL_CURRENT_PROGRAM:
      v->value_int =
	 ctx->Shader.ActiveProgram ? ctx->Shader.ActiveProgram->Name : 0;
      break;
   case GL_READ_FRAMEBUFFER_BINDING_EXT:
      v->value_int = ctx->ReadBuffer->Name;
      break;
   case GL_RENDERBUFFER_BINDING_EXT:
      v->value_int =
	 ctx->CurrentRenderbuffer ? ctx->CurrentRenderbuffer->Name : 0;
      break;
   case GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES:
      v->value_int = ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_POINT_SIZE].BufferObj->Name;
      break;

   case GL_FOG_COLOR:
      if (_mesa_get_clamp_fragment_color(ctx))
         COPY_4FV(v->value_float_4, ctx->Fog.Color);
      else
         COPY_4FV(v->value_float_4, ctx->Fog.ColorUnclamped);
      break;
   case GL_COLOR_CLEAR_VALUE:
      if (_mesa_get_clamp_fragment_color(ctx)) {
         v->value_float_4[0] = CLAMP(ctx->Color.ClearColor.f[0], 0.0F, 1.0F);
         v->value_float_4[1] = CLAMP(ctx->Color.ClearColor.f[1], 0.0F, 1.0F);
         v->value_float_4[2] = CLAMP(ctx->Color.ClearColor.f[2], 0.0F, 1.0F);
         v->value_float_4[3] = CLAMP(ctx->Color.ClearColor.f[3], 0.0F, 1.0F);
      } else
         COPY_4FV(v->value_float_4, ctx->Color.ClearColor.f);
      break;
   case GL_BLEND_COLOR_EXT:
      if (_mesa_get_clamp_fragment_color(ctx))
         COPY_4FV(v->value_float_4, ctx->Color.BlendColor);
      else
         COPY_4FV(v->value_float_4, ctx->Color.BlendColorUnclamped);
      break;
   case GL_ALPHA_TEST_REF:
      if (_mesa_get_clamp_fragment_color(ctx))
         v->value_float = ctx->Color.AlphaRef;
      else
         v->value_float = ctx->Color.AlphaRefUnclamped;
      break;
   case GL_MAX_VERTEX_UNIFORM_VECTORS:
      v->value_int = ctx->Const.VertexProgram.MaxUniformComponents / 4;
      break;

   case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
      v->value_int = ctx->Const.FragmentProgram.MaxUniformComponents / 4;
      break;

   /* GL_ARB_texture_buffer_object */
   case GL_TEXTURE_BUFFER_ARB:
      v->value_int = ctx->Texture.BufferObject->Name;
      break;
   case GL_TEXTURE_BINDING_BUFFER_ARB:
      unit = ctx->Texture.CurrentUnit;
      v->value_int =
         ctx->Texture.Unit[unit].CurrentTex[TEXTURE_BUFFER_INDEX]->Name;
      break;
   case GL_TEXTURE_BUFFER_DATA_STORE_BINDING_ARB:
      {
         struct gl_buffer_object *buf =
            ctx->Texture.Unit[ctx->Texture.CurrentUnit]
            .CurrentTex[TEXTURE_BUFFER_INDEX]->BufferObject;
         v->value_int = buf ? buf->Name : 0;
      }
      break;
   case GL_TEXTURE_BUFFER_FORMAT_ARB:
      v->value_int = ctx->Texture.Unit[ctx->Texture.CurrentUnit]
         .CurrentTex[TEXTURE_BUFFER_INDEX]->BufferObjectFormat;
      break;

   /* GL_ARB_sampler_objects */
   case GL_SAMPLER_BINDING:
      {
         struct gl_sampler_object *samp =
            ctx->Texture.Unit[ctx->Texture.CurrentUnit].Sampler;

         /*
          * The sampler object may have been deleted on another context,
          * so we try to lookup the sampler object before returning its Name.
          */
         if (samp && _mesa_lookup_samplerobj(ctx, samp->Name)) {
            v->value_int = samp->Name;
         } else {
            v->value_int = 0;
         }
      }
      break;
   /* GL_ARB_uniform_buffer_object */
   case GL_UNIFORM_BUFFER_BINDING:
      v->value_int = ctx->UniformBuffer->Name;
      break;
   /* GL_ARB_timer_query */
   case GL_TIMESTAMP:
      if (ctx->Driver.GetTimestamp) {
         v->value_int64 = ctx->Driver.GetTimestamp(ctx);
      }
      else {
         _mesa_problem(ctx, "driver doesn't implement GetTimestamp");
      }
      break;
   }
}

/**
 * Check extra constraints on a struct value_desc descriptor
 *
 * If a struct value_desc has a non-NULL extra pointer, it means that
 * there are a number of extra constraints to check or actions to
 * perform.  The extras is just an integer array where each integer
 * encode different constraints or actions.
 *
 * \param ctx current context
 * \param func name of calling glGet*v() function for error reporting
 * \param d the struct value_desc that has the extra constraints
 *
 * \return GL_FALSE if all of the constraints were not satisfied,
 *     otherwise GL_TRUE.
 */
static GLboolean
check_extra(struct gl_context *ctx, const char *func, const struct value_desc *d)
{
   const GLuint version = ctx->Version;
   int total, enabled;
   const int *e;

   total = 0;
   enabled = 0;
   for (e = d->extra; *e != EXTRA_END; e++)
      switch (*e) {
      case EXTRA_VERSION_30:
	 if (version >= 30) {
	    total++;
	    enabled++;
	 }
	 break;
      case EXTRA_VERSION_31:
	 if (version >= 31) {
	    total++;
	    enabled++;
	 }
	 break;
      case EXTRA_VERSION_32:
	 if (version >= 32) {
	    total++;
	    enabled++;
	 }
	 break;
      case EXTRA_NEW_FRAG_CLAMP:
         if (ctx->NewState & (_NEW_BUFFERS | _NEW_FRAG_CLAMP))
            _mesa_update_state(ctx);
         break;
      case EXTRA_API_ES2:
	 if (ctx->API == API_OPENGLES2) {
	    total++;
	    enabled++;
	 }
	 break;
      case EXTRA_API_ES3:
	 if (_mesa_is_gles3(ctx)) {
	    total++;
	    enabled++;
	 }
	 break;
      case EXTRA_API_GL:
	 if (_mesa_is_desktop_gl(ctx)) {
	    total++;
	    enabled++;
	 }
	 break;
      case EXTRA_API_GL_CORE:
	 if (ctx->API == API_OPENGL_CORE) {
	    total++;
	    enabled++;
	 }
	 break;
      case EXTRA_NEW_BUFFERS:
	 if (ctx->NewState & _NEW_BUFFERS)
	    _mesa_update_state(ctx);
	 break;
      case EXTRA_FLUSH_CURRENT:
	 FLUSH_CURRENT(ctx, 0);
	 break;
      case EXTRA_VALID_DRAW_BUFFER:
	 if (d->pname - GL_DRAW_BUFFER0_ARB >= ctx->Const.MaxDrawBuffers) {
	    _mesa_error(ctx, GL_INVALID_OPERATION, "%s(draw buffer %u)",
			func, d->pname - GL_DRAW_BUFFER0_ARB);
	    return GL_FALSE;
	 }
	 break;
      case EXTRA_VALID_TEXTURE_UNIT:
	 if (ctx->Texture.CurrentUnit >= ctx->Const.MaxTextureCoordUnits) {
	    _mesa_error(ctx, GL_INVALID_OPERATION, "%s(texture %u)",
			func, ctx->Texture.CurrentUnit);
	    return GL_FALSE;
	 }
	 break;
      case EXTRA_VALID_CLIP_DISTANCE:
	 if (d->pname - GL_CLIP_DISTANCE0 >= ctx->Const.MaxClipPlanes) {
	    _mesa_error(ctx, GL_INVALID_ENUM, "%s(clip distance %u)",
			func, d->pname - GL_CLIP_DISTANCE0);
	    return GL_FALSE;
	 }
	 break;
      case EXTRA_GLSL_130:
	 if (ctx->Const.GLSLVersion >= 130) {
	    total++;
	    enabled++;
	 }
	 break;
      case EXTRA_END:
	 break;
      default: /* *e is a offset into the extension struct */
	 total++;
	 if (*(GLboolean *) ((char *) &ctx->Extensions + *e))
	    enabled++;
	 break;
      }

   if (total > 0 && enabled == 0) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(pname=%s)", func,
                  _mesa_lookup_enum_by_nr(d->pname));
      return GL_FALSE;
   }

   return GL_TRUE;
}

static const struct value_desc error_value =
   { 0, 0, TYPE_INVALID, NO_OFFSET, NO_EXTRA };

/**
 * Find the struct value_desc corresponding to the enum 'pname'.
 * 
 * We hash the enum value to get an index into the 'table' array,
 * which holds the index in the 'values' array of struct value_desc.
 * Once we've found the entry, we do the extra checks, if any, then
 * look up the value and return a pointer to it.
 *
 * If the value has to be computed (for example, it's the result of a
 * function call or we need to add 1 to it), we use the tmp 'v' to
 * store the result.
 * 
 * \param func name of glGet*v() func for error reporting
 * \param pname the enum value we're looking up
 * \param p is were we return the pointer to the value
 * \param v a tmp union value variable in the calling glGet*v() function
 *
 * \return the struct value_desc corresponding to the enum or a struct
 *     value_desc of TYPE_INVALID if not found.  This lets the calling
 *     glGet*v() function jump right into a switch statement and
 *     handle errors there instead of having to check for NULL.
 */
static const struct value_desc *
find_value(const char *func, GLenum pname, void **p, union value *v)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *unit;
   int mask, hash;
   const struct value_desc *d;
   int api;

   api = ctx->API;
   /* We index into the table_set[] list of per-API hash tables using the API's
    * value in the gl_api enum. Since GLES 3 doesn't have an API_OPENGL* enum
    * value since it's compatible with GLES2 its entry in table_set[] is at the
    * end.
    */
   STATIC_ASSERT(Elements(table_set) == API_OPENGL_LAST + 2);
   if (_mesa_is_gles3(ctx)) {
      api = API_OPENGL_LAST + 1;
   }
   mask = Elements(table(api)) - 1;
   hash = (pname * prime_factor);
   while (1) {
      int idx = table(api)[hash & mask];

      /* If the enum isn't valid, the hash walk ends with index 0,
       * pointing to the first entry of values[] which doesn't hold
       * any valid enum. */
      if (unlikely(idx == 0)) {
         _mesa_error(ctx, GL_INVALID_ENUM, "%s(pname=%s)", func,
               _mesa_lookup_enum_by_nr(pname));
         return &error_value;
      }

      d = &values[idx];
      if (likely(d->pname == pname))
         break;

      hash += prime_step;
   }

   if (unlikely(d->extra && !check_extra(ctx, func, d)))
      return &error_value;

   switch (d->location) {
   case LOC_BUFFER:
      *p = ((char *) ctx->DrawBuffer + d->offset);
      return d;
   case LOC_CONTEXT:
      *p = ((char *) ctx + d->offset);
      return d;
   case LOC_ARRAY:
      *p = ((char *) ctx->Array.ArrayObj + d->offset);
      return d;
   case LOC_TEXUNIT:
      unit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      *p = ((char *) unit + d->offset);
      return d;
   case LOC_CUSTOM:
      find_custom_value(ctx, d, v);
      *p = v;
      return d;
   default:
      assert(0);
      break;
   }

   /* silence warning */
   return &error_value;
}

static const int transpose[] = {
   0, 4,  8, 12,
   1, 5,  9, 13,
   2, 6, 10, 14,
   3, 7, 11, 15
};

void GLAPIENTRY
_mesa_GetBooleanv(GLenum pname, GLboolean *params)
{
   const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
   void *p;

   d = find_value("glGetBooleanv", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
      break;
   case TYPE_CONST:
      params[0] = INT_TO_BOOLEAN(d->offset);
      break;

   case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
      params[3] = FLOAT_TO_BOOLEAN(((GLfloat *) p)[3]);
   case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
      params[2] = FLOAT_TO_BOOLEAN(((GLfloat *) p)[2]);
   case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
      params[1] = FLOAT_TO_BOOLEAN(((GLfloat *) p)[1]);
   case TYPE_FLOAT:
   case TYPE_FLOATN:
      params[0] = FLOAT_TO_BOOLEAN(((GLfloat *) p)[0]);
      break;

   case TYPE_DOUBLEN:
      params[0] = FLOAT_TO_BOOLEAN(((GLdouble *) p)[0]);
      break;

   case TYPE_INT_4:
      params[3] = INT_TO_BOOLEAN(((GLint *) p)[3]);
   case TYPE_INT_3:
      params[2] = INT_TO_BOOLEAN(((GLint *) p)[2]);
   case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = INT_TO_BOOLEAN(((GLint *) p)[1]);
   case TYPE_INT:
   case TYPE_ENUM:
      params[0] = INT_TO_BOOLEAN(((GLint *) p)[0]);
      break;

   case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
	 params[i] = INT_TO_BOOLEAN(v.value_int_n.ints[i]);
      break;

   case TYPE_INT64:
      params[0] = INT64_TO_BOOLEAN(((GLint64 *) p)[0]);
      break;

   case TYPE_BOOLEAN:
      params[0] = ((GLboolean*) p)[0];
      break;		

   case TYPE_MATRIX:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = FLOAT_TO_BOOLEAN(m->m[i]);
      break;

   case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = FLOAT_TO_BOOLEAN(m->m[transpose[i]]);
      break;

   case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
      params[0] = (*(GLbitfield *) p >> shift) & 1;
      break;
   }
}

void GLAPIENTRY
_mesa_GetFloatv(GLenum pname, GLfloat *params)
{
   const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
   void *p;

   d = find_value("glGetFloatv", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
      break;
   case TYPE_CONST:
      params[0] = (GLfloat) d->offset;
      break;

   case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
      params[3] = ((GLfloat *) p)[3];
   case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
      params[2] = ((GLfloat *) p)[2];
   case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
      params[1] = ((GLfloat *) p)[1];
   case TYPE_FLOAT:
   case TYPE_FLOATN:
      params[0] = ((GLfloat *) p)[0];
      break;

   case TYPE_DOUBLEN:
      params[0] = (GLfloat) (((GLdouble *) p)[0]);
      break;

   case TYPE_INT_4:
      params[3] = (GLfloat) (((GLint *) p)[3]);
   case TYPE_INT_3:
      params[2] = (GLfloat) (((GLint *) p)[2]);
   case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = (GLfloat) (((GLint *) p)[1]);
   case TYPE_INT:
   case TYPE_ENUM:
      params[0] = (GLfloat) (((GLint *) p)[0]);
      break;

   case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
	 params[i] = INT_TO_FLOAT(v.value_int_n.ints[i]);
      break;

   case TYPE_INT64:
      params[0] = (GLfloat) (((GLint64 *) p)[0]);
      break;

   case TYPE_BOOLEAN:
      params[0] = BOOLEAN_TO_FLOAT(*(GLboolean*) p);
      break;		

   case TYPE_MATRIX:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = m->m[i];
      break;

   case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = m->m[transpose[i]];
      break;

   case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
      params[0] = BOOLEAN_TO_FLOAT((*(GLbitfield *) p >> shift) & 1);
      break;
   }
}

void GLAPIENTRY
_mesa_GetIntegerv(GLenum pname, GLint *params)
{
   const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
   void *p;

   d = find_value("glGetIntegerv", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
      break;
   case TYPE_CONST:
      params[0] = d->offset;
      break;

   case TYPE_FLOAT_4:
      params[3] = IROUND(((GLfloat *) p)[3]);
   case TYPE_FLOAT_3:
      params[2] = IROUND(((GLfloat *) p)[2]);
   case TYPE_FLOAT_2:
      params[1] = IROUND(((GLfloat *) p)[1]);
   case TYPE_FLOAT:
      params[0] = IROUND(((GLfloat *) p)[0]);
      break;

   case TYPE_FLOATN_4:
      params[3] = FLOAT_TO_INT(((GLfloat *) p)[3]);
   case TYPE_FLOATN_3:
      params[2] = FLOAT_TO_INT(((GLfloat *) p)[2]);
   case TYPE_FLOATN_2:
      params[1] = FLOAT_TO_INT(((GLfloat *) p)[1]);
   case TYPE_FLOATN:
      params[0] = FLOAT_TO_INT(((GLfloat *) p)[0]);
      break;

   case TYPE_DOUBLEN:
      params[0] = FLOAT_TO_INT(((GLdouble *) p)[0]);
      break;

   case TYPE_INT_4:
      params[3] = ((GLint *) p)[3];
   case TYPE_INT_3:
      params[2] = ((GLint *) p)[2];
   case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = ((GLint *) p)[1];
   case TYPE_INT:
   case TYPE_ENUM:
      params[0] = ((GLint *) p)[0];
      break;

   case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
	 params[i] = v.value_int_n.ints[i];
      break;

   case TYPE_INT64:
      params[0] = INT64_TO_INT(((GLint64 *) p)[0]);
      break;

   case TYPE_BOOLEAN:
      params[0] = BOOLEAN_TO_INT(*(GLboolean*) p);
      break;		

   case TYPE_MATRIX:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = FLOAT_TO_INT(m->m[i]);
      break;

   case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = FLOAT_TO_INT(m->m[transpose[i]]);
      break;

   case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
      params[0] = (*(GLbitfield *) p >> shift) & 1;
      break;
   }
}

void GLAPIENTRY
_mesa_GetInteger64v(GLenum pname, GLint64 *params)
{
   const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
   void *p;

   d = find_value("glGetInteger64v", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
      break;
   case TYPE_CONST:
      params[0] = d->offset;
      break;

   case TYPE_FLOAT_4:
      params[3] = IROUND64(((GLfloat *) p)[3]);
   case TYPE_FLOAT_3:
      params[2] = IROUND64(((GLfloat *) p)[2]);
   case TYPE_FLOAT_2:
      params[1] = IROUND64(((GLfloat *) p)[1]);
   case TYPE_FLOAT:
      params[0] = IROUND64(((GLfloat *) p)[0]);
      break;

   case TYPE_FLOATN_4:
      params[3] = FLOAT_TO_INT64(((GLfloat *) p)[3]);
   case TYPE_FLOATN_3:
      params[2] = FLOAT_TO_INT64(((GLfloat *) p)[2]);
   case TYPE_FLOATN_2:
      params[1] = FLOAT_TO_INT64(((GLfloat *) p)[1]);
   case TYPE_FLOATN:
      params[0] = FLOAT_TO_INT64(((GLfloat *) p)[0]);
      break;

   case TYPE_DOUBLEN:
      params[0] = FLOAT_TO_INT64(((GLdouble *) p)[0]);
      break;

   case TYPE_INT_4:
      params[3] = ((GLint *) p)[3];
   case TYPE_INT_3:
      params[2] = ((GLint *) p)[2];
   case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = ((GLint *) p)[1];
   case TYPE_INT:
   case TYPE_ENUM:
      params[0] = ((GLint *) p)[0];
      break;

   case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
	 params[i] = INT_TO_BOOLEAN(v.value_int_n.ints[i]);
      break;

   case TYPE_INT64:
      params[0] = ((GLint64 *) p)[0];
      break;

   case TYPE_BOOLEAN:
      params[0] = ((GLboolean*) p)[0];
      break;		

   case TYPE_MATRIX:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = FLOAT_TO_INT64(m->m[i]);
      break;

   case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = FLOAT_TO_INT64(m->m[transpose[i]]);
      break;

   case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
      params[0] = (*(GLbitfield *) p >> shift) & 1;
      break;
   }
}

void GLAPIENTRY
_mesa_GetDoublev(GLenum pname, GLdouble *params)
{
   const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
   void *p;

   d = find_value("glGetDoublev", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
      break;
   case TYPE_CONST:
      params[0] = d->offset;
      break;

   case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
      params[3] = ((GLfloat *) p)[3];
   case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
      params[2] = ((GLfloat *) p)[2];
   case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
      params[1] = ((GLfloat *) p)[1];
   case TYPE_FLOAT:
   case TYPE_FLOATN:
      params[0] = ((GLfloat *) p)[0];
      break;

   case TYPE_DOUBLEN:
      params[0] = ((GLdouble *) p)[0];
      break;

   case TYPE_INT_4:
      params[3] = ((GLint *) p)[3];
   case TYPE_INT_3:
      params[2] = ((GLint *) p)[2];
   case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = ((GLint *) p)[1];
   case TYPE_INT:
   case TYPE_ENUM:
      params[0] = ((GLint *) p)[0];
      break;

   case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
	 params[i] = v.value_int_n.ints[i];
      break;

   case TYPE_INT64:
      params[0] = (GLdouble) (((GLint64 *) p)[0]);
      break;

   case TYPE_BOOLEAN:
      params[0] = *(GLboolean*) p;
      break;		

   case TYPE_MATRIX:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = m->m[i];
      break;

   case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = m->m[transpose[i]];
      break;

   case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
      params[0] = (*(GLbitfield *) p >> shift) & 1;
      break;
   }
}

static enum value_type
find_value_indexed(const char *func, GLenum pname, GLuint index, union value *v)
{
   GET_CURRENT_CONTEXT(ctx);

   switch (pname) {

   case GL_BLEND:
      if (index >= ctx->Const.MaxDrawBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.EXT_draw_buffers2)
	 goto invalid_enum;
      v->value_int = (ctx->Color.BlendEnabled >> index) & 1;
      return TYPE_INT;

   case GL_BLEND_SRC:
      /* fall-through */
   case GL_BLEND_SRC_RGB:
      if (index >= ctx->Const.MaxDrawBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.ARB_draw_buffers_blend)
	 goto invalid_enum;
      v->value_int = ctx->Color.Blend[index].SrcRGB;
      return TYPE_INT;
   case GL_BLEND_SRC_ALPHA:
      if (index >= ctx->Const.MaxDrawBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.ARB_draw_buffers_blend)
	 goto invalid_enum;
      v->value_int = ctx->Color.Blend[index].SrcA;
      return TYPE_INT;
   case GL_BLEND_DST:
      /* fall-through */
   case GL_BLEND_DST_RGB:
      if (index >= ctx->Const.MaxDrawBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.ARB_draw_buffers_blend)
	 goto invalid_enum;
      v->value_int = ctx->Color.Blend[index].DstRGB;
      return TYPE_INT;
   case GL_BLEND_DST_ALPHA:
      if (index >= ctx->Const.MaxDrawBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.ARB_draw_buffers_blend)
	 goto invalid_enum;
      v->value_int = ctx->Color.Blend[index].DstA;
      return TYPE_INT;
   case GL_BLEND_EQUATION_RGB:
      if (index >= ctx->Const.MaxDrawBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.ARB_draw_buffers_blend)
	 goto invalid_enum;
      v->value_int = ctx->Color.Blend[index].EquationRGB;
      return TYPE_INT;
   case GL_BLEND_EQUATION_ALPHA:
      if (index >= ctx->Const.MaxDrawBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.ARB_draw_buffers_blend)
	 goto invalid_enum;
      v->value_int = ctx->Color.Blend[index].EquationA;
      return TYPE_INT;

   case GL_COLOR_WRITEMASK:
      if (index >= ctx->Const.MaxDrawBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.EXT_draw_buffers2)
	 goto invalid_enum;
      v->value_int_4[0] = ctx->Color.ColorMask[index][RCOMP] ? 1 : 0;
      v->value_int_4[1] = ctx->Color.ColorMask[index][GCOMP] ? 1 : 0;
      v->value_int_4[2] = ctx->Color.ColorMask[index][BCOMP] ? 1 : 0;
      v->value_int_4[3] = ctx->Color.ColorMask[index][ACOMP] ? 1 : 0;
      return TYPE_INT_4;

   case GL_TRANSFORM_FEEDBACK_BUFFER_START:
      if (index >= ctx->Const.MaxTransformFeedbackBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.EXT_transform_feedback)
	 goto invalid_enum;
      v->value_int64 = ctx->TransformFeedback.CurrentObject->Offset[index];
      return TYPE_INT64;

   case GL_TRANSFORM_FEEDBACK_BUFFER_SIZE:
      if (index >= ctx->Const.MaxTransformFeedbackBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.EXT_transform_feedback)
	 goto invalid_enum;
      v->value_int64
         = ctx->TransformFeedback.CurrentObject->RequestedSize[index];
      return TYPE_INT64;

   case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
      if (index >= ctx->Const.MaxTransformFeedbackBuffers)
	 goto invalid_value;
      if (!ctx->Extensions.EXT_transform_feedback)
	 goto invalid_enum;
      v->value_int = ctx->TransformFeedback.CurrentObject->BufferNames[index];
      return TYPE_INT;

   case GL_UNIFORM_BUFFER_BINDING:
      if (index >= ctx->Const.MaxUniformBufferBindings)
	 goto invalid_value;
      if (!ctx->Extensions.ARB_uniform_buffer_object)
	 goto invalid_enum;
      v->value_int = ctx->UniformBufferBindings[index].BufferObject->Name;
      return TYPE_INT;

   case GL_UNIFORM_BUFFER_START:
      if (index >= ctx->Const.MaxUniformBufferBindings)
	 goto invalid_value;
      if (!ctx->Extensions.ARB_uniform_buffer_object)
	 goto invalid_enum;
      v->value_int = ctx->UniformBufferBindings[index].Offset;
      return TYPE_INT;

   case GL_UNIFORM_BUFFER_SIZE:
      if (index >= ctx->Const.MaxUniformBufferBindings)
	 goto invalid_value;
      if (!ctx->Extensions.ARB_uniform_buffer_object)
	 goto invalid_enum;
      v->value_int = ctx->UniformBufferBindings[index].Size;
      return TYPE_INT;

   /* ARB_texture_multisample / GL3.2 */
   case GL_SAMPLE_MASK_VALUE:
      if (index != 0)
         goto invalid_value;
      if (!ctx->Extensions.ARB_texture_multisample)
         goto invalid_enum;
      v->value_int = ctx->Multisample.SampleMaskValue;
      return TYPE_INT;
   }

 invalid_enum:
   _mesa_error(ctx, GL_INVALID_ENUM, "%s(pname=%s)", func,
               _mesa_lookup_enum_by_nr(pname));
   return TYPE_INVALID;
 invalid_value:
   _mesa_error(ctx, GL_INVALID_VALUE, "%s(pname=%s)", func,
               _mesa_lookup_enum_by_nr(pname));
   return TYPE_INVALID;
}

void GLAPIENTRY
_mesa_GetBooleani_v( GLenum pname, GLuint index, GLboolean *params )
{
   union value v;
   enum value_type type =
      find_value_indexed("glGetBooleani_v", pname, index, &v);

   switch (type) {
   case TYPE_INT:
      params[0] = INT_TO_BOOLEAN(v.value_int);
      break;
   case TYPE_INT_4:
      params[0] = INT_TO_BOOLEAN(v.value_int_4[0]);
      params[1] = INT_TO_BOOLEAN(v.value_int_4[1]);
      params[2] = INT_TO_BOOLEAN(v.value_int_4[2]);
      params[3] = INT_TO_BOOLEAN(v.value_int_4[3]);
      break;
   case TYPE_INT64:
      params[0] = INT64_TO_BOOLEAN(v.value_int);
      break;
   default:
      ; /* nothing - GL error was recorded */
   }
}

void GLAPIENTRY
_mesa_GetIntegeri_v( GLenum pname, GLuint index, GLint *params )
{
   union value v;
   enum value_type type =
      find_value_indexed("glGetIntegeri_v", pname, index, &v);

   switch (type) {
   case TYPE_INT:
      params[0] = v.value_int;
      break;
   case TYPE_INT_4:
      params[0] = v.value_int_4[0];
      params[1] = v.value_int_4[1];
      params[2] = v.value_int_4[2];
      params[3] = v.value_int_4[3];
      break;
   case TYPE_INT64:
      params[0] = INT64_TO_INT(v.value_int);
      break;
   default:
      ; /* nothing - GL error was recorded */
   }
}

void GLAPIENTRY
_mesa_GetInteger64i_v( GLenum pname, GLuint index, GLint64 *params )
{
   union value v;
   enum value_type type =
      find_value_indexed("glGetInteger64i_v", pname, index, &v);

   switch (type) {
   case TYPE_INT:
      params[0] = v.value_int;
      break;
   case TYPE_INT_4:
      params[0] = v.value_int_4[0];
      params[1] = v.value_int_4[1];
      params[2] = v.value_int_4[2];
      params[3] = v.value_int_4[3];
      break;
   case TYPE_INT64:
      params[0] = v.value_int;
      break;
   default:
      ; /* nothing - GL error was recorded */
   }
}

void GLAPIENTRY
_mesa_GetFixedv(GLenum pname, GLfixed *params)
{
   const struct value_desc *d;
   union value v;
   GLmatrix *m;
   int shift, i;
   void *p;

   d = find_value("glGetDoublev", pname, &p, &v);
   switch (d->type) {
   case TYPE_INVALID:
      break;
   case TYPE_CONST:
      params[0] = INT_TO_FIXED(d->offset);
      break;

   case TYPE_FLOAT_4:
   case TYPE_FLOATN_4:
      params[3] = FLOAT_TO_FIXED(((GLfloat *) p)[3]);
   case TYPE_FLOAT_3:
   case TYPE_FLOATN_3:
      params[2] = FLOAT_TO_FIXED(((GLfloat *) p)[2]);
   case TYPE_FLOAT_2:
   case TYPE_FLOATN_2:
      params[1] = FLOAT_TO_FIXED(((GLfloat *) p)[1]);
   case TYPE_FLOAT:
   case TYPE_FLOATN:
      params[0] = FLOAT_TO_FIXED(((GLfloat *) p)[0]);
      break;

   case TYPE_DOUBLEN:
      params[0] = FLOAT_TO_FIXED(((GLdouble *) p)[0]);
      break;

   case TYPE_INT_4:
      params[3] = INT_TO_FIXED(((GLint *) p)[3]);
   case TYPE_INT_3:
      params[2] = INT_TO_FIXED(((GLint *) p)[2]);
   case TYPE_INT_2:
   case TYPE_ENUM_2:
      params[1] = INT_TO_FIXED(((GLint *) p)[1]);
   case TYPE_INT:
   case TYPE_ENUM:
      params[0] = INT_TO_FIXED(((GLint *) p)[0]);
      break;

   case TYPE_INT_N:
      for (i = 0; i < v.value_int_n.n; i++)
	 params[i] = INT_TO_FIXED(v.value_int_n.ints[i]);
      break;

   case TYPE_INT64:
      params[0] = ((GLint64 *) p)[0];
      break;

   case TYPE_BOOLEAN:
      params[0] = BOOLEAN_TO_FIXED(((GLboolean*) p)[0]);
      break;		

   case TYPE_MATRIX:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = FLOAT_TO_FIXED(m->m[i]);
      break;

   case TYPE_MATRIX_T:
      m = *(GLmatrix **) p;
      for (i = 0; i < 16; i++)
	 params[i] = FLOAT_TO_FIXED(m->m[transpose[i]]);
      break;

   case TYPE_BIT_0:
   case TYPE_BIT_1:
   case TYPE_BIT_2:
   case TYPE_BIT_3:
   case TYPE_BIT_4:
   case TYPE_BIT_5:
   case TYPE_BIT_6:
   case TYPE_BIT_7:
      shift = d->type - TYPE_BIT_0;
      params[0] = BOOLEAN_TO_FIXED((*(GLbitfield *) p >> shift) & 1);
      break;
   }
}
