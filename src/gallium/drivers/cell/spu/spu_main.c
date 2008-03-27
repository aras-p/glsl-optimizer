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


/* main() for Cell SPU code */


#include <stdio.h>
#include <libmisc.h>

#include "spu_main.h"
#include "spu_render.h"
#include "spu_texture.h"
#include "spu_tile.h"
//#include "spu_test.h"
#include "spu_vertex_shader.h"
#include "spu_dcache.h"
#include "cell/common.h"
#include "pipe/p_defines.h"


/*
helpful headers:
/usr/lib/gcc/spu/4.1.1/include/spu_mfcio.h
/opt/ibm/cell-sdk/prototype/sysroot/usr/include/libmisc.h
*/

boolean Debug = FALSE;

struct spu_global spu;

struct spu_vs_context draw;

static unsigned char attribute_fetch_code_buffer[136 * PIPE_MAX_ATTRIBS]
    ALIGN16_ATTRIB;

static unsigned char depth_stencil_code_buffer[4 * 64]
    ALIGN16_ATTRIB;

static unsigned char fb_blend_code_buffer[4 * 64]
    ALIGN16_ATTRIB;

static unsigned char logicop_code_buffer[4 * 64]
    ALIGN16_ATTRIB;


/**
 * Tell the PPU that this SPU has finished copying a buffer to
 * local store and that it may be reused by the PPU.
 * This is done by writting a 16-byte batch-buffer-status block back into
 * main memory (in cell_context->buffer_status[]).
 */
static void
release_buffer(uint buffer)
{
   /* Evidently, using less than a 16-byte status doesn't work reliably */
   static const uint status[4] ALIGN16_ATTRIB
      = {CELL_BUFFER_STATUS_FREE, 0, 0, 0};

   const uint index = 4 * (spu.init.id * CELL_NUM_BUFFERS + buffer);
   uint *dst = spu.init.buffer_status + index;

   ASSERT(buffer < CELL_NUM_BUFFERS);

   mfc_put((void *) &status,    /* src in local memory */
           (unsigned int) dst,  /* dst in main memory */
           sizeof(status),      /* size */
           TAG_MISC,            /* tag is unimportant */
           0, /* tid */
           0  /* rid */);
}


/**
 * For tiles whose status is TILE_STATUS_CLEAR, write solid-filled
 * tiles back to the main framebuffer.
 */
static void
really_clear_tiles(uint surfaceIndex)
{
   const uint num_tiles = spu.fb.width_tiles * spu.fb.height_tiles;
   uint i;

   if (surfaceIndex == 0) {
      clear_c_tile(&spu.ctile);

      for (i = spu.init.id; i < num_tiles; i += spu.init.num_spus) {
         uint tx = i % spu.fb.width_tiles;
         uint ty = i / spu.fb.width_tiles;
         if (spu.ctile_status[ty][tx] == TILE_STATUS_CLEAR) {
            put_tile(tx, ty, &spu.ctile, TAG_SURFACE_CLEAR, 0);
         }
      }
   }
   else {
      clear_z_tile(&spu.ztile);

      for (i = spu.init.id; i < num_tiles; i += spu.init.num_spus) {
         uint tx = i % spu.fb.width_tiles;
         uint ty = i / spu.fb.width_tiles;
         if (spu.ztile_status[ty][tx] == TILE_STATUS_CLEAR)
            put_tile(tx, ty, &spu.ctile, TAG_SURFACE_CLEAR, 1);
      }
   }

#if 0
   wait_on_mask(1 << TAG_SURFACE_CLEAR);
#endif
}


