/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
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
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#include "util/u_memory.h"
#include "util/u_math.h"

#include "brw_batchbuffer.h"
#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "brw_debug.h"


/**
 * Partition the CURBE between the various users of constant values:
 * Note that vertex and fragment shaders can now fetch constants out
 * of constant buffers.  We no longer allocatea block of the GRF for
 * constants.  That greatly reduces the demand for space in the CURBE.
 * Some of the comments within are dated...
 */
static int calculate_curbe_offsets( struct brw_context *brw )
{
   /* CACHE_NEW_WM_PROG */
   const GLuint nr_fp_regs = brw->wm.prog_data->curb_read_length;
   
   /* BRW_NEW_VERTEX_PROGRAM */
   const GLuint nr_vp_regs = brw->vs.prog_data->curb_read_length;
   GLuint nr_clip_regs = 0;
   GLuint total_regs;

   /* PIPE_NEW_CLIP */
   if (brw->curr.ucp.nr) {
      GLuint nr_planes = 6 + brw->curr.ucp.nr;
      nr_clip_regs = (nr_planes * 4 + 15) / 16;
   }


   total_regs = nr_fp_regs + nr_vp_regs + nr_clip_regs;

   /* When this is > 32, want to use a true constant buffer to hold
    * the extra constants.
    */
   assert(total_regs <= 32);

   /* Lazy resize:
    */
   if (nr_fp_regs > brw->curbe.wm_size ||
       nr_vp_regs > brw->curbe.vs_size ||
       nr_clip_regs != brw->curbe.clip_size ||
       (total_regs < brw->curbe.total_size / 4 &&
	brw->curbe.total_size > 16)) {

      GLuint reg = 0;

      /* Calculate a new layout: 
       */
      reg = 0;
      brw->curbe.wm_start = reg;
      brw->curbe.wm_size = nr_fp_regs; reg += nr_fp_regs;
      brw->curbe.clip_start = reg;
      brw->curbe.clip_size = nr_clip_regs; reg += nr_clip_regs;
      brw->curbe.vs_start = reg;
      brw->curbe.vs_size = nr_vp_regs; reg += nr_vp_regs;
      brw->curbe.total_size = reg;

      if (BRW_DEBUG & DEBUG_CURBE)
	 debug_printf("curbe wm %d+%d clip %d+%d vs %d+%d\n",
		      brw->curbe.wm_start,
		      brw->curbe.wm_size,
		      brw->curbe.clip_start,
		      brw->curbe.clip_size,
		      brw->curbe.vs_start,
		      brw->curbe.vs_size );

      brw->state.dirty.brw |= BRW_NEW_CURBE_OFFSETS;
   }

   return 0;
}


const struct brw_tracked_state brw_curbe_offsets = {
   .dirty = {
      .mesa = PIPE_NEW_CLIP,
      .brw  = BRW_NEW_VERTEX_PROGRAM,
      .cache = CACHE_NEW_WM_PROG
   },
   .prepare = calculate_curbe_offsets
};




/* Define the number of curbes within CS's urb allocation.  Multiple
 * urb entries -> multiple curbes.  These will be used by
 * fixed-function hardware in a double-buffering scheme to avoid a
 * pipeline stall each time the contents of the curbe is changed.
 */
int brw_upload_cs_urb_state(struct brw_context *brw)
{
   struct brw_cs_urb_state cs_urb;
   memset(&cs_urb, 0, sizeof(cs_urb));

   /* It appears that this is the state packet for the CS unit, ie. the
    * urb entries detailed here are housed in the CS range from the
    * URB_FENCE command.
    */
   cs_urb.header.opcode = CMD_CS_URB_STATE;
   cs_urb.header.length = sizeof(cs_urb)/4 - 2;

   /* BRW_NEW_URB_FENCE */
   cs_urb.bits0.nr_urb_entries = brw->urb.nr_cs_entries;
   cs_urb.bits0.urb_entry_size = brw->urb.csize - 1;

   assert(brw->urb.nr_cs_entries);
   BRW_CACHED_BATCH_STRUCT(brw, &cs_urb);
   return 0;
}

static GLfloat fixed_plane[6][4] = {
   { 0,    0,   -1, 1 },
   { 0,    0,    1, 1 },
   { 0,   -1,    0, 1 },
   { 0,    1,    0, 1 },
   {-1,    0,    0, 1 },
   { 1,    0,    0, 1 }
};

