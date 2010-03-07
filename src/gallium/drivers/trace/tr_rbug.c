/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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


#include "os/os_thread.h"
#include "util/u_format.h"
#include "util/u_string.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"
#include "util/u_network.h"
#include "os/os_time.h"

#include "tgsi/tgsi_parse.h"

#include "tr_dump.h"
#include "tr_state.h"
#include "tr_buffer.h"
#include "tr_texture.h"

#include "rbug/rbug.h"

#include <errno.h>

#define U642VOID(x) ((void *)(unsigned long)(x))
#define VOID2U64(x) ((uint64_t)(unsigned long)(x))

struct trace_rbug
{
   struct trace_screen *tr_scr;
   struct rbug_connection *con;
   pipe_thread thread;
   boolean running;
};

PIPE_THREAD_ROUTINE(trace_rbug_thread, void_tr_rbug);


/**********************************************************
 * Helper functions
 */


static struct trace_context *
trace_rbug_get_context_locked(struct trace_screen *tr_scr, rbug_context_t ctx)
{
   struct trace_context *tr_ctx = NULL;
   struct tr_list *ptr;

   foreach(ptr, &tr_scr->contexts) {
      tr_ctx = (struct trace_context *)((char*)ptr - offsetof(struct trace_context, list));
      if (ctx == VOID2U64(tr_ctx))
         break;
      tr_ctx = NULL;
   }

   return tr_ctx;
}

static struct trace_shader *
trace_rbug_get_shader_locked(struct trace_context *tr_ctx, rbug_shader_t shdr)
{
   struct trace_shader *tr_shdr = NULL;
   struct tr_list *ptr;

   foreach(ptr, &tr_ctx->shaders) {
      tr_shdr = (struct trace_shader *)((char*)ptr - offsetof(struct trace_shader, list));
      if (shdr == VOID2U64(tr_shdr))
         break;
      tr_shdr = NULL;
   }

   return tr_shdr;
}

static void *
trace_shader_create_locked(struct pipe_context *pipe,
                           struct trace_shader *tr_shdr,
                           struct tgsi_token *tokens)
{
   void *state = NULL;
   struct pipe_shader_state pss = { 0 };
   pss.tokens = tokens;

   if (tr_shdr->type == TRACE_SHADER_FRAGMENT) {
      state = pipe->create_fs_state(pipe, &pss);
   } else if (tr_shdr->type == TRACE_SHADER_VERTEX) {
      state = pipe->create_vs_state(pipe, &pss);
   } else
      assert(0);

   return state;
}

static void
trace_shader_bind_locked(struct pipe_context *pipe,
                         struct trace_shader *tr_shdr,
                         void *state)
{
   if (tr_shdr->type == TRACE_SHADER_FRAGMENT) {
      pipe->bind_fs_state(pipe, state);
   } else if (tr_shdr->type == TRACE_SHADER_VERTEX) {
      pipe->bind_vs_state(pipe, state);
   } else
      assert(0);
}

static void
trace_shader_delete_locked(struct pipe_context *pipe,
                           struct trace_shader *tr_shdr,
                           void *state)
{
   if (tr_shdr->type == TRACE_SHADER_FRAGMENT) {
      pipe->delete_fs_state(pipe, state);
   } else if (tr_shdr->type == TRACE_SHADER_VERTEX) {
      pipe->delete_vs_state(pipe, state);
   } else
      assert(0);
}

/************************************************
 * Request handler functions
 */


static int
trace_rbug_texture_list(struct trace_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct trace_screen *tr_scr = tr_rbug->tr_scr;
   struct trace_texture *tr_tex = NULL;
   struct tr_list *ptr;
   rbug_texture_t *texs;
   int i = 0;

   pipe_mutex_lock(tr_scr->list_mutex);
   texs = MALLOC(tr_scr->num_textures * sizeof(rbug_texture_t));
   foreach(ptr, &tr_scr->textures) {
      tr_tex = (struct trace_texture *)((char*)ptr - offsetof(struct trace_texture, list));
      texs[i++] = VOID2U64(tr_tex);
   }
   pipe_mutex_unlock(tr_scr->list_mutex);

   rbug_send_texture_list_reply(tr_rbug->con, serial, texs, i, NULL);
   FREE(texs);

   return 0;
}

