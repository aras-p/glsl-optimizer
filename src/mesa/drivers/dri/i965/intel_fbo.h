/**************************************************************************
 *
 * Copyright 2006 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef INTEL_FBO_H
#define INTEL_FBO_H

#include <stdbool.h>
#include <assert.h>
#include "main/formats.h"
#include "main/macros.h"
#include "brw_context.h"
#include "intel_mipmap_tree.h"
#include "intel_screen.h"

#ifdef __cplusplus
extern "C" {
#endif

struct intel_mipmap_tree;
struct intel_texture_image;

/**
 * Intel renderbuffer, derived from gl_renderbuffer.
 */
struct intel_renderbuffer
{
   struct swrast_renderbuffer Base;
   /**
    * The real renderbuffer storage.
    *
    * This is multisampled if NumSamples is > 1.
    */
   struct intel_mipmap_tree *mt;

   /**
    * Downsampled contents for window-system MSAA renderbuffers.
    *
    * For window system MSAA color buffers, the singlesample_mt is shared with
    * other processes in DRI2 (and in DRI3, it's the image buffer managed by
    * glx_dri3.c), while mt is private to our process.  To do a swapbuffers,
    * we have to downsample out of mt into singlesample_mt.  For depth and
    * stencil buffers, the singlesample_mt is also private, and since we don't
    * expect to need to do resolves (except if someone does a glReadPixels()
    * or glCopyTexImage()), we just temporarily allocate singlesample_mt when
    * asked to map the renderbuffer.
    */
   struct intel_mipmap_tree *singlesample_mt;

   /**
    * \name Miptree view
    * \{
    *
    * Multiple renderbuffers may simultaneously wrap a single texture and each
    * provide a different view into that texture. The fields below indicate
    * which miptree slice is wrapped by this renderbuffer.  The fields' values
    * are consistent with the 'level' and 'layer' parameters of
    * glFramebufferTextureLayer().
    *
    * For renderbuffers not created with glFramebufferTexture*(), mt_level and
    * mt_layer are 0.
    *
    * Note: for a 2D multisample array texture on Gen7+ using
    * INTEL_MSAA_LAYOUT_UMS or INTEL_MSAA_LAYOUT_CMS, mt_layer is the physical
    * layer holding sample 0.  So, for example, if mt->num_samples == 4, then
    * logical layer n corresponds to mt_layer == 4*n.
    */
   unsigned int mt_level;
   unsigned int mt_layer;

   /* The number of attached logical layers. */
   unsigned int layer_count;
   /** \} */

   GLuint draw_x, draw_y; /**< Offset of drawing within the region */

   /**
    * Set to true at every draw call, to indicate if a window-system
    * renderbuffer needs to be downsampled before using singlesample_mt.
    */
   bool need_downsample;

   /**
    * Set to true when doing an intel_renderbuffer_map()/unmap() that requires
    * an upsample at the end.
    */
   bool need_map_upsample;

   /**
    * Set to true if singlesample_mt is temporary storage that persists only
    * for the duration of a mapping.
    */
   bool singlesample_mt_is_tmp;
};


/**
 * gl_renderbuffer is a base class which we subclass.  The Class field
 * is used for simple run-time type checking.
 */
#define INTEL_RB_CLASS 0x12345678


/**
 * Return a gl_renderbuffer ptr casted to intel_renderbuffer.
 * NULL will be returned if the rb isn't really an intel_renderbuffer.
 * This is determined by checking the ClassID.
 */
static inline struct intel_renderbuffer *
intel_renderbuffer(struct gl_renderbuffer *rb)
{
   struct intel_renderbuffer *irb = (struct intel_renderbuffer *) rb;
   if (irb && irb->Base.Base.ClassID == INTEL_RB_CLASS) {
      /*_mesa_warning(NULL, "Returning non-intel Rb\n");*/
      return irb;
   }
   else
      return NULL;
}


/**
 * \brief Return the framebuffer attachment specified by attIndex.
 *
 * If the framebuffer lacks the specified attachment, then return null.
 *
 * If the attached renderbuffer is a wrapper, then return wrapped
 * renderbuffer.
 */
static inline struct intel_renderbuffer *
intel_get_renderbuffer(struct gl_framebuffer *fb, gl_buffer_index attIndex)
{
   struct gl_renderbuffer *rb;

   assert((unsigned)attIndex < ARRAY_SIZE(fb->Attachment));

   rb = fb->Attachment[attIndex].Renderbuffer;
   if (!rb)
      return NULL;

   return intel_renderbuffer(rb);
}


static inline mesa_format
intel_rb_format(const struct intel_renderbuffer *rb)
{
   return rb->Base.Base.Format;
}

extern struct intel_renderbuffer *
intel_create_renderbuffer(mesa_format format, unsigned num_samples);

struct intel_renderbuffer *
intel_create_private_renderbuffer(mesa_format format, unsigned num_samples);

struct gl_renderbuffer*
intel_create_wrapped_renderbuffer(struct gl_context * ctx,
				  int width, int height,
				  mesa_format format);

extern void
intel_fbo_init(struct brw_context *brw);

void
intel_renderbuffer_set_draw_offset(struct intel_renderbuffer *irb);

static inline uint32_t
intel_renderbuffer_get_tile_offsets(struct intel_renderbuffer *irb,
                                    uint32_t *tile_x,
                                    uint32_t *tile_y)
{
   return intel_miptree_get_tile_offsets(irb->mt, irb->mt_level, irb->mt_layer,
                                         tile_x, tile_y);
}

bool
intel_renderbuffer_has_hiz(struct intel_renderbuffer *irb);

void
intel_renderbuffer_att_set_needs_depth_resolve(struct gl_renderbuffer_attachment *att);


/**
 * \brief Perform a HiZ resolve on the renderbuffer.
 *
 * It is safe to call this function on a renderbuffer without HiZ. In that
 * case, the function is a no-op.
 *
 * \return false if no resolve was needed
 */
bool
intel_renderbuffer_resolve_hiz(struct brw_context *brw,
			       struct intel_renderbuffer *irb);

/**
 * \brief Perform a depth resolve on the renderbuffer.
 *
 * It is safe to call this function on a renderbuffer without HiZ. In that
 * case, the function is a no-op.
 *
 * \return false if no resolve was needed
 */
bool
intel_renderbuffer_resolve_depth(struct brw_context *brw,
				 struct intel_renderbuffer *irb);

void intel_renderbuffer_move_to_temp(struct brw_context *brw,
                                     struct intel_renderbuffer *irb,
                                     bool invalidate);

void
intel_renderbuffer_downsample(struct brw_context *brw,
                              struct intel_renderbuffer *irb);

void
intel_renderbuffer_upsample(struct brw_context *brw,
                            struct intel_renderbuffer *irb);

void brw_render_cache_set_clear(struct brw_context *brw);
void brw_render_cache_set_add_bo(struct brw_context *brw, drm_intel_bo *bo);
void brw_render_cache_set_check_flush(struct brw_context *brw, drm_intel_bo *bo);

unsigned
intel_quantize_num_samples(struct intel_screen *intel, unsigned num_samples);

#ifdef __cplusplus
}
#endif

#endif /* INTEL_FBO_H */
