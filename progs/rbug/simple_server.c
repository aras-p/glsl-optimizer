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
#include "util/u_memory.h"
#include "util/u_debug.h"
#include "util/u_network.h"

#include "rbug/rbug.h"

static void wait()
{
   int s = u_socket_listen_on_port(13370);
   int c = u_socket_accept(s);
   struct rbug_connection *con = rbug_from_socket(c);
   struct rbug_header *header;
   rbug_texture_t texs[2];
   uint32_t serial;
   texs[0] = 1337;
   texs[1] = 7331;

   assert(s >= 0);
   assert(c >= 0);
   assert(con);
   debug_printf("Connection get!\n");

   debug_printf("Waiting for get textures\n");
   header = rbug_get_message(con, &serial);
   assert(header);
   assert(header->opcode == RBUG_OP_TEXTURE_LIST);
   rbug_free_header(header);

   rbug_send_texture_list_reply(con, serial, texs, 2, NULL);
   rbug_disconnect(con);
}

int main(int argc, char** argv)
{
   wait();
   return 0;
}
