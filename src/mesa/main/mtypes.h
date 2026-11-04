/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file mtypes.h
 * Main Mesa data structures.
 *
 * Please try to mark derived values with a leading underscore ('_').
 */

/* glsl-optimizer change: Keep compiler/program structs, limits and extensions; trim rendering
 * state, dispatch/VBO/threaded-rendering machinery and unused runtime mutexes.
 * gl_context contains offline compiler state only. Omit shader-cache hashes,
 * fallback source, driver blobs, binary-restore uniform copies and shared
 * include state. Active shared compiler caches are locked in their C++ owners.
 */

#ifndef MTYPES_H
#define MTYPES_H

#include <stdint.h>             /* uint32_t */
#include <stdbool.h>

#include "main/glheader.h"
#include "main/menums.h"
#include "main/config.h"
#include "compiler/shader_enums.h"
#include "compiler/shader_info.h"
#include "main/formats.h"       /* MESA_FORMAT_COUNT */
#include "compiler/glsl/list.h"
#include "util/u_dynarray.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GET_COLORMASK_BIT(mask, buf, chan) (((mask) >> (4 * (buf) + (chan))) & 0x1)
#define GET_COLORMASK(mask, buf) (((mask) >> (4 * (buf))) & 0xf)

/**
 * \name Some forward type declarations
 */
/*@{*/
struct _mesa_HashTable;
struct gl_attrib_node;
struct gl_list_extensions;
struct gl_meta_state;
struct gl_texture_object;
struct gl_debug_state;
struct gl_context;
struct st_context;
struct gl_uniform_storage;
struct prog_instruction;
struct gl_program_parameter_list;
struct gl_shader_spirv_data;
struct set;
struct shader_includes;
/*@}*/

/** Extra draw modes beyond GL_POINTS, GL_TRIANGLE_FAN, etc */
#define PRIM_MAX                 GL_PATCHES
#define PRIM_OUTSIDE_BEGIN_END   (PRIM_MAX + 1)
#define PRIM_UNKNOWN             (PRIM_MAX + 2)

/**
 * Determine if the given gl_varying_slot appears in the fragment shader.
 */
static inline GLboolean
_mesa_varying_slot_in_fs(gl_varying_slot slot)
{
   switch (slot) {
   case VARYING_SLOT_PSIZ:
   case VARYING_SLOT_BFC0:
   case VARYING_SLOT_BFC1:
   case VARYING_SLOT_EDGE:
   case VARYING_SLOT_CLIP_VERTEX:
   case VARYING_SLOT_LAYER:
   case VARYING_SLOT_TESS_LEVEL_OUTER:
   case VARYING_SLOT_TESS_LEVEL_INNER:
   case VARYING_SLOT_BOUNDING_BOX0:
   case VARYING_SLOT_BOUNDING_BOX1:
   case VARYING_SLOT_VIEWPORT_MASK:
      return GL_FALSE;
   default:
      return GL_TRUE;
   }
}

/**
 * Bit flags for all renderbuffers
 */
#define BUFFER_BIT_FRONT_LEFT   (1 << BUFFER_FRONT_LEFT)
#define BUFFER_BIT_BACK_LEFT    (1 << BUFFER_BACK_LEFT)
#define BUFFER_BIT_FRONT_RIGHT  (1 << BUFFER_FRONT_RIGHT)
#define BUFFER_BIT_BACK_RIGHT   (1 << BUFFER_BACK_RIGHT)
#define BUFFER_BIT_AUX0         (1 << BUFFER_AUX0)
#define BUFFER_BIT_AUX1         (1 << BUFFER_AUX1)
#define BUFFER_BIT_AUX2         (1 << BUFFER_AUX2)
#define BUFFER_BIT_AUX3         (1 << BUFFER_AUX3)
#define BUFFER_BIT_DEPTH        (1 << BUFFER_DEPTH)
#define BUFFER_BIT_STENCIL      (1 << BUFFER_STENCIL)
#define BUFFER_BIT_ACCUM        (1 << BUFFER_ACCUM)
#define BUFFER_BIT_COLOR0       (1 << BUFFER_COLOR0)
#define BUFFER_BIT_COLOR1       (1 << BUFFER_COLOR1)
#define BUFFER_BIT_COLOR2       (1 << BUFFER_COLOR2)
#define BUFFER_BIT_COLOR3       (1 << BUFFER_COLOR3)
#define BUFFER_BIT_COLOR4       (1 << BUFFER_COLOR4)
#define BUFFER_BIT_COLOR5       (1 << BUFFER_COLOR5)
#define BUFFER_BIT_COLOR6       (1 << BUFFER_COLOR6)
#define BUFFER_BIT_COLOR7       (1 << BUFFER_COLOR7)

/**
 * Mask of all the color buffer bits (but not accum).
 */
#define BUFFER_BITS_COLOR  (BUFFER_BIT_FRONT_LEFT | \
                            BUFFER_BIT_BACK_LEFT | \
                            BUFFER_BIT_FRONT_RIGHT | \
                            BUFFER_BIT_BACK_RIGHT | \
                            BUFFER_BIT_AUX0 | \
                            BUFFER_BIT_COLOR0 | \
                            BUFFER_BIT_COLOR1 | \
                            BUFFER_BIT_COLOR2 | \
                            BUFFER_BIT_COLOR3 | \
                            BUFFER_BIT_COLOR4 | \
                            BUFFER_BIT_COLOR5 | \
                            BUFFER_BIT_COLOR6 | \
                            BUFFER_BIT_COLOR7)

/* Mask of bits for depth+stencil buffers */
#define BUFFER_BITS_DEPTH_STENCIL (BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL)

/**
 * \name Bit flags used for updating material values.
 */
/*@{*/
#define MAT_ATTRIB_FRONT_AMBIENT           0
#define MAT_ATTRIB_BACK_AMBIENT            1
#define MAT_ATTRIB_FRONT_DIFFUSE           2
#define MAT_ATTRIB_BACK_DIFFUSE            3
#define MAT_ATTRIB_FRONT_SPECULAR          4
#define MAT_ATTRIB_BACK_SPECULAR           5
#define MAT_ATTRIB_FRONT_EMISSION          6
#define MAT_ATTRIB_BACK_EMISSION           7
#define MAT_ATTRIB_FRONT_SHININESS         8
#define MAT_ATTRIB_BACK_SHININESS          9
#define MAT_ATTRIB_FRONT_INDEXES           10
#define MAT_ATTRIB_BACK_INDEXES            11
#define MAT_ATTRIB_MAX                     12

#define MAT_ATTRIB_AMBIENT(f)  (MAT_ATTRIB_FRONT_AMBIENT+(f))
#define MAT_ATTRIB_DIFFUSE(f)  (MAT_ATTRIB_FRONT_DIFFUSE+(f))
#define MAT_ATTRIB_SPECULAR(f) (MAT_ATTRIB_FRONT_SPECULAR+(f))
#define MAT_ATTRIB_EMISSION(f) (MAT_ATTRIB_FRONT_EMISSION+(f))
#define MAT_ATTRIB_SHININESS(f)(MAT_ATTRIB_FRONT_SHININESS+(f))
#define MAT_ATTRIB_INDEXES(f)  (MAT_ATTRIB_FRONT_INDEXES+(f))

#define MAT_BIT_FRONT_AMBIENT         (1<<MAT_ATTRIB_FRONT_AMBIENT)
#define MAT_BIT_BACK_AMBIENT          (1<<MAT_ATTRIB_BACK_AMBIENT)
#define MAT_BIT_FRONT_DIFFUSE         (1<<MAT_ATTRIB_FRONT_DIFFUSE)
#define MAT_BIT_BACK_DIFFUSE          (1<<MAT_ATTRIB_BACK_DIFFUSE)
#define MAT_BIT_FRONT_SPECULAR        (1<<MAT_ATTRIB_FRONT_SPECULAR)
#define MAT_BIT_BACK_SPECULAR         (1<<MAT_ATTRIB_BACK_SPECULAR)
#define MAT_BIT_FRONT_EMISSION        (1<<MAT_ATTRIB_FRONT_EMISSION)
#define MAT_BIT_BACK_EMISSION         (1<<MAT_ATTRIB_BACK_EMISSION)
#define MAT_BIT_FRONT_SHININESS       (1<<MAT_ATTRIB_FRONT_SHININESS)
#define MAT_BIT_BACK_SHININESS        (1<<MAT_ATTRIB_BACK_SHININESS)
#define MAT_BIT_FRONT_INDEXES         (1<<MAT_ATTRIB_FRONT_INDEXES)
#define MAT_BIT_BACK_INDEXES          (1<<MAT_ATTRIB_BACK_INDEXES)

#define FRONT_MATERIAL_BITS   (MAT_BIT_FRONT_EMISSION | \
                               MAT_BIT_FRONT_AMBIENT | \
                               MAT_BIT_FRONT_DIFFUSE | \
                               MAT_BIT_FRONT_SPECULAR | \
                               MAT_BIT_FRONT_SHININESS | \
                               MAT_BIT_FRONT_INDEXES)

#define BACK_MATERIAL_BITS    (MAT_BIT_BACK_EMISSION | \
                               MAT_BIT_BACK_AMBIENT | \
                               MAT_BIT_BACK_DIFFUSE | \
                               MAT_BIT_BACK_SPECULAR | \
                               MAT_BIT_BACK_SHININESS | \
                               MAT_BIT_BACK_INDEXES)

#define ALL_MATERIAL_BITS     (FRONT_MATERIAL_BITS | BACK_MATERIAL_BITS)
/*@}*/

/**
 * Light state flags.
 */
/*@{*/
#define LIGHT_SPOT         0x1
#define LIGHT_LOCAL_VIEWER 0x2
#define LIGHT_POSITIONAL   0x4
#define LIGHT_NEED_VERTICES (LIGHT_POSITIONAL|LIGHT_LOCAL_VIEWER)
/*@}*/

/**
 * Used for storing clear color, texture border color, etc.
 * The float values are typically unclamped.
 */
union gl_color_union
{
   GLfloat f[4];
   GLint i[4];
   GLuint ui[4];
};

/**
 * Compressed fog mode.
 */
enum gl_fog_mode
{
   FOG_NONE,
   FOG_LINEAR,
   FOG_EXP,
   FOG_EXP2,
};

/**
 * Bit flags for each type of texture object
 */
/*@{*/
#define TEXTURE_2D_MULTISAMPLE_BIT (1 << TEXTURE_2D_MULTISAMPLE_INDEX)
#define TEXTURE_2D_MULTISAMPLE_ARRAY_BIT (1 << TEXTURE_2D_MULTISAMPLE_ARRAY_INDEX)
#define TEXTURE_CUBE_ARRAY_BIT (1 << TEXTURE_CUBE_ARRAY_INDEX)
#define TEXTURE_BUFFER_BIT   (1 << TEXTURE_BUFFER_INDEX)
#define TEXTURE_2D_ARRAY_BIT (1 << TEXTURE_2D_ARRAY_INDEX)
#define TEXTURE_1D_ARRAY_BIT (1 << TEXTURE_1D_ARRAY_INDEX)
#define TEXTURE_EXTERNAL_BIT (1 << TEXTURE_EXTERNAL_INDEX)
#define TEXTURE_CUBE_BIT     (1 << TEXTURE_CUBE_INDEX)
#define TEXTURE_3D_BIT       (1 << TEXTURE_3D_INDEX)
#define TEXTURE_RECT_BIT     (1 << TEXTURE_RECT_INDEX)
#define TEXTURE_2D_BIT       (1 << TEXTURE_2D_INDEX)
#define TEXTURE_1D_BIT       (1 << TEXTURE_1D_INDEX)
/*@}*/

/**
 * Texture image state.  Drivers will typically create a subclass of this
 * with extra fields for memory buffers, etc.
 */
struct gl_texture_image
{
   GLint InternalFormat;	/**< Internal format as given by the user */
   GLenum16 _BaseFormat;	/**< Either GL_RGB, GL_RGBA, GL_ALPHA,
                                 *   GL_LUMINANCE, GL_LUMINANCE_ALPHA,
                                 *   GL_INTENSITY, GL_DEPTH_COMPONENT or
                                 *   GL_DEPTH_STENCIL_EXT only. Used for
                                 *   choosing TexEnv arithmetic.
                                 */
   mesa_format TexFormat;         /**< The actual texture memory format */

