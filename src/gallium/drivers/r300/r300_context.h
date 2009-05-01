/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#ifndef R300_CONTEXT_H
#define R300_CONTEXT_H

#include "draw/draw_context.h"
#include "draw/draw_vertex.h"
#include "pipe/p_context.h"
#include "tgsi/tgsi_scan.h"
#include "util/u_memory.h"

#include "r300_clear.h"
#include "r300_query.h"
#include "r300_screen.h"
#include "r300_winsys.h"

struct r300_blend_state {
    uint32_t blend_control;       /* R300_RB3D_CBLEND: 0x4e04 */
    uint32_t alpha_blend_control; /* R300_RB3D_ABLEND: 0x4e08 */
    uint32_t rop;                 /* R300_RB3D_ROPCNTL: 0x4e18 */
    uint32_t dither;              /* R300_RB3D_DITHER_CTL: 0x4e50 */
};

struct r300_blend_color_state {
    /* RV515 and earlier */
    uint32_t blend_color;            /* R300_RB3D_BLEND_COLOR: 0x4e10 */
    /* R520 and newer */
    uint32_t blend_color_red_alpha;  /* R500_RB3D_CONSTANT_COLOR_AR: 0x4ef8 */
    uint32_t blend_color_green_blue; /* R500_RB3D_CONSTANT_COLOR_GB: 0x4efc */
};

struct r300_dsa_state {
    uint32_t alpha_function;    /* R300_FG_ALPHA_FUNC: 0x4bd4 */
    uint32_t alpha_reference;   /* R500_FG_ALPHA_VALUE: 0x4be0 */
    uint32_t z_buffer_control;  /* R300_ZB_CNTL: 0x4f00 */
    uint32_t z_stencil_control; /* R300_ZB_ZSTENCILCNTL: 0x4f04 */
    uint32_t stencil_ref_mask;  /* R300_ZB_STENCILREFMASK: 0x4f08 */
    uint32_t z_buffer_top;      /* R300_ZB_ZTOP: 0x4f14 */
    uint32_t stencil_ref_bf;    /* R500_ZB_STENCILREFMASK_BF: 0x4fd4 */
};

struct r300_rs_state {
    /* Draw-specific rasterizer state */
    struct pipe_rasterizer_state rs;

    uint32_t vap_control_status;    /* R300_VAP_CNTL_STATUS: 0x2140 */
    uint32_t point_size;            /* R300_GA_POINT_SIZE: 0x421c */
    uint32_t point_minmax;          /* R300_GA_POINT_MINMAX: 0x4230 */
    uint32_t line_control;          /* R300_GA_LINE_CNTL: 0x4234 */
    uint32_t depth_scale_front;  /* R300_SU_POLY_OFFSET_FRONT_SCALE: 0x42a4 */
    uint32_t depth_offset_front;/* R300_SU_POLY_OFFSET_FRONT_OFFSET: 0x42a8 */
    uint32_t depth_scale_back;    /* R300_SU_POLY_OFFSET_BACK_SCALE: 0x42ac */
    uint32_t depth_offset_back;  /* R300_SU_POLY_OFFSET_BACK_OFFSET: 0x42b0 */
    uint32_t polygon_offset_enable; /* R300_SU_POLY_OFFSET_ENABLE: 0x42b4 */
    uint32_t cull_mode;             /* R300_SU_CULL_MODE: 0x42b8 */
    uint32_t line_stipple_config;   /* R300_GA_LINE_STIPPLE_CONFIG: 0x4328 */
    uint32_t line_stipple_value;    /* R300_GA_LINE_STIPPLE_VALUE: 0x4260 */
    uint32_t color_control;         /* R300_GA_COLOR_CONTROL: 0x4278 */
};

struct r300_rs_block {
    uint32_t ip[8]; /* R300_RS_IP_[0-7], R500_RS_IP_[0-7] */
    uint32_t count; /* R300_RS_COUNT */
    uint32_t inst_count; /* R300_RS_INST_COUNT */
    uint32_t inst[8]; /* R300_RS_INST_[0-7] */
};

