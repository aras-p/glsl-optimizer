/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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


/**
 * SPU command processing code
 */


#include <stdio.h>
#include <libmisc.h>

#include "pipe/p_defines.h"

#include "spu_command.h"
#include "spu_main.h"
#include "spu_render.h"
#include "spu_per_fragment_op.h"
#include "spu_texture.h"
#include "spu_tile.h"
#include "spu_vertex_shader.h"
#include "spu_dcache.h"
#include "cell/common.h"


struct spu_vs_context draw;


/**
 * Buffers containing dynamically generated SPU code:
 */
PIPE_ALIGN_VAR(16) static unsigned char attribute_fetch_code_buffer[136 * PIPE_MAX_ATTRIBS];



static INLINE int
align(int value, int alignment)
{
   return (value + alignment - 1) & ~(alignment - 1);
}



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
   static const vector unsigned int status = {CELL_BUFFER_STATUS_FREE,
                                              CELL_BUFFER_STATUS_FREE,
                                              CELL_BUFFER_STATUS_FREE,
                                              CELL_BUFFER_STATUS_FREE};
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
 * Write CELL_FENCE_SIGNALLED back to the fence status qword in main memory.
 * There's a qword of status per SPU.
 */
static void
cmd_fence(struct cell_command_fence *fence_cmd)
{
   static const vector unsigned int status = {CELL_FENCE_SIGNALLED,
                                              CELL_FENCE_SIGNALLED,
                                              CELL_FENCE_SIGNALLED,
                                              CELL_FENCE_SIGNALLED};
   uint *dst = (uint *) fence_cmd->fence;
   dst += 4 * spu.init.id;  /* main store/memory address, not local store */
   ASSERT_ALIGN16(dst);
   mfc_put((void *) &status,    /* src in local memory */
           (unsigned int) dst,  /* dst in main memory */
           sizeof(status),      /* size */
           TAG_FENCE,           /* tag */
           0, /* tid */
           0  /* rid */);
}


static void
cmd_clear_surface(const struct cell_command_clear_surface *clear)
{
   D_PRINTF(CELL_DEBUG_CMD, "CLEAR SURF %u to 0x%08x\n", clear->surface, clear->value);

   if (clear->surface == 0) {
      spu.fb.color_clear_value = clear->value;
      if (spu.init.debug_flags & CELL_DEBUG_CHECKER) {
         uint x = (spu.init.id << 4) | (spu.init.id << 12) |
            (spu.init.id << 20) | (spu.init.id << 28);
         spu.fb.color_clear_value ^= x;
      }
   }
   else {
      spu.fb.depth_clear_value = clear->value;
   }

#define CLEAR_OPT 1
#if CLEAR_OPT

   /* Simply set all tiles' status to CLEAR.
    * When we actually begin rendering into a tile, we'll initialize it to
    * the clear value.  If any tiles go untouched during the frame,
    * really_clear_tiles() will set them to the clear value.
    */
   if (clear->surface == 0) {
      memset(spu.ctile_status, TILE_STATUS_CLEAR, sizeof(spu.ctile_status));
   }
   else {
      memset(spu.ztile_status, TILE_STATUS_CLEAR, sizeof(spu.ztile_status));
   }

#else

   /*
    * This path clears the whole framebuffer to the clear color right now.
    */

   /*
   printf("SPU: %s num=%d w=%d h=%d\n",
          __FUNCTION__, num_tiles, spu.fb.width_tiles, spu.fb.height_tiles);
   */

   /* init a single tile to the clear value */
   if (clear->surface == 0) {
      clear_c_tile(&spu.ctile);
   }
   else {
      clear_z_tile(&spu.ztile);
   }

   /* walk over my tiles, writing the 'clear' tile's data */
   {
      const uint num_tiles = spu.fb.width_tiles * spu.fb.height_tiles;
      uint i;
      for (i = spu.init.id; i < num_tiles; i += spu.init.num_spus) {
         uint tx = i % spu.fb.width_tiles;
         uint ty = i / spu.fb.width_tiles;
         if (clear->surface == 0)
            put_tile(tx, ty, &spu.ctile, TAG_SURFACE_CLEAR, 0);
         else
            put_tile(tx, ty, &spu.ztile, TAG_SURFACE_CLEAR, 1);
      }
   }

   if (spu.init.debug_flags & CELL_DEBUG_SYNC) {
      wait_on_mask(1 << TAG_SURFACE_CLEAR);
   }

#endif /* CLEAR_OPT */

   D_PRINTF(CELL_DEBUG_CMD, "CLEAR SURF done\n");
}


