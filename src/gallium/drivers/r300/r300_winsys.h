/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Copyright 2010 Marek Olšák <maraeo@gmail.com>
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

/* The public winsys interface header for the r300 pipe driver.
 * Any winsys hosting this pipe needs to implement r300_winsys_screen and then
 * call r300_screen_create to start things. */

#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "r300_defines.h"

#define R300_MAX_CMDBUF_DWORDS (16 * 1024)

struct winsys_handle;
struct r300_winsys_screen;

struct r300_winsys_buffer;      /* for map/unmap etc. */
struct r300_winsys_cs_buffer;   /* for write_reloc etc. */

struct r300_winsys_cs {
    unsigned cdw;       /* Number of used dwords. */
    uint32_t *buf;      /* The command buffer. */
};

enum r300_value_id {
    R300_VID_PCI_ID,
    R300_VID_GB_PIPES,
    R300_VID_Z_PIPES,
    R300_VID_SQUARE_TILING_SUPPORT,
    R300_VID_DRM_2_3_0,
    R300_VID_DRM_2_6_0,
    R300_CAN_HYPERZ,
};

enum r300_reference_domain { /* bitfield */
    R300_REF_CS = 1,
    R300_REF_HW = 2
};

struct r300_winsys_screen {
    /**
     * Destroy this winsys.
     *
     * \param ws        The winsys this function is called from.
     */
    void (*destroy)(struct r300_winsys_screen *ws);

    /**
     * Query a system value from a winsys.
     *
     * \param ws        The winsys this function is called from.
     * \param vid       One of the R300_VID_* enums.
     */
    uint32_t (*get_value)(struct r300_winsys_screen *ws,
                          enum r300_value_id vid);

    /**************************************************************************
     * Buffer management. Buffer attributes are mostly fixed over its lifetime.
     *
     * Remember that gallium gets to choose the interface it needs, and the
     * window systems must then implement that interface (rather than the
     * other way around...).
     *************************************************************************/

    /**
     * Create a buffer object.
     *
     * \param ws        The winsys this function is called from.
     * \param size      The size to allocate.
     * \param alignment An alignment of the buffer in memory.
     * \param bind      A bitmask of the PIPE_BIND_* flags.
     * \param usage     A bitmask of the PIPE_USAGE_* flags.
     * \param domain    A bitmask of the R300_DOMAIN_* flags.
     * \return          The created buffer object.
     */
    struct r300_winsys_buffer *(*buffer_create)(struct r300_winsys_screen *ws,
                                                unsigned size,
                                                unsigned alignment,
                                                unsigned bind,
                                                unsigned usage,
                                                enum r300_buffer_domain domain);

    struct r300_winsys_cs_buffer *(*buffer_get_cs_handle)(
            struct r300_winsys_screen *ws,
            struct r300_winsys_buffer *buf);

    /**
     * Reference a buffer object (assign with reference counting).
     *
     * \param ws        The winsys this function is called from.
     * \param pdst      A destination pointer to set the source buffer to.
     * \param src       A source buffer object.
     */
    void (*buffer_reference)(struct r300_winsys_screen *ws,
                             struct r300_winsys_buffer **pdst,
                             struct r300_winsys_buffer *src);

    /**
     * Map the entire data store of a buffer object into the client's address
     * space.
     *
     * \param ws        The winsys this function is called from.
     * \param buf       A winsys buffer object to map.
     * \param cs        A command stream to flush if the buffer is referenced by it.
     * \param usage     A bitmask of the PIPE_TRANSFER_* flags.
     * \return          The pointer at the beginning of the buffer.
     */
    void *(*buffer_map)(struct r300_winsys_screen *ws,
                        struct r300_winsys_buffer *buf,
                        struct r300_winsys_cs *cs,
                        enum pipe_transfer_usage usage);

    /**
     * Unmap a buffer object from the client's address space.
     *
     * \param ws        The winsys this function is called from.
     * \param buf       A winsys buffer object to unmap.
     */
    void (*buffer_unmap)(struct r300_winsys_screen *ws,
                         struct r300_winsys_buffer *buf);

    /**
     * Wait for a buffer object until it is not used by a GPU. This is
     * equivalent to a fence placed after the last command using the buffer,
     * and synchronizing to the fence.
     *
     * \param ws        The winsys this function is called from.
     * \param buf       A winsys buffer object to wait for.
     */
    void (*buffer_wait)(struct r300_winsys_screen *ws,
                        struct r300_winsys_buffer *buf);

    /**
     * Return tiling flags describing a memory layout of a buffer object.
     *
     * \param ws        The winsys this function is called from.
     * \param buf       A winsys buffer object to get the flags from.
     * \param macrotile A pointer to the return value of the microtile flag.
     * \param microtile A pointer to the return value of the macrotile flag.
     *
     * \note microtile and macrotile are not bitmasks!
     */
    void (*buffer_get_tiling)(struct r300_winsys_screen *ws,
                              struct r300_winsys_buffer *buf,
                              enum r300_buffer_tiling *microtile,
                              enum r300_buffer_tiling *macrotile);

