/*
 * Mesa 3-D graphics library
 *
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


#ifndef META_H
#define META_H

#include "main/mtypes.h"

/**
 * \name Flags for meta operations
 * \{
 *
 * These flags are passed to _mesa_meta_begin().
 */
#define MESA_META_ALL                      ~0x0
#define MESA_META_ALPHA_TEST                0x1
#define MESA_META_BLEND                     0x2  /**< includes logicop */
#define MESA_META_COLOR_MASK                0x4
#define MESA_META_DEPTH_TEST                0x8
#define MESA_META_FOG                      0x10
#define MESA_META_PIXEL_STORE              0x20
#define MESA_META_PIXEL_TRANSFER           0x40
#define MESA_META_RASTERIZATION            0x80
#define MESA_META_SCISSOR                 0x100
#define MESA_META_SHADER                  0x200
#define MESA_META_STENCIL_TEST            0x400
#define MESA_META_TRANSFORM               0x800 /**< modelview/projection matrix state */
#define MESA_META_TEXTURE                0x1000
#define MESA_META_VERTEX                 0x2000
#define MESA_META_VIEWPORT               0x4000
#define MESA_META_CLAMP_FRAGMENT_COLOR   0x8000
#define MESA_META_CLAMP_VERTEX_COLOR    0x10000
#define MESA_META_CONDITIONAL_RENDER    0x20000
#define MESA_META_CLIP                  0x40000
#define MESA_META_SELECT_FEEDBACK       0x80000
#define MESA_META_MULTISAMPLE          0x100000
#define MESA_META_FRAMEBUFFER_SRGB     0x200000
#define MESA_META_OCCLUSION_QUERY      0x400000
#define MESA_META_DRAW_BUFFERS         0x800000
/**\}*/

/**
 * State which we may save/restore across meta ops.
 * XXX this may be incomplete...
 */
struct save_state
{
   GLbitfield SavedState;  /**< bitmask of MESA_META_* flags */

   /* Always saved/restored with meta. */
   gl_api API;

   /** MESA_META_CLEAR (and others?) */
   struct gl_query_object *CurrentOcclusionObject;

   /** MESA_META_ALPHA_TEST */
   GLboolean AlphaEnabled;
   GLenum AlphaFunc;
   GLclampf AlphaRef;

   /** MESA_META_BLEND */
   GLbitfield BlendEnabled;
   GLboolean ColorLogicOpEnabled;

   /** MESA_META_COLOR_MASK */
   GLubyte ColorMask[MAX_DRAW_BUFFERS][4];

   /** MESA_META_DEPTH_TEST */
   struct gl_depthbuffer_attrib Depth;

   /** MESA_META_FOG */
   GLboolean Fog;

   /** MESA_META_PIXEL_STORE */
   struct gl_pixelstore_attrib Pack, Unpack;

   /** MESA_META_PIXEL_TRANSFER */
   GLfloat RedBias, RedScale;
   GLfloat GreenBias, GreenScale;
   GLfloat BlueBias, BlueScale;
   GLfloat AlphaBias, AlphaScale;
   GLfloat DepthBias, DepthScale;
   GLboolean MapColorFlag;

   /** MESA_META_RASTERIZATION */
   GLenum FrontPolygonMode, BackPolygonMode;
   GLboolean PolygonOffset;
   GLboolean PolygonSmooth;
   GLboolean PolygonStipple;
   GLboolean PolygonCull;

   /** MESA_META_SCISSOR */
   struct gl_scissor_attrib Scissor;

   /** MESA_META_SHADER */
   GLboolean VertexProgramEnabled;
   struct gl_vertex_program *VertexProgram;
   GLboolean FragmentProgramEnabled;
   struct gl_fragment_program *FragmentProgram;
   GLboolean ATIFragmentShaderEnabled;
   struct gl_shader_program *Shader[MESA_SHADER_STAGES];
   struct gl_shader_program *ActiveShader;
   struct gl_pipeline_object   *Pipeline;

   /** MESA_META_STENCIL_TEST */
   struct gl_stencil_attrib Stencil;

   /** MESA_META_TRANSFORM */
   GLenum MatrixMode;
   GLfloat ModelviewMatrix[16];
   GLfloat ProjectionMatrix[16];
   GLfloat TextureMatrix[16];

   /** MESA_META_CLIP */
   GLbitfield ClipPlanesEnabled;

   /** MESA_META_TEXTURE */
   GLuint ActiveUnit;
   GLuint ClientActiveUnit;
   /** for unit[0] only */
   struct gl_texture_object *CurrentTexture[NUM_TEXTURE_TARGETS];
   /** mask of TEXTURE_2D_BIT, etc */
   GLbitfield TexEnabled[MAX_TEXTURE_UNITS];
   GLbitfield TexGenEnabled[MAX_TEXTURE_UNITS];
   GLuint EnvMode;  /* unit[0] only */

