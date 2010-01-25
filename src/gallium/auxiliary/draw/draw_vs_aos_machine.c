/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "pipe/p_config.h"


#include "pipe/p_shader_tokens.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_exec.h"
#include "draw_vs.h"
#include "draw_vs_aos.h"
#include "draw_vertex.h"

#ifdef PIPE_ARCH_X86

#include "rtasm/rtasm_x86sse.h"


#define X87_CW_EXCEPTION_INV_OP       (1<<0)
#define X87_CW_EXCEPTION_DENORM_OP    (1<<1)
#define X87_CW_EXCEPTION_ZERO_DIVIDE  (1<<2)
#define X87_CW_EXCEPTION_OVERFLOW     (1<<3)
#define X87_CW_EXCEPTION_UNDERFLOW    (1<<4)
#define X87_CW_EXCEPTION_PRECISION    (1<<5)
#define X87_CW_PRECISION_SINGLE       (0<<8)
#define X87_CW_PRECISION_RESERVED     (1<<8)
#define X87_CW_PRECISION_DOUBLE       (2<<8)
#define X87_CW_PRECISION_DOUBLE_EXT   (3<<8)
#define X87_CW_PRECISION_MASK         (3<<8)
#define X87_CW_ROUND_NEAREST          (0<<10)
#define X87_CW_ROUND_DOWN             (1<<10)
#define X87_CW_ROUND_UP               (2<<10)
#define X87_CW_ROUND_ZERO             (3<<10)
#define X87_CW_ROUND_MASK             (3<<10)
#define X87_CW_INFINITY               (1<<12)


void PIPE_CDECL aos_do_lit( struct aos_machine *machine,
                            float *result,
                            const float *in,
                            unsigned count )
{
   if (in[0] > 0) 
   {
      if (in[1] <= 0.0) 
      {
         result[0] = 1.0F;
         result[1] = in[0];
         result[2] = 0.0F;
         result[3] = 1.0F;
      }
      else
      {
         const float epsilon = 1.0F / 256.0F;    
         float exponent = CLAMP(in[3], -(128.0F - epsilon), (128.0F - epsilon));
         result[0] = 1.0F;
         result[1] = in[0];
         result[2] = powf(in[1], exponent);
         result[3] = 1.0;
      }
   }
   else 
   {
      result[0] = 1.0F;
      result[1] = 0.0;
      result[2] = 0.0;
      result[3] = 1.0F;
   }
}


static void PIPE_CDECL do_lit_lut( struct aos_machine *machine,
                                   float *result,
                                   const float *in,
                                   unsigned count )
{
   if (in[0] > 0) 
   {
      if (in[1] <= 0.0) 
      {
         result[0] = 1.0F;
         result[1] = in[0];
         result[2] = 0.0F;
         result[3] = 1.0F;
         return;
      }
      
      if (machine->lit_info[count].shine_tab->exponent != in[3]) {
         machine->lit_info[count].func = aos_do_lit;
         goto no_luck;
      }

      if (in[1] <= 1.0)
      {
         const float *tab = machine->lit_info[count].shine_tab->values;
         float f = in[1] * 256;
         int k = (int)f;
         float frac = f - (float)k;
         
         result[0] = 1.0F;
         result[1] = in[0];
         result[2] = tab[k] + frac*(tab[k+1]-tab[k]);
         result[3] = 1.0;
         return;
      }
      
   no_luck:
      {
         const float epsilon = 1.0F / 256.0F;    
         float exponent = CLAMP(in[3], -(128.0F - epsilon), (128.0F - epsilon));
         result[0] = 1.0F;
         result[1] = in[0];
         result[2] = powf(in[1], exponent);
         result[3] = 1.0;
      }
   }
   else 
   {
      result[0] = 1.0F;
      result[1] = 0.0;
      result[2] = 0.0;
      result[3] = 1.0F;
   }
}


static void do_populate_lut( struct shine_tab *tab,
                             float unclamped_exponent )
{
   const float epsilon = 1.0F / 256.0F;    
   float exponent = CLAMP(unclamped_exponent, -(128.0F - epsilon), (128.0F - epsilon));
   unsigned i;

   tab->exponent = unclamped_exponent; /* for later comparison */
   
   tab->values[0] = 0;
   if (exponent == 0) {
      for (i = 1; i < 258; i++) {
         tab->values[i] = 1.0;
      }      
   }
   else {
      for (i = 1; i < 258; i++) {
         tab->values[i] = powf((float)i * epsilon, exponent);
      }
   }
}