static int
trace_rbug_texture_info(struct trace_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct trace_screen *tr_scr = tr_rbug->tr_scr;
   struct trace_texture *tr_tex = NULL;
   struct rbug_proto_texture_info *gpti = (struct rbug_proto_texture_info *)header;
   struct tr_list *ptr;
   struct pipe_texture *t;

   pipe_mutex_lock(tr_scr->list_mutex);
   foreach(ptr, &tr_scr->textures) {
      tr_tex = (struct trace_texture *)((char*)ptr - offsetof(struct trace_texture, list));
      if (gpti->texture == VOID2U64(tr_tex))
         break;
      tr_tex = NULL;
   }

   if (!tr_tex) {
      pipe_mutex_unlock(tr_scr->list_mutex);
      return -ESRCH;
   }

   t = tr_tex->texture;
   rbug_send_texture_info_reply(tr_rbug->con, serial,
                               t->target, t->format,
                               &t->width0, 1,
                               &t->height0, 1,
                               &t->depth0, 1,
                               util_format_get_blockwidth(t->format),
                               util_format_get_blockheight(t->format),
                               util_format_get_blocksize(t->format),
                               t->last_level,
                               t->nr_samples,
                               t->tex_usage,
                               NULL);

   pipe_mutex_unlock(tr_scr->list_mutex);

   return 0;
}

static int
trace_rbug_texture_read(struct trace_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_texture_read *gptr = (struct rbug_proto_texture_read *)header;

   struct trace_screen *tr_scr = tr_rbug->tr_scr;
   struct trace_texture *tr_tex = NULL;
   struct tr_list *ptr;

   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_texture *tex;
   struct pipe_transfer *t;

   void *map;

   pipe_mutex_lock(tr_scr->list_mutex);
   foreach(ptr, &tr_scr->textures) {
      tr_tex = (struct trace_texture *)((char*)ptr - offsetof(struct trace_texture, list));
      if (gptr->texture == VOID2U64(tr_tex))
         break;
      tr_tex = NULL;
   }

   if (!tr_tex) {
      pipe_mutex_unlock(tr_scr->list_mutex);
      return -ESRCH;
   }

   tex = tr_tex->texture;
   t = screen->get_tex_transfer(tr_scr->screen, tex,
                                gptr->face, gptr->level, gptr->zslice,
                                PIPE_TRANSFER_READ,
                                gptr->x, gptr->y, gptr->w, gptr->h);

   map = screen->transfer_map(screen, t);

   rbug_send_texture_read_reply(tr_rbug->con, serial,
                                t->texture->format,
                                util_format_get_blockwidth(t->texture->format),
                                util_format_get_blockheight(t->texture->format),
                                util_format_get_blocksize(t->texture->format),
                                (uint8_t*)map,
                                t->stride * util_format_get_nblocksy(t->texture->format, t->height),
                                t->stride,
                                NULL);

   screen->transfer_unmap(screen, t);
   screen->tex_transfer_destroy(t);

   pipe_mutex_unlock(tr_scr->list_mutex);

   return 0;
}

static int
trace_rbug_context_list(struct trace_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct trace_screen *tr_scr = tr_rbug->tr_scr;
   struct tr_list *ptr;
   struct trace_context *tr_ctx = NULL;
   rbug_context_t *ctxs;
   int i = 0;

   pipe_mutex_lock(tr_scr->list_mutex);
   ctxs = MALLOC(tr_scr->num_contexts * sizeof(rbug_context_t));
   foreach(ptr, &tr_scr->contexts) {
      tr_ctx = (struct trace_context *)((char*)ptr - offsetof(struct trace_context, list));
      ctxs[i++] = VOID2U64(tr_ctx);
   }
   pipe_mutex_unlock(tr_scr->list_mutex);

   rbug_send_context_list_reply(tr_rbug->con, serial, ctxs, i, NULL);
   FREE(ctxs);

   return 0;
}