   /** MESA_META_VERTEX */
   struct gl_vertex_array_object *VAO;
   struct gl_buffer_object *ArrayBufferObj;

   /** MESA_META_VIEWPORT */
   GLfloat ViewportX, ViewportY, ViewportW, ViewportH;
   GLclampd DepthNear, DepthFar;

   /** MESA_META_CLAMP_FRAGMENT_COLOR */
   GLenum ClampFragmentColor;

   /** MESA_META_CLAMP_VERTEX_COLOR */
   GLenum ClampVertexColor;

   /** MESA_META_CONDITIONAL_RENDER */
   struct gl_query_object *CondRenderQuery;
   GLenum CondRenderMode;

   /** MESA_META_SELECT_FEEDBACK */
   GLenum RenderMode;
   struct gl_selection Select;
   struct gl_feedback Feedback;

   /** MESA_META_MULTISAMPLE */
   struct gl_multisample_attrib Multisample;

   /** MESA_META_FRAMEBUFFER_SRGB */
   GLboolean sRGBEnabled;

   /** Miscellaneous (always disabled) */
   GLboolean Lighting;
   GLboolean RasterDiscard;
   GLboolean TransformFeedbackNeedsResume;

   GLuint DrawBufferName, ReadBufferName, RenderbufferName;

   /** MESA_META_DRAW_BUFFERS */
   GLenum ColorDrawBuffers[MAX_DRAW_BUFFERS];
};

/**
 * Temporary texture used for glBlitFramebuffer, glDrawPixels, etc.
 * This is currently shared by all the meta ops.  But we could create a
 * separate one for each of glDrawPixel, glBlitFramebuffer, glCopyPixels, etc.
 */
struct temp_texture
{
   GLuint TexObj;
   GLenum Target;         /**< GL_TEXTURE_2D or GL_TEXTURE_RECTANGLE */
   GLsizei MinSize;       /**< Min texture size to allocate */
   GLsizei MaxSize;       /**< Max possible texture size */
   GLboolean NPOT;        /**< Non-power of two size OK? */
   GLsizei Width, Height; /**< Current texture size */
   GLenum IntFormat;
   GLfloat Sright, Ttop;  /**< right, top texcoords */
};

/**
 * State for GLSL texture sampler which is used to generate fragment
 * shader in _mesa_meta_generate_mipmap().
 */
struct blit_shader {
   const char *type;
   const char *func;
   const char *texcoords;
   GLuint shader_prog;
};

/**
 * Table of all sampler types and shaders for accessing them.
 */
struct blit_shader_table {
   struct blit_shader sampler_1d;
   struct blit_shader sampler_2d;
   struct blit_shader sampler_3d;
   struct blit_shader sampler_rect;
   struct blit_shader sampler_cubemap;
   struct blit_shader sampler_1d_array;
   struct blit_shader sampler_2d_array;
   struct blit_shader sampler_cubemap_array;
};

/**
 * Indices in the blit_state->msaa_shaders[] array
 *
 * Note that setup_glsl_msaa_blit_shader() assumes that the _INT enums are one
 * more than the non-_INT version and _UINT is one beyond that.
 */
enum blit_msaa_shader {
   BLIT_MSAA_SHADER_2D_MULTISAMPLE_RESOLVE,
   BLIT_MSAA_SHADER_2D_MULTISAMPLE_RESOLVE_INT,
   BLIT_MSAA_SHADER_2D_MULTISAMPLE_RESOLVE_UINT,
   BLIT_MSAA_SHADER_2D_MULTISAMPLE_COPY,
   BLIT_MSAA_SHADER_2D_MULTISAMPLE_COPY_INT,
   BLIT_MSAA_SHADER_2D_MULTISAMPLE_COPY_UINT,
   BLIT_MSAA_SHADER_2D_MULTISAMPLE_DEPTH_RESOLVE,
   BLIT_MSAA_SHADER_2D_MULTISAMPLE_DEPTH_COPY,
   BLIT_MSAA_SHADER_2D_MULTISAMPLE_ARRAY_RESOLVE,
   BLIT_MSAA_SHADER_2D_MULTISAMPLE_ARRAY_RESOLVE_INT,
   BLIT_MSAA_SHADER_2D_MULTISAMPLE_ARRAY_RESOLVE_UINT,
   BLIT_MSAA_SHADER_2D_MULTISAMPLE_ARRAY_COPY,
   BLIT_MSAA_SHADER_2D_MULTISAMPLE_ARRAY_COPY_INT,
   BLIT_MSAA_SHADER_2D_MULTISAMPLE_ARRAY_COPY_UINT,
   BLIT_MSAA_SHADER_2D_MULTISAMPLE_ARRAY_DEPTH_RESOLVE,
   BLIT_MSAA_SHADER_2D_MULTISAMPLE_ARRAY_DEPTH_COPY,
   BLIT_MSAA_SHADER_COUNT,
};