struct r300_sampler_state {
    uint32_t filter0;      /* R300_TX_FILTER0: 0x4400 */
    uint32_t filter1;      /* R300_TX_FILTER1: 0x4440 */
    uint32_t border_color; /* R300_TX_BORDER_COLOR: 0x45c0 */
};

struct r300_scissor_state {
    uint32_t scissor_top_left;     /* R300_SC_SCISSORS_TL: 0x43e0 */
    uint32_t scissor_bottom_right; /* R300_SC_SCISSORS_BR: 0x43e4 */
};

struct r300_texture_state {
    uint32_t format0; /* R300_TX_FORMAT0: 0x4480 */
    uint32_t format1; /* R300_TX_FORMAT1: 0x44c0 */
    uint32_t format2; /* R300_TX_FORMAT2: 0x4500 */
};

struct r300_viewport_state {
    float xscale;         /* R300_VAP_VPORT_XSCALE:  0x2098 */
    float xoffset;        /* R300_VAP_VPORT_XOFFSET: 0x209c */
    float yscale;         /* R300_VAP_VPORT_YSCALE:  0x20a0 */
    float yoffset;        /* R300_VAP_VPORT_YOFFSET: 0x20a4 */
    float zscale;         /* R300_VAP_VPORT_ZSCALE:  0x20a8 */
    float zoffset;        /* R300_VAP_VPORT_ZOFFSET: 0x20ac */
    uint32_t vte_control; /* R300_VAP_VTE_CNTL:      0x20b0 */
};

#define R300_NEW_BLEND           0x0000001
#define R300_NEW_BLEND_COLOR     0x0000002
#define R300_NEW_CONSTANTS       0x0000004
#define R300_NEW_DSA             0x0000008
#define R300_NEW_FRAMEBUFFERS    0x0000010
#define R300_NEW_FRAGMENT_SHADER 0x0000020
#define R300_NEW_RASTERIZER      0x0000040
#define R300_NEW_RS_BLOCK        0x0000080
#define R300_NEW_SAMPLER         0x0000100
#define R300_ANY_NEW_SAMPLERS    0x000ff00
#define R300_NEW_SCISSOR         0x0010000
#define R300_NEW_TEXTURE         0x0020000
#define R300_ANY_NEW_TEXTURES    0x1fe0000
#define R300_NEW_VERTEX_FORMAT   0x2000000
#define R300_NEW_VERTEX_SHADER   0x4000000
#define R300_NEW_VIEWPORT        0x8000000
#define R300_NEW_KITCHEN_SINK    0xfffffff

/* The next several objects are not pure Radeon state; they inherit from
 * various Gallium classes. */

struct r300_constant_buffer {
    /* Buffer of constants */
    /* XXX first number should be raised */
    float constants[8][4];
    /* Number of user-defined constants */
    int user_count;
    /* Total number of constants */
    int count;
};

struct r3xx_fragment_shader {
    /* Parent class */
    struct pipe_shader_state state;
    struct tgsi_shader_info info;

    /* Has this shader been translated yet? */
    boolean translated;

    /* Pixel stack size */
    int stack_size;
};

struct r300_fragment_shader {
    /* Parent class */
    struct r3xx_fragment_shader shader;

    /* Number of ALU instructions */
    int alu_instruction_count;

    /* Number of texture instructions */
    int tex_instruction_count;

    /* Number of texture indirections */
    int indirections;

    /* Indirection node offsets */
    int alu_offset[4];

    /* Machine instructions */
    struct {
        uint32_t alu_rgb_inst;
        uint32_t alu_rgb_addr;
        uint32_t alu_alpha_inst;
        uint32_t alu_alpha_addr;
    } instructions[64]; /* XXX magic num */
};

struct r500_fragment_shader {
    /* Parent class */
    struct r3xx_fragment_shader shader;

    /* Number of used instructions */
    int instruction_count;