   GLuint Border;		/**< 0 or 1 */
   GLuint Width;		/**< = 2^WidthLog2 + 2*Border */
   GLuint Height;		/**< = 2^HeightLog2 + 2*Border */
   GLuint Depth;		/**< = 2^DepthLog2 + 2*Border */
   GLuint Width2;		/**< = Width - 2*Border */
   GLuint Height2;		/**< = Height - 2*Border */
   GLuint Depth2;		/**< = Depth - 2*Border */
   GLuint WidthLog2;		/**< = log2(Width2) */
   GLuint HeightLog2;		/**< = log2(Height2) */
   GLuint DepthLog2;		/**< = log2(Depth2) */
   GLuint MaxNumLevels;		/**< = maximum possible number of mipmap
                                       levels, computed from the dimensions */

   struct gl_texture_object *TexObject;  /**< Pointer back to parent object */
   GLuint Level;                /**< Which mipmap level am I? */
   /** Cube map face: index into gl_texture_object::Image[] array */
   GLuint Face;

   /** GL_ARB_texture_multisample */
   GLuint NumSamples;            /**< Sample count, or 0 for non-multisample */
   GLboolean FixedSampleLocations; /**< Same sample locations for all pixels? */
};

/**
 * Indexes for cube map faces.
 */
typedef enum
{
   FACE_POS_X = 0,
   FACE_NEG_X = 1,
   FACE_POS_Y = 2,
   FACE_NEG_Y = 3,
   FACE_POS_Z = 4,
   FACE_NEG_Z = 5,
   MAX_FACES = 6
} gl_face_index;

/**
 * Sampler object state.  These objects are new with GL_ARB_sampler_objects
 * and OpenGL 3.3.  Legacy texture objects also contain a sampler object.
 */
struct gl_sampler_object
{
   GLuint Name;
   GLchar *Label;               /**< GL_KHR_debug */
   GLint RefCount;

   GLenum16 WrapS;		/**< S-axis texture image wrap mode */
   GLenum16 WrapT;		/**< T-axis texture image wrap mode */
   GLenum16 WrapR;		/**< R-axis texture image wrap mode */
   GLenum16 MinFilter;		/**< minification filter */
   GLenum16 MagFilter;		/**< magnification filter */
   GLenum16 sRGBDecode;         /**< GL_DECODE_EXT or GL_SKIP_DECODE_EXT */
   union gl_color_union BorderColor;  /**< Interpreted according to texture format */
   GLfloat MinLod;		/**< min lambda, OpenGL 1.2 */
   GLfloat MaxLod;		/**< max lambda, OpenGL 1.2 */
   GLfloat LodBias;		/**< OpenGL 1.4 */
   GLfloat MaxAnisotropy;	/**< GL_EXT_texture_filter_anisotropic */
   GLenum16 CompareMode;		/**< GL_ARB_shadow */
   GLenum16 CompareFunc;		/**< GL_ARB_shadow */
   GLboolean CubeMapSeamless;   /**< GL_AMD_seamless_cubemap_per_texture */

   /** GL_ARB_bindless_texture */
   bool HandleAllocated;
   struct util_dynarray Handles;
};

/**
 * Texture object state.  Contains the array of mipmap images, border color,
 * wrap modes, filter modes, and shadow/texcompare state.
 */
struct gl_texture_object
{
   GLint RefCount;             /**< reference count */
   GLuint Name;                /**< the user-visible texture object ID */
   GLenum16 Target;            /**< GL_TEXTURE_1D, GL_TEXTURE_2D, etc. */
   GLenum16 DepthMode;         /**< GL_ARB_depth_texture */
   GLchar *Label;              /**< GL_KHR_debug */

   struct gl_sampler_object Sampler;

   gl_texture_index TargetIndex; /**< The gl_texture_unit::CurrentTex index.
                                      Only valid when Target is valid. */
   GLfloat Priority;           /**< in [0,1] */
   GLint MaxLevel;           /**< max mipmap level (max=1000), OpenGL 1.2 */
   GLint BaseLevel;           /**< min mipmap level, OpenGL 1.2 */
   GLbyte _MaxLevel;           /**< actual max mipmap level (q in the spec) */
   GLfloat _MaxLambda;         /**< = _MaxLevel - BaseLevel (q - p in spec) */
   GLint CropRect[4];          /**< GL_OES_draw_texture */
   GLenum Swizzle[4];          /**< GL_EXT_texture_swizzle */
   GLushort _Swizzle;          /**< same as Swizzle, but SWIZZLE_* format */
   GLbyte ImmutableLevels;     /**< ES 3.0 / ARB_texture_view */
   GLboolean GenerateMipmap;   /**< GL_SGIS_generate_mipmap */
   GLboolean _BaseComplete;    /**< Is the base texture level valid? */
   GLboolean _MipmapComplete;  /**< Is the whole mipmap valid? */
   GLboolean _IsIntegerFormat; /**< Does the texture store integer values? */
   GLboolean _RenderToTexture; /**< Any rendering to this texture? */
   GLboolean Purgeable;        /**< Is the buffer purgeable under memory
                                    pressure? */
   GLboolean Immutable;        /**< GL_ARB_texture_storage */
   GLboolean _IsFloat;         /**< GL_OES_float_texture */
   GLboolean _IsHalfFloat;     /**< GL_OES_half_float_texture */
   bool StencilSampling;       /**< Should we sample stencil instead of depth? */
   bool HandleAllocated;       /**< GL_ARB_bindless_texture */

   /** GL_OES_EGL_image_external */
   GLubyte RequiredTextureImageUnits;

   GLubyte MinLevel;            /**< GL_ARB_texture_view */
   GLubyte NumLevels;           /**< GL_ARB_texture_view */
   GLushort MinLayer;            /**< GL_ARB_texture_view */
   GLushort NumLayers;           /**< GL_ARB_texture_view */

   /** GL_EXT_memory_object */
   GLenum16 TextureTiling;

   /** GL_ARB_shader_image_load_store */
   GLenum16 ImageFormatCompatibilityType;

   /** GL_ARB_texture_buffer_object */
   GLenum16 BufferObjectFormat;
   /** Equivalent Mesa format for BufferObjectFormat. */
   mesa_format _BufferObjectFormat;
   struct gl_buffer_object *BufferObject;

   /** GL_ARB_texture_buffer_range */
   GLintptr BufferOffset;
   GLsizeiptr BufferSize; /**< if this is -1, use BufferObject->Size instead */

   /** Actual texture images, indexed by [cube face] and [mipmap level] */
   struct gl_texture_image *Image[MAX_FACES][MAX_TEXTURE_LEVELS];

   /** GL_ARB_bindless_texture */
   struct util_dynarray SamplerHandles;
   struct util_dynarray ImageHandles;
};

/** Up to four combiner sources are possible with GL_NV_texture_env_combine4 */
#define MAX_COMBINER_TERMS 4

/** Compressed TexEnv effective Combine mode */
enum gl_tex_env_mode
{
   TEXENV_MODE_REPLACE,                 /* r = a0 */
   TEXENV_MODE_MODULATE,                /* r = a0 * a1 */
   TEXENV_MODE_ADD,                     /* r = a0 + a1 */
   TEXENV_MODE_ADD_SIGNED,              /* r = a0 + a1 - 0.5 */
   TEXENV_MODE_INTERPOLATE,             /* r = a0 * a2 + a1 * (1 - a2) */
   TEXENV_MODE_SUBTRACT,                /* r = a0 - a1 */
   TEXENV_MODE_DOT3_RGB,                /* r = a0 . a1 */
   TEXENV_MODE_DOT3_RGB_EXT,            /* r = a0 . a1 */
   TEXENV_MODE_DOT3_RGBA,               /* r = a0 . a1 */
   TEXENV_MODE_DOT3_RGBA_EXT,           /* r = a0 . a1 */
   TEXENV_MODE_MODULATE_ADD_ATI,        /* r = a0 * a2 + a1 */
   TEXENV_MODE_MODULATE_SIGNED_ADD_ATI, /* r = a0 * a2 + a1 - 0.5 */
   TEXENV_MODE_MODULATE_SUBTRACT_ATI,   /* r = a0 * a2 - a1 */
   TEXENV_MODE_ADD_PRODUCTS_NV,         /* r = a0 * a1 + a2 * a3 */
   TEXENV_MODE_ADD_PRODUCTS_SIGNED_NV,  /* r = a0 * a1 + a2 * a3 - 0.5 */
};

/** Compressed TexEnv Combine source */
enum gl_tex_env_source
{
   TEXENV_SRC_TEXTURE0,
   TEXENV_SRC_TEXTURE1,
   TEXENV_SRC_TEXTURE2,
   TEXENV_SRC_TEXTURE3,
   TEXENV_SRC_TEXTURE4,
   TEXENV_SRC_TEXTURE5,
   TEXENV_SRC_TEXTURE6,
   TEXENV_SRC_TEXTURE7,
   TEXENV_SRC_TEXTURE,
   TEXENV_SRC_PREVIOUS,
   TEXENV_SRC_PRIMARY_COLOR,
   TEXENV_SRC_CONSTANT,
   TEXENV_SRC_ZERO,
   TEXENV_SRC_ONE,
};

/** Compressed TexEnv Combine operand */
enum gl_tex_env_operand
{
   TEXENV_OPR_COLOR,
   TEXENV_OPR_ONE_MINUS_COLOR,
   TEXENV_OPR_ALPHA,
   TEXENV_OPR_ONE_MINUS_ALPHA,
};

/**
 * TexGenEnabled flags.
 */
/*@{*/
#define S_BIT 1
#define T_BIT 2
#define R_BIT 4
#define Q_BIT 8
#define STR_BITS (S_BIT | T_BIT | R_BIT)
/*@}*/

/**
 * Bit flag versions of the corresponding GL_ constants.
 */
/*@{*/
#define TEXGEN_SPHERE_MAP        0x1
#define TEXGEN_OBJ_LINEAR        0x2
#define TEXGEN_EYE_LINEAR        0x4
#define TEXGEN_REFLECTION_MAP_NV 0x8
#define TEXGEN_NORMAL_MAP_NV     0x10

#define TEXGEN_NEED_NORMALS   (TEXGEN_SPHERE_MAP        | \
                               TEXGEN_REFLECTION_MAP_NV | \
                               TEXGEN_NORMAL_MAP_NV)
#define TEXGEN_NEED_EYE_COORD (TEXGEN_SPHERE_MAP        | \
                               TEXGEN_REFLECTION_MAP_NV | \
                               TEXGEN_NORMAL_MAP_NV     | \
                               TEXGEN_EYE_LINEAR)
/*@}*/

/** Tex-gen enabled for texture unit? */
#define ENABLE_TEXGEN(unit) (1 << (unit))

/** Non-identity texture matrix for texture unit? */
#define ENABLE_TEXMAT(unit) (1 << (unit))

/**
 * Data structure representing a single clip plane (e.g. one of the elements
 * of the ctx->Transform.EyeUserPlane or ctx->Transform._ClipUserPlane array).
 */
typedef GLfloat gl_clip_plane[4];

/**
 * Fields describing a mapped buffer range.
 */
struct gl_buffer_mapping
{
   GLbitfield AccessFlags; /**< Mask of GL_MAP_x_BIT flags */
   GLvoid *Pointer;        /**< User-space address of mapping */
   GLintptr Offset;        /**< Mapped offset */
   GLsizeiptr Length;      /**< Mapped length */
};

/**
 * Usages we've seen for a buffer object.
 */
typedef enum
{
   USAGE_UNIFORM_BUFFER = 0x1,
   USAGE_TEXTURE_BUFFER = 0x2,
   USAGE_ATOMIC_COUNTER_BUFFER = 0x4,
   USAGE_SHADER_STORAGE_BUFFER = 0x8,
   USAGE_TRANSFORM_FEEDBACK_BUFFER = 0x10,
   USAGE_PIXEL_PACK_BUFFER = 0x20,
   USAGE_ARRAY_BUFFER = 0x40,
   USAGE_ELEMENT_ARRAY_BUFFER = 0x80,
   USAGE_DISABLE_MINMAX_CACHE = 0x100,
} gl_buffer_usage;

/**
 * GL_ARB_vertex/pixel_buffer_object buffer object
 */
