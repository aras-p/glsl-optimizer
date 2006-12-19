/*
 * Copyright (C) 2006 Ben Skeggs.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * Authors:
 *   Ben Skeggs <darktama@iinet.net.au>
 */

#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "nouveau_shader.h"

#define PASS1_OK	0
#define PASS1_KILL	1
#define PASS1_FAIL	2

struct pass1_rec {
   unsigned int temp[NVS_MAX_TEMPS];
   unsigned int result[NVS_MAX_ATTRIBS];
   unsigned int address[NVS_MAX_ADDRESS];
   unsigned int cc[2];
};

static void
pass1_remove_fragment(nvsPtr nvs, nvsFragmentList *item)
{
   if (item->prev) item->prev->next = item->next;
   if (item->next) item->next->prev = item->prev;
   if (nvs->list_head == item) nvs->list_head = item->next;
   if (nvs->list_tail == item) nvs->list_tail = item->prev;

   nvs->inst_count--;
}

static int
pass1_result_needed(struct pass1_rec *rec, nvsInstruction *inst)
{
   if (inst->cond_update && rec->cc[inst->cond_reg])
      return 1;
   /* Only write components that are read later */
   if (inst->dest.file == NVS_FILE_TEMP)
      return (inst->mask & rec->temp[inst->dest.index]);
   if (inst->dest.file == NVS_FILE_ADDRESS)
      return (inst->mask & rec->address[inst->dest.index]);
   /* No point writing result components that are written later */
   if (inst->dest.file == NVS_FILE_RESULT)
      return (inst->mask & ~rec->result[inst->dest.index]);
   assert(0);
}

static void
pass1_track_result(struct pass1_rec *rec, nvsInstruction *inst)
{
   if (inst->cond_test)
      rec->cc[inst->cond_reg] = 1;
   if (inst->dest.file == NVS_FILE_TEMP) {
      inst->mask &= rec->temp[inst->dest.index];
   } else if (inst->dest.file == NVS_FILE_RESULT) {
      inst->mask &= ~rec->result[inst->dest.index];
      rec->result[inst->dest.index] |= inst->mask;
   } else if (inst->dest.file == NVS_FILE_ADDRESS) {
      inst->mask &= rec->address[inst->dest.index];
   }
}

static void
pass1_track_source(nouveauShader *nvs, nvsInstruction *inst, int pos,
      		   unsigned int read)
{
   struct pass1_rec *rec = nvs->pass_rec;
   nvsRegister *src = &inst->src[pos];
   unsigned int really_read = 0;
   int i,sc;

   /* Account for swizzling */
   for (i=0; i<4; i++)
      if (read & (1<<i)) really_read |= (1 << src->swizzle[i]);

   /* Track register reads */
   if (src->file == NVS_FILE_TEMP) {
      if (nvs->temps[src->index].last_use == -1)
	 nvs->temps[src->index].last_use = inst->header.position;
      rec->temp [src->index] |= really_read;
   } else if (src->indexed) {
      rec->address[src->addr_reg] |= (1<<src->addr_comp);
   }

   /* Modify swizzle to only access read components */
   /* Find a component that is used.. */
   for (sc=0;sc<4;sc++)
      if (really_read & (1<<sc))
	 break;
   /* Now set all unused components to that value */
   for (i=0;i<4;i++)
      if (!(read & (1<<i))) src->swizzle[i] = sc;
}

