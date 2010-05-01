#ifndef LION_RENDER_H
#define LION_RENDER_H

#include <VG/openvg.h>

#define LION_SIZE 132
struct lion {
   VGPath paths[LION_SIZE];
   VGPaint fills[LION_SIZE];
};

struct lion *lion_create(void);
void lion_render(struct lion *l);
void lion_destroy(struct lion *l);

#endif