struct gl_buffer_object
{
   GLint RefCount;
   GLuint Name;
   GLchar *Label;       /**< GL_KHR_debug */
   GLenum16 Usage;      /**< GL_STREAM_DRAW_ARB, GL_STREAM_READ_ARB, etc. */
   GLbitfield StorageFlags; /**< GL_MAP_PERSISTENT_BIT, etc. */
   GLsizeiptrARB Size;  /**< Size of buffer storage in bytes */
   GLubyte *Data;       /**< Location of storage either in RAM or VRAM. */
   GLboolean DeletePending;   /**< true if buffer object is removed from the hash */
   GLboolean Written;   /**< Ever written to? (for debugging) */
   GLboolean Purgeable; /**< Is the buffer purgeable under memory pressure? */
   GLboolean Immutable; /**< GL_ARB_buffer_storage */
   gl_buffer_usage UsageHistory; /**< How has this buffer been used so far? */

   /** Counters used for buffer usage warnings */
   GLuint NumSubDataCalls;
   GLuint NumMapBufferWriteCalls;

   struct gl_buffer_mapping Mappings[MAP_COUNT];

   /** Memoization of min/max index computations for static index buffers */
   struct hash_table *MinMaxCache;
   unsigned MinMaxCacheHitIndices;
   unsigned MinMaxCacheMissIndices;
   bool MinMaxCacheDirty;

   bool HandleAllocated; /**< GL_ARB_bindless_texture */
};

/**
 * Enum for defining the mapping for the position/generic0 attribute.
 *
 * Do not change the order of the values as these are used as
 * array indices.
 */
typedef enum
{
   ATTRIBUTE_MAP_MODE_IDENTITY, /**< 1:1 mapping */
   ATTRIBUTE_MAP_MODE_POSITION, /**< get position and generic0 from position */
   ATTRIBUTE_MAP_MODE_GENERIC0, /**< get position and generic0 from generic0 */
   ATTRIBUTE_MAP_MODE_MAX       /**< for sizing arrays */
} gl_attribute_map_mode;

struct gl_transform_feedback_varying_info
{
   char *Name;
   GLenum16 Type;
   GLint BufferIndex;
   GLint Size;
   GLint Offset;
};

/**
 * Per-output info vertex shaders for transform feedback.
 */
struct gl_transform_feedback_output
{
   uint32_t OutputRegister;
   uint32_t OutputBuffer;
   uint32_t NumComponents;
   uint32_t StreamId;

   /** offset (in DWORDs) of this output within the interleaved structure */
   uint32_t DstOffset;

   /**
    * Offset into the output register of the data to output.  For example,
    * if NumComponents is 2 and ComponentOffset is 1, then the data to
    * offset is in the y and z components of the output register.
    */
   uint32_t ComponentOffset;
};

struct gl_transform_feedback_buffer
{
   uint32_t Binding;

   uint32_t NumVaryings;

   /**
    * Total number of components stored in each buffer.  This may be used by
    * hardware back-ends to determine the correct stride when interleaving
    * multiple transform feedback outputs in the same buffer.
    */
   uint32_t Stride;

   /**
    * Which transform feedback stream this buffer binding is associated with.
    */
   uint32_t Stream;
};

/** Post-link transform feedback info. */
struct gl_transform_feedback_info
{
   unsigned NumOutputs;

   /* Bitmask of active buffer indices. */
   unsigned ActiveBuffers;

   struct gl_transform_feedback_output *Outputs;

   /** Transform feedback varyings used for the linking of this shader program.
    *
    * Use for glGetTransformFeedbackVarying().
    */
   struct gl_transform_feedback_varying_info *Varyings;
   GLint NumVarying;

   struct gl_transform_feedback_buffer Buffers[MAX_FEEDBACK_BUFFERS];
};

union gl_perf_monitor_counter_value
{
   float f;
   uint64_t u64;
   uint32_t u32;
};

/**
 * A bindless sampler object.
 */
struct gl_bindless_sampler
{
   /** Texture unit (set by glUniform1()). */
   GLubyte unit;

   /** Whether this bindless sampler is bound to a unit. */
   GLboolean bound;

   /** Texture Target (TEXTURE_1D/2D/3D/etc_INDEX). */
   gl_texture_index target;

   /** Pointer to the base of the data. */
   GLvoid *data;
};

/**
 * A bindless image object.
 */
struct gl_bindless_image
{
   /** Image unit (set by glUniform1()). */
   GLubyte unit;

   /** Whether this bindless image is bound to a unit. */
   GLboolean bound;

   /** Access qualifier (GL_READ_WRITE, GL_READ_ONLY, GL_WRITE_ONLY, or
    * GL_NONE to indicate both read-only and write-only)
    */
   GLenum16 access;

   /** Pointer to the base of the data. */
   GLvoid *data;
};

/**
 * Base class for any kind of program object
 */
struct gl_program
{
   /** FIXME: This must be first until we split shader_info from nir_shader */
   struct shader_info info;

   GLuint Id;
   GLint RefCount;
   GLubyte *String;  /**< Null-terminated program text */

   /** GL_VERTEX/FRAGMENT_PROGRAM_ARB, GL_GEOMETRY_PROGRAM_NV */
   GLenum16 Target;
   GLenum16 Format;    /**< String encoding format */

   GLboolean _Used;        /**< Ever used for drawing? Used for debugging */

   struct nir_shader *nir;

   bool is_arb_asm; /** Is this an ARB assembly-style program */

   /** A bitfield indicating which vertex shader inputs consume two slots
    *
    * This is used for mapping from single-slot input locations in the GL API
    * to dual-slot double input locations in the shader.  This field is set
    * once as part of linking and never updated again to ensure the mapping
    * remains consistent.
    *
    * Note: There may be dual-slot variables in the original shader source
    * which do not appear in this bitfield due to having been eliminated by
    * the compiler prior to DualSlotInputs being calculated.  There may also
    * be bits set in this bitfield which are set but which the shader never
    * reads due to compiler optimizations eliminating such variables after
    * DualSlotInputs is calculated.
    */
   GLbitfield64 DualSlotInputs;
   /** Subset of OutputsWritten outputs written with non-zero index. */
   GLbitfield64 SecondaryOutputsWritten;
   /** TEXTURE_x_BIT bitmask */
   GLbitfield16 TexturesUsed[MAX_COMBINED_TEXTURE_IMAGE_UNITS];
   /** Bitfield of which samplers are used */
   GLbitfield SamplersUsed;
   /** Texture units used for shadow sampling. */
   GLbitfield ShadowSamplers;
   /** Texture units used for samplerExternalOES */
   GLbitfield ExternalSamplersUsed;

   /** Named parameters, constants, etc. from program text */
   struct gl_program_parameter_list *Parameters;

   /** Map from sampler unit to texture unit (set by glUniform1i()) */
   GLubyte SamplerUnits[MAX_SAMPLERS];

   /* FIXME: We should be able to make this struct a union. However some
    * drivers (i915/fragment_programs, swrast/prog_execute) mix the use of
    * these fields, we should fix this.
    */
   struct {
      /** Fields used by GLSL programs */
      struct {
         /** Data shared by gl_program and gl_shader_program */
         struct gl_shader_program_data *data;

         struct gl_active_atomic_buffer **AtomicBuffers;

         /** Post-link transform feedback info. */
         struct gl_transform_feedback_info *LinkedTransformFeedback;

         /**
          * Number of types for subroutine uniforms.
          */
         GLuint NumSubroutineUniformTypes;

         /**
          * Subroutine uniform remap table
          * based on the program level uniform remap table.
          */
         GLuint NumSubroutineUniforms; /* non-sparse total */
         GLuint NumSubroutineUniformRemapTable;
         struct gl_uniform_storage **SubroutineUniformRemapTable;

         /**
          * Num of subroutine functions for this stage and storage for them.
          */
         GLuint NumSubroutineFunctions;
         GLuint MaxSubroutineFunctionIndex;
         struct gl_subroutine_function *SubroutineFunctions;

         /**
          * Map from image uniform index to image unit (set by glUniform1i())
          *
          * An image uniform index is associated with each image uniform by
          * the linker.  The image index associated with each uniform is
          * stored in the \c gl_uniform_storage::image field.
          */
         GLubyte ImageUnits[MAX_IMAGE_UNIFORMS];

         /**
          * Access qualifier specified in the shader for each image uniform
          * index.  Either \c GL_READ_ONLY, \c GL_WRITE_ONLY, \c
          * GL_READ_WRITE, or \c GL_NONE to indicate both read-only and
          * write-only.
          *
          * It may be different, though only more strict than the value of
          * \c gl_image_unit::Access for the corresponding image unit.
          */
         GLenum16 ImageAccess[MAX_IMAGE_UNIFORMS];

         GLuint NumUniformBlocks;
         struct gl_uniform_block **UniformBlocks;
         struct gl_uniform_block **ShaderStorageBlocks;

         /**
          * Bitmask of shader storage blocks not declared as read-only.
          */
         unsigned ShaderStorageBlocksWriteAccess;

         /** Which texture target is being sampled
          * (TEXTURE_1D/2D/3D/etc_INDEX)
          */
         GLubyte SamplerTargets[MAX_SAMPLERS];

         /**
          * Number of samplers declared with the bindless_sampler layout
          * qualifier as specified by ARB_bindless_texture.
          */
         GLuint NumBindlessSamplers;
         GLboolean HasBoundBindlessSampler;
         struct gl_bindless_sampler *BindlessSamplers;

         /**
          * Number of images declared with the bindless_image layout qualifier
          * as specified by ARB_bindless_texture.
          */
         GLuint NumBindlessImages;
         GLboolean HasBoundBindlessImage;
         struct gl_bindless_image *BindlessImages;

         union {
            struct {
               /**
                * A bitmask of gl_advanced_blend_mode values
                */
               GLbitfield BlendSupport;
            } fs;
         };
      } sh;

      /** ARB assembly-style program fields */
      struct {
         struct prog_instruction *Instructions;

         /**
          * Local parameters used by the program.
          *
          * It's dynamically allocated because it is rarely used (just
          * assembly-style programs), and MAX_PROGRAM_LOCAL_PARAMS entries
          * once it's allocated.
          */
         GLfloat (*LocalParams)[4];

         /** Bitmask of which register files are read/written with indirect
          * addressing.  Mask of (1 << PROGRAM_x) bits.
          */
         GLbitfield IndirectRegisterFiles;

         /** Logical counts */
         /*@{*/
         GLuint NumInstructions;
         GLuint NumTemporaries;
         GLuint NumParameters;
         GLuint NumAttributes;
         GLuint NumAddressRegs;
         GLuint NumAluInstructions;
         GLuint NumTexInstructions;
         GLuint NumTexIndirections;
         /*@}*/
         /** Native, actual h/w counts */
         /*@{*/
         GLuint NumNativeInstructions;
         GLuint NumNativeTemporaries;
         GLuint NumNativeParameters;
         GLuint NumNativeAttributes;
         GLuint NumNativeAddressRegs;
         GLuint NumNativeAluInstructions;
         GLuint NumNativeTexInstructions;
         GLuint NumNativeTexIndirections;
         /*@}*/

         /** Used by ARB assembly-style programs. Can only be true for vertex
          * programs.
          */
         GLboolean IsPositionInvariant;
      } arb;
   };
};

/**
 * Context state for vertex programs.
 */

/**
 * ATI_fragment_shader runtime state
 */

struct atifs_instruction;
struct atifs_setupinst;

/**
 *  Shader subroutine function definition
 */
struct gl_subroutine_function
{
   char *name;
   int index;
   int num_compat_types;
   const struct glsl_type **types;
};

/**
 * Shader information needed by both gl_shader and gl_linked shader.
 */
struct gl_shader_info
{
   /**
    * Tessellation Control shader state from layout qualifiers.
    */
   struct {
      /**
       * 0 - vertices not declared in shader, or
       * 1 .. GL_MAX_PATCH_VERTICES
       */
      GLint VerticesOut;
   } TessCtrl;

   /**
    * Tessellation Evaluation shader state from layout qualifiers.
    */
   struct {
      /**
       * GL_TRIANGLES, GL_QUADS, GL_ISOLINES or PRIM_UNKNOWN if it's not set
       * in this shader.
       */
      GLenum16 PrimitiveMode;

      enum gl_tess_spacing Spacing;

      /**
       * GL_CW, GL_CCW, or 0 if it's not set in this shader.
       */
      GLenum16 VertexOrder;
      /**
       * 1, 0, or -1 if it's not set in this shader.
       */
      int PointMode;
   } TessEval;

