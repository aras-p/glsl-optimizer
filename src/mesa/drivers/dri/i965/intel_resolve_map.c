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

#include "intel_resolve_map.h"

#include <assert.h>
#include <stdlib.h>

/**
 * \brief Set that the miptree slice at (level, layer) needs a resolve.
 *
 * If a map element already exists with the given key, then the value is
 * changed to the given value of \c need.
 */
void
intel_resolve_map_set(struct exec_list *resolve_map,
		      uint32_t level,
		      uint32_t layer,
		      enum gen6_hiz_op need)
{
   foreach_list_typed(struct intel_resolve_map, map, link, resolve_map) {
      if (map->level == level && map->layer == layer) {
         map->need = need;
	 return;
      }
   }

   struct intel_resolve_map *m = malloc(sizeof(struct intel_resolve_map));
   exec_node_init(&m->link);
   m->level = level;
   m->layer = layer;
   m->need = need;

   exec_list_push_tail(resolve_map, &m->link);
}

/**
 * \brief Get an element from the map.
 * \return null if element is not contained in map.
 */
struct intel_resolve_map *
intel_resolve_map_get(struct exec_list *resolve_map,
		      uint32_t level,
		      uint32_t layer)
{
   foreach_list_typed(struct intel_resolve_map, map, link, resolve_map) {
      if (map->level == level && map->layer == layer)
         return map;
   }

   return NULL;
}

/**
 * \brief Remove and free an element from the map.
 */
void
intel_resolve_map_remove(struct intel_resolve_map *elem)
{
   exec_node_remove(&elem->link);
   free(elem);
}

/**
 * \brief Remove and free all elements of the map.
 */
void
intel_resolve_map_clear(struct exec_list *resolve_map)
{
   foreach_in_list_safe(struct exec_node, node, resolve_map) {
      free(node);
   }

   exec_list_make_empty(resolve_map);
}