static int
trace_rbug_context_info(struct trace_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_context_info *info = (struct rbug_proto_context_info *)header;

   struct trace_screen *tr_scr = tr_rbug->tr_scr;
   struct trace_context *tr_ctx = NULL;
   rbug_texture_t cbufs[PIPE_MAX_COLOR_BUFS];
   rbug_texture_t texs[PIPE_MAX_SAMPLERS];
   int i;

   pipe_mutex_lock(tr_scr->list_mutex);
   tr_ctx = trace_rbug_get_context_locked(tr_scr, info->context);

   if (!tr_ctx) {
      pipe_mutex_unlock(tr_scr->list_mutex);
      return -ESRCH;
   }

   /* protect the pipe context */
   pipe_mutex_lock(tr_ctx->draw_mutex);
   trace_dump_call_lock();

   for (i = 0; i < tr_ctx->curr.nr_cbufs; i++)
      cbufs[i] = VOID2U64(tr_ctx->curr.cbufs[i]);

   for (i = 0; i < tr_ctx->curr.num_texs; i++)
      texs[i] = VOID2U64(tr_ctx->curr.tex[i]);

   rbug_send_context_info_reply(tr_rbug->con, serial,
                                VOID2U64(tr_ctx->curr.vs), VOID2U64(tr_ctx->curr.fs),
                                texs, tr_ctx->curr.num_texs,
                                cbufs, tr_ctx->curr.nr_cbufs,
                                VOID2U64(tr_ctx->curr.zsbuf),
                                tr_ctx->draw_blocker, tr_ctx->draw_blocked, NULL);

   trace_dump_call_unlock();
   pipe_mutex_unlock(tr_ctx->draw_mutex);

   pipe_mutex_unlock(tr_scr->list_mutex);

   return 0;
}

static int
trace_rbug_context_draw_block(struct trace_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_context_draw_block *block = (struct rbug_proto_context_draw_block *)header;

   struct trace_screen *tr_scr = tr_rbug->tr_scr;
   struct trace_context *tr_ctx = NULL;

   pipe_mutex_lock(tr_scr->list_mutex);
   tr_ctx = trace_rbug_get_context_locked(tr_scr, block->context);

   if (!tr_ctx) {
      pipe_mutex_unlock(tr_scr->list_mutex);
      return -ESRCH;
   }

   pipe_mutex_lock(tr_ctx->draw_mutex);
   tr_ctx->draw_blocker |= block->block;
   pipe_mutex_unlock(tr_ctx->draw_mutex);

   pipe_mutex_unlock(tr_scr->list_mutex);

   return 0;
}

static int
trace_rbug_context_draw_step(struct trace_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_context_draw_step *step = (struct rbug_proto_context_draw_step *)header;

   struct trace_screen *tr_scr = tr_rbug->tr_scr;
   struct trace_context *tr_ctx = NULL;

   pipe_mutex_lock(tr_scr->list_mutex);
   tr_ctx = trace_rbug_get_context_locked(tr_scr, step->context);

   if (!tr_ctx) {
      pipe_mutex_unlock(tr_scr->list_mutex);
      return -ESRCH;
   }

   pipe_mutex_lock(tr_ctx->draw_mutex);
   if (tr_ctx->draw_blocked & RBUG_BLOCK_RULE) {
      if (step->step & RBUG_BLOCK_RULE)
         tr_ctx->draw_blocked &= ~RBUG_BLOCK_MASK;
   } else {
      tr_ctx->draw_blocked &= ~step->step;
   }
   pipe_mutex_unlock(tr_ctx->draw_mutex);

#ifdef PIPE_THREAD_HAVE_CONDVAR
   pipe_condvar_broadcast(tr_ctx->draw_cond);
#endif

   pipe_mutex_unlock(tr_scr->list_mutex);

   return 0;
}