   /**
    * Geometry shader state from GLSL 1.50 layout qualifiers.
    */
   struct {
      GLint VerticesOut;
      /**
       * 0 - Invocations count not declared in shader, or
       * 1 .. Const.MaxGeometryShaderInvocations
       */
      GLint Invocations;
      /**
       * GL_POINTS, GL_LINES, GL_LINES_ADJACENCY, GL_TRIANGLES, or
       * GL_TRIANGLES_ADJACENCY, or PRIM_UNKNOWN if it's not set in this
       * shader.
       */
      GLenum16 InputType;
       /**
        * GL_POINTS, GL_LINE_STRIP or GL_TRIANGLE_STRIP, or PRIM_UNKNOWN if
        * it's not set in this shader.
        */
      GLenum16 OutputType;
   } Geom;

   /**
    * Compute shader state from ARB_compute_shader and
    * ARB_compute_variable_group_size layout qualifiers.
    */
   struct {
      /**
       * Size specified using local_size_{x,y,z}, or all 0's to indicate that
       * it's not set in this shader.
       */
      unsigned LocalSize[3];

      /**
       * Whether a variable work group size has been specified as defined by
       * ARB_compute_variable_group_size.
       */
      bool LocalSizeVariable;

      /*
       * Arrangement of invocations used to calculate derivatives in a compute
       * shader.  From NV_compute_shader_derivatives.
       */
      enum gl_derivative_group DerivativeGroup;
   } Comp;
};

/**
 * A linked GLSL shader object.
 */
struct gl_linked_shader
{
   gl_shader_stage Stage;

#ifdef DEBUG
   unsigned SourceChecksum;
#endif

   struct gl_program *Program;  /**< Post-compile assembly code */

   /**
    * \name Sampler tracking
    *
    * \note Each of these fields is only set post-linking.
    */
   /*@{*/
   GLbitfield shadow_samplers;	/**< Samplers used for shadow sampling. */
   /*@}*/

   /**
    * Number of default uniform block components used by this shader.
    *
    * This field is only set post-linking.
    */
   unsigned num_uniform_components;

   /**
    * Number of combined uniform components used by this shader.
    *
    * This field is only set post-linking.  It is the sum of the uniform block
    * sizes divided by sizeof(float), and num_uniform_compoennts.
    */
   unsigned num_combined_uniform_components;

   struct exec_list *ir;
   struct exec_list *packed_varyings;
   struct exec_list *fragdata_arrays;
   struct glsl_symbol_table *symbols;

   /**
    * ARB_gl_spirv related data.
    *
    * This is actually a reference to the gl_shader::spirv_data, which
    * stores information that is also needed during linking.
    */
   struct gl_shader_spirv_data *spirv_data;
};

/**
 * Shader compilation status.
 */
enum gl_compile_status
{
   COMPILE_FAILURE = 0,
   COMPILE_SUCCESS
};

/**
 * A GLSL shader object.
 */
struct gl_shader
{
   /** GL_FRAGMENT_SHADER || GL_VERTEX_SHADER || GL_GEOMETRY_SHADER_ARB ||
    *  GL_TESS_CONTROL_SHADER || GL_TESS_EVALUATION_SHADER.
    * Must be the first field.
    */
   GLenum16 Type;
   gl_shader_stage Stage;
   GLuint Name;  /**< AKA the handle */
   GLint RefCount;  /**< Reference count */
   GLchar *Label;   /**< GL_KHR_debug */
   GLboolean DeletePending;
   bool IsES;              /**< True if this shader uses GLSL ES */

   enum gl_compile_status CompileStatus;

#ifdef DEBUG
   unsigned SourceChecksum;       /**< for debug/logging purposes */
#endif
   const GLchar *Source;  /**< Source code string */


   GLchar *InfoLog;

   unsigned Version;       /**< GLSL version used for linking */

   /**
    * A bitmask of gl_advanced_blend_mode values
    */
   GLbitfield BlendSupport;

   struct exec_list *ir;
   struct glsl_symbol_table *symbols;

   /**
    * Whether early fragment tests are enabled as defined by
    * ARB_shader_image_load_store.
    */
   bool EarlyFragmentTests;

   bool ARB_fragment_coord_conventions_enable;

   bool redeclares_gl_fragcoord;
   bool uses_gl_fragcoord;

   bool PostDepthCoverage;
   bool PixelInterlockOrdered;
   bool PixelInterlockUnordered;
   bool SampleInterlockOrdered;
   bool SampleInterlockUnordered;
   bool InnerCoverage;

   /**
    * Fragment shader state from GLSL 1.50 layout qualifiers.
    */
   bool origin_upper_left;
   bool pixel_center_integer;

   /**
    * Whether bindless_sampler/bindless_image, and respectively
    * bound_sampler/bound_image are declared at global scope as defined by
    * ARB_bindless_texture.
    */
   bool bindless_sampler;
   bool bindless_image;
   bool bound_sampler;
   bool bound_image;

   /**
    * Whether layer output is viewport-relative.
    */
   bool redeclares_gl_layer;
   bool layer_viewport_relative;

   /** Global xfb_stride out qualifier if any */
   GLuint TransformFeedbackBufferStride[MAX_FEEDBACK_BUFFERS];

   struct gl_shader_info info;

   /* ARB_gl_spirv related data */
   struct gl_shader_spirv_data *spirv_data;
};

struct gl_uniform_buffer_variable
{
   char *Name;

   /**
    * Name of the uniform as seen by glGetUniformIndices.
    *
    * glGetUniformIndices requires that the block instance index \b not be
    * present in the name of queried uniforms.
    *
    * \note
    * \c gl_uniform_buffer_variable::IndexName and
    * \c gl_uniform_buffer_variable::Name may point to identical storage.
    */
   char *IndexName;

   const struct glsl_type *Type;
   unsigned int Offset;
   GLboolean RowMajor;
};

struct gl_uniform_block
{
   /** Declared name of the uniform block */
   char *Name;

   /** Array of supplemental information about UBO ir_variables. */
   struct gl_uniform_buffer_variable *Uniforms;
   GLuint NumUniforms;

   /**
    * Index (GL_UNIFORM_BLOCK_BINDING) into ctx->UniformBufferBindings[] to use
    * with glBindBufferBase to bind a buffer object to this uniform block.
    */
   GLuint Binding;

   /**
    * Minimum size (in bytes) of a buffer object to back this uniform buffer
    * (GL_UNIFORM_BLOCK_DATA_SIZE).
    */
   GLuint UniformBufferSize;

   /** Stages that reference this block */
   uint8_t stageref;

   /**
    * Linearized array index for uniform block instance arrays
    *
    * Given a uniform block instance array declared with size
    * blk[s_0][s_1]..[s_m], the block referenced by blk[i_0][i_1]..[i_m] will
    * have the linearized array index
    *
    *           m-1       m
    *     i_m + ∑   i_j * ∏     s_k
    *           j=0       k=j+1
    *
    * For a uniform block instance that is not an array, this is always 0.
    */
   uint8_t linearized_array_index;

   /**
    * Layout specified in the shader
    *
    * This isn't accessible through the API, but it is used while
    * cross-validating uniform blocks.
    */
   enum glsl_interface_packing _Packing;
   GLboolean _RowMajor;
};

/**
 * Structure that represents a reference to an atomic buffer from some
 * shader program.
 */
struct gl_active_atomic_buffer
{
   /** Uniform indices of the atomic counters declared within it. */
   GLuint *Uniforms;
   GLuint NumUniforms;

   /** Binding point index associated with it. */
   GLuint Binding;

   /** Minimum reasonable size it is expected to have. */
   GLuint MinimumSize;

   /** Shader stages making use of it. */
   GLboolean StageReferences[MESA_SHADER_STAGES];
};

/**
 * Data container for shader queries. This holds only the minimal
 * amount of required information for resource queries to work.
 */
struct gl_shader_variable
{
   /**
    * Declared type of the variable
    */
   const struct glsl_type *type;

   /**
    * If the variable is in an interface block, this is the type of the block.
    */
   const struct glsl_type *interface_type;

   /**
    * For variables inside structs (possibly recursively), this is the
    * outermost struct type.
    */
   const struct glsl_type *outermost_struct_type;

   /**
    * Declared name of the variable
    */
   char *name;

   /**
    * Storage location of the base of this variable
    *
    * The precise meaning of this field depends on the nature of the variable.
    *
    *   - Vertex shader input: one of the values from \c gl_vert_attrib.
    *   - Vertex shader output: one of the values from \c gl_varying_slot.
    *   - Geometry shader input: one of the values from \c gl_varying_slot.
    *   - Geometry shader output: one of the values from \c gl_varying_slot.
    *   - Fragment shader input: one of the values from \c gl_varying_slot.
    *   - Fragment shader output: one of the values from \c gl_frag_result.
    *   - Uniforms: Per-stage uniform slot number for default uniform block.
    *   - Uniforms: Index within the uniform block definition for UBO members.
    *   - Non-UBO Uniforms: explicit location until linking then reused to
    *     store uniform slot number.
    *   - Other: This field is not currently used.
    *
    * If the variable is a uniform, shader input, or shader output, and the
    * slot has not been assigned, the value will be -1.
    */
   int location;

   /**
    * Specifies the first component the variable is stored in as per
    * ARB_enhanced_layouts.
    */
   unsigned component:2;

   /**
    * Output index for dual source blending.
    *
    * \note
    * The GLSL spec only allows the values 0 or 1 for the index in \b dual
    * source blending.
    */
   unsigned index:1;

   /**
    * Specifies whether a shader input/output is per-patch in tessellation
    * shader stages.
    */
   unsigned patch:1;

   /**
    * Storage class of the variable.
    *
    * \sa (n)ir_variable_mode
    */
   unsigned mode:4;

   /**
    * Interpolation mode for shader inputs / outputs
    *
    * \sa glsl_interp_mode
    */
   unsigned interpolation:2;

   /**
    * Was the location explicitly set in the shader?
    *
    * If the location is explicitly set in the shader, it \b cannot be changed
    * by the linker or by the API (e.g., calls to \c glBindAttribLocation have
    * no effect).
    */
   unsigned explicit_location:1;

   /**
    * Precision qualifier.
    */
   unsigned precision:2;
};

/**
 * Active resource in a gl_shader_program
 */
struct gl_program_resource
{
   GLenum16 Type; /** Program interface type. */
   const void *Data; /** Pointer to resource associated data structure. */
   uint8_t StageReferences; /** Bitmask of shader stage references. */
};

/**
 * Shader link status.
 */
enum gl_link_status
{
   LINKING_FAILURE = 0,
   LINKING_SUCCESS
};

/**
 * A data structure to be shared by gl_shader_program and gl_program.
 */
struct gl_shader_program_data
{
   GLint RefCount;  /**< Reference count */

   unsigned NumUniformStorage;
   unsigned NumHiddenUniforms;
   struct gl_uniform_storage *UniformStorage;

   unsigned NumUniformBlocks;
   unsigned NumShaderStorageBlocks;

   struct gl_uniform_block *UniformBlocks;
   struct gl_uniform_block *ShaderStorageBlocks;

   struct gl_active_atomic_buffer *AtomicBuffers;
   unsigned NumAtomicBuffers;

   /* Linked uniform values. */
   unsigned NumUniformDataSlots;
   union gl_constant_value *UniformDataSlots;

   /** Hash for quick search by name. */
   struct hash_table_u64 *ProgramResourceHash;

   GLboolean Validated;

   /** List of all active resources after linking. */
   struct gl_program_resource *ProgramResourceList;
   unsigned NumProgramResourceList;

   enum gl_link_status LinkStatus;   /**< GL_LINK_STATUS */
   GLchar *InfoLog;

   unsigned Version;       /**< GLSL version used for linking */

   /* Mask of stages this program was linked against */
   unsigned linked_stages;

   /* Whether the shaders of this program are loaded from SPIR-V binaries
    * (all have the SPIR_V_BINARY_ARB state). This was introduced by the
    * ARB_gl_spirv extension.
    */
   bool spirv;
};

/**
 * A GLSL program object.
 * Basically a linked collection of vertex and fragment shaders.
 */
struct gl_shader_program
{
   GLenum16 Type;   /**< Always GL_SHADER_PROGRAM (internal token) */
   GLuint Name;  /**< aka handle or ID */
   GLchar *Label;   /**< GL_KHR_debug */
   GLint RefCount;  /**< Reference count */
   GLboolean DeletePending;

