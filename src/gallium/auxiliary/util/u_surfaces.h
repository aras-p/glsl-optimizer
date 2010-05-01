#ifndef U_SURFACES_H_
#define U_SURFACES_H_

#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "util/u_atomic.h"

struct util_hash_table;

struct util_surfaces
{
   union
   {
      struct util_hash_table *table;
      struct pipe_surface **array;
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
