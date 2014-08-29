/*
 * Copyright 2014 Ilia Mirkin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <errno.h>

#include "tgsi/tgsi_text.h"
#include "util/u_debug.h"

#include "codegen/nv50_ir_driver.h"
#include "nv50/nv50_context.h"

/* these headers weren't really meant to be included together */
#undef SB_DATA

#include "nv30/nv30_state.h"
#include "nv30/nvfx_shader.h"

static int
nv30_fp(int chipset, struct tgsi_token tokens[],
        unsigned *size, unsigned **code) {
   struct nv30_fragprog fp;
   memset(&fp, 0, sizeof(fp));
   fp.pipe.tokens = tokens;
   tgsi_scan_shader(fp.pipe.tokens, &fp.info);
   _nvfx_fragprog_translate(chipset >= 0x40 ? 0x4097 : 0x3097, &fp);
   *size = fp.insn_len * 4;
   *code = fp.insn;
   return !fp.translated;
}

static int
nv30_vp(int chipset, struct tgsi_token tokens[],
        unsigned *size, unsigned **code) {
   struct nv30_vertprog vp;
   memset(&vp, 0, sizeof(vp));

   vp.pipe.tokens = tokens;
   tgsi_scan_shader(vp.pipe.tokens, &vp.info);
   _nvfx_vertprog_translate(chipset >= 0x40 ? 0x4097 : 0x3097, &vp);
   *size = vp.nr_insns * 16;
   *code = (unsigned *)vp.insns;
   return !vp.translated;
}

static int
nv30_codegen(int chipset, int type, struct tgsi_token tokens[],
             unsigned *size, unsigned **code) {
   switch (type) {
      case PIPE_SHADER_FRAGMENT:
         return nv30_fp(chipset, tokens, size, code);
      case PIPE_SHADER_VERTEX:
         return nv30_vp(chipset, tokens, size, code);
   }
   _debug_printf("Unexpected shader type: %d\n", type);
   return 1;
}

static int
dummy_assign_slots(struct nv50_ir_prog_info *info)
{
   unsigned i, n, c;

   n = 0;
   for (i = 0; i < info->numInputs; ++i) {
      for (c = 0; c < 4; ++c)
         if (info->in[i].mask & (1 << c))
            info->in[i].slot[c] = n++;
   }

   /* VertexID before InstanceID */
   if (info->io.vertexId < info->numSysVals)
      info->sv[info->io.vertexId].slot[0] = n++;
   if (info->io.instanceId < info->numSysVals)
      info->sv[info->io.instanceId].slot[0] = n++;

   n = 0;
   for (i = 0; i < info->numOutputs; ++i) {
      for (c = 0; c < 4; ++c)
         if (info->out[i].mask & (1 << c))
            info->out[i].slot[c] = n++;
   }
   return 0;
}

static int
nouveau_codegen(int chipset, int type, struct tgsi_token tokens[],
                unsigned *size, unsigned **code) {
   struct nv50_ir_prog_info info = {0};
   int ret;

   info.type = type;
   info.target = chipset;
   info.bin.sourceRep = NV50_PROGRAM_IR_TGSI;
   info.bin.source = tokens;

   info.io.ucpCBSlot = 15;
   info.io.ucpBase = NV50_CB_AUX_UCP_OFFSET;

   info.io.resInfoCBSlot = 15;
   info.io.suInfoBase = NV50_CB_AUX_TEX_MS_OFFSET;
   info.io.msInfoCBSlot = 15;
   info.io.msInfoBase = NV50_CB_AUX_MS_OFFSET;

   info.assignSlots = dummy_assign_slots;

   info.optLevel = debug_get_num_option("NV50_PROG_OPTIMIZE", 3);
   info.dbgFlags = debug_get_num_option("NV50_PROG_DEBUG", 0);

   ret = nv50_ir_generate_code(&info);
   if (ret) {
      _debug_printf("Error compiling program: %d\n", ret);
      return ret;
   }

   *size = info.bin.codeSize;
   *code = info.bin.code;
   return 0;
}

int
main(int argc, char *argv[])
{
   struct tgsi_token tokens[4096];
   int i, chipset = 0, type = -1;
   const char *filename = NULL;
   FILE *f;
   char text[65536] = {0};
   unsigned size, *code;

   for (i = 1; i < argc; i++) {
      if (!strcmp(argv[i], "-a"))
         chipset = strtol(argv[++i], NULL, 16);
      else
         filename = argv[i];
   }

   if (!chipset) {
      _debug_printf("Must specify a chipset (-a)\n");
      return 1;
   }

   if (!filename) {
      _debug_printf("Must specify a filename\n");
      return 1;
   }

   if (!strcmp(filename, "-"))
      f = stdin;
   else
      f = fopen(filename, "r");

   if (f == NULL) {
      _debug_printf("Error opening file '%s': %s\n", filename, strerror(errno));
      return 1;
   }

   if (!fread(text, 1, sizeof(text), f) || ferror(f)) {
      _debug_printf("Error reading file '%s'\n", filename);
      fclose(f);
      return 1;
   }
   fclose(f);

   _debug_printf("Compiling for NV%X\n", chipset);

   if (!strncmp(text, "FRAG", 4))
      type = PIPE_SHADER_FRAGMENT;
   else if (!strncmp(text, "VERT", 4))
      type = PIPE_SHADER_VERTEX;
   else if (!strncmp(text, "GEOM", 4))
      type = PIPE_SHADER_GEOMETRY;
   else if (!strncmp(text, "COMP", 4))
      type = PIPE_SHADER_COMPUTE;
   else {
      _debug_printf("Unrecognized TGSI header\n");
      return 1;
   }

   if (!tgsi_text_translate(text, tokens, Elements(tokens))) {
      _debug_printf("Failed to parse TGSI shader\n");
      return 1;
   }

   if (chipset >= 0x50) {
      i = nouveau_codegen(chipset, type, tokens, &size, &code);
   } else if (chipset >= 0x30) {
      i = nv30_codegen(chipset, type, tokens, &size, &code);
   } else {
      _debug_printf("chipset NV%02X not supported\n", chipset);
      i = 1;
   }
   if (i)
      return i;

   _debug_printf("program binary (%d bytes)\n", size);
   for (i = 0; i < size; i += 4) {
      printf("%08x ", code[i / 4]);
      if (i % (8 * 4) == (7 * 4))
         printf("\n");
   }
   if (i % (8 * 4) != 0)
      printf("\n");

   return 0;
}
