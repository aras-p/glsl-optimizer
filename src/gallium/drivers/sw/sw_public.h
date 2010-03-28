#ifndef SW_PUBLIC_H
#define SW_PUBLIC_H

/* A convenience library, primarily to isolate the logic required to
 * figure out which if any software rasterizers have been built and
 * select between them.
 */
struct sw_winsys;

struct pipe_screen *
swrast_create_screen(struct sw_winsys *winsys);

#endif
