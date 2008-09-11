/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "util/u_memory.h"
#include "pipe/p_state.h"
#include "pipe/p_inlines.h"
#include "sct.h"


struct texture_list
{
   struct pipe_texture *texture;
   struct texture_list *next;
};



#define MAX_SURFACES  ((PIPE_MAX_COLOR_BUFS) + 1)

struct sct_context
{
   const struct pipe_context *context;

   /** surfaces the context is drawing into */
   struct pipe_surface *surfaces[MAX_SURFACES];

   /** currently bound textures */
   struct pipe_texture *textures[PIPE_MAX_SAMPLERS];

   /** previously bound textures, used but not flushed */
   struct texture_list *textures_used;

   boolean needs_flush;

   struct sct_context *next;
};



struct sct_surface
{
   const struct pipe_surface *surface;

   /** list of contexts drawing to this surface */
   struct sct_context_list *contexts;

   struct sct_surface *next;
};



/**
 * Find the surface_info for the given pipe_surface
 */
static struct sct_surface *
find_surface_info(struct surface_context_tracker *sct,
                  const struct pipe_surface *surface)
{
   struct sct_surface *si;
   for (si = sct->surfaces; si; si = si->next)
      if (si->surface == surface)
         return si;
   return NULL;
}


/**
 * As above, but create new surface_info if surface is new.
 */
static struct sct_surface *
find_create_surface_info(struct surface_context_tracker *sct,
                         const struct pipe_surface *surface)
{
   struct sct_surface *si = find_surface_info(sct, surface);
   if (si)
      return si;

   /* alloc new */
   si = CALLOC_STRUCT(sct_surface);
   if (si) {
      si->surface = surface;

      /* insert at head */
      si->next = sct->surfaces;
      sct->surfaces = si;
   }

   return si;
}


/**
 * Find a context_info for the given context.
 */
static struct sct_context *
find_context_info(struct surface_context_tracker *sct,
                  const struct pipe_context *context)
{
   struct sct_context *ci;
   for (ci = sct->contexts; ci; ci = ci->next)
      if (ci->context == context)
         return ci;
   return NULL;
}


/**
 * As above, but create new context_info if context is new.
 */
static struct sct_context *
find_create_context_info(struct surface_context_tracker *sct,
                         const struct pipe_context *context)
{
   struct sct_context *ci = find_context_info(sct, context);
   if (ci)
      return ci;

   /* alloc new */
   ci = CALLOC_STRUCT(sct_context);
   if (ci) {
      ci->context = context;

      /* insert at head */
      ci->next = sct->contexts;
      sct->contexts = ci;
   }

   return ci;
}


/**
 * Is the context already bound to the surface?
 */
static boolean
find_surface_context(const struct sct_surface *si,
                     const struct pipe_context *context)
{
   const struct sct_context_list *cl;
   for (cl = si->contexts; cl; cl = cl->next) {
      if (cl->context == context) {
         return TRUE;
      }
   }
   return FALSE;
}


/**
 * Add a context to the list of contexts associated with a surface.
 */
static void
add_context_to_surface(struct sct_surface *si,
                       const struct pipe_context *context)
{
   struct sct_context_list *cl = CALLOC_STRUCT(sct_context_list);
   if (cl) {
      cl->context = context;
      /* insert at head of list of contexts */
      cl->next = si->contexts;
      si->contexts = cl;
   }
}


/**
 * Remove a context from the list of contexts associated with a surface.
 */
static void
remove_context_from_surface(struct sct_surface *si,
                            const struct pipe_context *context)
{
   struct sct_context_list *prev = NULL, *curr, *next;

   for (curr = si->contexts; curr; curr = next) {
      if (curr->context == context) {
         /* remove */
         if (prev)
            prev->next = curr->next;
         else
            si->contexts = curr->next;
         next = curr->next;
         FREE(curr);
      }
      else {
         prev = curr;
         next = curr->next;
      }
   }
}


/**
 * Unbind context from surface.
 */
static void
unbind_context_surface(struct surface_context_tracker *sct,
                       struct pipe_context *context,
                       struct pipe_surface *surface)
{
   struct sct_surface *si = find_surface_info(sct, surface);
   if (si) {
      remove_context_from_surface(si, context);
   }
}


/**
 * Bind context to a set of surfaces (color + Z).
 * Like MakeCurrent().
 */
void
sct_bind_surfaces(struct surface_context_tracker *sct,
                  struct pipe_context *context,
                  uint num_surf,
                  struct pipe_surface **surfaces)
{
   struct sct_context *ci = find_create_context_info(sct, context);
   uint i;

   if (!ci) {
      return; /* out of memory */
   }

