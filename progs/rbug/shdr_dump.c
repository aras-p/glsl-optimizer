/*
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VMWARE AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"
#include "util/u_memory.h"
#include "util/u_debug.h"
#include "util/u_network.h"

#include "rbug/rbug.h"

#include "tgsi/tgsi_dump.h"

static void shader_info(struct rbug_connection *con, rbug_context_t ctx)
{
   struct rbug_header *header;
   struct rbug_proto_shader_list_reply *list;
   struct rbug_proto_shader_info_reply *info;
   int i;

   debug_printf("Sending get shaders to %llu\n", (unsigned long long)ctx);
   rbug_send_shader_list(con, ctx, NULL);

   debug_printf("Waiting for shaders from %llu\n", (unsigned long long)ctx);
   header = rbug_get_message(con, NULL);
   assert(header->opcode == RBUG_OP_SHADER_LIST_REPLY);
   list = (struct rbug_proto_shader_list_reply *)header;

   debug_printf("Got shaders:\n");
   for (i = 0; i < list->shaders_len; i++) {
      rbug_send_shader_info(con, ctx, list->shaders[i], NULL);

      header = rbug_get_message(con, NULL);
      assert(header->opcode == RBUG_OP_SHADER_INFO_REPLY);
      info = (struct rbug_proto_shader_info_reply *)header;

      debug_printf("#####################################################\n");
      debug_printf("ctx: %llu shdr: %llu disabled %u\n",
                   (unsigned long long)ctx,
                   (unsigned long long)list->shaders[i],
                   info->disabled);

      /* just to be sure */
      assert(sizeof(struct tgsi_token) == 4);

      debug_printf("-----------------------------------------------------\n");
      tgsi_dump((struct tgsi_token *)info->original, 0);

      if (info->replaced_len > 0) {
         debug_printf("-----------------------------------------------------\n");
         tgsi_dump((struct tgsi_token *)info->replaced, 0);
      }

      rbug_free_header(header);
   }

   debug_printf("#####################################################\n");
   rbug_free_header(&list->header);
}

static void talk()
{
   int c = u_socket_connect("localhost", 13370);
   struct rbug_connection *con = rbug_from_socket(c);
   struct rbug_header *header;
   struct rbug_proto_context_list_reply *list;
   int i;

   assert(c >= 0);
   assert(con);
   debug_printf("Connection get!\n");

   debug_printf("Sending get contexts\n");
   rbug_send_context_list(con, NULL);

   debug_printf("Waiting for contexts\n");
   header = rbug_get_message(con, NULL);
   assert(header->opcode == RBUG_OP_CONTEXT_LIST_REPLY);
   list = (struct rbug_proto_context_list_reply *)header;

   debug_printf("Got contexts:\n");
   for (i = 0; i < list->contexts_len; i++) {
      shader_info(con, list->contexts[i]);
   }

   rbug_free_header(&list->header);
   rbug_disconnect(con);
}

int main(int argc, char** argv)
{
   talk();
   return 0;
}