/**
 * State for glBlitFramebufer()
 */
struct blit_state
{
   GLuint VAO;
   GLuint VBO;
   struct blit_shader_table shaders;
   GLuint msaa_shaders[BLIT_MSAA_SHADER_COUNT];
   struct temp_texture depthTex;
   bool no_ctsi_fallback;
};

struct fb_tex_blit_state
{
   GLint baseLevelSave, maxLevelSave;
   GLuint sampler, samplerSave, stencilSamplingSave;
   GLuint tempTex;
};


/**
 * State for glClear()
 */
struct clear_state
{
   GLuint VAO;
   GLuint VBO;
   GLuint ShaderProg;
   GLint ColorLocation;
   GLint LayerLocation;

   GLuint IntegerShaderProg;
   GLint IntegerColorLocation;
   GLint IntegerLayerLocation;
};


/**
 * State for glCopyPixels()
 */
struct copypix_state
{
   GLuint VAO;
   GLuint VBO;
};


/**
 * State for glDrawPixels()
 */
struct drawpix_state
{
   GLuint VAO;
   GLuint VBO;

   GLuint StencilFP;  /**< Fragment program for drawing stencil images */
   GLuint DepthFP;  /**< Fragment program for drawing depth images */
};


/**
 * State for glBitmap()
 */
struct bitmap_state
{
   GLuint VAO;
   GLuint VBO;
   struct temp_texture Tex;  /**< separate texture from other meta ops */
};

/**
 * State for _mesa_meta_generate_mipmap()
 */
struct gen_mipmap_state
{
   GLuint VAO;
   GLuint VBO;
   GLuint FBO;
   GLuint Sampler;

   struct blit_shader_table shaders;
};

/**
 * State for texture decompression
 */
struct decompress_state
{
   GLuint VAO;
   GLuint VBO, FBO, RBO, Sampler;
   GLint Width, Height;

   struct blit_shader_table shaders;
};

/**
 * State for glDrawTex()
 */
struct drawtex_state
{
   GLuint VAO;
   GLuint VBO;
};

#define MAX_META_OPS_DEPTH      8
/**
 * All per-context meta state.
 */
struct gl_meta_state
{
   /** Stack of state saved during meta-ops */
   struct save_state Save[MAX_META_OPS_DEPTH];
   /** Save stack depth */
   GLuint SaveStackDepth;

   struct temp_texture TempTex;

   struct blit_state Blit;    /**< For _mesa_meta_BlitFramebuffer() */
   struct clear_state Clear;  /**< For _mesa_meta_Clear() */
   struct copypix_state CopyPix;  /**< For _mesa_meta_CopyPixels() */
   struct drawpix_state DrawPix;  /**< For _mesa_meta_DrawPixels() */
   struct bitmap_state Bitmap;    /**< For _mesa_meta_Bitmap() */
   struct gen_mipmap_state Mipmap;    /**< For _mesa_meta_GenerateMipmap() */
   struct decompress_state Decompress;  /**< For texture decompression */
   struct drawtex_state DrawTex;  /**< For _mesa_meta_DrawTex() */
};

struct vertex {
   GLfloat x, y, z, tex[4];
   GLfloat r, g, b, a;
};

extern void
_mesa_meta_init(struct gl_context *ctx);

extern void
_mesa_meta_free(struct gl_context *ctx);

extern void
_mesa_meta_begin(struct gl_context *ctx, GLbitfield state);

extern void
_mesa_meta_end(struct gl_context *ctx);

extern GLboolean
_mesa_meta_in_progress(struct gl_context *ctx);

extern void
_mesa_meta_fb_tex_blit_begin(const struct gl_context *ctx,
                             struct fb_tex_blit_state *blit);

extern void
_mesa_meta_fb_tex_blit_end(struct gl_context *ctx, GLenum target,
                           struct fb_tex_blit_state *blit);

extern GLboolean
_mesa_meta_bind_rb_as_tex_image(struct gl_context *ctx,
                                struct gl_renderbuffer *rb,
                                GLuint *tex,
                                struct gl_texture_object **texObj,
                                GLenum *target);

GLuint
_mesa_meta_setup_sampler(struct gl_context *ctx,
                         const struct gl_texture_object *texObj,
                         GLenum target, GLenum filter, GLuint srcLevel);

extern GLbitfield
_mesa_meta_BlitFramebuffer(struct gl_context *ctx,
                           GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                           GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                           GLbitfield mask, GLenum filter);