    /* Machine instructions */
    struct {
        uint32_t inst0;
        uint32_t inst1;
        uint32_t inst2;
        uint32_t inst3;
        uint32_t inst4;
        uint32_t inst5;
    } instructions[256]; /*< XXX magic number */
};

struct r300_texture {
    /* Parent class */
    struct pipe_texture tex;

    /* Offsets into the buffer. */
    unsigned offset[PIPE_MAX_TEXTURE_LEVELS];

    /* Stride (pitch?) of this texture in bytes */
    unsigned stride;
    
    /* Total size of this texture, in bytes. */
    unsigned size;

    /* Pipe buffer backing this texture. */
    struct pipe_buffer* buffer;

    /* Registers carrying texture format data. */
    struct r300_texture_state state;
};

struct r300_vertex_format {
    /* Parent class */
    struct vertex_info vinfo;
    /* R300_VAP_PROG_STREAK_CNTL_[0-7] */
    uint32_t vap_prog_stream_cntl[8];
    /* R300_VAP_PROG_STREAK_CNTL_EXT_[0-7] */
    uint32_t vap_prog_stream_cntl_ext[8];
    /* Map of vertex attributes into PVS memory for HW TCL,
     * or GA memory for SW TCL. */
    int vs_tab[16];
    /* Map of rasterizer attributes from GB through RS to US. */
    int fs_tab[16];
};

struct r300_vertex_shader {
    /* Parent class */
    struct pipe_shader_state state;
    struct tgsi_shader_info info;

    /* Fallback shader, because Draw has issues */
    struct draw_vertex_shader* draw;

    /* Has this shader been translated yet? */
    boolean translated;

    /* Number of used instructions */
    int instruction_count;

    /* Machine instructions */
    struct {
        uint32_t inst0;
        uint32_t inst1;
        uint32_t inst2;
        uint32_t inst3;
    } instructions[128]; /*< XXX magic number */
};

struct r300_context {
    /* Parent class */
    struct pipe_context context;

    /* The interface to the windowing system, etc. */
    struct r300_winsys* winsys;
    /* Draw module. Used mostly for SW TCL. */
    struct draw_context* draw;

    /* Various CSO state objects. */
    /* Blend state. */
    struct r300_blend_state* blend_state;
    /* Blend color state. */
    struct r300_blend_color_state* blend_color_state;
    /* Shader constants. */
    struct r300_constant_buffer shader_constants[PIPE_SHADER_TYPES];
    /* Depth, stencil, and alpha state. */
    struct r300_dsa_state* dsa_state;
    /* Fragment shader. */
    struct r3xx_fragment_shader* fs;
    /* Framebuffer state. We currently don't need our own version of this. */
    struct pipe_framebuffer_state framebuffer_state;
    /* Rasterizer state. */
    struct r300_rs_state* rs_state;
    /* RS block state. */
    struct r300_rs_block* rs_block;
    /* Sampler states. */
    struct r300_sampler_state* sampler_states[8];
    int sampler_count;
    /* Scissor state. */
    struct r300_scissor_state* scissor_state;
    /* Texture states. */
    struct r300_texture* textures[8];
    int texture_count;
    /* Vertex buffers. */
    struct pipe_vertex_buffer vertex_buffers[PIPE_MAX_ATTRIBS];
    int vertex_buffer_count;
    /* Vertex information. */
    struct r300_vertex_format vertex_info;
    /* Vertex shader. */
    struct r300_vertex_shader* vs;
    /* Viewport state. */
    struct r300_viewport_state* viewport_state;
    /* Bitmask of dirty state objects. */
    uint32_t dirty_state;
    /* Flag indicating whether or not the HW is dirty. */
    uint32_t dirty_hw;
};

/* Convenience cast wrapper. */
static struct r300_context* r300_context(struct pipe_context* context) {
    return (struct r300_context*)context;
}

/* Context initialization. */
struct draw_stage* r300_draw_stage(struct r300_context* r300);
void r300_init_state_functions(struct r300_context* r300);
void r300_init_surface_functions(struct r300_context* r300);

#endif /* R300_CONTEXT_H */