static void
cmd_release_verts(const struct cell_command_release_verts *release)
{
   D_PRINTF(CELL_DEBUG_CMD, "RELEASE VERTS %u\n", release->vertex_buf);
   ASSERT(release->vertex_buf != ~0U);
   release_buffer(release->vertex_buf);
}


/**
 * Process a CELL_CMD_STATE_FRAGMENT_OPS command.
 * This involves installing new fragment ops SPU code.
 * If this function is never called, we'll use a regular C fallback function
 * for fragment processing.
 */
static void
cmd_state_fragment_ops(const struct cell_command_fragment_ops *fops)
{
   D_PRINTF(CELL_DEBUG_CMD, "CMD_STATE_FRAGMENT_OPS\n");

   /* Copy state info (for fallback case only - this will eventually
    * go away when the fallback case goes away)
    */
   memcpy(&spu.depth_stencil_alpha, &fops->dsa, sizeof(fops->dsa));
   memcpy(&spu.blend, &fops->blend, sizeof(fops->blend));
   memcpy(&spu.blend_color, &fops->blend_color, sizeof(fops->blend_color));

   /* Make sure the SPU knows which buffers it's expected to read when
    * it's told to pull tiles.
    */
   spu.read_depth_stencil = (spu.depth_stencil_alpha.depth.enabled || spu.depth_stencil_alpha.stencil[0].enabled);

   /* If we're forcing the fallback code to be used (for debug purposes),
    * install that.  Otherwise install the incoming SPU code.
    */
   if ((spu.init.debug_flags & CELL_DEBUG_FRAGMENT_OP_FALLBACK) != 0) {
      static unsigned int warned = 0;
      if (!warned) {
         fprintf(stderr, "Cell Warning: using fallback per-fragment code\n");
         warned = 1;
      }
      /* The following two lines aren't really necessary if you
       * know the debug flags won't change during a run, and if you
       * know that the function pointers are initialized correctly.
       * We set them here to allow a person to change the debug
       * flags during a run (from inside a debugger).
       */
      spu.fragment_ops[CELL_FACING_FRONT] = spu_fallback_fragment_ops;
      spu.fragment_ops[CELL_FACING_BACK] = spu_fallback_fragment_ops;
      return;
   }

   /* Make sure the SPU code buffer is large enough to hold the incoming code.
    * Note that we *don't* use align_malloc() and align_free(), because
    * those utility functions are *not* available in SPU code.
    * */
   if (spu.fragment_ops_code_size < fops->total_code_size) {
      if (spu.fragment_ops_code != NULL) {
         free(spu.fragment_ops_code);
      }
      spu.fragment_ops_code_size = fops->total_code_size;
      spu.fragment_ops_code = malloc(fops->total_code_size);
      if (spu.fragment_ops_code == NULL) {
         /* Whoops. */
         fprintf(stderr, "CELL Warning: failed to allocate fragment ops code (%d bytes) - using fallback\n", fops->total_code_size);
         spu.fragment_ops_code = NULL;
         spu.fragment_ops_code_size = 0;
         spu.fragment_ops[CELL_FACING_FRONT] = spu_fallback_fragment_ops;
         spu.fragment_ops[CELL_FACING_BACK] = spu_fallback_fragment_ops;
         return;
      }
   }

   /* Copy the SPU code from the command buffer to the spu buffer */
   memcpy(spu.fragment_ops_code, fops->code, fops->total_code_size);

   /* Set the pointers for the front-facing and back-facing fragments
    * to the specified offsets within the code.  Note that if the
    * front-facing and back-facing code are the same, they'll have
    * the same offset.
    */
   spu.fragment_ops[CELL_FACING_FRONT] = (spu_fragment_ops_func) &spu.fragment_ops_code[fops->front_code_index];
   spu.fragment_ops[CELL_FACING_BACK] = (spu_fragment_ops_func) &spu.fragment_ops_code[fops->back_code_index];
}

