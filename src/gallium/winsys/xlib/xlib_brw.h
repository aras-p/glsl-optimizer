#ifndef XLIB_BRW_H
#define XLIB_BRW_H

struct pipe_winsys;
struct pipe_buffer;
struct pipe_surface;
struct xmesa_buffer;

unsigned xlib_brw_get_buffer_offset( struct pipe_winsys *pws,
                                     struct pipe_buffer *buf,
                                     unsigned access_flags );

void xlib_brw_buffer_subdata_typed( struct pipe_winsys *pws,
                                    struct pipe_buffer *buf,
                                    unsigned long offset, 
                                    unsigned long size, 
                                    const void *data,
                                    unsigned data_type );



void xlib_brw_commands_aub(struct pipe_winsys *winsys,
                           unsigned *cmds,
                           unsigned nr_dwords);

struct pipe_context *
xlib_create_brw_context( struct pipe_screen *screen,
                         void *unused );

#endif
