#ifndef U_SURFACES_H_
#define U_SURFACES_H_

#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "util/u_atomic.h"
#include "cso_cache/cso_hash.h"

struct util_surfaces
{
   union
   {
      struct cso_hash *hash;
      struct pipe_surface **array;
      void* pv;
   } u;
};

struct pipe_surface *util_surfaces_do_get(struct util_surfaces *us, unsigned surface_struct_size, struct pipe_screen *pscreen, struct pipe_resource *pt, unsigned face, unsigned level, unsigned zslice, unsigned flags);

/* fast inline path for the very common case */
static INLINE struct pipe_surface *
util_surfaces_get(struct util_surfaces *us, unsigned surface_struct_size, struct pipe_screen *pscreen, struct pipe_resource *pt, unsigned face, unsigned level, unsigned zslice, unsigned flags)
{
   if(likely(pt->target == PIPE_TEXTURE_2D && us->u.array))
   {
      struct pipe_surface *ps = us->u.array[level];
      if(ps)
      {
	 p_atomic_inc(&ps->reference.count);
	 return ps;
      }
   }

   return util_surfaces_do_get(us, surface_struct_size, pscreen, pt, face, level, zslice, flags);
}

static INLINE struct pipe_surface *
util_surfaces_peek(struct util_surfaces *us, struct pipe_resource *pt, unsigned face, unsigned level, unsigned zslice)
{
   if(!us->u.pv)
      return 0;

   if(unlikely(pt->target == PIPE_TEXTURE_3D || pt->target == PIPE_TEXTURE_CUBE))
      return cso_hash_iter_data(cso_hash_find(us->u.hash, ((zslice + face) << 8) | level));
   else
      return us->u.array[level];
}

void util_surfaces_do_detach(struct util_surfaces *us, struct pipe_surface *ps);

static INLINE void
util_surfaces_detach(struct util_surfaces *us, struct pipe_surface *ps)
{
   if(likely(ps->texture->target == PIPE_TEXTURE_2D))
   {
      us->u.array[ps->level] = 0;
      return;
   }

   util_surfaces_do_detach(us, ps);
}

void util_surfaces_destroy(struct util_surfaces *us, struct pipe_resource *pt, void (*destroy_surface) (struct pipe_surface *));

#endif