/* Upload a new set of constants.  Too much variability to go into the
 * cache mechanism, but maybe would benefit from a comparison against
 * the current uploaded set of constants.
 */
static enum pipe_error prepare_curbe_buffer(struct brw_context *brw)
{
   struct pipe_screen *screen = brw->base.screen;
   const GLuint sz = brw->curbe.total_size;
   const GLuint bufsz = sz * 16 * sizeof(GLfloat);
   enum pipe_error ret;
   GLfloat *buf;
   GLuint i;

   if (sz == 0) {
      if (brw->curbe.last_buf) {
	 free(brw->curbe.last_buf);
	 brw->curbe.last_buf = NULL;
	 brw->curbe.last_bufsz  = 0;
      }
      return 0;
   }

   buf = (GLfloat *) CALLOC(bufsz, 1);

   /* fragment shader constants */
   if (brw->curbe.wm_size) {
      const struct brw_fragment_shader *fs = brw->curr.fragment_shader;
      GLuint offset = brw->curbe.wm_start * 16;
      GLuint nr_immediate, nr_const;

      nr_immediate = fs->immediates.nr;
      if (nr_immediate) {
         memcpy(&buf[offset], 
                fs->immediates.data,
                nr_immediate * 4 * sizeof(float));

         offset += nr_immediate * 4;
      }

      nr_const = fs->info.file_max[TGSI_FILE_CONSTANT] + 1;
/*      nr_const = brw->wm.prog_data->nr_params; */
      if (nr_const) {
         const GLfloat *value = screen->buffer_map( screen,
                                                    brw->curr.fragment_constants,
                                                    PIPE_BUFFER_USAGE_CPU_READ);

         memcpy(&buf[offset], value,
                nr_const * 4 * sizeof(float));
         
         screen->buffer_unmap( screen, 
                               brw->curr.fragment_constants );
      }
   }


   /* The clipplanes are actually delivered to both CLIP and VS units.
    * VS uses them to calculate the outcode bitmasks.
    */
   if (brw->curbe.clip_size) {
      GLuint offset = brw->curbe.clip_start * 16;
      GLuint j;

      /* If any planes are going this way, send them all this way:
       */
      for (i = 0; i < 6; i++) {
	 buf[offset + i * 4 + 0] = fixed_plane[i][0];
	 buf[offset + i * 4 + 1] = fixed_plane[i][1];
	 buf[offset + i * 4 + 2] = fixed_plane[i][2];
	 buf[offset + i * 4 + 3] = fixed_plane[i][3];
      }

      /* Clip planes:
       */
      assert(brw->curr.ucp.nr <= 6);
      for (j = 0; j < brw->curr.ucp.nr; j++) {
	 buf[offset + i * 4 + 0] = brw->curr.ucp.ucp[j][0];
	 buf[offset + i * 4 + 1] = brw->curr.ucp.ucp[j][1];
	 buf[offset + i * 4 + 2] = brw->curr.ucp.ucp[j][2];
	 buf[offset + i * 4 + 3] = brw->curr.ucp.ucp[j][3];
	 i++;
      }
   }

   /* vertex shader constants */
   if (brw->curbe.vs_size) {
      GLuint offset = brw->curbe.vs_start * 16;
      const struct brw_vertex_shader *vs = brw->curr.vertex_shader;
      GLuint nr_immediate, nr_const;

      nr_immediate = vs->immediates.nr;
      if (nr_immediate) {
         memcpy(&buf[offset], 
                vs->immediates.data,
                nr_immediate * 4 * sizeof(float));

         offset += nr_immediate * 4;
      }

      nr_const = vs->info.file_max[TGSI_FILE_CONSTANT] + 1;
      if (nr_const) {
         /* XXX: note that constant buffers are currently *already* in
          * buffer objects.  If we want to keep on putting them into the
          * curbe, makes sense to treat constbuf's specially with malloc.
          */
         const GLfloat *value = screen->buffer_map( screen,
                                                    brw->curr.vertex_constants,
                                                    PIPE_BUFFER_USAGE_CPU_READ);
         
         /* XXX: what if user's constant buffer is too small?
          */
         memcpy(&buf[offset], value, nr_const * 4 * sizeof(float));
         
         screen->buffer_unmap( screen, brw->curr.vertex_constants );
      }
   }

