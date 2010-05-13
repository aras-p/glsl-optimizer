#include "u_surfaces.h"
#include "util/u_hash_table.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"

/* TODO: ouch, util_hash_table should do these by default when passed a null function pointer
 * this indirect function call is quite bad
 */
static unsigned
hash(void *key)
{
   return (unsigned)(uintptr_t)key;
}

static int
compare(void *key1, void *key2)
{
   return (unsigned)(uintptr_t)key1 - (unsigned)(uintptr_t)key2;
}

struct pipe_surface *
util_surfaces_do_get(struct util_surfaces *us, unsigned surface_struct_size, struct pipe_screen *pscreen, struct pipe_resource *pt, unsigned face, unsigned level, unsigned zslice, unsigned flags)
{
   struct pipe_surface *ps;
   void *key = NULL;

   if(pt->target == PIPE_TEXTURE_3D || pt->target == PIPE_TEXTURE_CUBE)
   {	/* or 2D array */
      if(!us->u.table)
	 us->u.table = util_hash_table_create(hash, compare);
      key = (void *)(uintptr_t)(((zslice + face) << 8) | level);
      /* TODO: ouch, should have a get-reference function...
       * also, shouldn't allocate a two-pointer structure for each item... */
      ps = util_hash_table_get(us->u.table, key);
   }
   else
   {
      if(!us->u.array)
	 us->u.array = CALLOC(pt->last_level + 1, sizeof(struct pipe_surface *));
      ps = us->u.array[level];
   }

   if(ps)
   {
      p_atomic_inc(&ps->reference.count);
      return ps;
   }

   ps = (struct pipe_surface *)CALLOC(1, surface_struct_size);
   if(!ps)
      return NULL;

   pipe_surface_init(ps, pt, face, level, zslice, flags);
   ps->offset = ~0;

   if(pt->target == PIPE_TEXTURE_3D || pt->target == PIPE_TEXTURE_CUBE)
      util_hash_table_set(us->u.table, key, ps);
   else
      us->u.array[level] = ps;

   return ps;
}

void
util_surfaces_do_detach(struct util_surfaces *us, struct pipe_surface *ps)
{
   struct pipe_resource *pt = ps->texture;
   if(pt->target == PIPE_TEXTURE_3D || pt->target == PIPE_TEXTURE_CUBE)
   {	/* or 2D array */
      void* key = (void*)(uintptr_t)(((ps->zslice + ps->face) << 8) | ps->level);
      util_hash_table_remove(us->u.table, key);
   }
   else
      us->u.array[ps->level] = 0;
}

static enum pipe_error
util_surfaces_destroy_callback(void *key, void *value, void *data)
{
   void (*destroy_surface) (struct pipe_surface * ps) = data;
   destroy_surface((struct pipe_surface *)value);
   return PIPE_OK;
}

void
util_surfaces_destroy(struct util_surfaces *us, struct pipe_resource *pt, void (*destroy_surface) (struct pipe_surface *))
{
   if(pt->target == PIPE_TEXTURE_3D || pt->target == PIPE_TEXTURE_CUBE)
   {	/* or 2D array */
      if(us->u.table)
      {
	 util_hash_table_foreach(us->u.table, util_surfaces_destroy_callback, destroy_surface);
	 util_hash_table_destroy(us->u.table);
	 us->u.table = NULL;
      }
   }
   else
   {
      if(us->u.array)
      {
	 unsigned i;
	 for(i = 0; i < pt->last_level; ++i)
	 {
	    struct pipe_surface *ps = us->u.array[i];
	    if(ps)
	       destroy_surface(ps);
	 }
	 FREE(us->u.array);
	 us->u.array = NULL;
      }
   }
}