extern void
_mesa_meta_and_swrast_BlitFramebuffer(struct gl_context *ctx,
                                      GLint srcX0, GLint srcY0,
                                      GLint srcX1, GLint srcY1,
                                      GLint dstX0, GLint dstY0,
                                      GLint dstX1, GLint dstY1,
                                      GLbitfield mask, GLenum filter);

extern void
_mesa_meta_Clear(struct gl_context *ctx, GLbitfield buffers);

extern void
_mesa_meta_glsl_Clear(struct gl_context *ctx, GLbitfield buffers);

extern void
_mesa_meta_CopyPixels(struct gl_context *ctx, GLint srcx, GLint srcy,
                      GLsizei width, GLsizei height,
                      GLint dstx, GLint dsty, GLenum type);

extern void
_mesa_meta_DrawPixels(struct gl_context *ctx,
                      GLint x, GLint y, GLsizei width, GLsizei height,
                      GLenum format, GLenum type,
                      const struct gl_pixelstore_attrib *unpack,
                      const GLvoid *pixels);

extern void
_mesa_meta_Bitmap(struct gl_context *ctx,
                  GLint x, GLint y, GLsizei width, GLsizei height,
                  const struct gl_pixelstore_attrib *unpack,
                  const GLubyte *bitmap);

extern void
_mesa_meta_GenerateMipmap(struct gl_context *ctx, GLenum target,
                          struct gl_texture_object *texObj);

extern void
_mesa_meta_CopyTexSubImage(struct gl_context *ctx, GLuint dims,
                           struct gl_texture_image *texImage,
                           GLint xoffset, GLint yoffset, GLint slice,
                           struct gl_renderbuffer *rb,
                           GLint x, GLint y,
                           GLsizei width, GLsizei height);

extern void
_mesa_meta_GetTexImage(struct gl_context *ctx,
                       GLenum format, GLenum type, GLvoid *pixels,
                       struct gl_texture_image *texImage);

extern void
_mesa_meta_DrawTex(struct gl_context *ctx, GLfloat x, GLfloat y, GLfloat z,
                   GLfloat width, GLfloat height);

/* meta-internal functions */
GLuint
_mesa_meta_compile_shader_with_debug(struct gl_context *ctx, GLenum target,
                                     const GLcharARB *source);


GLuint
_mesa_meta_link_program_with_debug(struct gl_context *ctx, GLuint program);

void
_mesa_meta_compile_and_link_program(struct gl_context *ctx,
                                    const char *vs_source,
                                    const char *fs_source,
                                    const char *name,
                                    GLuint *program);

GLboolean
_mesa_meta_alloc_texture(struct temp_texture *tex,
                         GLsizei width, GLsizei height, GLenum intFormat);

void
_mesa_meta_setup_texture_coords(GLenum faceTarget,
                                GLint slice,
                                GLint width,
                                GLint height,
                                GLint depth,
                                GLfloat coords0[4],
                                GLfloat coords1[4],
                                GLfloat coords2[4],
                                GLfloat coords3[4]);

struct temp_texture *
_mesa_meta_get_temp_texture(struct gl_context *ctx);

struct temp_texture *
_mesa_meta_get_temp_depth_texture(struct gl_context *ctx);

void
_mesa_meta_setup_vertex_objects(GLuint *VAO, GLuint *VBO,
                                bool use_generic_attributes,
                                unsigned vertex_size, unsigned texcoord_size,
                                unsigned color_size);

void
_mesa_meta_setup_ff_tnl_for_blit(GLuint *VAO, GLuint *VBO,
                                 unsigned texcoord_size);

void
_mesa_meta_setup_drawpix_texture(struct gl_context *ctx,
                                 struct temp_texture *tex,
                                 GLboolean newTex,
                                 GLsizei width, GLsizei height,
                                 GLenum format, GLenum type,
                                 const GLvoid *pixels);

void
_mesa_meta_setup_copypix_texture(struct gl_context *ctx,
                                 struct temp_texture *tex,
                                 GLint srcX, GLint srcY,
                                 GLsizei width, GLsizei height,
                                 GLenum intFormat,
                                 GLenum filter);

void
_mesa_meta_setup_blit_shader(struct gl_context *ctx,
                             GLenum target,
                             struct blit_shader_table *table);

void
_mesa_meta_glsl_blit_cleanup(struct blit_state *blit);

void
_mesa_meta_blit_shader_table_cleanup(struct blit_shader_table *table);

void
_mesa_meta_glsl_generate_mipmap_cleanup(struct gen_mipmap_state *mipmap);

void
_mesa_meta_bind_fbo_image(GLenum attachment,
                          struct gl_texture_image *texImage, GLuint layer);

#endif /* META_H */
