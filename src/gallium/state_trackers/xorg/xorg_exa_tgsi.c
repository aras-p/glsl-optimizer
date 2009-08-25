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
#include "cso_cache/cso_hash.h"

struct xorg_shaders {
   struct exa_context *exa;

   struct cso_hash *vs_hash;
   struct cso_hash *fs_hash;
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

static void *
create_vs(struct pipe_context *ctx,
          unsigned vs_traits)
{
   return NULL;
}

static void *
create_fs(struct pipe_context *ctx,
          unsigned vs_traits)
{
   return NULL;
}

static struct xorg_shader
xorg_shader_construct(struct exa_context *exa,
                      int op,
                      PicturePtr src_picture,
                      PicturePtr mask_picture,
                      PicturePtr dst_picture)
{
   struct xorg_shader shader = {0};
#if 0
   struct ureg_program *ureg;
   struct ureg_src dst_sampler, src_sampler, mask_sampler;
   struct ureg_src dst_pos, src_pos, mask_pos;
   struct ureg_src src, mask;

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

#endif
   return shader;
}

struct xorg_shaders * xorg_shaders_create(struct exa_context *exa)
{
   struct xorg_shaders *sc = CALLOC_STRUCT(xorg_shaders);

   sc->exa = exa;
   sc->vs_hash = cso_hash_create();
   sc->fs_hash = cso_hash_create();

   return sc;
}

static void
cache_destroy(struct cso_context *cso,
              struct cso_hash *hash,
              unsigned processor)
{
   struct cso_hash_iter iter = cso_hash_first_node(hash);
   while (!cso_hash_iter_is_null(iter)) {
      void *shader = (void *)cso_hash_iter_data(iter);
      if (processor == PIPE_SHADER_FRAGMENT) {
         cso_delete_fragment_shader(cso, shader);
      } else if (processor == PIPE_SHADER_VERTEX) {
         cso_delete_vertex_shader(cso, shader);
      }
      iter = cso_hash_erase(hash, iter);
   }
   cso_hash_delete(hash);
}

void xorg_shaders_destroy(struct xorg_shaders *sc)
{
   cache_destroy(sc->exa->cso, sc->vs_hash,
                 PIPE_SHADER_VERTEX);
   cache_destroy(sc->exa->cso, sc->fs_hash,
                 PIPE_SHADER_FRAGMENT);

   free(sc);
}

static INLINE void *
shader_from_cache(struct pipe_context *pipe,
                  unsigned type,
                  struct cso_hash *hash,
                  unsigned key)
{
   void *shader = 0;

   struct cso_hash_iter iter = cso_hash_find(hash, key);

   if (cso_hash_iter_is_null(iter)) {
      if (type == PIPE_SHADER_VERTEX)
         shader = create_vs(pipe, key);
      else
         shader = create_fs(pipe, key);
      cso_hash_insert(hash, key, shader);
   } else
      shader = (void *)cso_hash_iter_data(iter);

   return shader;
}

struct xorg_shader xorg_shaders_get(struct xorg_shaders *sc,
                                    unsigned vs_traits,
                                    unsigned fs_traits)
{
   struct xorg_shader shader = {0};
   void *vs, *fs;

   vs = shader_from_cache(sc->exa->ctx, PIPE_SHADER_VERTEX,
                          sc->vs_hash, vs_traits);
   fs = shader_from_cache(sc->exa->ctx, PIPE_SHADER_FRAGMENT,
                          sc->fs_hash, fs_traits);

   debug_assert(vs && fs);
   if (!vs || !fs)
      return shader;

   shader.vs = vs;
   shader.fs = fs;

   return shader;
}
