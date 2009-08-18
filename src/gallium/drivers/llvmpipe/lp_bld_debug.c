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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include <udis86.h>

#include "util/u_debug.h"
#include "lp_bld_debug.h"


void
lp_disassemble(const void* func)
{
   ud_t ud_obj;

   ud_init(&ud_obj);

   ud_set_input_buffer(&ud_obj, (void*)func, 0xffff);
   ud_set_pc(&ud_obj, (uint64_t) (uintptr_t) func);

#ifdef PIPE_ARCH_X86
   ud_set_mode(&ud_obj, 32);
#endif
#ifdef PIPE_ARCH_X86_64
   ud_set_mode(&ud_obj, 64);
#endif

   ud_set_syntax(&ud_obj, UD_SYN_ATT);

   while (ud_disassemble(&ud_obj)) {
#ifdef PIPE_ARCH_X86
      debug_printf("%08lx: ", (unsigned long)ud_insn_off(&ud_obj));
#endif
#ifdef PIPE_ARCH_X86_64
      debug_printf("%016llx: ", (unsigned long long)ud_insn_off(&ud_obj));
#endif

#if 0
      debug_printf("%-16s ", ud_insn_hex(&ud_obj));
#endif

      debug_printf("%s\n", ud_insn_asm(&ud_obj));

      if (ud_obj.mnemonic == UD_Iret)
         break;
   }
   debug_printf("\n");
}