static void
cmd_state_fragment_program(const struct cell_command_fragment_program *fp)
{
   D_PRINTF(CELL_DEBUG_CMD, "CMD_STATE_FRAGMENT_PROGRAM\n");
   /* Copy SPU code from batch buffer to spu buffer */
   memcpy(spu.fragment_program_code, fp->code,
          SPU_MAX_FRAGMENT_PROGRAM_INSTS * 4);
#if 01
   /* Point function pointer at new code */
   spu.fragment_program = (spu_fragment_program_func)spu.fragment_program_code;
#endif
}


static uint
cmd_state_fs_constants(const qword *buffer, uint pos)
{
   const uint num_const = spu_extract((vector unsigned int)buffer[pos+1], 0);
   const float *constants = (const float *) &buffer[pos+2];
   uint i;

   D_PRINTF(CELL_DEBUG_CMD, "CMD_STATE_FS_CONSTANTS (%u)\n", num_const);

   /* Expand each float to float[4] for SOA execution */
   for (i = 0; i < num_const; i++) {
      D_PRINTF(CELL_DEBUG_CMD, "  const[%u] = %f\n", i, constants[i]);
      spu.constants[i] = spu_splats(constants[i]);
   }

   /* return new buffer pos (in 16-byte words) */
   return pos + 2 + (ROUNDUP16(num_const * sizeof(float)) / 16);
}


static void
cmd_state_framebuffer(const struct cell_command_framebuffer *cmd)
{
   D_PRINTF(CELL_DEBUG_CMD, "FRAMEBUFFER: %d x %d at %p, cformat 0x%x  zformat 0x%x\n",
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
      spu.fb.zsize = 4;
      spu.fb.zscale = (float) 0xffffffffu;
      break;
   case PIPE_FORMAT_S8Z24_UNORM:
   case PIPE_FORMAT_Z24S8_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:
      spu.fb.zsize = 4;
      spu.fb.zscale = (float) 0x00ffffffu;
      break;
   case PIPE_FORMAT_Z16_UNORM:
      spu.fb.zsize = 2;
      spu.fb.zscale = (float) 0xffffu;
      break;
   default:
      spu.fb.zsize = 0;
      break;
   }
}


/**
 * Tex texture mask_s/t and scale_s/t fields depend on the texture size and
 * sampler wrap modes.
 */
static void
update_tex_masks(struct spu_texture *texture,
                 const struct pipe_sampler_state *sampler)
{
   uint i;

   for (i = 0; i < CELL_MAX_TEXTURE_LEVELS; i++) {
      int width = texture->level[i].width;
      int height = texture->level[i].height;

      if (sampler->wrap_s == PIPE_TEX_WRAP_REPEAT)
         texture->level[i].mask_s = spu_splats(width - 1);
      else
         texture->level[i].mask_s = spu_splats(~0);

      if (sampler->wrap_t == PIPE_TEX_WRAP_REPEAT)
         texture->level[i].mask_t = spu_splats(height - 1);
      else
         texture->level[i].mask_t = spu_splats(~0);

      if (sampler->normalized_coords) {
         texture->level[i].scale_s = spu_splats((float) width);
         texture->level[i].scale_t = spu_splats((float) height);
      }
      else {
         texture->level[i].scale_s = spu_splats(1.0f);
         texture->level[i].scale_t = spu_splats(1.0f);
      }
   }
}