static int
trace_rbug_context_draw_unblock(struct trace_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_context_draw_unblock *unblock = (struct rbug_proto_context_draw_unblock *)header;

   struct trace_screen *tr_scr = tr_rbug->tr_scr;
   struct trace_context *tr_ctx = NULL;

   pipe_mutex_lock(tr_scr->list_mutex);
   tr_ctx = trace_rbug_get_context_locked(tr_scr, unblock->context);

   if (!tr_ctx) {
      pipe_mutex_unlock(tr_scr->list_mutex);
      return -ESRCH;
   }

   pipe_mutex_lock(tr_ctx->draw_mutex);
   if (tr_ctx->draw_blocked & RBUG_BLOCK_RULE) {
      if (unblock->unblock & RBUG_BLOCK_RULE)
         tr_ctx->draw_blocked &= ~RBUG_BLOCK_MASK;
   } else {
      tr_ctx->draw_blocked &= ~unblock->unblock;
   }
   tr_ctx->draw_blocker &= ~unblock->unblock;
   pipe_mutex_unlock(tr_ctx->draw_mutex);

#ifdef PIPE_THREAD_HAVE_CONDVAR
   pipe_condvar_broadcast(tr_ctx->draw_cond);
#endif

   pipe_mutex_unlock(tr_scr->list_mutex);

   return 0;
}

static int
trace_rbug_context_draw_rule(struct trace_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_context_draw_rule *rule = (struct rbug_proto_context_draw_rule *)header;

   struct trace_screen *tr_scr = tr_rbug->tr_scr;
   struct trace_context *tr_ctx = NULL;

   pipe_mutex_lock(tr_scr->list_mutex);
   tr_ctx = trace_rbug_get_context_locked(tr_scr, rule->context);

   if (!tr_ctx) {
      pipe_mutex_unlock(tr_scr->list_mutex);
      return -ESRCH;
   }

   pipe_mutex_lock(tr_ctx->draw_mutex);
   tr_ctx->draw_rule.vs = U642VOID(rule->vertex);
   tr_ctx->draw_rule.fs = U642VOID(rule->fragment);
   tr_ctx->draw_rule.tex = U642VOID(rule->texture);
   tr_ctx->draw_rule.surf = U642VOID(rule->surface);
   tr_ctx->draw_rule.blocker = rule->block;
   tr_ctx->draw_blocker |= RBUG_BLOCK_RULE;
   pipe_mutex_unlock(tr_ctx->draw_mutex);

#ifdef PIPE_THREAD_HAVE_CONDVAR
   pipe_condvar_broadcast(tr_ctx->draw_cond);
#endif

   pipe_mutex_unlock(tr_scr->list_mutex);

   return 0;
}

static int
trace_rbug_context_flush(struct trace_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_context_flush *flush = (struct rbug_proto_context_flush *)header;

   struct trace_screen *tr_scr = tr_rbug->tr_scr;
   struct trace_context *tr_ctx = NULL;

   pipe_mutex_lock(tr_scr->list_mutex);
   tr_ctx = trace_rbug_get_context_locked(tr_scr, flush->context);

   if (!tr_ctx) {
      pipe_mutex_unlock(tr_scr->list_mutex);
      return -ESRCH;
   }

   /* protect the pipe context */
   trace_dump_call_lock();

   tr_ctx->pipe->flush(tr_ctx->pipe, flush->flags, NULL);

   trace_dump_call_unlock();
   pipe_mutex_unlock(tr_scr->list_mutex);

   return 0;
}

