/*
 * Copyright © 2011 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "gen6_hiz.h"

#include <assert.h>

#include "mesa/drivers/common/meta.h"

#include "mesa/main/arrayobj.h"
#include "mesa/main/bufferobj.h"
#include "mesa/main/depth.h"
#include "mesa/main/enable.h"
#include "mesa/main/fbobject.h"
#include "mesa/main/framebuffer.h"
#include "mesa/main/get.h"
#include "mesa/main/renderbuffer.h"
#include "mesa/main/shaderapi.h"
#include "mesa/main/varray.h"

#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_tex.h"

#include "brw_context.h"
#include "brw_defines.h"

static const uint32_t gen6_hiz_meta_save =

      /* Disable alpha, depth, and stencil test.
       *
       * See the following sections of the Sandy Bridge PRM, Volume 1, Part2:
       *   - 7.5.3.1 Depth Buffer Clear
       *   - 7.5.3.2 Depth Buffer Resolve
       *   - 7.5.3.3 Hierarchical Depth Buffer Resolve
       */
      MESA_META_ALPHA_TEST |
      MESA_META_DEPTH_TEST |
      MESA_META_STENCIL_TEST |

      /* Disable viewport mapping.
       *
       * From page 11 of the Sandy Bridge PRM, Volume 2, Part 1, Section 1.3
       * 3D Primitives Overview:
       *    RECTLIST:
       *    Viewport Mapping must be DISABLED (as is typical with the use of
       *    screen- space coordinates).
       *
       * We must also manually disable 3DSTATE_SF.Viewport_Transform_Enable.
       */
      MESA_META_VIEWPORT |

      /* Disable clipping.
       *
       * From page 11 of the Sandy Bridge PRM, Volume 2, Part 1, Section 1.3
       * 3D Primitives Overview:
       *     Either the CLIP unit should be DISABLED, or the CLIP unit’s Clip
       *     Mode should be set to a value other than CLIPMODE_NORMAL.
       */
      MESA_META_CLIP |

      /* Render a solid rectangle (set 3DSTATE_SF.FrontFace_Fill_Mode).
       *
       * From page 249 of the Sandy Bridge PRM, Volume 2, Part 1, Section
       * 6.4.1.1 3DSTATE_SF, FrontFace_Fill_Mode:
       *     SOLID: Any triangle or rectangle object found to be front-facing
       *     is rendered as a solid object. This setting is required when
       *     (rendering rectangle (RECTLIST) objects.
       * Also see field BackFace_Fill_Mode.
       *
       * Note: MESA_META_RASTERIZAION also disables culling, but that is
       * irrelevant. See 3DSTATE_SF.Cull_Mode.
       */
      MESA_META_RASTERIZATION |

      /* Each HiZ operation uses a vertex shader and VAO. */
      MESA_META_SHADER |
      MESA_META_VERTEX |

      /* Disable scissoring.
       *
       * Scissoring is disabled for resolves because a resolve operation
       * should resolve the entire buffer. Scissoring is disabled for depth
       * clears because, if we are performing a partial depth clear, then we
       * specify the clear region with the RECTLIST vertices.
       */
      MESA_META_SCISSOR |

      MESA_META_SELECT_FEEDBACK;

static void
gen6_hiz_get_framebuffer_enum(struct gl_context *ctx,
                              GLenum *bind_enum,
                              GLenum *get_enum)
{
   if (ctx->Extensions.EXT_framebuffer_blit && ctx->API == API_OPENGL) {
      /* Different buffers may be bound to GL_DRAW_FRAMEBUFFER and
       * GL_READ_FRAMEBUFFER. Take care to not disrupt the read buffer.
       */
      *bind_enum = GL_DRAW_FRAMEBUFFER;
      *get_enum = GL_DRAW_FRAMEBUFFER_BINDING;
   } else {
      /* The enums GL_DRAW_FRAMEBUFFER and GL_READ_FRAMEBUFFER do not exist.
       * The bound framebuffer is both the read and draw buffer.
       */
      *bind_enum = GL_FRAMEBUFFER;
      *get_enum = GL_FRAMEBUFFER_BINDING;
   }
}

/**
 * Initialize static data needed for HiZ operations.
 */