static void
cmd_clear_surface(const struct cell_command_clear_surface *clear)
{
   const uint num_tiles = spu.fb.width_tiles * spu.fb.height_tiles;
   uint i;

   if (Debug)
      printf("SPU %u: CLEAR SURF %u to 0x%08x\n", spu.init.id,
             clear->surface, clear->value);

#define CLEAR_OPT 1
#if CLEAR_OPT
   /* set all tile's status to CLEAR */
   if (clear->surface == 0) {
      memset(spu.ctile_status, TILE_STATUS_CLEAR, sizeof(spu.ctile_status));
      spu.fb.color_clear_value = clear->value;
   }
   else {
      memset(spu.ztile_status, TILE_STATUS_CLEAR, sizeof(spu.ztile_status));
      spu.fb.depth_clear_value = clear->value;
   }
   return;
#endif

   if (clear->surface == 0) {
      spu.fb.color_clear_value = clear->value;
      clear_c_tile(&spu.ctile);
   }
   else {
      spu.fb.depth_clear_value = clear->value;
      clear_z_tile(&spu.ztile);
   }

   /*
   printf("SPU: %s num=%d w=%d h=%d\n",
          __FUNCTION__, num_tiles, spu.fb.width_tiles, spu.fb.height_tiles);
   */

   for (i = spu.init.id; i < num_tiles; i += spu.init.num_spus) {
      uint tx = i % spu.fb.width_tiles;
      uint ty = i / spu.fb.width_tiles;
      if (clear->surface == 0)
         put_tile(tx, ty, &spu.ctile, TAG_SURFACE_CLEAR, 0);
      else
         put_tile(tx, ty, &spu.ztile, TAG_SURFACE_CLEAR, 1);
      /* XXX we don't want this here, but it fixes bad tile results */
   }

#if 0
   wait_on_mask(1 << TAG_SURFACE_CLEAR);
#endif

   if (Debug)
      printf("SPU %u: CLEAR SURF done\n", spu.init.id);
}


static void
cmd_release_verts(const struct cell_command_release_verts *release)
{
   if (Debug)
      printf("SPU %u: RELEASE VERTS %u\n",
             spu.init.id, release->vertex_buf);
   ASSERT(release->vertex_buf != ~0U);
   release_buffer(release->vertex_buf);
}


static void
cmd_state_framebuffer(const struct cell_command_framebuffer *cmd)
{
   if (Debug)
      printf("SPU %u: FRAMEBUFFER: %d x %d at %p, cformat 0x%x  zformat 0x%x\n",
             spu.init.id,
             cmd->width,
             cmd->height,
             cmd->color_start,
             cmd->color_format,
             cmd->depth_format);

   ASSERT_ALIGN16(cmd->color_start);
   ASSERT_ALIGN16(cmd->depth_start);

   spu.fb.color_start = cmd->color_start;
   spu.fb.depth_start = cmd->depth_start;
   spu.fb.color_format = cmd->color_format;
   spu.fb.depth_format = cmd->depth_format;
   spu.fb.width = cmd->width;
   spu.fb.height = cmd->height;
   spu.fb.width_tiles = (spu.fb.width + TILE_SIZE - 1) / TILE_SIZE;
   spu.fb.height_tiles = (spu.fb.height + TILE_SIZE - 1) / TILE_SIZE;

   switch (spu.fb.depth_format) {
   case PIPE_FORMAT_Z32_UNORM:
   case PIPE_FORMAT_Z24S8_UNORM:
   case PIPE_FORMAT_S8Z24_UNORM:
      spu.fb.zsize = 4;
      break;
   case PIPE_FORMAT_Z16_UNORM:
      spu.fb.zsize = 2;
      break;
   default:
      spu.fb.zsize = 0;
      break;
   }

   if (spu.fb.color_format == PIPE_FORMAT_A8R8G8B8_UNORM)
      spu.color_shuffle = ((vector unsigned char) {
                              12, 0, 4, 8, 0, 0, 0, 0, 
                              0, 0, 0, 0, 0, 0, 0, 0});
   else if (spu.fb.color_format == PIPE_FORMAT_B8G8R8A8_UNORM)
      spu.color_shuffle = ((vector unsigned char) {
                              8, 4, 0, 12, 0, 0, 0, 0, 
                              0, 0, 0, 0, 0, 0, 0, 0});
   else
      ASSERT(0);
}


static void
cmd_state_blend(const struct cell_command_blend *state)
{
   if (Debug)
      printf("SPU %u: BLEND: enabled %d\n",
             spu.init.id,
             (state->size != 0));

   ASSERT_ALIGN16(state->base);

   if (state->size != 0) {
      mfc_get(fb_blend_code_buffer,
              (unsigned int) state->base,  /* src */
              ROUNDUP16(state->size),
              TAG_BATCH_BUFFER,
              0, /* tid */
              0  /* rid */);
      wait_on_mask(1 << TAG_BATCH_BUFFER);
      spu.blend = (blend_func) fb_blend_code_buffer;
      spu.read_fb = state->read_fb;
   } else {
      spu.read_fb = FALSE;
   }
}