static int
trace_rbug_shader_list(struct trace_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_shader_list *list = (struct rbug_proto_shader_list *)header;

   struct trace_screen *tr_scr = tr_rbug->tr_scr;
   struct trace_context *tr_ctx = NULL;
   struct trace_shader *tr_shdr = NULL;
   struct tr_list *ptr;
   rbug_shader_t *shdrs;
   int i = 0;

   pipe_mutex_lock(tr_scr->list_mutex);
   tr_ctx = trace_rbug_get_context_locked(tr_scr, list->context);

   if (!tr_ctx) {
      pipe_mutex_unlock(tr_scr->list_mutex);
      return -ESRCH;
   }

   pipe_mutex_lock(tr_ctx->list_mutex);
   shdrs = MALLOC(tr_ctx->num_shaders * sizeof(rbug_shader_t));
   foreach(ptr, &tr_ctx->shaders) {
      tr_shdr = (struct trace_shader *)((char*)ptr - offsetof(struct trace_shader, list));
      shdrs[i++] = VOID2U64(tr_shdr);
   }

   pipe_mutex_unlock(tr_ctx->list_mutex);
   pipe_mutex_unlock(tr_scr->list_mutex);

   rbug_send_shader_list_reply(tr_rbug->con, serial, shdrs, i, NULL);
   FREE(shdrs);

   return 0;
}

static int
trace_rbug_shader_info(struct trace_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   struct rbug_proto_shader_info *info = (struct rbug_proto_shader_info *)header;

   struct trace_screen *tr_scr = tr_rbug->tr_scr;
   struct trace_context *tr_ctx = NULL;
   struct trace_shader *tr_shdr = NULL;
   unsigned original_len;
   unsigned replaced_len;

   pipe_mutex_lock(tr_scr->list_mutex);
   tr_ctx = trace_rbug_get_context_locked(tr_scr, info->context);

   if (!tr_ctx) {
      pipe_mutex_unlock(tr_scr->list_mutex);
      return -ESRCH;
   }

   pipe_mutex_lock(tr_ctx->list_mutex);

   tr_shdr = trace_rbug_get_shader_locked(tr_ctx, info->shader);

   if (!tr_shdr) {
      pipe_mutex_unlock(tr_ctx->list_mutex);
      pipe_mutex_unlock(tr_scr->list_mutex);
      return -ESRCH;
   }

   /* just in case */
   assert(sizeof(struct tgsi_token) == 4);

   original_len = tgsi_num_tokens(tr_shdr->tokens);
   if (tr_shdr->replaced_tokens)
      replaced_len = tgsi_num_tokens(tr_shdr->replaced_tokens);
   else
      replaced_len = 0;

   rbug_send_shader_info_reply(tr_rbug->con, serial,
                               (uint32_t*)tr_shdr->tokens, original_len,
                               (uint32_t*)tr_shdr->replaced_tokens, replaced_len,
                               tr_shdr->disabled,
                               NULL);

   pipe_mutex_unlock(tr_ctx->list_mutex);
   pipe_mutex_unlock(tr_scr->list_mutex);

   return 0;
}

static int
trace_rbug_shader_disable(struct trace_rbug *tr_rbug, struct rbug_header *header)
{
   struct rbug_proto_shader_disable *dis = (struct rbug_proto_shader_disable *)header;

   struct trace_screen *tr_scr = tr_rbug->tr_scr;
   struct trace_context *tr_ctx = NULL;
   struct trace_shader *tr_shdr = NULL;

   pipe_mutex_lock(tr_scr->list_mutex);
   tr_ctx = trace_rbug_get_context_locked(tr_scr, dis->context);

   if (!tr_ctx) {
      pipe_mutex_unlock(tr_scr->list_mutex);
      return -ESRCH;
   }

   pipe_mutex_lock(tr_ctx->list_mutex);

   tr_shdr = trace_rbug_get_shader_locked(tr_ctx, dis->shader);

   if (!tr_shdr) {
      pipe_mutex_unlock(tr_ctx->list_mutex);
      pipe_mutex_unlock(tr_scr->list_mutex);
      return -ESRCH;
   }

   tr_shdr->disabled = dis->disable;

   pipe_mutex_unlock(tr_ctx->list_mutex);
   pipe_mutex_unlock(tr_scr->list_mutex);

   return 0;
}

