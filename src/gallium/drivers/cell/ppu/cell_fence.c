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

#include <unistd.h>
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "cell_context.h"
#include "cell_batch.h"
#include "cell_fence.h"
#include "cell_texture.h"


void
cell_fence_init(struct cell_fence *fence)
{
   uint i;
   ASSERT_ALIGN16(fence->status);
   for (i = 0; i < CELL_MAX_SPUS; i++) {
      fence->status[i][0] = CELL_FENCE_IDLE;
   }
}


boolean
cell_fence_signalled(const struct cell_context *cell,
                     const struct cell_fence *fence)
{
   uint i;
   for (i = 0; i < cell->num_spus; i++) {
      if (fence->status[i][0] != CELL_FENCE_SIGNALLED)
         return FALSE;
      /*assert(fence->status[i][0] == CELL_FENCE_EMITTED);*/
   }
   return TRUE;
}


boolean
cell_fence_finish(const struct cell_context *cell,
                  const struct cell_fence *fence,
                  uint64_t timeout)
{
   while (!cell_fence_signalled(cell, fence)) {
      usleep(10);
   }

#ifdef DEBUG
   {
      uint i;
      for (i = 0; i < cell->num_spus; i++) {
         assert(fence->status[i][0] == CELL_FENCE_SIGNALLED);
      }
   }
#endif
   return TRUE;
}




struct cell_buffer_node
{
   struct pipe_resource *buffer;
   struct cell_buffer_node *next;
};


#if 0
static void
cell_add_buffer_to_list(struct cell_context *cell,
                        struct cell_buffer_list *list,
                        struct pipe_resource *buffer)
{
   struct cell_buffer_node *node = CALLOC_STRUCT(cell_buffer_node);
   /* create new list node which references the buffer, insert at head */
   if (node) {
      pipe_resource_reference(&node->buffer, buffer);
      node->next = list->head;
      list->head = node;
   }
}
#endif


/**
 * Wait for completion of the given fence, then unreference any buffers
 * on the list.
 * This typically unrefs/frees texture buffers after any rendering which uses
 * them has completed.
 */
void
cell_free_fenced_buffers(struct cell_context *cell,
                         struct cell_buffer_list *list)
{
   if (list->head) {
      /*struct pipe_screen *ps = cell->pipe.screen;*/
      struct cell_buffer_node *node;

      cell_fence_finish(cell, &list->fence);

      /* traverse the list, unreferencing buffers, freeing nodes */
      node = list->head;
      while (node) {
         struct cell_buffer_node *next = node->next;
         assert(node->buffer);
         /* XXX need this? pipe_buffer_unmap(ps, node->buffer);*/
#if 0
         printf("Unref buffer %p\n", node->buffer);
         if (node->buffer->reference.count == 1)
            printf("   Delete!\n");
#endif
         pipe_resource_reference(&node->buffer, NULL);
         FREE(node);
         node = next;
      }
      list->head = NULL;
   }
}


/**
 * This should be called for each render command.
 * Any texture buffers that are current bound will be added to a fenced
 * list to be freed later when the fence is executed/signalled.
 */
void
cell_add_fenced_textures(struct cell_context *cell)
{
   /*struct cell_buffer_list *list = &cell->fenced_buffers[cell->cur_batch];*/
   uint i;

   for (i = 0; i < cell->num_textures; i++) {
      struct cell_resource *ct = cell->texture[i];
      if (ct) {
#if 0
         printf("Adding texture %p buffer %p to list\n",
                ct, ct->tiled_buffer[level]);
#endif
#if 00
         /* XXX this needs to be fixed/restored!
          * Maybe keep pointers to textures, not buffers.
          */
         if (ct->base.buffer)
            cell_add_buffer_to_list(cell, list, ct->buffer);
#endif
      }
   }
}