   /**
    * Is the application intending to glGetProgramBinary this program?
    *
    * BinaryRetrievableHint is the currently active hint that gets set
    * during initialization and after linking and BinaryRetrievableHintPending
    * is the hint set by the user to be active when program is linked next time.
    */
   GLboolean BinaryRetrievableHint;
   GLboolean BinaryRetrievableHintPending;

   /**
    * Indicates whether program can be bound for individual pipeline stages
    * using UseProgramStages after it is next linked.
    */
   GLboolean SeparateShader;

   GLuint NumShaders;          /**< number of attached shaders */
   struct gl_shader **Shaders; /**< List of attached the shaders */

   /**
    * User-defined attribute bindings
    *
    * These are set via \c glBindAttribLocation and are used to direct the
    * GLSL linker.  These are \b not the values used in the compiled shader,
    * and they are \b not the values returned by \c glGetAttribLocation.
    */
   struct string_to_uint_map *AttributeBindings;

   /**
    * User-defined fragment data bindings
    *
    * These are set via \c glBindFragDataLocation and are used to direct the
    * GLSL linker.  These are \b not the values used in the compiled shader,
    * and they are \b not the values returned by \c glGetFragDataLocation.
    */
   struct string_to_uint_map *FragDataBindings;
   struct string_to_uint_map *FragDataIndexBindings;

   /**
    * Transform feedback varyings last specified by
    * glTransformFeedbackVaryings().
    *
    * For the current set of transform feedback varyings used for transform
    * feedback output, see LinkedTransformFeedback.
    */
   struct {
      GLenum16 BufferMode;
      /** Global xfb_stride out qualifier if any */
      GLuint BufferStride[MAX_FEEDBACK_BUFFERS];
      GLuint NumVarying;
      GLchar **VaryingNames;  /**< Array [NumVarying] of char * */
   } TransformFeedback;

   struct gl_program *last_vert_prog;

   /** Post-link gl_FragDepth layout for ARB_conservative_depth. */
   enum gl_frag_depth_layout FragDepthLayout;

   /**
    * Geometry shader state - copied into gl_program by
    * _mesa_copy_linked_program_data().
    */
   struct {
      GLint VerticesIn;

      bool UsesEndPrimitive;
      unsigned ActiveStreamMask;
   } Geom;

   /**
    * Compute shader state - copied into gl_program by
    * _mesa_copy_linked_program_data().
    */
   struct {
      /**
       * Size of shared variables accessed by the compute shader.
       */
      unsigned SharedSize;
   } Comp;

   /** Data shared by gl_program and gl_shader_program */
   struct gl_shader_program_data *data;

   /**
    * Mapping from GL uniform locations returned by \c glUniformLocation to
    * UniformStorage entries. Arrays will have multiple contiguous slots
    * in the UniformRemapTable, all pointing to the same UniformStorage entry.
    */
   unsigned NumUniformRemapTable;
   struct gl_uniform_storage **UniformRemapTable;

   /**
    * Sometimes there are empty slots left over in UniformRemapTable after we
    * allocate slots to explicit locations. This list stores the blocks of
    * continuous empty slots inside UniformRemapTable.
    */
   struct exec_list EmptyUniformLocations;

   /**
    * Total number of explicit uniform location including inactive uniforms.
    */
   unsigned NumExplicitUniformLocations;

   /**
    * Map of active uniform names to locations
    *
    * Maps any active uniform that is not an array element to a location.
    * Each active uniform, including individual structure members will appear
    * in this map.  This roughly corresponds to the set of names that would be
    * enumerated by \c glGetActiveUniform.
    */
   struct string_to_uint_map *UniformHash;

   GLboolean SamplersValidated; /**< Samplers validated against texture units? */

   bool IsES;              /**< True if this program uses GLSL ES */

   /**
    * Per-stage shaders resulting from the first stage of linking.
    *
    * Set of linked shaders for this program.  The array is accessed using the
    * \c MESA_SHADER_* defines.  Entries for non-existent stages will be
    * \c NULL.
    */
   struct gl_linked_shader *_LinkedShaders[MESA_SHADER_STAGES];

   /**
    * True if any of the fragment shaders attached to this program use:
    * #extension ARB_fragment_coord_conventions: enable
    */
   GLboolean ARB_fragment_coord_conventions_enable;
};

#define GLSL_DUMP      0x1  /**< Dump shaders to stdout */
#define GLSL_LOG       0x2  /**< Write shaders to files */
#define GLSL_UNIFORMS  0x4  /**< Print glUniform calls */
#define GLSL_NOP_VERT  0x8  /**< Force no-op vertex shaders */
#define GLSL_NOP_FRAG 0x10  /**< Force no-op fragment shaders */
#define GLSL_USE_PROG 0x20  /**< Log glUseProgram calls */
#define GLSL_REPORT_ERRORS 0x40  /**< Print compilation errors */
#define GLSL_DUMP_ON_ERROR 0x80 /**< Dump shaders to stderr on compile error */

/**
 * Context state for GLSL vertex/fragment shaders.
 * Extended to support pipeline object
 */
struct gl_pipeline_object
{
   /** Name of the pipeline object as received from glGenProgramPipelines.
    * It would be 0 for shaders without separate shader objects.
    */
   GLuint Name;

   GLint RefCount;

   GLchar *Label;   /**< GL_KHR_debug */

   /**
    * Programs used for rendering
    *
    * There is a separate program set for each shader stage.
    */
   struct gl_program *CurrentProgram[MESA_SHADER_STAGES];

   struct gl_shader_program *ReferencedPrograms[MESA_SHADER_STAGES];

   /**
    * Program used by glUniform calls.
    *
    * Explicitly set by \c glUseProgram and \c glActiveProgramEXT.
    */
   struct gl_shader_program *ActiveProgram;

   GLbitfield Flags;         /**< Mask of GLSL_x flags */
   GLboolean EverBound;      /**< Has the pipeline object been created */
   GLboolean Validated;      /**< Pipeline Validation status */

   GLchar *InfoLog;
};

/**
 * Compiler options for a single GLSL shaders type
 */
struct gl_shader_compiler_options
{
   /** Driver-selectable options: */
   GLboolean EmitNoLoops;
   GLboolean EmitNoCont;                  /**< Emit CONT opcode? */
   GLboolean EmitNoMainReturn;            /**< Emit CONT/RET opcodes? */
   GLboolean EmitNoPow;                   /**< Emit POW opcodes? */
   GLboolean EmitNoSat;                   /**< Emit SAT opcodes? */
   GLboolean LowerCombinedClipCullDistance; /** Lower gl_ClipDistance and
                                              * gl_CullDistance together from
                                              * float[8] to vec4[2]
                                              **/
   GLbitfield LowerBuiltinVariablesXfb;   /**< Which builtin variables should
                                           * be lowered for transform feedback
                                           **/

   /**
    * If we can lower the precision of variables based on precision
    * qualifiers
    */
   GLboolean LowerPrecisionFloat16;
   GLboolean LowerPrecisionInt16;
   GLboolean LowerPrecisionDerivatives;

   /**
    * This enables lowering of 16b constants.  Some drivers may not
    * to lower constants to 16b (ie. if the hw can do automatic
    * narrowing on constant load)
    */
   GLboolean LowerPrecisionConstants;

   /**
    * \name Forms of indirect addressing the driver cannot do.
    */
   /*@{*/
   GLboolean EmitNoIndirectInput;   /**< No indirect addressing of inputs */
   GLboolean EmitNoIndirectOutput;  /**< No indirect addressing of outputs */
   GLboolean EmitNoIndirectTemp;    /**< No indirect addressing of temps */
   GLboolean EmitNoIndirectUniform; /**< No indirect addressing of constants */
   GLboolean EmitNoIndirectSampler; /**< No indirect addressing of samplers */
   /*@}*/

   GLuint MaxIfDepth;               /**< Maximum nested IF blocks */
   GLuint MaxUnrollIterations;

   /**
    * Optimize code for array of structures backends.
    *
    * This is a proxy for:
    *   - preferring DP4 instructions (rather than MUL/MAD) for
    *     matrix * vector operations, such as position transformation.
    */
   GLboolean OptimizeForAOS;

   /** Lower UBO and SSBO access to intrinsics. */
   GLboolean LowerBufferInterfaceBlocks;

   /** Clamp UBO and SSBO block indices so they don't go out-of-bounds. */
   GLboolean ClampBlockIndicesToArrayBounds;

   /** (driconf) Force gl_Position to be considered invariant */
   GLboolean PositionAlwaysInvariant;

   const struct nir_shader_compiler_options *NirOptions;
};

/**
 * State which can be shared by multiple contexts:
 */

/**
 * Precision info for shader datatypes.  See glGetShaderPrecisionFormat().
 */
struct gl_precision
{
   GLushort RangeMin;   /**< min value exponent */
   GLushort RangeMax;   /**< max value exponent */
   GLushort Precision;  /**< number of mantissa bits */
};

/**
 * Limits for vertex, geometry and fragment programs/shaders.
 */
struct gl_program_constants
{
   /* logical limits */
   GLuint MaxInstructions;
   GLuint MaxAluInstructions;
   GLuint MaxTexInstructions;
   GLuint MaxTexIndirections;
   GLuint MaxAttribs;
   GLuint MaxTemps;
   GLuint MaxAddressRegs;
   GLuint MaxAddressOffset;  /**< [-MaxAddressOffset, MaxAddressOffset-1] */
   GLuint MaxParameters;
   GLuint MaxLocalParams;
   GLuint MaxEnvParams;
   /* native/hardware limits */
   GLuint MaxNativeInstructions;
   GLuint MaxNativeAluInstructions;
   GLuint MaxNativeTexInstructions;
   GLuint MaxNativeTexIndirections;
   GLuint MaxNativeAttribs;
   GLuint MaxNativeTemps;
   GLuint MaxNativeAddressRegs;
   GLuint MaxNativeParameters;
   /* For shaders */
   GLuint MaxUniformComponents;  /**< Usually == MaxParameters * 4 */

   /**
    * \name Per-stage input / output limits
    *
    * Previous to OpenGL 3.2, the intrastage data limits were advertised with
    * a single value: GL_MAX_VARYING_COMPONENTS (GL_MAX_VARYING_VECTORS in
    * ES).  This is stored as \c gl_constants::MaxVarying.
    *
    * Starting with OpenGL 3.2, the limits are advertised with per-stage
    * variables.  Each stage as a certain number of outputs that it can feed
    * to the next stage and a certain number inputs that it can consume from
    * the previous stage.
    *
    * Vertex shader inputs do not participate this in this accounting.
    * These are tracked exclusively by \c gl_program_constants::MaxAttribs.
    *
    * Fragment shader outputs do not participate this in this accounting.
    * These are tracked exclusively by \c gl_constants::MaxDrawBuffers.
    */
   /*@{*/
   GLuint MaxInputComponents;
   GLuint MaxOutputComponents;
   /*@}*/

   /* ES 2.0 and GL_ARB_ES2_compatibility */
   struct gl_precision LowFloat, MediumFloat, HighFloat;
   struct gl_precision LowInt, MediumInt, HighInt;
   /* GL_ARB_uniform_buffer_object */
   GLuint MaxUniformBlocks;
   uint64_t MaxCombinedUniformComponents;
   GLuint MaxTextureImageUnits;

   /* GL_ARB_shader_atomic_counters */
   GLuint MaxAtomicBuffers;
   GLuint MaxAtomicCounters;

   /* GL_ARB_shader_image_load_store */
   GLuint MaxImageUniforms;

   /* GL_ARB_shader_storage_buffer_object */
   GLuint MaxShaderStorageBlocks;
};

/**
 * Constants which may be overridden by device driver during context creation
 * but are never changed after that.
 */
struct gl_constants
{
   GLuint MaxTextureMbytes;      /**< Max memory per image, in MB */
   GLuint MaxTextureSize;        /**< Max 1D/2D texture size, in pixels*/
   GLuint Max3DTextureLevels;    /**< Max mipmap levels for 3D textures */
   GLuint MaxCubeTextureLevels;  /**< Max mipmap levels for cube textures */
   GLuint MaxArrayTextureLayers; /**< Max layers in array textures */
   GLuint MaxTextureRectSize;    /**< Max rectangle texture size, in pixes */
   GLuint MaxTextureCoordUnits;
   GLuint MaxCombinedTextureImageUnits;
   GLuint MaxTextureUnits; /**< = MIN(CoordUnits, FragmentProgram.ImageUnits) */
   GLfloat MaxTextureMaxAnisotropy;  /**< GL_EXT_texture_filter_anisotropic */
   GLfloat MaxTextureLodBias;        /**< GL_EXT_texture_lod_bias */
   GLuint MaxTextureBufferSize;      /**< GL_ARB_texture_buffer_object */

