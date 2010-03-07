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

#include <stdio.h>

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"
#include "util/u_memory.h"
#include "util/u_debug.h"
#include "util/u_network.h"

#include "rbug/rbug.h"

static void talk(rbug_context_t ctx, rbug_shader_t shdr)
{
   int c = u_socket_connect("localhost", 13370);
   struct rbug_connection *con;
   struct rbug_header *header;

   if (c < 0)
      c = u_socket_connect("localhost", 13370);

   con = rbug_from_socket(c);
   assert(c >= 0);
   assert(con);
   debug_printf("Connection get!\n");

   rbug_send_context_draw_rule(con, ctx, 0, shdr, 0, 0, RBUG_BLOCK_AFTER, NULL);

   rbug_send_ping(con, NULL);

   debug_printf("Sent waiting for reply\n");
   header = rbug_get_message(con, NULL);

   if (header->opcode != RBUG_OP_PING_REPLY)
      debug_printf("Error\n");
   else
      debug_printf("Ok!\n");

   rbug_free_header(header);
   rbug_disconnect(con);
}

static void print_usage()
{
   printf("Usage ctx_rule <context> <fragment>\n");
   exit(-1);
}

int main(int argc, char** argv)
{
   long ctx;
   long shdr;

   if (argc < 3)
      print_usage();

   ctx = atol(argv[1]);
   shdr = atol(argv[2]);

   if (ctx <= 0 && ctx <= 0)
      print_usage();

   talk((uint64_t)ctx, (uint64_t)shdr);

   return 0;
}
