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

#ifndef R300_WINSYS_H
#define R300_WINSYS_H

/* The public interface header for the r300 pipe driver.
 * Any winsys hosting this pipe needs to implement r300_winsys and then
 * call r300_create_screen to start things. */

#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "r300_defines.h"

struct r300_winsys_screen;

/* Creates a new r300 screen. */
struct pipe_screen* r300_create_screen(struct r300_winsys_screen *rws);

struct r300_winsys_buffer;


boolean r300_get_texture_buffer(struct pipe_screen* screen,
                                struct pipe_texture* texture,
                                struct r300_winsys_buffer** buffer,
                                unsigned *stride);

enum r300_value_id {
    R300_VID_PCI_ID,
    R300_VID_GB_PIPES,
    R300_VID_Z_PIPES,
    R300_VID_SQUARE_TILING_SUPPORT
};

struct r300_winsys_screen {
    void (*destroy)(struct r300_winsys_screen *ws);
    
    /**
     * Buffer management. Buffer attributes are mostly fixed over its lifetime.
     *
     * Remember that gallium gets to choose the interface it needs, and the
     * window systems must then implement that interface (rather than the
     * other way around...).
     *
     * usage is a bitmask of R300_WINSYS_BUFFER_USAGE_PIXEL/VERTEX/INDEX/CONSTANT. This
     * usage argument is only an optimization hint, not a guarantee, therefore
     * proper behavior must be observed in all circumstances.
     *
     * alignment indicates the client's alignment requirements, eg for
     * SSE instructions.
     */
    struct r300_winsys_buffer *(*buffer_create)(struct r300_winsys_screen *ws,
						unsigned alignment,
						unsigned usage,
						unsigned size);
    
    /**
     * Map the entire data store of a buffer object into the client's address.
     * flags is bitmask of R300_WINSYS_BUFFER_USAGE_CPU_READ/WRITE flags.
     */
    void *(*buffer_map)( struct r300_winsys_screen *ws,
			 struct r300_winsys_buffer *buf,
			 unsigned usage);

    void (*buffer_unmap)( struct r300_winsys_screen *ws,
			  struct r300_winsys_buffer *buf );

    void (*buffer_destroy)( struct r300_winsys_buffer *buf );


    void (*buffer_reference)(struct r300_winsys_screen *rws,
			     struct r300_winsys_buffer **pdst,
			     struct r300_winsys_buffer *src);

    boolean (*buffer_references)(struct r300_winsys_buffer *a,
				 struct r300_winsys_buffer *b);

    void (*buffer_flush_range)(struct r300_winsys_screen *rws,
			       struct r300_winsys_buffer *buf,
			       unsigned offset,
			       unsigned length);

    /* Add a pipe_buffer to the list of buffer objects to validate. */
    boolean (*add_buffer)(struct r300_winsys_screen *winsys,
                          struct r300_winsys_buffer *buf,
                          uint32_t rd,
                          uint32_t wd);


    /* Revalidate all currently setup pipe_buffers.
     * Returns TRUE if a flush is required. */
    boolean (*validate)(struct r300_winsys_screen* winsys);

    /* Check to see if there's room for commands. */
    boolean (*check_cs)(struct r300_winsys_screen* winsys, int size);

    /* Start a command emit. */
    void (*begin_cs)(struct r300_winsys_screen* winsys,
                     int size,
                     const char* file,
                     const char* function,
                     int line);

    /* Write a dword to the command buffer. */
    void (*write_cs_dword)(struct r300_winsys_screen* winsys, uint32_t dword);

    /* Write a relocated dword to the command buffer. */
    void (*write_cs_reloc)(struct r300_winsys_screen *winsys,
                           struct r300_winsys_buffer *buf,
                           uint32_t rd,
                           uint32_t wd,
                           uint32_t flags);

    /* Finish a command emit. */
    void (*end_cs)(struct r300_winsys_screen* winsys,
                   const char* file,
                   const char* function,
                   int line);

    /* Flush the CS. */
    void (*flush_cs)(struct r300_winsys_screen* winsys);

    /* winsys flush - callback from winsys when flush required */
    void (*set_flush_cb)(struct r300_winsys_screen *winsys,
			 void (*flush_cb)(void *), void *data);

    void (*reset_bos)(struct r300_winsys_screen *winsys);

    void (*buffer_set_tiling)(struct r300_winsys_screen *winsys,
                              struct r300_winsys_buffer *buffer,
                              uint32_t pitch,
                              enum r300_buffer_tiling microtiled,
                              enum r300_buffer_tiling macrotiled);

    uint32_t (*get_value)(struct r300_winsys_screen *winsys,
			  enum r300_value_id vid);

    struct r300_winsys_buffer *(*buffer_from_handle)(struct r300_winsys_screen *winsys,
						     struct pipe_screen *screen,
						     struct winsys_handle *whandle,
						     unsigned *stride);
    boolean (*buffer_get_handle)(struct r300_winsys_screen *winsys,
				 struct r300_winsys_buffer *buffer,
				 unsigned stride,
				 struct winsys_handle *whandle);

    boolean (*is_buffer_referenced)(struct r300_winsys_screen *winsys,
                                    struct r300_winsys_buffer *buffer);

  
};

struct r300_winsys_screen *
r300_winsys_screen(struct pipe_screen *screen);

#endif /* R300_WINSYS_H */
