#ifndef EGLCOMMON_H
#define EGLCOMMON_H

typedef void (*init_func)();
typedef void (*reshape_func)(int, int);
typedef void (*draw_func)();
typedef int  (*key_func)(unsigned key);


void set_window_size(int width, int height);
int window_width(void);
int window_height(void);

int run(int argc, char **argv,
        init_func init,
        reshape_func resh,
        draw_func draw,
        key_func key);

#endif
