
#define _NEED_GL_CORE_IF
#include <GL/xmesa.h>
#include <GL/internal/glcore.h>
#include "xmesaP.h"

PUBLIC
__GLcoreModule GL_Core = {
    XMesaCreateVisual,
    XMesaDestroyVisual,

    XMesaCreateWindowBuffer,
    XMesaCreatePixmapBuffer,
    XMesaDestroyBuffer,
    XMesaSwapBuffers,
    XMesaResizeBuffers,

    XMesaCreateContext,
    XMesaDestroyContext,
    XMesaCopyContext,
    XMesaMakeCurrent2,
    XMesaForceCurrent,
    XMesaLoseCurrent
};