static void
cmd_state_sampler(const struct cell_command_sampler *sampler)
{
   uint unit = sampler->unit;

   D_PRINTF(CELL_DEBUG_CMD, "SAMPLER [%u]\n", unit);

   spu.sampler[unit] = sampler->state;

   switch (spu.sampler[unit].min_img_filter) {
   case PIPE_TEX_FILTER_LINEAR:
      spu.min_sample_texture_2d[unit] = sample_texture_2d_bilinear;
      break;
   case PIPE_TEX_FILTER_NEAREST:
      spu.min_sample_texture_2d[unit] = sample_texture_2d_nearest;
      break;
   default:
      ASSERT(0);
   }

   switch (spu.sampler[sampler->unit].mag_img_filter) {
   case PIPE_TEX_FILTER_LINEAR:
      spu.mag_sample_texture_2d[unit] = sample_texture_2d_bilinear;
      break;
   case PIPE_TEX_FILTER_NEAREST:
      spu.mag_sample_texture_2d[unit] = sample_texture_2d_nearest;
      break;
   default:
      ASSERT(0);
   }

   switch (spu.sampler[sampler->unit].min_mip_filter) {
   case PIPE_TEX_MIPFILTER_NEAREST:
   case PIPE_TEX_MIPFILTER_LINEAR:
      spu.sample_texture_2d[unit] = sample_texture_2d_lod;
      break;
   case PIPE_TEX_MIPFILTER_NONE:
      spu.sample_texture_2d[unit] = spu.mag_sample_texture_2d[unit];
      break;
   default:
      ASSERT(0);
   }

   update_tex_masks(&spu.texture[unit], &spu.sampler[unit]);
}


static void
cmd_state_texture(const struct cell_command_texture *texture)
{
   const uint unit = texture->unit;
   uint i;

   D_PRINTF(CELL_DEBUG_CMD, "TEXTURE [%u]\n", texture->unit);

   spu.texture[unit].max_level = 0;
   spu.texture[unit].target = texture->target;

   for (i = 0; i < CELL_MAX_TEXTURE_LEVELS; i++) {
      uint width = texture->width[i];
      uint height = texture->height[i];
      uint depth = texture->depth[i];

      D_PRINTF(CELL_DEBUG_CMD, "  LEVEL %u: at %p  size[0] %u x %u\n", i,
             texture->start[i], texture->width[i], texture->height[i]);

      spu.texture[unit].level[i].start = texture->start[i];
      spu.texture[unit].level[i].width = width;
      spu.texture[unit].level[i].height = height;
      spu.texture[unit].level[i].depth = depth;

      spu.texture[unit].level[i].tiles_per_row =
         (width + TILE_SIZE - 1) / TILE_SIZE;

      spu.texture[unit].level[i].bytes_per_image =
         4 * align(width, TILE_SIZE) * align(height, TILE_SIZE) * depth;

      spu.texture[unit].level[i].max_s = spu_splats((int) width - 1);
      spu.texture[unit].level[i].max_t = spu_splats((int) height - 1);

      if (texture->start[i])
         spu.texture[unit].max_level = i;
   }

   update_tex_masks(&spu.texture[unit], &spu.sampler[unit]);
}


