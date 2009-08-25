#include "xorg_exa_tgsi.h"

/*### stupidity defined in X11/extensions/XI.h */
#undef Absolute

#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_inlines.h"
#include "pipe/p_shader_tokens.h"

#include "util/u_memory.h"
#include "util/u_simple_shaders.h"

#include "tgsi/tgsi_ureg.h"

#include "cso_cache/cso_context.h"

#define UNSUPPORTED_OP 0

struct shader_id {
   int op : 8;
   int mask : 1;
   int component_alpha : 1;
   int is_fill : 1;
};

/* SAMP[0]  = dst
 * SAMP[1]  = src
 * SAMP[2]  = mask
 * IN[0]    = pos dst
 * IN[1]    = pos src
 * IN[2]    = pos mask
 * CONST[0] = (0, 0, 0, 1)
 */

static const char over_op[] =
   "SUB TEMP[3], CONST[0].wwww, TEMP[1].wwww\n"
   "MUL TEMP[3], TEMP[0], TEMP[3]\n"
   "ADD TEMP[0], TEMP[3], TEMP[0]\n";


static INLINE void
create_preamble(struct ureg_program *ureg)
{
}


static INLINE void
src_in_mask(struct ureg_program *ureg,
            struct ureg_dst dst,
            struct ureg_src src,
            struct ureg_src mask)
{
   /* MUL dst, src, mask.wwww */
   ureg_MUL(ureg, dst, src,
            ureg_scalar(mask, TGSI_SWIZZLE_W));
}

static INLINE
struct shader_id shader_state(int op,
                              PicturePtr src_picture,
                              PicturePtr mask_picture,
                              PicturePtr dst_picture)
{
   struct shader_id sid;

   sid.op = op;
   sid.mask = (mask_picture != 0);
   sid.component_alpha = (mask_picture->componentAlpha);
   sid.is_fill = (src_picture->pSourcePict != 0);
   if (sid.is_fill) {
      sid.is_fill =
         (src_picture->pSourcePict->type == SourcePictTypeSolidFill);
   }

   return sid;
}

struct xorg_shader xorg_shader_construct(struct exa_context *exa,
                                         int op,
                                         PicturePtr src_picture,
                                         PicturePtr mask_picture,
                                         PicturePtr dst_picture)
{
   struct ureg_program *ureg;
   struct ureg_src dst_sampler, src_sampler, mask_sampler;
   struct ureg_src dst_pos, src_pos, mask_pos;
   struct ureg_src src, mask;
   struct shader_id sid = shader_state(op, src_picture,
                                       mask_picture,
                                       dst_picture);
   struct xorg_shader shader = {0};

   ureg = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (ureg == NULL)
      return shader;

   if (sid.is_fill)
      return shader;

#if 0  /* unused right now */
   dst_sampler = ureg_DECL_sampler(ureg);
   dst_pos = ureg_DECL_fs_input(ureg,
                                TGSI_SEMANTIC_POSITION,
                                0,
                                TGSI_INTERPOLATE_PERSPECTIVE);
#endif

   src_sampler = ureg_DECL_sampler(ureg);
   src_pos = ureg_DECL_fs_input(ureg,
                                TGSI_SEMANTIC_POSITION,
                                1,
                                TGSI_INTERPOLATE_PERSPECTIVE);

   if (sid.mask) {
      mask_sampler = ureg_DECL_sampler(ureg);
      src_pos = ureg_DECL_fs_input(ureg,
                                   TGSI_SEMANTIC_POSITION,
                                   2,
                                   TGSI_INTERPOLATE_PERSPECTIVE);
   }

   ureg_TEX(ureg, ureg_dst(src),
            TGSI_TEXTURE_2D, src_pos, src_sampler);

   if (sid.mask) {
      ureg_TEX(ureg, ureg_dst(mask),
               TGSI_TEXTURE_2D, mask_pos, mask_sampler);
      /* src IN mask */
      src_in_mask(ureg, ureg_dst(src), src, mask);
   }

   ureg_END(ureg);

   return shader;
}