static int
pass1_check_instruction(nouveauShader *nvs, nvsInstruction *inst)
{
   struct pass1_rec *rec = nvs->pass_rec;
   unsigned int read0, read1, read2;

   if (inst->op != NVS_OP_KIL) {
      if (!pass1_result_needed(rec, inst))
	 return PASS1_KILL;
   }
   pass1_track_result(rec, inst);

   read0 = read1 = read2 = 0;

   switch (inst->op) {
   case NVS_OP_FLR:
   case NVS_OP_FRC:
   case NVS_OP_MOV:
   case NVS_OP_SSG:
   case NVS_OP_ARL:
      read0 = inst->mask;
      break;
   case NVS_OP_ADD:
   case NVS_OP_MAX:
   case NVS_OP_MIN:
   case NVS_OP_MUL:
   case NVS_OP_SEQ:
   case NVS_OP_SFL:
   case NVS_OP_SGE:
   case NVS_OP_SGT:
   case NVS_OP_SLE:
   case NVS_OP_SLT:
   case NVS_OP_SNE:
   case NVS_OP_STR:
   case NVS_OP_SUB:
      read0 = inst->mask;
      read1 = inst->mask;
      break;
   case NVS_OP_CMP:
   case NVS_OP_LRP:
   case NVS_OP_MAD:
      read0 = inst->mask;
      read1 = inst->mask;
      read2 = inst->mask;
      break;
   case NVS_OP_XPD:
      if (inst->mask & SMASK_X) read0 |= SMASK_Y|SMASK_Z;
      if (inst->mask & SMASK_Y) read0 |= SMASK_X|SMASK_Z;
      if (inst->mask & SMASK_Z) read0 |= SMASK_X|SMASK_Y;
      read1 = read0;
      break;
   case NVS_OP_COS:
   case NVS_OP_EX2:
   case NVS_OP_EXP:
   case NVS_OP_LG2:
   case NVS_OP_LOG:
   case NVS_OP_RCC:
   case NVS_OP_RCP:
   case NVS_OP_RSQ:
   case NVS_OP_SCS:
   case NVS_OP_SIN:
      read0 = SMASK_X;
      break;
   case NVS_OP_POW:
      read0 = SMASK_X;
      read1 = SMASK_X;
      break;
   case NVS_OP_DIV:
      read0 = inst->mask;
      read1 = SMASK_X;
      break;
   case NVS_OP_DP2:
      read0 = SMASK_X|SMASK_Y;
      read1 = SMASK_X|SMASK_Y;
      break;
   case NVS_OP_DP3:
   case NVS_OP_RFL:
      read0 = SMASK_X|SMASK_Y|SMASK_Z;
      read1 = SMASK_X|SMASK_Y|SMASK_Z;
      break;
   case NVS_OP_DP4:
      read0 = SMASK_ALL;
      read1 = SMASK_ALL;
      break;
   case NVS_OP_DPH:
      read0 = SMASK_X|SMASK_Y|SMASK_Z;
      read1 = SMASK_ALL;
      break;
   case NVS_OP_DST:
      if (inst->mask & SMASK_Y) read0 = read1 = SMASK_Y;
      if (inst->mask & SMASK_Z) read0 |= SMASK_Z;
      if (inst->mask & SMASK_W) read1 |= SMASK_W;
      break;
   case NVS_OP_NRM:
      read0 = SMASK_X|SMASK_Y|SMASK_Z;
      break;
   case NVS_OP_PK2H:
   case NVS_OP_PK2US:
      read0 = SMASK_X|SMASK_Y;
      break;
   case NVS_OP_DDX:
   case NVS_OP_DDY:
   case NVS_OP_UP2H:
   case NVS_OP_UP2US:
   case NVS_OP_PK4B:
   case NVS_OP_PK4UB:
   case NVS_OP_UP4B:
   case NVS_OP_UP4UB:
      read0 = SMASK_ALL;
      break;
   case NVS_OP_X2D:
      read1 = SMASK_X|SMASK_Y;
      if (inst->mask & (SMASK_X|SMASK_Z)) {
	 read0 |= SMASK_X;
	 read2 |= SMASK_X|SMASK_Y;
      }
      if (inst->mask & (SMASK_Y|SMASK_W)) {
	 read0 |= SMASK_Y;
	 read2 |= SMASK_Z|SMASK_W;
      }
      break;
   case NVS_OP_LIT:
      read0 |= SMASK_X|SMASK_Y|SMASK_W;
      break;
   case NVS_OP_TEX:
   case NVS_OP_TXP:
   case NVS_OP_TXL:
   case NVS_OP_TXB:
      read0 = SMASK_ALL;
      break;
   case NVS_OP_TXD:
      read0 = SMASK_ALL;
      read1 = SMASK_ALL;
      read2 = SMASK_ALL;
      break;
   case NVS_OP_KIL:
      break;
   default:
      fprintf(stderr, "Unknown sop=%d", inst->op);
      return PASS1_FAIL;
   }

   /* Any values that are written by this inst can't have been read further up */
   if (inst->dest.file == NVS_FILE_TEMP)
      rec->temp[inst->dest.index] &= ~inst->mask;

   if (read0) pass1_track_source(nvs, inst, 0, read0);
   if (read1) pass1_track_source(nvs, inst, 1, read1);
   if (read2) pass1_track_source(nvs, inst, 2, read2);

   return PASS1_OK;
}

/* Some basic dead code elimination
 *   - Remove unused instructions
 *   - Don't write unused register components
 *   - Modify swizzles to not reference unneeded components.
 */
GLboolean
nouveau_shader_pass1(nvsPtr nvs)
{
   nvsFragmentList *list = nvs->list_tail;
   int i;

   for (i=0; i<NVS_MAX_TEMPS; i++)
      nvs->temps[i].last_use = -1;

   nvs->pass_rec = calloc(1, sizeof(struct pass1_rec));

   while (list) {
      assert(list->fragment->type == NVS_INSTRUCTION);

      switch(pass1_check_instruction(nvs, (nvsInstruction *)list->fragment)) {
      case PASS1_OK:
	 break;
      case PASS1_KILL:
	 pass1_remove_fragment(nvs, list);
	 break;
      case PASS1_FAIL:
      default:
	 free(nvs->pass_rec);
	 nvs->pass_rec = NULL;
	 return GL_FALSE;
      }

      list = list->prev;
   }

   free(nvs->pass_rec);
   nvs->pass_rec = NULL;

   return GL_TRUE;
}