   /* unbind currently bound surfaces */
   for (i = 0; i < MAX_SURFACES; i++) {
      if (ci->surfaces[i]) {
         unbind_context_surface(sct, context, ci->surfaces[i]);
      }
   }

   /* bind new surfaces */
   for (i = 0; i < num_surf; i++) {
      struct sct_surface *si = find_create_surface_info(sct, surfaces[i]);
      if (!find_surface_context(si, context)) {
         add_context_to_surface(si, context);
      }
   }
}


/**
 * Return list of contexts bound to a surface.
 */
const struct sct_context_list *
sct_get_surface_contexts(struct surface_context_tracker *sct,
                         const struct pipe_surface *surface)
{
   const struct sct_surface *si = find_surface_info(sct, surface);
   return si->contexts;
}



static boolean
find_texture(const struct sct_context *ci,
             const struct pipe_texture *texture)
{
   const struct texture_list *tl;

   for (tl = ci->textures_used; tl; tl = tl->next) {
      if (tl->texture == texture) {
         return TRUE;
      }
   }
   return FALSE;
}


/**
 * Add the given texture to the context's list of used textures.
 */
static void
add_texture_used(struct sct_context *ci,
                 struct pipe_texture *texture)
{
   if (!find_texture(ci, texture)) {
      /* add to list */
      struct texture_list *tl = CALLOC_STRUCT(texture_list);
      if (tl) {
         pipe_texture_reference(&tl->texture, texture);
         /* insert at head */
         tl->next = ci->textures_used;
         ci->textures_used = tl;
      }
   }
}


/**
 * Bind a texture to a rendering context.
 */
void
sct_bind_texture(struct surface_context_tracker *sct,
                 struct pipe_context *context,
                 uint unit,
                 struct pipe_texture *tex)
{
   struct sct_context *ci = find_context_info(sct, context);

   if (ci->textures[unit] != tex) {
      /* put texture on the 'used' list */
      add_texture_used(ci, tex);
      /* bind new */
      pipe_texture_reference(&ci->textures[unit], tex);
   }
}


/**
 * Check if the given texture has been used by the rendering context
 * since the last call to sct_flush_textures().
 */
boolean
sct_is_texture_used(struct surface_context_tracker *sct,
                    const struct pipe_context *context,
                    const struct pipe_texture *texture)
{
   const struct sct_context *ci = find_context_info(sct, context);
   return find_texture(ci, texture);
}


/**
 * To be called when the image contents of a texture are changed, such
 * as for gl[Copy]TexSubImage().
 * XXX this may not be needed
 */
void
sct_update_texture(struct pipe_texture *tex)
{

}


/**
 * When a scene is flushed/rendered we can release the list of
 * used textures.
 */
void
sct_flush_textures(struct surface_context_tracker *sct,
                   struct pipe_context *context)
{
   struct sct_context *ci = find_context_info(sct, context);
   struct texture_list *tl, *next;
   uint i;

   for (tl = ci->textures_used; tl; tl = next) {
      next = tl->next;
      pipe_texture_release(&tl->texture);
      FREE(tl);
   }
   ci->textures_used = NULL;

   /* put the currently bound textures on the 'used' list */
   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      add_texture_used(ci, ci->textures[i]);
   }
}



void
sct_destroy_context(struct surface_context_tracker *sct,
                    struct pipe_context *context)
{
   /* XXX should we require an unbinding first? */
   {
      struct sct_surface *si;
      for (si = sct->surfaces; si; si = si->next) {
         remove_context_from_surface(si, context);
      }
   }

   /* remove context from context_info list */
   {
      struct sct_context *ci, *next, *prev = NULL;
      for (ci = sct->contexts; ci; ci = next) {
         next = ci->next;
         if (ci->context == context) {
            if (prev)
               prev->next = ci->next;
            else
               sct->contexts = ci->next;
            FREE(ci);
         }
         else {
            prev = ci;
         }
      }
   }

}


void
sct_destroy_surface(struct surface_context_tracker *sct,
                    struct pipe_surface *surface)
{
   if (1) {
      /* debug/sanity: no context should be bound to surface */
      struct sct_context *ci;
      uint i;
      for (ci = sct->contexts; ci; ci = ci->next) {
         for (i = 0; i < MAX_SURFACES; i++) {
            assert(ci->surfaces[i] != surface);
         }
      }
   }

   /* remove surface from sct_surface list */
   {
      struct sct_surface *si, *next, *prev = NULL;
      for (si = sct->surfaces; si; si = next) {
         next = si->next;
         if (si->surface == surface) {
            /* unlink */
            if (prev)
               prev->next = si->next;
            else
               sct->surfaces = si->next;
            FREE(si);
         }
         else {
            prev = si;
         }
      }
   }
}