   if (BRW_DEBUG & DEBUG_CURBE) {
      for (i = 0; i < sz*16; i+=4) 
	 debug_printf("curbe %d.%d: %f %f %f %f\n", i/8, i&4,
		      buf[i+0], buf[i+1], buf[i+2], buf[i+3]);

      debug_printf("last_buf %p buf %p sz %d/%d cmp %d\n",
		   (void *)brw->curbe.last_buf, (void *)buf,
		   bufsz, brw->curbe.last_bufsz,
		   brw->curbe.last_buf ? memcmp(buf, brw->curbe.last_buf, bufsz) : -1);
   }

   if (brw->curbe.curbe_bo != NULL &&
       brw->curbe.last_buf &&
       bufsz == brw->curbe.last_bufsz &&
       memcmp(buf, brw->curbe.last_buf, bufsz) == 0) {
      /* constants have not changed */
      FREE(buf);
   } 
   else {
      /* constants have changed */
      FREE(brw->curbe.last_buf);

      brw->curbe.last_buf = buf;
      brw->curbe.last_bufsz = bufsz;

      if (brw->curbe.curbe_bo != NULL &&
	  (brw->curbe.need_new_bo ||
	   brw->curbe.curbe_next_offset + bufsz > brw->curbe.curbe_bo->size))
      {
	 bo_reference(&brw->curbe.curbe_bo, NULL);
      }

      if (brw->curbe.curbe_bo == NULL) {
	 /* Allocate a single page for CURBE entries for this
	  * batchbuffer.  They're generally around 64b.  We will
	  * discard the curbe buffer after the batch is flushed to
	  * avoid synchronous updates.
	  */
	 ret = brw->sws->bo_alloc(brw->sws, 
                                  BRW_BUFFER_TYPE_CURBE,
                                  4096, 1 << 6,
                                  &brw->curbe.curbe_bo);
         if (ret)
            return ret;

	 brw->curbe.curbe_next_offset = 0;
      }

      brw->curbe.curbe_offset = brw->curbe.curbe_next_offset;
      brw->curbe.curbe_next_offset += bufsz;
      brw->curbe.curbe_next_offset = align(brw->curbe.curbe_next_offset, 64);

      /* Copy data to the buffer:
       */
      brw->sws->bo_subdata(brw->curbe.curbe_bo,
                           BRW_DATA_CONSTANT_BUFFER,
			   brw->curbe.curbe_offset,
			   bufsz,
			   buf,
                           NULL, 0);
   }

   brw_add_validated_bo(brw, brw->curbe.curbe_bo);

   /* Because this provokes an action (ie copy the constants into the
    * URB), it shouldn't be shortcircuited if identical to the
    * previous time - because eg. the urb destination may have
    * changed, or the urb contents different to last time.
    *
    * Note that the data referred to is actually copied internally,
    * not just used in place according to passed pointer.
    *
    * It appears that the CS unit takes care of using each available
    * URB entry (Const URB Entry == CURBE) in turn, and issuing
    * flushes as necessary when doublebuffering of CURBEs isn't
    * possible.
    */

   return 0;
}

static enum pipe_error emit_curbe_buffer(struct brw_context *brw)
{
   GLuint sz = brw->curbe.total_size;

   BEGIN_BATCH(2, IGNORE_CLIPRECTS);
   if (sz == 0) {
      OUT_BATCH((CMD_CONST_BUFFER << 16) | (2 - 2));
      OUT_BATCH(0);
   } else {
      OUT_BATCH((CMD_CONST_BUFFER << 16) | (1 << 8) | (2 - 2));
      OUT_RELOC(brw->curbe.curbe_bo,
		BRW_USAGE_STATE,
		(sz - 1) + brw->curbe.curbe_offset);
   }
   ADVANCE_BATCH();
   return 0;
}

const struct brw_tracked_state brw_curbe_buffer = {
   .dirty = {
      .mesa = (PIPE_NEW_FRAGMENT_CONSTANTS |
	       PIPE_NEW_VERTEX_CONSTANTS |
	       PIPE_NEW_CLIP),
      .brw  = (BRW_NEW_FRAGMENT_PROGRAM |
	       BRW_NEW_VERTEX_PROGRAM |
	       BRW_NEW_URB_FENCE | /* Implicit - hardware requires this, not used above */
	       BRW_NEW_PSP | /* Implicit - hardware requires this, not used above */
	       BRW_NEW_CURBE_OFFSETS |
	       BRW_NEW_BATCH),
      .cache = (CACHE_NEW_WM_PROG) 
   },
   .prepare = prepare_curbe_buffer,
   .emit = emit_curbe_buffer,
};

