#ifndef CELL_PUBLIC_H
#define CELL_PUBLIC_H

struct pipe_screen;
struct sw_winsys;

struct pipe_screen *
cell_create_screen(struct sw_winsys *winsys);

#endif