    /**
     * Set tiling flags describing a memory layout of a buffer object.
     *
     * \param ws        The winsys this function is called from.
     * \param buf       A winsys buffer object to set the flags for.
     * \param macrotile A macrotile flag.
     * \param microtile A microtile flag.
     * \param stride    A stride of the buffer in bytes, for texturing.
     *
     * \note microtile and macrotile are not bitmasks!
     */
    void (*buffer_set_tiling)(struct r300_winsys_screen *ws,
                              struct r300_winsys_buffer *buf,
                              enum r300_buffer_tiling microtile,
                              enum r300_buffer_tiling macrotile,
                              unsigned stride);

    /**
     * Get a winsys buffer from a winsys handle. The internal structure
     * of the handle is platform-specific and only a winsys should access it.
     *
     * \param ws        The winsys this function is called from.
     * \param whandle   A winsys handle pointer as was received from a state
     *                  tracker.
     * \param stride    The returned buffer stride in bytes.
     * \param size      The returned buffer size.
     */
    struct r300_winsys_buffer *(*buffer_from_handle)(struct r300_winsys_screen *ws,
                                                     struct winsys_handle *whandle,
                                                     unsigned *stride,
                                                     unsigned *size);

    /**
     * Get a winsys handle from a winsys buffer. The internal structure
     * of the handle is platform-specific and only a winsys should access it.
     *
     * \param ws        The winsys this function is called from.
     * \param buf       A winsys buffer object to get the handle from.
     * \param whandle   A winsys handle pointer.
     * \param stride    A stride of the buffer in bytes, for texturing.
     * \return          TRUE on success.
     */
    boolean (*buffer_get_handle)(struct r300_winsys_screen *ws,
                                 struct r300_winsys_buffer *buf,
                                 unsigned stride,
                                 struct winsys_handle *whandle);

    /**************************************************************************
     * Command submission.
     *
     * Each pipe context should create its own command stream and submit
     * commands independently of other contexts.
     *************************************************************************/

    /**
     * Create a command stream.
     *
     * \param ws        The winsys this function is called from.
     */
    struct r300_winsys_cs *(*cs_create)(struct r300_winsys_screen *ws);

    /**
     * Destroy a command stream.
     *
     * \param cs        A command stream to destroy.
     */
    void (*cs_destroy)(struct r300_winsys_cs *cs);

    /**
     * Add a buffer object to the list of buffers to validate.
     *
     * \param cs        A command stream to add buffer for validation against.
     * \param buf       A winsys buffer to validate.
     * \param rd        A read domain containing a bitmask
     *                  of the R300_DOMAIN_* flags.
     * \param wd        A write domain containing a bitmask
     *                  of the R300_DOMAIN_* flags.
     */
    void (*cs_add_buffer)(struct r300_winsys_cs *cs,
                          struct r300_winsys_cs_buffer *buf,
                          enum r300_buffer_domain rd,
                          enum r300_buffer_domain wd);

    /**
     * Revalidate all currently set up winsys buffers.
     * Returns TRUE if a flush is required.
     *
     * \param cs        A command stream to validate.
     */
    boolean (*cs_validate)(struct r300_winsys_cs *cs);

    /**
     * Write a relocated dword to a command buffer.
     *
     * \param cs        A command stream the relocation is written to.
     * \param buf       A winsys buffer to write the relocation for.
     * \param rd        A read domain containing a bitmask of the R300_DOMAIN_* flags.
     * \param wd        A write domain containing a bitmask of the R300_DOMAIN_* flags.
     */
    void (*cs_write_reloc)(struct r300_winsys_cs *cs,
                           struct r300_winsys_cs_buffer *buf,
                           enum r300_buffer_domain rd,
                           enum r300_buffer_domain wd);

    /**
     * Flush a command stream.
     *
     * \param cs        A command stream to flush.
     */
    void (*cs_flush)(struct r300_winsys_cs *cs);

    /**
     * Set a flush callback which is called from winsys when flush is
     * required.
     *
     * \param cs        A command stream to set the callback for.
     * \param flush     A flush callback function associated with the command stream.
     * \param user      A user pointer that will be passed to the flush callback.
     */
    void (*cs_set_flush)(struct r300_winsys_cs *cs,
                         void (*flush)(void *),
                         void *user);

    /**
     * Reset the list of buffer objects to validate, usually called
     * prior to adding buffer objects for validation.
     *
     * \param cs        A command stream to reset buffers for.
     */
    void (*cs_reset_buffers)(struct r300_winsys_cs *cs);

    /**
     * Return TRUE if a buffer is referenced by a command stream or by hardware
     * (i.e. is busy), based on the domain parameter.
     *
     * \param cs        A command stream.
     * \param buf       A winsys buffer.
     * \param domain    A bitmask of the R300_REF_* enums.
     */
    boolean (*cs_is_buffer_referenced)(struct r300_winsys_cs *cs,
                                       struct r300_winsys_cs_buffer *buf,
                                       enum r300_reference_domain domain);
};

#endif /* R300_WINSYS_H */
