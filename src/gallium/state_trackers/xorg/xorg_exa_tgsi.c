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

/* Vertex shader:
 * IN[0]    = vertex pos
 * IN[1]    = src tex coord | solid fill color
 * IN[2]    = mask tex coord
 * IN[3]    = dst tex coord
 * CONST[0] = (2/dst_width, 2/dst_height, 1, 1)
 * CONST[1] = (-1, -1, 0, 0)
 *
 * OUT[0]   = vertex pos
 * OUT[1]   = src tex coord | solid fill color
 * OUT[2]   = mask tex coord
 * OUT[3]   = dst tex coord
 */

/* Fragment shader:
 * SAMP[0]  = src
 * SAMP[1]  = mask
 * SAMP[2]  = dst
 * IN[0]    = pos src | solid fill color
 * IN[1]    = pos mask
 * IN[2]    = pos dst
 * CONST[0] = (0, 0, 0, 1)
 *
 * OUT[0] = color
 */

struct xorg_shaders {
   struct exa_context *exa;

   struct cso_hash *vs_hash;
   struct cso_hash *fs_hash;
};

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

static struct ureg_src
vs_normalize_coords(struct ureg_program *ureg, struct ureg_src coords,
                    struct ureg_src const0, struct ureg_src const1)
{
   struct ureg_dst tmp = ureg_DECL_temporary(ureg);
   struct ureg_src ret;
   ureg_MUL(ureg, tmp, coords, const0);
   ureg_ADD(ureg, tmp, ureg_src(tmp), const1);
   ret = ureg_src(tmp);
   ureg_release_temporary(ureg, tmp);
   return ret;
}

static void *
create_vs(struct pipe_context *pipe,
          unsigned vs_traits)
{
   struct ureg_program *ureg;
   struct ureg_src src;
   struct ureg_dst dst;
   struct ureg_src const0, const1;
   boolean is_fill = vs_traits & VS_FILL;
   boolean is_composite = vs_traits & VS_COMPOSITE;
   boolean has_mask = vs_traits & VS_MASK;

   ureg = ureg_create(TGSI_PROCESSOR_VERTEX);
   if (ureg == NULL)
      return 0;

   const0 = ureg_DECL_constant(ureg);
   const1 = ureg_DECL_constant(ureg);

   /* it has to be either a fill or a composite op */
   debug_assert(is_fill ^ is_composite);

   src = ureg_DECL_vs_input(ureg,
                            TGSI_SEMANTIC_POSITION, 0);
   dst = ureg_DECL_output(ureg, TGSI_SEMANTIC_POSITION, 0);
   src = vs_normalize_coords(ureg, src,
                             const0, const1);
   ureg_MOV(ureg, dst, src);


   if (is_composite) {
      src = ureg_DECL_vs_input(ureg,
                               TGSI_SEMANTIC_GENERIC, 1);
      dst = ureg_DECL_output(ureg, TGSI_SEMANTIC_GENERIC, 1);
      ureg_MOV(ureg, dst, src);
   }
   if (is_fill) {
      src = ureg_DECL_vs_input(ureg,
                               TGSI_SEMANTIC_COLOR, 1);
      dst = ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, 1);
      ureg_MOV(ureg, dst, src);
   }

   if (has_mask) {
      src = ureg_DECL_vs_input(ureg,
                               TGSI_SEMANTIC_GENERIC, 2);
      dst = ureg_DECL_output(ureg, TGSI_SEMANTIC_POSITION, 2);
      ureg_MOV(ureg, dst, src);
   }

   ureg_END(ureg);

   return ureg_create_shader_and_destroy(ureg, pipe);
}

static void *
create_fs(struct pipe_context *pipe,
          unsigned fs_traits)
{
   struct ureg_program *ureg;
   struct ureg_src /*dst_sampler,*/ src_sampler, mask_sampler;
   struct ureg_src /*dst_pos,*/ src_pos, mask_pos;
   struct ureg_src src, mask;
   struct ureg_dst out;
   boolean is_fill = fs_traits & VS_FILL;
   boolean is_composite = fs_traits & VS_COMPOSITE;
   /*boolean has_mask = fs_traits & VS_MASK;*/

   ureg = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (ureg == NULL)
      return 0;

   /* it has to be either a fill or a composite op */
   debug_assert(is_fill ^ is_composite);

   out = ureg_DECL_output(ureg,
                          TGSI_SEMANTIC_COLOR,
                          0);

   if (is_composite) {
      src_sampler = ureg_DECL_sampler(ureg, 0);
      src_pos = ureg_DECL_fs_input(ureg,
                                   TGSI_SEMANTIC_POSITION,
                                   0,
                                   TGSI_INTERPOLATE_PERSPECTIVE);
   }
   if (is_fill) {
      src_pos = ureg_DECL_fs_input(ureg,
                                   TGSI_SEMANTIC_COLOR,
                                   0,
                                   TGSI_INTERPOLATE_PERSPECTIVE);
   }

   if ((fs_traits & FS_MASK)) {
      mask_sampler = ureg_DECL_sampler(ureg, 1);
      mask_pos = ureg_DECL_fs_input(ureg,
                                    TGSI_SEMANTIC_POSITION,
                                    1,
                                    TGSI_INTERPOLATE_PERSPECTIVE);
   }

#if 0  /* unused right now */
   dst_sampler = ureg_DECL_sampler(ureg, 2);
   dst_pos = ureg_DECL_fs_input(ureg,
                                TGSI_SEMANTIC_POSITION,
                                2,
                                TGSI_INTERPOLATE_PERSPECTIVE);
#endif

   if ((fs_traits & FS_MASK)) {
      ureg_TEX(ureg, ureg_dst(mask),
               TGSI_TEXTURE_2D, mask_pos, mask_sampler);
      /* src IN mask */
      src_in_mask(ureg, out, src, mask);
   } else {
      ureg_TEX(ureg, out,
               TGSI_TEXTURE_2D, src_pos, src_sampler);
   }

   ureg_END(ureg);

   return ureg_create_shader_and_destroy(ureg, pipe);
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