   GLuint TextureBufferOffsetAlignment; /**< GL_ARB_texture_buffer_range */

   GLuint MaxArrayLockSize;

   GLint SubPixelBits;

   GLfloat MinPointSize, MaxPointSize;	     /**< aliased */
   GLfloat MinPointSizeAA, MaxPointSizeAA;   /**< antialiased */
   GLfloat PointSizeGranularity;
   GLfloat MinLineWidth, MaxLineWidth;       /**< aliased */
   GLfloat MinLineWidthAA, MaxLineWidthAA;   /**< antialiased */
   GLfloat LineWidthGranularity;

   GLuint MaxClipPlanes;
   GLuint MaxLights;
   GLfloat MaxShininess;                     /**< GL_NV_light_max_exponent */
   GLfloat MaxSpotExponent;                  /**< GL_NV_light_max_exponent */

   GLuint MaxViewportWidth, MaxViewportHeight;
   GLuint MaxViewports;                      /**< GL_ARB_viewport_array */
   GLuint ViewportSubpixelBits;              /**< GL_ARB_viewport_array */
   struct {
      GLfloat Min;
      GLfloat Max;
   } ViewportBounds;                         /**< GL_ARB_viewport_array */
   GLuint MaxWindowRectangles;               /**< GL_EXT_window_rectangles */

   struct gl_program_constants Program[MESA_SHADER_STAGES];
   GLuint MaxProgramMatrices;
   GLuint MaxProgramMatrixStackDepth;

   struct {
      GLuint SamplesPassed;
      GLuint TimeElapsed;
      GLuint Timestamp;
      GLuint PrimitivesGenerated;
      GLuint PrimitivesWritten;
      GLuint VerticesSubmitted;
      GLuint PrimitivesSubmitted;
      GLuint VsInvocations;
      GLuint TessPatches;
      GLuint TessInvocations;
      GLuint GsInvocations;
      GLuint GsPrimitives;
      GLuint FsInvocations;
      GLuint ComputeInvocations;
      GLuint ClInPrimitives;
      GLuint ClOutPrimitives;
   } QueryCounterBits;

   GLuint MaxDrawBuffers;    /**< GL_ARB_draw_buffers */

   GLuint MaxColorAttachments;   /**< GL_EXT_framebuffer_object */
   GLuint MaxRenderbufferSize;   /**< GL_EXT_framebuffer_object */
   GLuint MaxSamples;            /**< GL_ARB_framebuffer_object */

   /**
    * GL_ARB_framebuffer_no_attachments
    */
   GLuint MaxFramebufferWidth;
   GLuint MaxFramebufferHeight;
   GLuint MaxFramebufferLayers;
   GLuint MaxFramebufferSamples;

   /** Number of varying vectors between any two shader stages. */
   GLuint MaxVarying;

   /** @{
    * GL_ARB_uniform_buffer_object
    */
   GLuint MaxCombinedUniformBlocks;
   GLuint MaxUniformBufferBindings;
   GLuint MaxUniformBlockSize;
   GLuint UniformBufferOffsetAlignment;
   /** @} */

   /** @{
    * GL_ARB_shader_storage_buffer_object
    */
   GLuint MaxCombinedShaderStorageBlocks;
   GLuint MaxShaderStorageBufferBindings;
   GLuint MaxShaderStorageBlockSize;
   GLuint ShaderStorageBufferOffsetAlignment;
   /** @} */

   /**
    * GL_ARB_explicit_uniform_location
    */
   GLuint MaxUserAssignableUniformLocations;

   /** geometry shader */
   GLuint MaxGeometryOutputVertices;
   GLuint MaxGeometryTotalOutputComponents;
   GLuint MaxGeometryShaderInvocations;

   GLuint GLSLVersion;  /**< Desktop GLSL version supported (ex: 120 = 1.20) */
   GLuint GLSLVersionCompat;  /**< Desktop compat GLSL version supported  */

   /**
    * Changes default GLSL extension behavior from "error" to "warn".  It's out
    * of spec, but it can make some apps work that otherwise wouldn't.
    */
   GLboolean ForceGLSLExtensionsWarn;

   /**
    * If non-zero, forces GLSL shaders to behave as if they began
    * with "#version ForceGLSLVersion".
    */
   GLuint ForceGLSLVersion;

   /**
    * Allow GLSL #extension directives in the middle of shaders.
    */
   GLboolean AllowGLSLExtensionDirectiveMidShader;

   /**
    * Allow a subset of GLSL 1.20 in GLSL 1.10 as needed by SPECviewperf13.
    */
   GLboolean AllowGLSL120SubsetIn110;

   /**
    * Allow builtins as part of constant expressions. This was not allowed
    * until GLSL 1.20 this allows it everywhere.
    */
   GLboolean AllowGLSLBuiltinConstantExpression;

   /**
    * Allow some relaxation of GLSL ES shader restrictions. This encompasses
    * a number of relaxations to the ES shader rules.
    */
   GLboolean AllowGLSLRelaxedES;

   /**
    * Allow GLSL built-in variables to be redeclared verbatim
    */
   GLboolean AllowGLSLBuiltinVariableRedeclaration;

   /**
    * Allow GLSL interpolation qualifier mismatch across shader stages.
    */
   GLboolean AllowGLSLCrossStageInterpolationMismatch;

   /**
    * Allow creating a higher compat profile (version 3.1+) for apps that
    * request it. Be careful when adding that driconf option because some
    * features are unimplemented and might not work correctly.
    */
   GLboolean AllowHigherCompatVersion;

   /**
    * Allow extra tokens at end of preprocessor directives. The CTS now tests
    * to make sure these are not allowed. However, previously drivers would
    * allow them to exist and just issue a warning so some old applications
    * depend on this.
    */
   GLboolean AllowExtraPPTokens;

   /**
    * Force computing the absolute value for sqrt() and inversesqrt() to follow
    * D3D9 when apps rely on this behaviour.
    */
   GLboolean ForceGLSLAbsSqrt;

   /**
    * Types of variable to default initialized to zero. Supported values are:
    *   - 0: no zero initialization
    *   - 1: all shader variables and gl_FragColor are initialiazed to 0
    *   - 2: same as 1, but shader out variables are *not* initialized, while
    *        function out variables are now initialized.
    */
   GLchar GLSLZeroInit;

   /**
    * Force GL names reuse. Needed by SPECviewperf13.
    */
   GLboolean ForceGLNamesReuse;

   /**
    * Treat integer textures using GL_LINEAR filters as GL_NEAREST.
    */
   GLboolean ForceIntegerTexNearest;

   /**
    * Does the driver support real 32-bit integers?  (Otherwise, integers are
    * simulated via floats.)
    */
   GLboolean NativeIntegers;

   /**
    * Does VertexID count from zero or from base vertex?
    *
    * \note
    * If desktop GLSL 1.30 or GLSL ES 3.00 are not supported, this field is
    * ignored and need not be set.
    */
   bool VertexID_is_zero_based;

   /**
    * If the driver supports real 32-bit integers, what integer value should be
    * used for boolean true in uniform uploads?  (Usually 1 or ~0.)
    */
   GLuint UniformBooleanTrue;

   /**
    * Maximum amount of time, measured in nanseconds, that the server can wait.
    */
   GLuint64 MaxServerWaitTimeout;

   /** GL_EXT_provoking_vertex */
   GLboolean QuadsFollowProvokingVertexConvention;

   /** GL_ARB_viewport_array */
   GLenum16 LayerAndVPIndexProvokingVertex;

   /** OpenGL version 3.0 */
   GLbitfield ContextFlags;  /**< Ex: GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT */

   /** OpenGL version 3.2 */
   GLbitfield ProfileMask;   /**< Mask of CONTEXT_x_PROFILE_BIT */

   /** OpenGL version 4.4 */
   GLuint MaxVertexAttribStride;

   /** GL_EXT_transform_feedback */
   GLuint MaxTransformFeedbackBuffers;
   GLuint MaxTransformFeedbackSeparateComponents;
   GLuint MaxTransformFeedbackInterleavedComponents;
   GLuint MaxVertexStreams;

   /** GL_EXT_gpu_shader4 */
   GLint MinProgramTexelOffset, MaxProgramTexelOffset;

   /** GL_ARB_texture_gather */
   GLuint MinProgramTextureGatherOffset;
   GLuint MaxProgramTextureGatherOffset;
   GLuint MaxProgramTextureGatherComponents;

   /* GL_ARB_robustness */
   GLenum16 ResetStrategy;

   /* GL_KHR_robustness */
   GLboolean RobustAccess;

   /* GL_ARB_blend_func_extended */
   GLuint MaxDualSourceDrawBuffers;

   /**
    * Whether the implementation strips out and ignores texture borders.
    *
    * Many GPU hardware implementations don't support rendering with texture
    * borders and mipmapped textures.  (Note: not static border color, but the
    * old 1-pixel border around each edge).  Implementations then have to do
    * slow fallbacks to be correct, or just ignore the border and be fast but
    * wrong.  Setting the flag strips the border off of TexImage calls,
    * providing "fast but wrong" at significantly reduced driver complexity.
    *
    * Texture borders are deprecated in GL 3.0.
    **/
   GLboolean StripTextureBorder;

   /**
    * For drivers which can do a better job at eliminating unused uniforms
    * than the GLSL compiler.
    *
    * XXX Remove these as soon as a better solution is available.
    */
   GLboolean GLSLSkipStrictMaxUniformLimitCheck;

   /**
    * Whether gl_FragCoord, gl_PointCoord and gl_FrontFacing
    * are system values.
    **/
   bool GLSLFragCoordIsSysVal;
   bool GLSLPointCoordIsSysVal;
   bool GLSLFrontFacingIsSysVal;

   /**
    * Run the minimum amount of GLSL optimizations to be able to link
    * shaders optimally (eliminate dead varyings and uniforms) and just do
    * all the necessary lowering.
    */
   bool GLSLOptimizeConservatively;

   /**
    * Whether to call lower_const_arrays_to_uniforms() during linking.
    */
   bool GLSLLowerConstArrays;

   /**
    * True if gl_TessLevelInner/Outer[] in the TES should be inputs
    * (otherwise, they're system values).
    */
   bool GLSLTessLevelsAsInputs;

   /**
    * Always use the GetTransformFeedbackVertexCount() driver hook, rather
    * than passing the transform feedback object to the drawing function.
    */
   GLboolean AlwaysUseGetTransformFeedbackVertexCount;

   /** GL_ARB_map_buffer_alignment */
   GLuint MinMapBufferAlignment;

   /**
    * Disable varying packing.  This is out of spec, but potentially useful
    * for older platforms that supports a limited number of texture
    * indirections--on these platforms, unpacking the varyings in the fragment
    * shader increases the number of texture indirections by 1, which might
    * make some shaders not executable at all.
    *
    * Drivers that support transform feedback must set this value to GL_FALSE.
    */
   GLboolean DisableVaryingPacking;

   /**
    * Disable varying packing if used for transform feedback.  This is needed
    * for some drivers (e.g. Panfrost) where transform feedback requires
    * unpacked varyings.
    *
    * This variable is mutually exlusive with DisableVaryingPacking.
    */
   GLboolean DisableTransformFeedbackPacking;

   /**
    * UBOs and SSBOs can be packed tightly by the OpenGL implementation when
    * layout is set as shared (the default) or packed. However most Mesa drivers
    * just use STD140 for these layouts. This flag allows drivers to use STD430
    * for packed and shared layouts which allows arrays to be packed more
    * tightly.
    */
   bool UseSTD430AsDefaultPacking;

   /**
    * Should meaningful names be generated for compiler temporary variables?
    *
    * Generally, it is not useful to have the compiler generate "meaningful"
    * names for temporary variables that it creates.  This can, however, be a
    * useful debugging aid.  In Mesa debug builds or release builds when
    * MESA_GLSL is set at run-time, meaningful names will be generated.
    * Drivers can also force names to be generated by setting this field.
    * For example, the i965 driver may set it when INTEL_DEBUG=vs (to dump
    * vertex shader assembly) is set at run-time.
    */
   bool GenerateTemporaryNames;