static int
trace_rbug_shader_replace(struct trace_rbug *tr_rbug, struct rbug_header *header)
{
   struct rbug_proto_shader_replace *rep = (struct rbug_proto_shader_replace *)header;

   struct trace_screen *tr_scr = tr_rbug->tr_scr;
   struct trace_context *tr_ctx = NULL;
   struct trace_shader *tr_shdr = NULL;
   struct pipe_context *pipe = NULL;
   void *state;

   pipe_mutex_lock(tr_scr->list_mutex);
   tr_ctx = trace_rbug_get_context_locked(tr_scr, rep->context);

   if (!tr_ctx) {
      pipe_mutex_unlock(tr_scr->list_mutex);
      return -ESRCH;
   }

   pipe_mutex_lock(tr_ctx->list_mutex);

   tr_shdr = trace_rbug_get_shader_locked(tr_ctx, rep->shader);

   if (!tr_shdr) {
      pipe_mutex_unlock(tr_ctx->list_mutex);
      pipe_mutex_unlock(tr_scr->list_mutex);
      return -ESRCH;
   }

   /* protect the pipe context */
   trace_dump_call_lock();

   pipe = tr_ctx->pipe;

   /* remove old replaced shader */
   if (tr_shdr->replaced) {
      if (tr_ctx->curr.fs == tr_shdr || tr_ctx->curr.vs == tr_shdr)
         trace_shader_bind_locked(pipe, tr_shdr, tr_shdr->state);

      FREE(tr_shdr->replaced_tokens);
      trace_shader_delete_locked(pipe, tr_shdr, tr_shdr->replaced);
      tr_shdr->replaced = NULL;
      tr_shdr->replaced_tokens = NULL;
   }

   /* empty inputs means restore old which we did above */
   if (rep->tokens_len == 0)
      goto out;

   tr_shdr->replaced_tokens = tgsi_dup_tokens((struct tgsi_token *)rep->tokens);
   if (!tr_shdr->replaced_tokens)
      goto err;

   state = trace_shader_create_locked(pipe, tr_shdr, tr_shdr->replaced_tokens);
   if (!state)
      goto err;

   /* bind new shader if the shader is currently a bound */
   if (tr_ctx->curr.fs == tr_shdr || tr_ctx->curr.vs == tr_shdr)
      trace_shader_bind_locked(pipe, tr_shdr, state);

   /* save state */
   tr_shdr->replaced = state;

out:
   trace_dump_call_unlock();
   pipe_mutex_unlock(tr_ctx->list_mutex);
   pipe_mutex_unlock(tr_scr->list_mutex);

   return 0;

err:
   FREE(tr_shdr->replaced_tokens);
   tr_shdr->replaced = NULL;
   tr_shdr->replaced_tokens = NULL;

   trace_dump_call_unlock();
   pipe_mutex_unlock(tr_ctx->list_mutex);
   pipe_mutex_unlock(tr_scr->list_mutex);
   return -EINVAL;
}

static boolean
trace_rbug_header(struct trace_rbug *tr_rbug, struct rbug_header *header, uint32_t serial)
{
   int ret = 0;

   switch(header->opcode) {
      case RBUG_OP_PING:
         rbug_send_ping_reply(tr_rbug->con, serial, NULL);
         break;
      case RBUG_OP_TEXTURE_LIST:
         ret = trace_rbug_texture_list(tr_rbug, header, serial);
         break;
      case RBUG_OP_TEXTURE_INFO:
         ret = trace_rbug_texture_info(tr_rbug, header, serial);
         break;
      case RBUG_OP_TEXTURE_READ:
         ret = trace_rbug_texture_read(tr_rbug, header, serial);
         break;
      case RBUG_OP_CONTEXT_LIST:
         ret = trace_rbug_context_list(tr_rbug, header, serial);
         break;
      case RBUG_OP_CONTEXT_INFO:
         ret = trace_rbug_context_info(tr_rbug, header, serial);
         break;
      case RBUG_OP_CONTEXT_DRAW_BLOCK:
         ret = trace_rbug_context_draw_block(tr_rbug, header, serial);
         break;
      case RBUG_OP_CONTEXT_DRAW_STEP:
         ret = trace_rbug_context_draw_step(tr_rbug, header, serial);
         break;
      case RBUG_OP_CONTEXT_DRAW_UNBLOCK:
         ret = trace_rbug_context_draw_unblock(tr_rbug, header, serial);
         break;
      case RBUG_OP_CONTEXT_DRAW_RULE:
         ret = trace_rbug_context_draw_rule(tr_rbug, header, serial);
         break;
      case RBUG_OP_CONTEXT_FLUSH:
         ret = trace_rbug_context_flush(tr_rbug, header, serial);
         break;
      case RBUG_OP_SHADER_LIST:
         ret = trace_rbug_shader_list(tr_rbug, header, serial);
         break;
      case RBUG_OP_SHADER_INFO:
         ret = trace_rbug_shader_info(tr_rbug, header, serial);
         break;
      case RBUG_OP_SHADER_DISABLE:
         ret = trace_rbug_shader_disable(tr_rbug, header);
         break;
      case RBUG_OP_SHADER_REPLACE:
         ret = trace_rbug_shader_replace(tr_rbug, header);
         break;
      default:
         debug_printf("%s - unsupported opcode %u\n", __FUNCTION__, header->opcode);
         ret = -ENOSYS;
         break;
   }
   rbug_free_header(header);

   if (ret)
      rbug_send_error_reply(tr_rbug->con, serial, ret, NULL);

   return TRUE;
}

