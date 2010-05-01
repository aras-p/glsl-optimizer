#include <VG/openvg.h>
#include <EGL/egl.h>

#include "lion-render.h"
#include "eglut.h"

static VGint width, height;
struct lion *lion = 0;
VGfloat angle = 0;

static void
draw(void)
{
   vgClear(0, 0, width, height);

   vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
   vgLoadIdentity();
   vgTranslate(width/2, height/2);
   vgRotate(angle);
   vgTranslate(-width/2, -height/2);

   lion_render(lion);

   ++angle;
   eglutPostRedisplay();
}


/* new window size or exposure */
static void
reshape(int w, int h)
{
   width  = w;
   height = h;
}


static void
init(void)
{
   float clear_color[4] = {1.0, 1.0, 1.0, 1.0};
   vgSetfv(VG_CLEAR_COLOR, 4, clear_color);

   lion = lion_create();
}


int
main(int argc, char *argv[])
{
   eglutInitWindowSize(350, 450);
   eglutInitAPIMask(EGLUT_OPENVG_BIT);
   eglutInit(argc, argv);

   eglutCreateWindow("Lion Example");

   eglutReshapeFunc(reshape);
   eglutDisplayFunc(draw);

   init();

   eglutMainLoop();

   return 0;
}