   /*
    * Maximum value supported for an index in DrawElements and friends.
    *
    * This must be at least (1ull<<24)-1.  The default value is
    * (1ull<<32)-1.
    *
    * \since ES 3.0 or GL_ARB_ES3_compatibility
    * \sa _mesa_init_constants
    */
   GLuint64 MaxElementIndex;

   /**
    * Disable interpretation of line continuations (lines ending with a
    * backslash character ('\') in GLSL source.
    */
   GLboolean DisableGLSLLineContinuations;

   /** GL_ARB_texture_multisample */
   GLint MaxColorTextureSamples;
   GLint MaxDepthTextureSamples;
   GLint MaxIntegerSamples;

   /** GL_AMD_framebuffer_multisample_advanced */
   GLint MaxColorFramebufferSamples;
   GLint MaxColorFramebufferStorageSamples;
   GLint MaxDepthStencilFramebufferSamples;

   /* An array of supported MSAA modes allowing different sample
    * counts per attachment type.
    */
   struct {
      GLint NumColorSamples;
      GLint NumColorStorageSamples;
      GLint NumDepthStencilSamples;
   } SupportedMultisampleModes[40];
   GLint NumSupportedMultisampleModes;

   /** GL_ARB_shader_atomic_counters */
   GLuint MaxAtomicBufferBindings;
   GLuint MaxAtomicBufferSize;
   GLuint MaxCombinedAtomicBuffers;
   GLuint MaxCombinedAtomicCounters;

   /** GL_ARB_vertex_attrib_binding */
   GLint MaxVertexAttribRelativeOffset;
   GLint MaxVertexAttribBindings;

   /* GL_ARB_shader_image_load_store */
   GLuint MaxImageUnits;
   GLuint MaxCombinedShaderOutputResources;
   GLuint MaxImageSamples;
   GLuint MaxCombinedImageUniforms;

   /** GL_ARB_compute_shader */
   GLuint MaxComputeWorkGroupCount[3]; /* Array of x, y, z dimensions */
   GLuint MaxComputeWorkGroupSize[3]; /* Array of x, y, z dimensions */
   GLuint MaxComputeWorkGroupInvocations;
   GLuint MaxComputeSharedMemorySize;

   /** GL_ARB_compute_variable_group_size */
   GLuint MaxComputeVariableGroupSize[3]; /* Array of x, y, z dimensions */
   GLuint MaxComputeVariableGroupInvocations;

   /** GL_ARB_gpu_shader5 */
   GLfloat MinFragmentInterpolationOffset;
   GLfloat MaxFragmentInterpolationOffset;

   GLboolean FakeSWMSAA;

   /** GL_KHR_context_flush_control */
   GLenum16 ContextReleaseBehavior;

   struct gl_shader_compiler_options ShaderCompilerOptions[MESA_SHADER_STAGES];

   /** GL_ARB_tessellation_shader */
   GLuint MaxPatchVertices;
   GLuint MaxTessGenLevel;
   GLuint MaxTessPatchComponents;
   GLuint MaxTessControlTotalOutputComponents;
   bool LowerTessLevel; /**< Lower gl_TessLevel* from float[n] to vecn? */
   bool PrimitiveRestartForPatches;
   bool LowerCsDerivedVariables;    /**< Lower gl_GlobalInvocationID and
                                     *   gl_LocalInvocationIndex based on
                                     *   other builtin variables. */

   /** GL_OES_primitive_bounding_box */
   bool NoPrimitiveBoundingBoxOutput;

   /** GL_ARB_sparse_buffer */
   GLuint SparseBufferPageSize;


   /** When drivers are OK with mapped buffers during draw and other calls. */
   bool AllowMappedBuffersDuringExecution;

   /**
    * Whether buffer creation, unsynchronized mapping, unmapping, and
    * deletion is thread-safe.
    */
   bool BufferCreateMapUnsynchronizedThreadSafe;

   /** GL_ARB_get_program_binary */
   GLuint NumProgramBinaryFormats;

   /** GL_NV_conservative_raster */
   GLuint MaxSubpixelPrecisionBiasBits;

   /** GL_NV_conservative_raster_dilate */
   GLfloat ConservativeRasterDilateRange[2];
   GLfloat ConservativeRasterDilateGranularity;

   /** Is the drivers uniform storage packed or padded to 16 bytes. */
   bool PackedDriverUniformStorage;

   /** Does the driver make use of the NIR based GLSL linker */
   bool UseNIRGLSLLinker;

   /** Wether or not glBitmap uses red textures rather than alpha */
   bool BitmapUsesRed;

   /** Whether the vertex buffer offset is a signed 32-bit integer. */
   bool VertexBufferOffsetIsInt32;

   /** Whether the driver can handle MultiDrawElements with non-VBO indices. */
   bool MultiDrawWithUserIndices;

   /** Whether out-of-order draw (Begin/End) optimizations are allowed. */
   bool AllowDrawOutOfOrder;

   /** Whether to allow the fast path for frequently updated VAOs. */
   bool AllowDynamicVAOFastPath;

   /** Whether the driver can support primitive restart with a fixed index.
    * This is essentially a subset of NV_primitive_restart with enough support
    * to be able to enable GLES 3.1. Some hardware can support this but not the
    * full NV extension with arbitrary restart indices.
    */
   bool PrimitiveRestartFixedIndex;

   /** GL_ARB_gl_spirv */
   struct spirv_supported_capabilities SpirVCapabilities;

   /** GL_ARB_spirv_extensions */
   struct spirv_supported_extensions *SpirVExtensions;

   char *VendorOverride;

   /** Buffer size used to upload vertices from glBegin/glEnd. */
   unsigned glBeginEndBufferSize;

   /** Whether the driver doesn't want x/y/width/height clipped based on src size
    *  when doing a copy texture operation (eg: may want out-of-bounds reads that
    *  produce 0 instead of leaving the texture content undefined).
    */
   bool NoClippingOnCopyTex;
};

/**
 * Enable flag for each OpenGL extension.  Different device drivers will
 * enable different extensions at runtime.
 */
struct gl_extensions
{
   GLboolean dummy;  /* don't remove this! */
   GLboolean dummy_true;  /* Set true by _mesa_init_extensions(). */
   GLboolean dummy_false; /* Set false by _mesa_init_extensions(). */
   GLboolean ANGLE_texture_compression_dxt;
   GLboolean ARB_ES2_compatibility;
   GLboolean ARB_ES3_compatibility;
   GLboolean ARB_ES3_1_compatibility;
   GLboolean ARB_ES3_2_compatibility;
   GLboolean ARB_arrays_of_arrays;
   GLboolean ARB_base_instance;
   GLboolean ARB_bindless_texture;
   GLboolean ARB_blend_func_extended;
   GLboolean ARB_buffer_storage;
   GLboolean ARB_clear_texture;
   GLboolean ARB_clip_control;
   GLboolean ARB_color_buffer_float;
   GLboolean ARB_compatibility;
   GLboolean ARB_compute_shader;
   GLboolean ARB_compute_variable_group_size;
   GLboolean ARB_conditional_render_inverted;
   GLboolean ARB_conservative_depth;
   GLboolean ARB_copy_image;
   GLboolean ARB_cull_distance;
   GLboolean EXT_color_buffer_half_float;
   GLboolean ARB_depth_buffer_float;
   GLboolean ARB_depth_clamp;
   GLboolean ARB_depth_texture;
   GLboolean ARB_derivative_control;
   GLboolean ARB_draw_buffers_blend;
   GLboolean ARB_draw_elements_base_vertex;
   GLboolean ARB_draw_indirect;
   GLboolean ARB_draw_instanced;
   GLboolean ARB_fragment_coord_conventions;
   GLboolean ARB_fragment_layer_viewport;
   GLboolean ARB_fragment_program;
   GLboolean ARB_fragment_program_shadow;
   GLboolean ARB_fragment_shader;
   GLboolean ARB_framebuffer_no_attachments;
   GLboolean ARB_framebuffer_object;
   GLboolean ARB_fragment_shader_interlock;
   GLboolean ARB_enhanced_layouts;
   GLboolean ARB_explicit_attrib_location;
   GLboolean ARB_explicit_uniform_location;
   GLboolean ARB_gl_spirv;
   GLboolean ARB_gpu_shader5;
   GLboolean ARB_gpu_shader_fp64;
   GLboolean ARB_gpu_shader_int64;
   GLboolean ARB_half_float_vertex;
   GLboolean ARB_indirect_parameters;
   GLboolean ARB_instanced_arrays;
   GLboolean ARB_internalformat_query;
   GLboolean ARB_internalformat_query2;
   GLboolean ARB_map_buffer_range;
   GLboolean ARB_occlusion_query;
   GLboolean ARB_occlusion_query2;
   GLboolean ARB_pipeline_statistics_query;
   GLboolean ARB_point_sprite;
   GLboolean ARB_polygon_offset_clamp;
   GLboolean ARB_post_depth_coverage;
   GLboolean ARB_query_buffer_object;
   GLboolean ARB_robust_buffer_access_behavior;
   GLboolean ARB_sample_locations;
   GLboolean ARB_sample_shading;
   GLboolean ARB_seamless_cube_map;
   GLboolean ARB_shader_atomic_counter_ops;
   GLboolean ARB_shader_atomic_counters;
   GLboolean ARB_shader_ballot;
   GLboolean ARB_shader_bit_encoding;
   GLboolean ARB_shader_clock;
   GLboolean ARB_shader_draw_parameters;
   GLboolean ARB_shader_group_vote;
   GLboolean ARB_shader_image_load_store;
   GLboolean ARB_shader_image_size;
   GLboolean ARB_shader_precision;
   GLboolean ARB_shader_stencil_export;
   GLboolean ARB_shader_storage_buffer_object;
   GLboolean ARB_shader_texture_image_samples;
   GLboolean ARB_shader_texture_lod;
   GLboolean ARB_shader_viewport_layer_array;
   GLboolean ARB_shading_language_packing;
   GLboolean ARB_shading_language_420pack;
   GLboolean ARB_shadow;
   GLboolean ARB_sparse_buffer;
   GLboolean ARB_stencil_texturing;
   GLboolean ARB_spirv_extensions;
   GLboolean ARB_sync;
   GLboolean ARB_tessellation_shader;
   GLboolean ARB_texture_border_clamp;
   GLboolean ARB_texture_buffer_object;
   GLboolean ARB_texture_buffer_object_rgb32;
   GLboolean ARB_texture_buffer_range;
   GLboolean ARB_texture_compression_bptc;
   GLboolean ARB_texture_compression_rgtc;
   GLboolean ARB_texture_cube_map;
   GLboolean ARB_texture_cube_map_array;
   GLboolean ARB_texture_env_combine;
   GLboolean ARB_texture_env_crossbar;
   GLboolean ARB_texture_env_dot3;
   GLboolean ARB_texture_filter_anisotropic;
   GLboolean ARB_texture_float;
   GLboolean ARB_texture_gather;
   GLboolean ARB_texture_mirror_clamp_to_edge;
   GLboolean ARB_texture_multisample;
   GLboolean ARB_texture_non_power_of_two;
   GLboolean ARB_texture_stencil8;
   GLboolean ARB_texture_query_levels;
   GLboolean ARB_texture_query_lod;
   GLboolean ARB_texture_rg;
   GLboolean ARB_texture_rgb10_a2ui;
   GLboolean ARB_texture_view;
   GLboolean ARB_timer_query;
   GLboolean ARB_transform_feedback2;
   GLboolean ARB_transform_feedback3;
   GLboolean ARB_transform_feedback_instanced;
   GLboolean ARB_transform_feedback_overflow_query;
   GLboolean ARB_uniform_buffer_object;
   GLboolean ARB_vertex_attrib_64bit;
   GLboolean ARB_vertex_program;
   GLboolean ARB_vertex_shader;
   GLboolean ARB_vertex_type_10f_11f_11f_rev;
   GLboolean ARB_vertex_type_2_10_10_10_rev;
   GLboolean ARB_viewport_array;
   GLboolean EXT_blend_color;
   GLboolean EXT_blend_equation_separate;
   GLboolean EXT_blend_func_separate;
   GLboolean EXT_blend_minmax;
   GLboolean EXT_demote_to_helper_invocation;
   GLboolean EXT_depth_bounds_test;
   GLboolean EXT_disjoint_timer_query;
   GLboolean EXT_draw_buffers2;
   GLboolean EXT_EGL_image_storage;
   GLboolean EXT_float_blend;
   GLboolean EXT_framebuffer_multisample;
   GLboolean EXT_framebuffer_multisample_blit_scaled;
   GLboolean EXT_framebuffer_sRGB;
   GLboolean EXT_gpu_program_parameters;
   GLboolean EXT_gpu_shader4;
   GLboolean EXT_memory_object;
   GLboolean EXT_memory_object_fd;
   GLboolean EXT_multisampled_render_to_texture;
   GLboolean EXT_packed_float;
   GLboolean EXT_pixel_buffer_object;
   GLboolean EXT_point_parameters;
   GLboolean EXT_provoking_vertex;
   GLboolean EXT_render_snorm;
   GLboolean EXT_semaphore;
   GLboolean EXT_semaphore_fd;
   GLboolean EXT_shader_image_load_formatted;
   GLboolean EXT_shader_image_load_store;
   GLboolean EXT_shader_integer_mix;
   GLboolean EXT_shader_samples_identical;
   GLboolean EXT_sRGB;
   GLboolean EXT_stencil_two_side;
   GLboolean EXT_texture_array;
   GLboolean EXT_texture_buffer_object;
   GLboolean EXT_texture_compression_latc;
   GLboolean EXT_texture_compression_s3tc;
   GLboolean EXT_texture_compression_s3tc_srgb;
   GLboolean EXT_texture_env_dot3;
   GLboolean EXT_texture_filter_anisotropic;
   GLboolean EXT_texture_integer;
   GLboolean EXT_texture_mirror_clamp;
   GLboolean EXT_texture_norm16;
   GLboolean EXT_texture_shadow_lod;
   GLboolean EXT_texture_shared_exponent;
   GLboolean EXT_texture_snorm;
   GLboolean EXT_texture_sRGB;
   GLboolean EXT_texture_sRGB_R8;
   GLboolean EXT_texture_sRGB_decode;
   GLboolean EXT_texture_swizzle;
   GLboolean EXT_texture_type_2_10_10_10_REV;
   GLboolean EXT_transform_feedback;
   GLboolean EXT_timer_query;
   GLboolean EXT_vertex_array_bgra;
   GLboolean EXT_window_rectangles;
   GLboolean OES_copy_image;
   GLboolean OES_primitive_bounding_box;
   GLboolean OES_sample_variables;
   GLboolean OES_standard_derivatives;
   GLboolean OES_texture_buffer;
   GLboolean OES_texture_cube_map_array;
   GLboolean OES_texture_view;
   GLboolean OES_viewport_array;
   /* vendor extensions */
   GLboolean AMD_compressed_ATC_texture;
   GLboolean AMD_framebuffer_multisample_advanced;
   GLboolean AMD_depth_clamp_separate;
   GLboolean AMD_performance_monitor;
   GLboolean AMD_pinned_memory;
   GLboolean AMD_seamless_cubemap_per_texture;
   GLboolean AMD_vertex_shader_layer;
   GLboolean AMD_vertex_shader_viewport_index;
   GLboolean ANDROID_extension_pack_es31a;
   GLboolean APPLE_object_purgeable;
   GLboolean ATI_meminfo;
   GLboolean ATI_texture_compression_3dc;
   GLboolean ATI_texture_mirror_once;
   GLboolean ATI_texture_env_combine3;
   GLboolean ATI_fragment_shader;
   GLboolean GREMEDY_string_marker;
   GLboolean INTEL_blackhole_render;
   GLboolean INTEL_conservative_rasterization;
   GLboolean INTEL_performance_query;
   GLboolean INTEL_shader_atomic_float_minmax;
   GLboolean INTEL_shader_integer_functions2;
   GLboolean KHR_blend_equation_advanced;
   GLboolean KHR_blend_equation_advanced_coherent;
   GLboolean KHR_robustness;
   GLboolean KHR_texture_compression_astc_hdr;
   GLboolean KHR_texture_compression_astc_ldr;
   GLboolean KHR_texture_compression_astc_sliced_3d;
   GLboolean MESA_framebuffer_flip_y;
   GLboolean MESA_tile_raster_order;
   GLboolean EXT_shader_framebuffer_fetch;
   GLboolean EXT_shader_framebuffer_fetch_non_coherent;
   GLboolean MESA_shader_integer_functions;
   GLboolean MESA_ycbcr_texture;
   GLboolean NV_alpha_to_coverage_dither_control;
   GLboolean NV_compute_shader_derivatives;
   GLboolean NV_conditional_render;
   GLboolean NV_copy_depth_to_color;
   GLboolean NV_copy_image;
   GLboolean NV_fill_rectangle;
   GLboolean NV_fog_distance;
   GLboolean NV_point_sprite;
   GLboolean NV_primitive_restart;
   GLboolean NV_shader_atomic_float;
   GLboolean NV_shader_atomic_int64;
   GLboolean NV_texture_barrier;
   GLboolean NV_texture_env_combine4;
   GLboolean NV_texture_rectangle;
   GLboolean NV_vdpau_interop;
   GLboolean NV_conservative_raster;
   GLboolean NV_conservative_raster_dilate;
   GLboolean NV_conservative_raster_pre_snap_triangles;
   GLboolean NV_conservative_raster_pre_snap;
   GLboolean NV_viewport_array2;
   GLboolean NV_viewport_swizzle;
   GLboolean NVX_gpu_memory_info;
   GLboolean TDFX_texture_compression_FXT1;
   GLboolean OES_EGL_image;
   GLboolean OES_draw_texture;
   GLboolean OES_depth_texture_cube_map;
   GLboolean OES_EGL_image_external;
   GLboolean OES_texture_float;
   GLboolean OES_texture_float_linear;
   GLboolean OES_texture_half_float;
   GLboolean OES_texture_half_float_linear;
   GLboolean OES_compressed_ETC1_RGB8_texture;
   GLboolean OES_geometry_shader;
   GLboolean OES_texture_compression_astc;
   GLboolean extension_sentinel;
   /** The extension string */
   const GLubyte *String;
   /** Number of supported extensions */
   GLuint Count;
   /**
    * The context version which extension helper functions compare against.
    * By default, the value is equal to ctx->Version. This changes to ~0
    * while meta is in progress.
    */
   GLubyte Version;
};