static void PIPE_CDECL populate_lut( struct aos_machine *machine,
                                     float *result,
                                     const float *in,
                                     unsigned count )
{
   unsigned i, tab;

   /* Search for an existing table for this value.  Note that without
    * static analysis we don't really know if in[3] will be constant,
    * but it usually is...
    */
   for (tab = 0; tab < 4; tab++) {
      if (machine->shine_tab[tab].exponent == in[3]) {
         goto found;
      }
   }

   for (tab = 0, i = 1; i < 4; i++) {
      if (machine->shine_tab[i].last_used < machine->shine_tab[tab].last_used)
         tab = i;
   }

   if (machine->shine_tab[tab].last_used == machine->now) {
      /* No unused tables (this is not a ffvertex program...).  Just
       * call pow each time:
       */
      machine->lit_info[count].func = aos_do_lit;
      machine->lit_info[count].func( machine, result, in, count );
      return;
   }
   else {
      do_populate_lut( &machine->shine_tab[tab], in[3] );
   }

 found:
   machine->shine_tab[tab].last_used = machine->now;
   machine->lit_info[count].shine_tab = &machine->shine_tab[tab];
   machine->lit_info[count].func = do_lit_lut;
   machine->lit_info[count].func( machine, result, in, count );
}


void
draw_vs_aos_machine_constants(struct aos_machine *machine,
                              unsigned slot,
                              const void *constants)
{
   machine->constants[slot] = constants;

   {
      unsigned i;
      for (i = 0; i < MAX_LIT_INFO; i++) {
         machine->lit_info[i].func = populate_lut;
         machine->now++;
      }
   }
}


void draw_vs_aos_machine_viewport( struct aos_machine *machine,
                                   const struct pipe_viewport_state *viewport )
{
   memcpy(machine->scale, viewport->scale, 4 * sizeof(float));
   memcpy(machine->translate, viewport->translate, 4 * sizeof(float));
}



void draw_vs_aos_machine_destroy( struct aos_machine *machine )
{
   align_free(machine);
}

struct aos_machine *draw_vs_aos_machine( void )
{
   struct aos_machine *machine;
   unsigned i;
   float inv = 1.0f/255.0f;
   float f255 = 255.0f;

   machine = align_malloc(sizeof(struct aos_machine), 16);
   if (!machine)
      return NULL;

   memset(machine, 0, sizeof(*machine));

   ASSIGN_4V(machine->internal[IMM_SWZ],       1.0f,  -1.0f,  0.0f, 1.0f);
   *(unsigned *)&machine->internal[IMM_SWZ][3] = 0xffffffff;

   ASSIGN_4V(machine->internal[IMM_ONES],      1.0f,  1.0f,  1.0f,  1.0f);
   ASSIGN_4V(machine->internal[IMM_NEGS],     -1.0f, -1.0f, -1.0f, -1.0f);
   ASSIGN_4V(machine->internal[IMM_IDENTITY],  0.0f,  0.0f,  0.0f,  1.0f);
   ASSIGN_4V(machine->internal[IMM_INV_255],   inv,   inv,   inv,   inv);
   ASSIGN_4V(machine->internal[IMM_255],       f255,  f255,  f255,  f255);
   ASSIGN_4V(machine->internal[IMM_RSQ],       -.5f,  1.5f,  0.0f,  0.0f);


   machine->fpu_rnd_nearest = (X87_CW_EXCEPTION_INV_OP |
                               X87_CW_EXCEPTION_DENORM_OP |
                               X87_CW_EXCEPTION_ZERO_DIVIDE |
                               X87_CW_EXCEPTION_OVERFLOW |
                               X87_CW_EXCEPTION_UNDERFLOW |
                               X87_CW_EXCEPTION_PRECISION |
                               (1<<6) |
                               X87_CW_ROUND_NEAREST |
                               X87_CW_PRECISION_DOUBLE_EXT);

   assert(machine->fpu_rnd_nearest == 0x37f);
                               
   machine->fpu_rnd_neg_inf = (X87_CW_EXCEPTION_INV_OP |
                               X87_CW_EXCEPTION_DENORM_OP |
                               X87_CW_EXCEPTION_ZERO_DIVIDE |
                               X87_CW_EXCEPTION_OVERFLOW |
                               X87_CW_EXCEPTION_UNDERFLOW |
                               X87_CW_EXCEPTION_PRECISION |
                               (1<<6) |
                               X87_CW_ROUND_DOWN |
                               X87_CW_PRECISION_DOUBLE_EXT);

   for (i = 0; i < MAX_SHINE_TAB; i++)
      do_populate_lut( &machine->shine_tab[i], 1.0f );

   return machine;
}

#else

void draw_vs_aos_machine_viewport( struct aos_machine *machine,
                                   const struct pipe_viewport_state *viewport )
{
}

void
draw_vs_aos_machine_constants(struct aos_machine *machine,
                              unsigned slot,
                              const void *constants)
{
}

void draw_vs_aos_machine_destroy( struct aos_machine *machine )
{
}

struct aos_machine *draw_vs_aos_machine( void )
{
   return NULL;
}
#endif

