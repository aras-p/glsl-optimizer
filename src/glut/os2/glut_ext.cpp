
/* Copyright (c) Mark J. Kilgard, 1994, 1997. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <string.h>

#include "glutint.h"

/* CENTRY */
int GLUTAPIENTRY
glutExtensionSupported(const char *extension)
{
  static const GLubyte *extensions = NULL;
  const GLubyte *start;
  GLubyte *where, *terminator;

  /* Extension names should not have spaces. */
  where = (GLubyte *) strchr(extension, ' ');
  if (where || *extension == '\0')
    return 0;

  if (!extensions) {
    extensions = glGetString(GL_EXTENSIONS);
  }
  /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string.  Don't be fooled by sub-strings,
     etc. */
  start = extensions;
  for (;;) {
    /* If your application crashes in the strstr routine below,
       you are probably calling glutExtensionSupported without
       having a current window.  Calling glGetString without
       a current OpenGL context has unpredictable results.
       Please fix your program. */
    where = (GLubyte *) strstr((const char *) start, extension);
    if (!where)
      break;
    terminator = where + strlen(extension);
    if (where == start || *(where - 1) == ' ') {
      if (*terminator == ' ' || *terminator == '\0') {
        return 1;
      }
    }
    start = terminator;
  }
  return 0;
}


struct name_address_pair {
   const char *name;
   const void *address;
};

