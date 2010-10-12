
#ifndef BRW_PUBLIC_H
#define BRW_PUBLIC_H

struct brw_winsys_screen;
struct pipe_screen;

/**
 * Create brw AKA i965 pipe_screen.
 */
struct pipe_screen * brw_screen_create(struct brw_winsys_screen *bws);

#endif