static void
cmd_state_vertex_info(const struct vertex_info *vinfo)
{
   D_PRINTF(CELL_DEBUG_CMD, "VERTEX_INFO num_attribs=%u\n", vinfo->num_attribs);
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
cmd_state_attrib_fetch(const struct cell_attribute_fetch_code *code)
{
   mfc_get(attribute_fetch_code_buffer,
           (unsigned int) code->base,  /* src */
           code->size,
           TAG_BATCH_BUFFER,
           0, /* tid */
           0  /* rid */);
   wait_on_mask(1 << TAG_BATCH_BUFFER);

   draw.vertex_fetch.code = attribute_fetch_code_buffer;
}


static void
cmd_finish(void)
{
   D_PRINTF(CELL_DEBUG_CMD, "FINISH\n");
   really_clear_tiles(0);
   /* wait for all outstanding DMAs to finish */
   mfc_write_tag_mask(~0);
   mfc_read_tag_status_all();
   /* send mbox message to PPU */
   spu_write_out_mbox(CELL_CMD_FINISH);
}


/**
 * Execute a batch of commands which was sent to us by the PPU.
 * See the cell_emit_state.c code to see where the commands come from.
 *
 * The opcode param encodes the location of the buffer and its size.
 */
static void
cmd_batch(uint opcode)
{
   const uint buf = (opcode >> 8) & 0xff;
   uint size = (opcode >> 16);
   PIPE_ALIGN_VAR(16) qword buffer[CELL_BUFFER_SIZE / 16];
   const unsigned usize = ROUNDUP16(size) / sizeof(buffer[0]);
   uint pos;

   D_PRINTF(CELL_DEBUG_CMD, "BATCH buffer %u, len %u, from %p\n",
             buf, size, spu.init.buffers[buf]);

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
   D_PRINTF(CELL_DEBUG_CMD, "release batch buf %u\n", buf);
   release_buffer(buf);

   /*
    * Loop over commands in the batch buffer
    */
   for (pos = 0; pos < usize; /* no incr */) {
      switch (si_to_uint(buffer[pos])) {
      /*
       * rendering commands
       */
      case CELL_CMD_CLEAR_SURFACE:
         {
            struct cell_command_clear_surface *clr
               = (struct cell_command_clear_surface *) &buffer[pos];
            cmd_clear_surface(clr);
            pos += sizeof(*clr) / 16;
         }
         break;
      case CELL_CMD_RENDER:
         {
            struct cell_command_render *render
               = (struct cell_command_render *) &buffer[pos];
            uint pos_incr;
            cmd_render(render, &pos_incr);
            pos += ((pos_incr+1)&~1) / 2; // should 'fix' cmd_render return
         }
         break;
      /*
       * state-update commands
       */
      case CELL_CMD_STATE_FRAMEBUFFER:
         {
            struct cell_command_framebuffer *fb
               = (struct cell_command_framebuffer *) &buffer[pos];
            cmd_state_framebuffer(fb);
            pos += sizeof(*fb) / 16;
         }
         break;
      case CELL_CMD_STATE_FRAGMENT_OPS:
         {
            struct cell_command_fragment_ops *fops
               = (struct cell_command_fragment_ops *) &buffer[pos];
            cmd_state_fragment_ops(fops);
            /* This is a variant-sized command */
            pos += ROUNDUP16(sizeof(*fops) + fops->total_code_size) / 16;
         }
         break;
      case CELL_CMD_STATE_FRAGMENT_PROGRAM:
         {
            struct cell_command_fragment_program *fp
               = (struct cell_command_fragment_program *) &buffer[pos];
            cmd_state_fragment_program(fp);
            pos += sizeof(*fp) / 16;
         }
         break;
      case CELL_CMD_STATE_FS_CONSTANTS:
         pos = cmd_state_fs_constants(buffer, pos);
         break;
      case CELL_CMD_STATE_RASTERIZER:
         {
            struct cell_command_rasterizer *rast =
               (struct cell_command_rasterizer *) &buffer[pos];
            spu.rasterizer = rast->rasterizer;
            pos += sizeof(*rast) / 16;
         }
         break;
      case CELL_CMD_STATE_SAMPLER:
         {
            struct cell_command_sampler *sampler
               = (struct cell_command_sampler *) &buffer[pos];
            cmd_state_sampler(sampler);
            pos += sizeof(*sampler) / 16;
         }
         break;
      case CELL_CMD_STATE_TEXTURE:
         {
            struct cell_command_texture *texture
               = (struct cell_command_texture *) &buffer[pos];
            cmd_state_texture(texture);
            pos += sizeof(*texture) / 16;
         }
         break;
      case CELL_CMD_STATE_VERTEX_INFO:
         cmd_state_vertex_info((struct vertex_info *) &buffer[pos+1]);
         pos += 1 + ROUNDUP16(sizeof(struct vertex_info)) / 16;
         break;
      case CELL_CMD_STATE_VIEWPORT:
         (void) memcpy(& draw.viewport, &buffer[pos+1],
                       sizeof(struct pipe_viewport_state));
         pos += 1 + ROUNDUP16(sizeof(struct pipe_viewport_state)) / 16;
         break;
      case CELL_CMD_STATE_UNIFORMS:
         draw.constants = (const float (*)[4]) (uintptr_t)spu_extract((vector unsigned int)buffer[pos+1],0);
         pos += 2;
         break;
      case CELL_CMD_STATE_VS_ARRAY_INFO:
         cmd_state_vs_array_info((struct cell_array_info *) &buffer[pos+1]);
         pos += 1 + ROUNDUP16(sizeof(struct cell_array_info)) / 16;
         break;
      case CELL_CMD_STATE_BIND_VS:
#if 0
         spu_bind_vertex_shader(&draw,
                                (struct cell_shader_info *) &buffer[pos+1]);
#endif
         pos += 1 + ROUNDUP16(sizeof(struct cell_shader_info)) / 16;
         break;
      case CELL_CMD_STATE_ATTRIB_FETCH:
         cmd_state_attrib_fetch((struct cell_attribute_fetch_code *)
                                &buffer[pos+1]);
         pos += 1 + ROUNDUP16(sizeof(struct cell_attribute_fetch_code)) / 16;
         break;
      /*
       * misc commands
       */
      case CELL_CMD_FINISH:
         cmd_finish();
         pos += 1;
         break;
      case CELL_CMD_FENCE:
         {
            struct cell_command_fence *fence_cmd =
               (struct cell_command_fence *) &buffer[pos];
            cmd_fence(fence_cmd);
            pos += sizeof(*fence_cmd) / 16;
         }
         break;
      case CELL_CMD_RELEASE_VERTS:
         {
            struct cell_command_release_verts *release
               = (struct cell_command_release_verts *) &buffer[pos];
            cmd_release_verts(release);
            pos += sizeof(*release) / 16;
         }
         break;
      case CELL_CMD_FLUSH_BUFFER_RANGE: {
	 struct cell_buffer_range *br = (struct cell_buffer_range *)
	     &buffer[pos+1];

	 spu_dcache_mark_dirty((unsigned) br->base, br->size);
         pos += 1 + ROUNDUP16(sizeof(struct cell_buffer_range)) / 16;
	 break;
      }
      default:
         printf("SPU %u: bad opcode: 0x%x\n", spu.init.id, si_to_uint(buffer[pos]));
         ASSERT(0);
         break;
      }
   }

   D_PRINTF(CELL_DEBUG_CMD, "BATCH complete\n");
}


#define PERF 0


/**
 * Main loop for SPEs: Get a command, execute it, repeat.
 */
void
command_loop(void)
{
   int exitFlag = 0;
   uint t0, t1;

   D_PRINTF(CELL_DEBUG_CMD, "Enter command loop\n");

   while (!exitFlag) {
      unsigned opcode;

      D_PRINTF(CELL_DEBUG_CMD, "Wait for cmd...\n");

      if (PERF)
         spu_write_decrementer(~0);

      /* read/wait from mailbox */
      opcode = (unsigned int) spu_read_in_mbox();
      D_PRINTF(CELL_DEBUG_CMD, "got cmd 0x%x\n", opcode);

      if (PERF)
         t0 = spu_read_decrementer();

      switch (opcode & CELL_CMD_OPCODE_MASK) {
      case CELL_CMD_EXIT:
         D_PRINTF(CELL_DEBUG_CMD, "EXIT\n");
         exitFlag = 1;
         break;
      case CELL_CMD_VS_EXECUTE:
#if 0
         spu_execute_vertex_shader(&draw, &cmd.vs);
#endif
         break;
      case CELL_CMD_BATCH:
         cmd_batch(opcode);
         break;
      default:
         printf("Bad opcode 0x%x!\n", opcode & CELL_CMD_OPCODE_MASK);
      }

      if (PERF) {
         t1 = spu_read_decrementer();
         printf("wait mbox time: %gms   batch time: %gms\n",
                (~0u - t0) * spu.init.inv_timebase,
                (t0 - t1) * spu.init.inv_timebase);
      }
   }

   D_PRINTF(CELL_DEBUG_CMD, "Exit command loop\n");

   if (spu.init.debug_flags & CELL_DEBUG_CACHE)
      spu_dcache_report();
}

/* Initialize this module; we manage the fragment ops buffer here. */
void
spu_command_init(void)
{
   /* Install default/fallback fragment processing function.
    * This will normally be overriden by a code-gen'd function
    * unless CELL_FORCE_FRAGMENT_OPS_FALLBACK is set.
    */
   spu.fragment_ops[CELL_FACING_FRONT] = spu_fallback_fragment_ops;
   spu.fragment_ops[CELL_FACING_BACK] = spu_fallback_fragment_ops;

   /* Set up the basic empty buffer for code-gen'ed fragment ops */
   spu.fragment_ops_code = NULL;
   spu.fragment_ops_code_size = 0;
}

void
spu_command_close(void)
{
   /* Deallocate the code-gen buffer for fragment ops, and reset the
    * fragment ops functions to their initial setting (just to leave
    * things in a good state).
    */
   if (spu.fragment_ops_code != NULL) {
      free(spu.fragment_ops_code);
   }
   spu_command_init();
}