static void
cmd_state_depth_stencil(const struct cell_command_depth_stencil_alpha_test *state)
{
   if (Debug)
      printf("SPU %u: DEPTH_STENCIL: ztest %d\n",
             spu.init.id,
             state->read_depth);

   ASSERT_ALIGN16(state->base);

   if (state->size != 0) {
      mfc_get(depth_stencil_code_buffer,
	      (unsigned int) state->base,  /* src */
	      ROUNDUP16(state->size),
	      TAG_BATCH_BUFFER,
	      0, /* tid */
	      0  /* rid */);
      wait_on_mask(1 << TAG_BATCH_BUFFER);
   } else {
      /* If there is no code, emit a return instruction.
       */
      depth_stencil_code_buffer[0] = 0x35;
      depth_stencil_code_buffer[1] = 0x00;
      depth_stencil_code_buffer[2] = 0x00;
      depth_stencil_code_buffer[3] = 0x00;
   }

   spu.frag_test = (frag_test_func) depth_stencil_code_buffer;
   spu.read_depth = state->read_depth;
   spu.read_stencil = state->read_stencil;
}


static void
cmd_state_sampler(const struct pipe_sampler_state *state)
{
   if (Debug)
      printf("SPU %u: SAMPLER\n",
             spu.init.id);

   memcpy(&spu.sampler[0], state, sizeof(*state));
   if (spu.sampler[0].min_img_filter == PIPE_TEX_FILTER_LINEAR)
      spu.sample_texture = sample_texture_bilinear;
   else
      spu.sample_texture = sample_texture_nearest;
}


static void
cmd_state_texture(const struct cell_command_texture *texture)
{
   if (Debug)
      printf("SPU %u: TEXTURE at %p  size %u x %u\n",
             spu.init.id, texture->start, texture->width, texture->height);

   memcpy(&spu.texture, texture, sizeof(*texture));
   spu.tex_size = (vector float)
      { spu.texture.width, spu.texture.height, 0.0, 0.0};
   spu.tex_size_mask = (vector unsigned int)
      { spu.texture.width - 1, spu.texture.height - 1, 0, 0 };
   spu.tex_size_x_mask = spu_splats(spu.texture.width - 1);
   spu.tex_size_y_mask = spu_splats(spu.texture.height - 1);
}


static void
cmd_state_vertex_info(const struct vertex_info *vinfo)
{
   if (Debug) {
      printf("SPU %u: VERTEX_INFO num_attribs=%u\n", spu.init.id,
             vinfo->num_attribs);
   }
   ASSERT(vinfo->num_attribs >= 1);
   ASSERT(vinfo->num_attribs <= 8);
   memcpy(&spu.vertex_info, vinfo, sizeof(*vinfo));
}


static void
cmd_state_vs_array_info(const struct cell_array_info *vs_info)
{
   const unsigned attr = vs_info->attr;

   ASSERT(attr < PIPE_MAX_ATTRIBS);
   draw.vertex_fetch.src_ptr[attr] = vs_info->base;
   draw.vertex_fetch.pitch[attr] = vs_info->pitch;
   draw.vertex_fetch.size[attr] = vs_info->size;
   draw.vertex_fetch.code_offset[attr] = vs_info->function_offset;
   draw.vertex_fetch.dirty = 1;
}


static void
cmd_finish(void)
{
   if (Debug)
      printf("SPU %u: FINISH\n", spu.init.id);
   really_clear_tiles(0);
   /* wait for all outstanding DMAs to finish */
   mfc_write_tag_mask(~0);
   mfc_read_tag_status_all();
   /* send mbox message to PPU */
   spu_write_out_mbox(CELL_CMD_FINISH);
}


/**
 * Execute a batch of commands
 * The opcode param encodes the location of the buffer and its size.
 */