/**
 * \name Bits for image transfer operations
 * \sa __struct gl_contextRec::ImageTransferState.
 */
/*@{*/
#define IMAGE_SCALE_BIAS_BIT                      0x1
#define IMAGE_SHIFT_OFFSET_BIT                    0x2
#define IMAGE_MAP_COLOR_BIT                       0x4
#define IMAGE_CLAMP_BIT                           0x800

/** Pixel Transfer ops */
#define IMAGE_BITS (IMAGE_SCALE_BIAS_BIT | \
                    IMAGE_SHIFT_OFFSET_BIT | \
                    IMAGE_MAP_COLOR_BIT)

/**
 * \name Bits to indicate what state has changed.
 */
/*@{*/
#define _NEW_MODELVIEW         (1u << 0)   /**< gl_context::ModelView */
#define _NEW_PROJECTION        (1u << 1)   /**< gl_context::Projection */
#define _NEW_TEXTURE_MATRIX    (1u << 2)   /**< gl_context::TextureMatrix */
#define _NEW_COLOR             (1u << 3)   /**< gl_context::Color */
#define _NEW_DEPTH             (1u << 4)   /**< gl_context::Depth */
/* gap */
#define _NEW_FOG               (1u << 6)   /**< gl_context::Fog */
#define _NEW_HINT              (1u << 7)   /**< gl_context::Hint */
#define _NEW_LIGHT             (1u << 8)   /**< gl_context::Light */
#define _NEW_LINE              (1u << 9)   /**< gl_context::Line */
#define _NEW_PIXEL             (1u << 10)  /**< gl_context::Pixel */
#define _NEW_POINT             (1u << 11)  /**< gl_context::Point */
#define _NEW_POLYGON           (1u << 12)  /**< gl_context::Polygon */
#define _NEW_POLYGONSTIPPLE    (1u << 13)  /**< gl_context::PolygonStipple */
#define _NEW_SCISSOR           (1u << 14)  /**< gl_context::Scissor */
#define _NEW_STENCIL           (1u << 15)  /**< gl_context::Stencil */
#define _NEW_TEXTURE_OBJECT    (1u << 16)  /**< gl_context::Texture (bindings only) */
#define _NEW_TRANSFORM         (1u << 17)  /**< gl_context::Transform */
#define _NEW_VIEWPORT          (1u << 18)  /**< gl_context::Viewport */
#define _NEW_TEXTURE_STATE     (1u << 19)  /**< gl_context::Texture (states only) */
/* gap */
#define _NEW_RENDERMODE        (1u << 21)  /**< gl_context::RenderMode, etc */
#define _NEW_BUFFERS           (1u << 22)  /**< gl_context::Visual, DrawBuffer, */
#define _NEW_CURRENT_ATTRIB    (1u << 23)  /**< gl_context::Current */
#define _NEW_MULTISAMPLE       (1u << 24)  /**< gl_context::Multisample */
#define _NEW_TRACK_MATRIX      (1u << 25)  /**< gl_context::VertexProgram */
#define _NEW_PROGRAM           (1u << 26)  /**< New program/shader state */
#define _NEW_PROGRAM_CONSTANTS (1u << 27)
/* gap */
#define _NEW_FRAG_CLAMP        (1u << 29)
/* gap, re-use for core Mesa state only; use ctx->DriverFlags otherwise */
#define _NEW_VARYING_VP_INPUTS (1u << 31) /**< gl_context::varying_vp_inputs */
#define _NEW_ALL ~0
/*@}*/

/**
 * Composite state flags
 */
/*@{*/
#define _NEW_TEXTURE   (_NEW_TEXTURE_OBJECT | _NEW_TEXTURE_STATE)

#define _MESA_NEW_NEED_EYE_COORDS         (_NEW_LIGHT |		\
                                           _NEW_TEXTURE_STATE |	\
                                           _NEW_POINT |		\
                                           _NEW_PROGRAM |	\
                                           _NEW_MODELVIEW)

#define _MESA_NEW_SEPARATE_SPECULAR        (_NEW_LIGHT | \
                                            _NEW_FOG | \
                                            _NEW_PROGRAM)

/*@}*/

/* This has to be included here. */
#include "dd.h"

/** Opaque declaration of display list payload data type */
union gl_dlist_node;

/**
 * State used during display list compilation and execution.
 */

/* Compiler-only context: rendering/dispatch/VBO state is intentionally absent. */
struct gl_context
{
   gl_api API;
   struct dd_function_table Driver;
   struct gl_constants Const;
   struct gl_extensions Extensions;
   GLuint Version;
};

#ifndef NDEBUG
extern int MESA_VERBOSE;
extern int MESA_DEBUG_FLAGS;
#else
# define MESA_VERBOSE 0
# define MESA_DEBUG_FLAGS 0
#endif

/** The MESA_VERBOSE var is a bitmask of these flags */
enum _verbose
{
   VERBOSE_VARRAY		= 0x0001,
   VERBOSE_TEXTURE		= 0x0002,
   VERBOSE_MATERIAL		= 0x0004,
   VERBOSE_PIPELINE		= 0x0008,
   VERBOSE_DRIVER		= 0x0010,
   VERBOSE_STATE		= 0x0020,
   VERBOSE_API			= 0x0040,
   VERBOSE_DISPLAY_LIST		= 0x0100,
   VERBOSE_LIGHTING		= 0x0200,
   VERBOSE_PRIMS		= 0x0400,
   VERBOSE_VERTS		= 0x0800,
   VERBOSE_DISASSEM		= 0x1000,
   VERBOSE_DRAW                 = 0x2000,
   VERBOSE_SWAPBUFFERS          = 0x4000
};

/** The MESA_DEBUG_FLAGS var is a bitmask of these flags */
enum _debug
{
   DEBUG_SILENT                 = (1 << 0),
   DEBUG_ALWAYS_FLUSH		= (1 << 1),
   DEBUG_INCOMPLETE_TEXTURE     = (1 << 2),
   DEBUG_INCOMPLETE_FBO         = (1 << 3),
   DEBUG_CONTEXT                = (1 << 4)
};

#ifdef __cplusplus
}
#endif

#endif /* MTYPES_H */