static void
gen6_hiz_init(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct brw_hiz_state *hiz = &brw->hiz;
   GLenum fb_bind_enum, fb_get_enum;

   if (hiz->fbo != 0)
      return;

   gen6_hiz_get_framebuffer_enum(ctx, &fb_bind_enum, &fb_get_enum);

   /* Create depthbuffer.
    *
    * Until glRenderbufferStorage is called, the renderbuffer hash table
    * maps the renderbuffer name to a dummy renderbuffer. We need the
    * renderbuffer to be registered in the hash table so that framebuffer
    * validation succeeds, so we hackishly allocate storage then immediately
    * discard it.
    */
   GLuint depth_rb_name;
   _mesa_GenRenderbuffersEXT(1, &depth_rb_name);
   _mesa_BindRenderbufferEXT(GL_RENDERBUFFER, depth_rb_name);
   _mesa_RenderbufferStorageEXT(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 32, 32);
   _mesa_reference_renderbuffer(&hiz->depth_rb,
                                _mesa_lookup_renderbuffer(ctx, depth_rb_name));
   intel_miptree_release(&((struct intel_renderbuffer*) hiz->depth_rb)->mt);

   /* Setup FBO. */
   _mesa_GenFramebuffersEXT(1, &hiz->fbo);
   _mesa_BindFramebufferEXT(fb_bind_enum, hiz->fbo);
   _mesa_FramebufferRenderbufferEXT(fb_bind_enum,
                                    GL_DEPTH_ATTACHMENT,
                                    GL_RENDERBUFFER,
                                    hiz->depth_rb->Name);

   /* Compile vertex shader. */
   const char *vs_source =
      "attribute vec4 position;\n"
      "void main()\n"
      "{\n"
      "   gl_Position = position;\n"
      "}\n";
   GLuint vs = _mesa_CreateShaderObjectARB(GL_VERTEX_SHADER);
   _mesa_ShaderSourceARB(vs, 1, &vs_source, NULL);
   _mesa_CompileShaderARB(vs);

   /* Compile fragment shader. */
   const char *fs_source = "void main() {}";
   GLuint fs = _mesa_CreateShaderObjectARB(GL_FRAGMENT_SHADER);
   _mesa_ShaderSourceARB(fs, 1, &fs_source, NULL);
   _mesa_CompileShaderARB(fs);

   /* Link and use program. */
   hiz->shader.program = _mesa_CreateProgramObjectARB();
   _mesa_AttachShader(hiz->shader.program, vs);
   _mesa_AttachShader(hiz->shader.program, fs);
   _mesa_LinkProgramARB(hiz->shader.program);
   _mesa_UseProgramObjectARB(hiz->shader.program);

   /* Create and bind VAO. */
   _mesa_GenVertexArrays(1, &hiz->vao);
   _mesa_BindVertexArray(hiz->vao);

   /* Setup VBO for 'position'. */
   hiz->shader.position_location =
      _mesa_GetAttribLocationARB(hiz->shader.program, "position");
   _mesa_GenBuffersARB(1, &hiz->shader.position_vbo);
   _mesa_BindBufferARB(GL_ARRAY_BUFFER_ARB, hiz->shader.position_vbo);
   _mesa_VertexAttribPointerARB(hiz->shader.position_location,
				2, /*components*/
				GL_FLOAT,
				GL_FALSE, /*normalized?*/
				0, /*stride*/
				NULL);
   _mesa_EnableVertexAttribArrayARB(hiz->shader.position_location);

   /* Cleanup. */
   _mesa_DeleteShader(vs);
   _mesa_DeleteShader(fs);
}

/**
 * Wrap \c brw->hiz.depth_rb around a miptree.
 *
 * \see gen6_hiz_teardown_depth_buffer()
 */
static void
gen6_hiz_setup_depth_buffer(struct brw_context *brw,
			    struct intel_mipmap_tree *mt,
			    unsigned int level,
			    unsigned int layer)
{
   struct gl_renderbuffer *rb = brw->hiz.depth_rb;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   rb->Format = mt->format;
   rb->_BaseFormat = _mesa_get_format_base_format(rb->Format);
   rb->DataType = intel_mesa_format_to_rb_datatype(rb->Format);
   rb->InternalFormat = rb->_BaseFormat;
   rb->Width = mt->level[level].width;
   rb->Height = mt->level[level].height;

   irb->mt_level = level;
   irb->mt_layer = layer;

   intel_miptree_reference(&irb->mt, mt);
   intel_renderbuffer_set_draw_offset(irb);
}

/**
 * Release the region from \c brw->hiz.depth_rb.
 *
 * \see gen6_hiz_setup_depth_buffer()
 */