static void
cmd_batch(uint opcode)
{
   const uint buf = (opcode >> 8) & 0xff;
   uint size = (opcode >> 16);
   uint64_t buffer[CELL_BUFFER_SIZE / 8] ALIGN16_ATTRIB;
   const unsigned usize = size / sizeof(buffer[0]);
   uint pos;

   if (Debug)
      printf("SPU %u: BATCH buffer %u, len %u, from %p\n",
             spu.init.id, buf, size, spu.init.buffers[buf]);

   ASSERT((opcode & CELL_CMD_OPCODE_MASK) == CELL_CMD_BATCH);

   ASSERT_ALIGN16(spu.init.buffers[buf]);

   size = ROUNDUP16(size);

   ASSERT_ALIGN16(spu.init.buffers[buf]);

   mfc_get(buffer,  /* dest */
           (unsigned int) spu.init.buffers[buf],  /* src */
           size,
           TAG_BATCH_BUFFER,
           0, /* tid */
           0  /* rid */);
   wait_on_mask(1 << TAG_BATCH_BUFFER);

   /* Tell PPU we're done copying the buffer to local store */
   if (Debug)
      printf("SPU %u: release batch buf %u\n", spu.init.id, buf);
   release_buffer(buf);

   for (pos = 0; pos < usize; /* no incr */) {
      switch (buffer[pos]) {
      case CELL_CMD_STATE_FRAMEBUFFER:
         {
            struct cell_command_framebuffer *fb
               = (struct cell_command_framebuffer *) &buffer[pos];
            cmd_state_framebuffer(fb);
            pos += sizeof(*fb) / 8;
         }
         break;
      case CELL_CMD_CLEAR_SURFACE:
         {
            struct cell_command_clear_surface *clr
               = (struct cell_command_clear_surface *) &buffer[pos];
            cmd_clear_surface(clr);
            pos += sizeof(*clr) / 8;
         }
         break;
      case CELL_CMD_RENDER:
         {
            struct cell_command_render *render
               = (struct cell_command_render *) &buffer[pos];
            uint pos_incr;
            cmd_render(render, &pos_incr);
            pos += pos_incr;
         }
         break;
      case CELL_CMD_RELEASE_VERTS:
         {
            struct cell_command_release_verts *release
               = (struct cell_command_release_verts *) &buffer[pos];
            cmd_release_verts(release);
            pos += sizeof(*release) / 8;
         }
         break;
      case CELL_CMD_FINISH:
         cmd_finish();
         pos += 1;
         break;
      case CELL_CMD_STATE_BLEND:
         cmd_state_blend((struct cell_command_blend *) &buffer[pos+1]);
         pos += (1 + ROUNDUP8(sizeof(struct cell_command_blend)) / 8);
         break;
      case CELL_CMD_STATE_DEPTH_STENCIL:
         cmd_state_depth_stencil((struct cell_command_depth_stencil_alpha_test *)
                                 &buffer[pos+1]);
         pos += (1 + ROUNDUP8(sizeof(struct cell_command_depth_stencil_alpha_test)) / 8);
         break;
      case CELL_CMD_STATE_SAMPLER:
         cmd_state_sampler((struct pipe_sampler_state *) &buffer[pos+1]);
         pos += (1 + ROUNDUP8(sizeof(struct pipe_sampler_state)) / 8);
         break;
      case CELL_CMD_STATE_TEXTURE:
         cmd_state_texture((struct cell_command_texture *) &buffer[pos+1]);
         pos += (1 + ROUNDUP8(sizeof(struct cell_command_texture)) / 8);
         break;
      case CELL_CMD_STATE_VERTEX_INFO:
         cmd_state_vertex_info((struct vertex_info *) &buffer[pos+1]);
         pos += (1 + ROUNDUP8(sizeof(struct vertex_info)) / 8);
         break;
      case CELL_CMD_STATE_VIEWPORT:
         (void) memcpy(& draw.viewport, &buffer[pos+1],
                       sizeof(struct pipe_viewport_state));
         pos += (1 + ROUNDUP8(sizeof(struct pipe_viewport_state)) / 8);
         break;
      case CELL_CMD_STATE_UNIFORMS:
         draw.constants = (const float (*)[4]) (uintptr_t) buffer[pos + 1];
         pos += 2;
         break;
      case CELL_CMD_STATE_VS_ARRAY_INFO:
         cmd_state_vs_array_info((struct cell_array_info *) &buffer[pos+1]);
         pos += (1 + ROUNDUP8(sizeof(struct cell_array_info)) / 8);
         break;
      case CELL_CMD_STATE_BIND_VS:
         spu_bind_vertex_shader(&draw,
                                (struct cell_shader_info *) &buffer[pos+1]);
         pos += (1 + ROUNDUP8(sizeof(struct cell_shader_info)) / 8);
         break;
      case CELL_CMD_STATE_ATTRIB_FETCH: {
         struct cell_attribute_fetch_code *code =
             (struct cell_attribute_fetch_code *) &buffer[pos+1];

              mfc_get(attribute_fetch_code_buffer,
                      (unsigned int) code->base,  /* src */
                      code->size,
                      TAG_BATCH_BUFFER,
                      0, /* tid */
                      0  /* rid */);
         wait_on_mask(1 << TAG_BATCH_BUFFER);

         draw.vertex_fetch.code = attribute_fetch_code_buffer;
         pos += (1 + ROUNDUP8(sizeof(struct cell_attribute_fetch_code)) / 8);
         break;
      }
      case CELL_CMD_STATE_LOGICOP: {
         struct cell_command_logicop *code =
             (struct cell_command_logicop *) &buffer[pos+1];

              mfc_get(logicop_code_buffer,
                      (unsigned int) code->base,  /* src */
                      code->size,
                      TAG_BATCH_BUFFER,
                      0, /* tid */
                      0  /* rid */);
         wait_on_mask(1 << TAG_BATCH_BUFFER);

	 spu.logicop = (logicop_func) logicop_code_buffer;
         pos += (1 + ROUNDUP8(sizeof(struct cell_command_logicop)) / 8);
         break;
      }
      case CELL_CMD_FLUSH_BUFFER_RANGE: {
	 struct cell_buffer_range *br = (struct cell_buffer_range *)
	     &buffer[pos+1];

	 spu_dcache_mark_dirty((unsigned) br->base, br->size);
         pos += (1 + ROUNDUP8(sizeof(struct cell_buffer_range)) / 8);
	 break;
      }
      default:
         printf("SPU %u: bad opcode: 0x%llx\n", spu.init.id, buffer[pos]);
         ASSERT(0);
         break;
      }
   }

   if (Debug)
      printf("SPU %u: BATCH complete\n", spu.init.id);
}


