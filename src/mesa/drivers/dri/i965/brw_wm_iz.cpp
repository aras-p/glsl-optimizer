/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */


#include "main/mtypes.h"
#include "brw_fs.h"


#undef P                        /* prompted depth */
#undef C                        /* computed */
#undef N                        /* non-promoted? */

#define P 0
#define C 1
#define N 2

static const struct {
   GLuint mode:2;
   GLuint sd_present:1;
   GLuint sd_to_rt:1;
   GLuint dd_present:1;
   GLuint ds_present:1;
} wm_iz_table[IZ_BIT_MAX] =
{
 { P, 0, 0, 0, 0 },
 { P, 0, 0, 0, 0 },
 { P, 0, 0, 0, 0 },
 { P, 0, 0, 0, 0 },
 { P, 0, 0, 0, 0 },
 { N, 1, 1, 0, 0 },
 { N, 0, 1, 0, 0 },
 { N, 0, 1, 0, 0 },
 { P, 0, 0, 0, 0 },
 { P, 0, 0, 0, 0 },
 { C, 0, 1, 1, 0 },
 { C, 0, 1, 1, 0 },
 { P, 0, 0, 0, 0 },
 { N, 1, 1, 0, 0 },
 { C, 0, 1, 1, 0 },
 { C, 0, 1, 1, 0 },
 { P, 0, 0, 0, 0 },
 { P, 0, 0, 0, 0 },
 { P, 0, 0, 0, 0 },
 { P, 0, 0, 0, 0 },
 { P, 0, 0, 0, 0 },
 { N, 1, 1, 0, 0 },
 { N, 0, 1, 0, 0 },
 { N, 0, 1, 0, 0 },
 { P, 0, 0, 0, 0 },
 { P, 0, 0, 0, 0 },
 { C, 0, 1, 1, 0 },
 { C, 0, 1, 1, 0 },
 { P, 0, 0, 0, 0 },
 { N, 1, 1, 0, 0 },
 { C, 0, 1, 1, 0 },
 { C, 0, 1, 1, 0 },
 { P, 0, 0, 0, 0 },
 { P, 0, 0, 0, 0 },
 { P, 0, 0, 0, 0 },
 { P, 0, 0, 0, 0 },
 { P, 0, 0, 0, 0 },
 { N, 1, 1, 0, 1 },
 { N, 0, 1, 0, 1 },
 { N, 0, 1, 0, 1 },
 { P, 0, 0, 0, 0 },
 { P, 0, 0, 0, 0 },
 { C, 0, 1, 1, 1 },
 { C, 0, 1, 1, 1 },
 { P, 0, 0, 0, 0 },
 { N, 1, 1, 0, 1 },
 { C, 0, 1, 1, 1 },
 { C, 0, 1, 1, 1 },
 { P, 0, 0, 0, 0 },
 { C, 0, 0, 0, 1 },
 { P, 0, 0, 0, 0 },
 { C, 0, 1, 0, 1 },
 { P, 0, 0, 0, 0 },
 { C, 1, 1, 0, 1 },
 { C, 0, 1, 0, 1 },
 { C, 0, 1, 0, 1 },
 { P, 0, 0, 0, 0 },
 { C, 1, 1, 1, 1 },
 { C, 0, 1, 1, 1 },
 { C, 0, 1, 1, 1 },
 { P, 0, 0, 0, 0 },
 { C, 1, 1, 1, 1 },
 { C, 0, 1, 1, 1 },
 { C, 0, 1, 1, 1 }
};

/**
 * \param line_aa  AA_NEVER, AA_ALWAYS or AA_SOMETIMES
 * \param lookup  bitmask of IZ_* flags
 */
void fs_visitor::setup_payload_gen4()
{
   assert(stage == MESA_SHADER_FRAGMENT);
   brw_wm_prog_key *key = (brw_wm_prog_key*) this->key;
   gl_fragment_program *fp = (gl_fragment_program*) prog;
   GLuint reg = 2;
   bool kill_stats_promoted_workaround = false;
   int lookup = key->iz_lookup;
   bool uses_depth =
      (fp->Base.InputsRead & (1 << VARYING_SLOT_POS)) != 0;

   assert(lookup < IZ_BIT_MAX);

   /* Crazy workaround in the windowizer, which we need to track in
    * our register allocation and render target writes.  See the "If
    * statistics are enabled..." paragraph of 11.5.3.2: Early Depth
    * Test Cases [Pre-DevGT] of the 3D Pipeline - Windower B-Spec.
    */
   if (key->stats_wm &&
       (lookup & IZ_PS_KILL_ALPHATEST_BIT) &&
       wm_iz_table[lookup].mode == P) {
      kill_stats_promoted_workaround = true;
   }

   if (wm_iz_table[lookup].sd_present || uses_depth ||
       kill_stats_promoted_workaround) {
      payload.source_depth_reg = reg;
      reg += 2;
   }

   if (wm_iz_table[lookup].sd_to_rt || kill_stats_promoted_workaround)
      source_depth_to_render_target = true;

   if (wm_iz_table[lookup].ds_present || key->line_aa != AA_NEVER) {
      payload.aa_dest_stencil_reg = reg;
      runtime_check_aads_emit =
         !wm_iz_table[lookup].ds_present && key->line_aa == AA_SOMETIMES;
      reg++;
   }

   if (wm_iz_table[lookup].dd_present) {
      payload.dest_depth_reg = reg;
      reg+=2;
   }

   payload.num_regs = reg;
}

