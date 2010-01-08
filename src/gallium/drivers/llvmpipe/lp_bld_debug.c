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


#ifdef HAVE_UDIS86
#include <udis86.h>
#endif

#include "util/u_math.h"
#include "util/u_debug.h"
#include "lp_bld_debug.h"


/**
 * Check alignment.
 *
 * It is important that this check is not implemented as a macro or inlined
 * function, as the compiler assumptions in respect to alignment of global
 * and stack variables would often make the check a no op, defeating the
 * whole purpose of the exercise.
 */
boolean
lp_check_alignment(const void *ptr, unsigned alignment)
{
   assert(util_is_pot(alignment));
   return ((uintptr_t)ptr & (alignment - 1)) == 0;
}


void
lp_disassemble(const void* func)
{
#ifdef HAVE_UDIS86
   ud_t ud_obj;
   uint64_t max_jmp_pc;

   ud_init(&ud_obj);

   ud_set_input_buffer(&ud_obj, (void*)func, 0xffff);

   max_jmp_pc = (uint64_t) (uintptr_t) func;
   ud_set_pc(&ud_obj, max_jmp_pc);

#ifdef PIPE_ARCH_X86
   ud_set_mode(&ud_obj, 32);
#endif
#ifdef PIPE_ARCH_X86_64
   ud_set_mode(&ud_obj, 64);
#endif

   ud_set_syntax(&ud_obj, UD_SYN_ATT);

   while (ud_disassemble(&ud_obj)) {

#ifdef PIPE_ARCH_X86
      debug_printf("0x%08lx:\t", (unsigned long)ud_insn_off(&ud_obj));
#endif
#ifdef PIPE_ARCH_X86_64
      debug_printf("0x%016llx:\t", (unsigned long long)ud_insn_off(&ud_obj));
#endif

#if 0
      debug_printf("%-16s ", ud_insn_hex(&ud_obj));
#endif

      debug_printf("%s\n", ud_insn_asm(&ud_obj));

      if(ud_obj.mnemonic != UD_Icall) {
         unsigned i;
         for(i = 0; i < 3; ++i) {
            const struct ud_operand *op = &ud_obj.operand[i];
            if (op->type == UD_OP_JIMM){
               uint64_t pc = ud_obj.pc;

               switch (op->size) {
               case 8:
                  pc += op->lval.sbyte;
                  break;
               case 16:
                  pc += op->lval.sword;
                  break;
               case 32:
                  pc += op->lval.sdword;
                  break;
               default:
                  break;
               }
               if(pc > max_jmp_pc)
                  max_jmp_pc = pc;
            }
         }
      }

      if ((ud_insn_off(&ud_obj) >= max_jmp_pc && ud_obj.mnemonic == UD_Iret) ||
           ud_obj.mnemonic == UD_Iinvalid)
         break;
   }

#if 0
   /* Print GDB command, useful to verify udis86 output */
   debug_printf("disassemble %p %p\n", func, (void*)(uintptr_t)ud_obj.pc);
#endif

   debug_printf("\n");
#else
   (void)func;
#endif
}