/**
 * Temporary/simple main loop for SPEs: Get a command, execute it, repeat.
 */
static void
main_loop(void)
{
   struct cell_command cmd;
   int exitFlag = 0;

   if (Debug)
      printf("SPU %u: Enter main loop\n", spu.init.id);

   ASSERT((sizeof(struct cell_command) & 0xf) == 0);
   ASSERT_ALIGN16(&cmd);

   while (!exitFlag) {
      unsigned opcode;
      int tag = 0;

      if (Debug)
         printf("SPU %u: Wait for cmd...\n", spu.init.id);

      /* read/wait from mailbox */
      opcode = (unsigned int) spu_read_in_mbox();

      if (Debug)
         printf("SPU %u: got cmd 0x%x\n", spu.init.id, opcode);

      /* command payload */
      mfc_get(&cmd,  /* dest */
              (unsigned int) spu.init.cmd, /* src */
              sizeof(struct cell_command), /* bytes */
              tag,
              0, /* tid */
              0  /* rid */);
      wait_on_mask( 1 << tag );

      /*
       * NOTE: most commands should be contained in a batch buffer
       */

      switch (opcode & CELL_CMD_OPCODE_MASK) {
      case CELL_CMD_EXIT:
         if (Debug)
            printf("SPU %u: EXIT\n", spu.init.id);
         exitFlag = 1;
         break;
      case CELL_CMD_VS_EXECUTE:
         spu_execute_vertex_shader(&draw, &cmd.vs);
         break;
      case CELL_CMD_BATCH:
         cmd_batch(opcode);
         break;
      default:
         printf("Bad opcode!\n");
      }

   }

   if (Debug)
      printf("SPU %u: Exit main loop\n", spu.init.id);
}



static void
one_time_init(void)
{
   memset(spu.ctile_status, TILE_STATUS_DEFINED, sizeof(spu.ctile_status));
   memset(spu.ztile_status, TILE_STATUS_DEFINED, sizeof(spu.ztile_status));
   invalidate_tex_cache();
}



/* In some versions of the SDK the SPE main takes 'unsigned long' as a
 * parameter.  In others it takes 'unsigned long long'.  Use a define to
 * select between the two.
 */
#ifdef SPU_MAIN_PARAM_LONG_LONG
typedef unsigned long long main_param_t;
#else
typedef unsigned long main_param_t;
#endif

/**
 * SPE entrypoint.
 */
int
main(main_param_t speid, main_param_t argp)
{
   int tag = 0;

   (void) speid;

   ASSERT(sizeof(tile_t) == TILE_SIZE * TILE_SIZE * 4);
   ASSERT(sizeof(struct cell_command_render) % 8 == 0);

   one_time_init();

   if (Debug)
      printf("SPU: main() speid=%lu\n", (unsigned long) speid);

   mfc_get(&spu.init,  /* dest */
           (unsigned int) argp, /* src */
           sizeof(struct cell_init_info), /* bytes */
           tag,
           0, /* tid */
           0  /* rid */);
   wait_on_mask( 1 << tag );

#if 0
   if (spu.init.id==0)
      spu_test_misc();
#endif

   main_loop();

   return 0;
}