static void
trace_rbug_con(struct trace_rbug *tr_rbug)
{
   struct rbug_header *header;
   uint32_t serial;

   debug_printf("%s - connection received\n", __FUNCTION__);

   while(tr_rbug->running) {
      header = rbug_get_message(tr_rbug->con, &serial);
      if (!header)
         break;

      if (!trace_rbug_header(tr_rbug, header, serial))
         break;
   }

   debug_printf("%s - connection closed\n", __FUNCTION__);

   rbug_disconnect(tr_rbug->con);
   tr_rbug->con = NULL;
}

PIPE_THREAD_ROUTINE(trace_rbug_thread, void_tr_rbug)
{
   struct trace_rbug *tr_rbug = void_tr_rbug;
   uint16_t port = 13370;
   int s = -1;
   int c;

   u_socket_init();

   for (;port <= 13379 && s < 0; port++)
      s = u_socket_listen_on_port(port);

   if (s < 0) {
      debug_printf("trace_rbug - failed to listen\n");
      return NULL;
   }

   u_socket_block(s, false);

   debug_printf("trace_rbug - remote debugging listening on port %u\n", --port);

   while(tr_rbug->running) {
      os_time_sleep(1);

      c = u_socket_accept(s);
      if (c < 0)
         continue;

      u_socket_block(c, true);
      tr_rbug->con = rbug_from_socket(c);

      trace_rbug_con(tr_rbug);

      u_socket_close(c);
   }

   u_socket_close(s);

   u_socket_stop();

   return NULL;
}

/**********************************************************
 *
 */

struct trace_rbug *
trace_rbug_start(struct trace_screen *tr_scr)
{
   struct trace_rbug *tr_rbug = CALLOC_STRUCT(trace_rbug);
   if (!tr_rbug)
      return NULL;

   tr_rbug->tr_scr = tr_scr;
   tr_rbug->running = TRUE;
   tr_rbug->thread = pipe_thread_create(trace_rbug_thread, tr_rbug);

   return tr_rbug;
}

void
trace_rbug_stop(struct trace_rbug *tr_rbug)
{
   if (!tr_rbug)
      return;

   tr_rbug->running = false;
   pipe_thread_wait(tr_rbug->thread);

   FREE(tr_rbug);

   return;
}

void
trace_rbug_notify_draw_blocked(struct trace_context *tr_ctx)
{
   struct trace_screen *tr_scr = trace_screen(tr_ctx->base.screen);
   struct trace_rbug *tr_rbug = tr_scr->rbug;

   if (tr_rbug && tr_rbug->con)
      rbug_send_context_draw_blocked(tr_rbug->con,
                                     VOID2U64(tr_ctx), tr_ctx->draw_blocked, NULL);
}