static struct name_address_pair glut_functions[] = {
   { "glutInit", (const void *) glutInit },
   { "glutInitDisplayMode", (const void *) glutInitDisplayMode },
   { "glutInitDisplayString", (const void *) glutInitDisplayString },
   { "glutInitWindowPosition", (const void *) glutInitWindowPosition },
   { "glutInitWindowSize", (const void *) glutInitWindowSize },
   { "glutMainLoop", (const void *) glutMainLoop },
   { "glutCreateWindow", (const void *) glutCreateWindow },
   { "glutCreateSubWindow", (const void *) glutCreateSubWindow },
   { "glutDestroyWindow", (const void *) glutDestroyWindow },
   { "glutPostRedisplay", (const void *) glutPostRedisplay },
   { "glutPostWindowRedisplay", (const void *) glutPostWindowRedisplay },
   { "glutSwapBuffers", (const void *) glutSwapBuffers },
   { "glutGetWindow", (const void *) glutGetWindow },
   { "glutSetWindow", (const void *) glutSetWindow },
   { "glutSetWindowTitle", (const void *) glutSetWindowTitle },
   { "glutSetIconTitle", (const void *) glutSetIconTitle },
   { "glutPositionWindow", (const void *) glutPositionWindow },
   { "glutReshapeWindow", (const void *) glutReshapeWindow },
   { "glutPopWindow", (const void *) glutPopWindow },
   { "glutPushWindow", (const void *) glutPushWindow },
   { "glutIconifyWindow", (const void *) glutIconifyWindow },
   { "glutShowWindow", (const void *) glutShowWindow },
   { "glutHideWindow", (const void *) glutHideWindow },
   { "glutFullScreen", (const void *) glutFullScreen },
   { "glutSetCursor", (const void *) glutSetCursor },
   { "glutWarpPointer", (const void *) glutWarpPointer },
   { "glutEstablishOverlay", (const void *) glutEstablishOverlay },
   { "glutRemoveOverlay", (const void *) glutRemoveOverlay },
   { "glutUseLayer", (const void *) glutUseLayer },
   { "glutPostOverlayRedisplay", (const void *) glutPostOverlayRedisplay },
   { "glutPostWindowOverlayRedisplay", (const void *) glutPostWindowOverlayRedisplay },
   { "glutShowOverlay", (const void *) glutShowOverlay },
   { "glutHideOverlay", (const void *) glutHideOverlay },
   { "glutCreateMenu", (const void *) glutCreateMenu },
   { "glutDestroyMenu", (const void *) glutDestroyMenu },
   { "glutGetMenu", (const void *) glutGetMenu },
   { "glutSetMenu", (const void *) glutSetMenu },
   { "glutAddMenuEntry", (const void *) glutAddMenuEntry },
   { "glutAddSubMenu", (const void *) glutAddSubMenu },
   { "glutChangeToMenuEntry", (const void *) glutChangeToMenuEntry },
   { "glutChangeToSubMenu", (const void *) glutChangeToSubMenu },
   { "glutRemoveMenuItem", (const void *) glutRemoveMenuItem },
   { "glutAttachMenu", (const void *) glutAttachMenu },
   { "glutDetachMenu", (const void *) glutDetachMenu },
   { "glutDisplayFunc", (const void *) glutDisplayFunc },
   { "glutReshapeFunc", (const void *) glutReshapeFunc },
   { "glutKeyboardFunc", (const void *) glutKeyboardFunc },
   { "glutMouseFunc", (const void *) glutMouseFunc },
   { "glutMotionFunc", (const void *) glutMotionFunc },
   { "glutPassiveMotionFunc", (const void *) glutPassiveMotionFunc },
   { "glutEntryFunc", (const void *) glutEntryFunc },
   { "glutVisibilityFunc", (const void *) glutVisibilityFunc },
   { "glutIdleFunc", (const void *) glutIdleFunc },
   { "glutTimerFunc", (const void *) glutTimerFunc },
   { "glutMenuStateFunc", (const void *) glutMenuStateFunc },
   { "glutSpecialFunc", (const void *) glutSpecialFunc },
   { "glutSpaceballMotionFunc", (const void *) glutSpaceballMotionFunc },
   { "glutSpaceballRotateFunc", (const void *) glutSpaceballRotateFunc },
   { "glutSpaceballButtonFunc", (const void *) glutSpaceballButtonFunc },
   { "glutButtonBoxFunc", (const void *) glutButtonBoxFunc },
   { "glutDialsFunc", (const void *) glutDialsFunc },
   { "glutTabletMotionFunc", (const void *) glutTabletMotionFunc },
   { "glutTabletButtonFunc", (const void *) glutTabletButtonFunc },
   { "glutMenuStatusFunc", (const void *) glutMenuStatusFunc },
   { "glutOverlayDisplayFunc", (const void *) glutOverlayDisplayFunc },
   { "glutWindowStatusFunc", (const void *) glutWindowStatusFunc },
   { "glutKeyboardUpFunc", (const void *) glutKeyboardUpFunc },
   { "glutSpecialUpFunc", (const void *) glutSpecialUpFunc },
   { "glutJoystickFunc", (const void *) glutJoystickFunc },
   { "glutSetColor", (const void *) glutSetColor },
   { "glutGetColor", (const void *) glutGetColor },
   { "glutCopyColormap", (const void *) glutCopyColormap },
   { "glutGet", (const void *) glutGet },
   { "glutDeviceGet", (const void *) glutDeviceGet },
   { "glutExtensionSupported", (const void *) glutExtensionSupported },
   { "glutGetModifiers", (const void *) glutGetModifiers },
   { "glutLayerGet", (const void *) glutLayerGet },
   { "glutGetProcAddress", (const void *) glutGetProcAddress },
   { "glutBitmapCharacter", (const void *) glutBitmapCharacter },
   { "glutBitmapWidth", (const void *) glutBitmapWidth },
   { "glutStrokeCharacter", (const void *) glutStrokeCharacter },
   { "glutStrokeWidth", (const void *) glutStrokeWidth },
   { "glutBitmapLength", (const void *) glutBitmapLength },
   { "glutStrokeLength", (const void *) glutStrokeLength },
   { "glutWireSphere", (const void *) glutWireSphere },
   { "glutSolidSphere", (const void *) glutSolidSphere },
   { "glutWireCone", (const void *) glutWireCone },
   { "glutSolidCone", (const void *) glutSolidCone },
   { "glutWireCube", (const void *) glutWireCube },
   { "glutSolidCube", (const void *) glutSolidCube },
   { "glutWireTorus", (const void *) glutWireTorus },
   { "glutSolidTorus", (const void *) glutSolidTorus },
   { "glutWireDodecahedron", (const void *) glutWireDodecahedron },
   { "glutSolidDodecahedron", (const void *) glutSolidDodecahedron },
   { "glutWireTeapot", (const void *) glutWireTeapot },
   { "glutSolidTeapot", (const void *) glutSolidTeapot },
   { "glutWireOctahedron", (const void *) glutWireOctahedron },
   { "glutSolidOctahedron", (const void *) glutSolidOctahedron },
   { "glutWireTetrahedron", (const void *) glutWireTetrahedron },
   { "glutSolidTetrahedron", (const void *) glutSolidTetrahedron },
   { "glutWireIcosahedron", (const void *) glutWireIcosahedron },
   { "glutSolidIcosahedron", (const void *) glutSolidIcosahedron },
   { "glutVideoResizeGet", (const void *) glutVideoResizeGet },
   { "glutSetupVideoResizing", (const void *) glutSetupVideoResizing },
   { "glutStopVideoResizing", (const void *) glutStopVideoResizing },
   { "glutVideoResize", (const void *) glutVideoResize },
   { "glutVideoPan", (const void *) glutVideoPan },
   { "glutReportErrors", (const void *) glutReportErrors },
   { "glutIgnoreKeyRepeat", (const void *) glutIgnoreKeyRepeat },
   { "glutSetKeyRepeat", (const void *) glutSetKeyRepeat },
   { "glutForceJoystickFunc", (const void *) glutForceJoystickFunc },
   { "glutGameModeString", (const void *) glutGameModeString },
   { "glutEnterGameMode", (const void *) glutEnterGameMode },
   { "glutLeaveGameMode", (const void *) glutLeaveGameMode },
   { "glutGameModeGet", (const void *) glutGameModeGet },
   { NULL, NULL }
};


/* XXX This isn't an official GLUT function, yet */
void * GLUTAPIENTRY
glutGetProcAddress(const char *procName)
{
   /* Try GLUT functions first */
   int i;
   for (i = 0; glut_functions[i].name; i++) {
      if (strcmp(glut_functions[i].name, procName) == 0)
         return (void *) glut_functions[i].address;
   }

   /* Try core GL functions */
#if defined(_WIN32)
  return (void *) wglGetProcAddress((LPCSTR) procName);

#elif  defined(__OS2PM__)
  return (void *) wglGetProcAddress((char *) procName);
#elif defined(GLX_ARB_get_proc_address)
  return (void *) glXGetProcAddressARB((const GLubyte *) procName);
#else
  return NULL;
#endif
}


/* ENDCENTRY */