static void
gen6_hiz_teardown_depth_buffer(struct gl_renderbuffer *rb)
{
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   intel_miptree_release(&irb->mt);
}

static void
gen6_resolve_slice(struct intel_context *intel,
	         struct intel_mipmap_tree *mt,
		 unsigned int level,
		 unsigned int layer,
                 enum brw_hiz_op op)
{
   struct gl_context *ctx = &intel->ctx;
   struct brw_context *brw = brw_context(ctx);
   struct brw_hiz_state *hiz = &brw->hiz;
   GLenum fb_bind_enum, fb_get_enum;

   /* Do not recurse. */
   assert(!brw->hiz.op);

   assert(mt->hiz_mt != NULL);
   assert(level >= mt->first_level);
   assert(level <= mt->last_level);
   assert(layer < mt->level[level].depth);

   gen6_hiz_get_framebuffer_enum(ctx, &fb_bind_enum, &fb_get_enum);

   /* Save state. */
   GLint save_drawbuffer;
   GLint save_renderbuffer;
   _mesa_meta_begin(ctx, gen6_hiz_meta_save);
   _mesa_GetIntegerv(fb_get_enum, &save_drawbuffer);
   _mesa_GetIntegerv(GL_RENDERBUFFER_BINDING, &save_renderbuffer);

   /* Initialize context data for HiZ operations. */
   gen6_hiz_init(brw);

   /* Set depth state. */
   if (!ctx->Depth.Mask) {
      /* This sets 3DSTATE_WM.Depth_Buffer_Write_Enable. */
      _mesa_DepthMask(GL_TRUE);
   }
   if (op == BRW_HIZ_OP_DEPTH_RESOLVE) {
      _mesa_set_enable(ctx, GL_DEPTH_TEST, GL_TRUE);
      _mesa_DepthFunc(GL_NEVER);
   }

   /* Setup FBO. */
   gen6_hiz_setup_depth_buffer(brw, mt, level, layer);
   _mesa_BindFramebufferEXT(fb_bind_enum, hiz->fbo);


   /* A rectangle primitive (3DPRIM_RECTLIST) consists of only three vertices.
    * The vertices reside in screen space with DirectX coordinates (this is,
    * (0, 0) is the upper left corner).
    *
    *   v2 ------ implied
    *    |        |
    *    |        |
    *   v0 ----- v1
    */
   const int width = hiz->depth_rb->Width;
   const int height = hiz->depth_rb->Height;
   const GLfloat positions[] = {
          0, height,
      width, height,
          0,      0,
   };

   /* Setup program and vertex attributes. */
   _mesa_UseProgramObjectARB(hiz->shader.program);
   _mesa_BindVertexArray(hiz->vao);
   _mesa_BindBufferARB(GL_ARRAY_BUFFER, hiz->shader.position_vbo);
   _mesa_BufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(positions), positions,
		       GL_DYNAMIC_DRAW_ARB);

   /* Execute the HiZ operation. */
   brw->hiz.op = op;
   brw->state.dirty.brw |= BRW_NEW_HIZ;
   _mesa_DrawArrays(GL_TRIANGLES, 0, 3);
   brw->state.dirty.brw |= BRW_NEW_HIZ;
   brw->hiz.op = BRW_HIZ_OP_NONE;

   /* Restore state.
    *
    * The order in which state is restored is significant. The draw buffer
    * used for the HiZ op has no stencil buffer, and glStencilFunc() clamps
    * the stencil reference value to the range allowed by the draw buffer's
    * number of stencil bits. So, the draw buffer binding must be restored
    * before the stencil state, or else the stencil ref will be clamped to 0.
    */
   gen6_hiz_teardown_depth_buffer(hiz->depth_rb);
   _mesa_BindRenderbufferEXT(GL_RENDERBUFFER, save_renderbuffer);
   _mesa_BindFramebufferEXT(fb_bind_enum, save_drawbuffer);
   _mesa_meta_end(ctx);
}

void
gen6_resolve_hiz_slice(struct intel_context *intel,
                       struct intel_mipmap_tree *mt,
                       uint32_t level,
                       uint32_t layer)
{
   gen6_resolve_slice(intel, mt, level, layer, BRW_HIZ_OP_HIZ_RESOLVE);
}


void
gen6_resolve_depth_slice(struct intel_context *intel,
                         struct intel_mipmap_tree *mt,
                         uint32_t level,
                         uint32_t layer)
{
   gen6_resolve_slice(intel, mt, level, layer, BRW_HIZ_OP_DEPTH_RESOLVE);
}
