/*
 * Mesa 3-D graphics library
 * Version:  6.5
 * Copyright (C) 1995-2006  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Library for glut using mesa fbdev driver
 *
 * Written by Sean D'Epagnier (c) 2006
 */

#include <stdio.h>

#include <GL/gl.h>
#include <GL/glut.h>

#include "internal.h"

void glutReportErrors(void)
{
   GLenum error;

   while ((error = glGetError()) != GL_NO_ERROR)
      sprintf(exiterror, "GL error: %s", gluErrorString(error));
}

static struct {
   const char *name;
   const GLUTproc address;
} glut_functions[] = {
   { "glutInit", (const GLUTproc) glutInit },
   { "glutInitDisplayMode", (const GLUTproc) glutInitDisplayMode },
   { "glutInitWindowPosition", (const GLUTproc) glutInitWindowPosition },
   { "glutInitWindowSize", (const GLUTproc) glutInitWindowSize },
   { "glutMainLoop", (const GLUTproc) glutMainLoop },
   { "glutCreateWindow", (const GLUTproc) glutCreateWindow },
   { "glutCreateSubWindow", (const GLUTproc) glutCreateSubWindow },
   { "glutDestroyWindow", (const GLUTproc) glutDestroyWindow },
   { "glutPostRedisplay", (const GLUTproc) glutPostRedisplay },
   { "glutSwapBuffers", (const GLUTproc) glutSwapBuffers },
   { "glutGetWindow", (const GLUTproc) glutGetWindow },
   { "glutSetWindow", (const GLUTproc) glutSetWindow },
   { "glutSetWindowTitle", (const GLUTproc) glutSetWindowTitle },
   { "glutSetIconTitle", (const GLUTproc) glutSetIconTitle },
   { "glutPositionWindow", (const GLUTproc) glutPositionWindow },
   { "glutReshapeWindow", (const GLUTproc) glutReshapeWindow },
   { "glutPopWindow", (const GLUTproc) glutPopWindow },
   { "glutPushWindow", (const GLUTproc) glutPushWindow },
   { "glutIconifyWindow", (const GLUTproc) glutIconifyWindow },
   { "glutShowWindow", (const GLUTproc) glutShowWindow },
   { "glutHideWindow", (const GLUTproc) glutHideWindow },
   { "glutFullScreen", (const GLUTproc) glutFullScreen },
   { "glutSetCursor", (const GLUTproc) glutSetCursor },
   { "glutWarpPointer", (const GLUTproc) glutWarpPointer },
   { "glutEstablishOverlay", (const GLUTproc) glutEstablishOverlay },
   { "glutRemoveOverlay", (const GLUTproc) glutRemoveOverlay },
   { "glutUseLayer", (const GLUTproc) glutUseLayer },
   { "glutPostOverlayRedisplay", (const GLUTproc) glutPostOverlayRedisplay },
   { "glutShowOverlay", (const GLUTproc) glutShowOverlay },
   { "glutHideOverlay", (const GLUTproc) glutHideOverlay },
   { "glutCreateMenu", (const GLUTproc) glutCreateMenu },
   { "glutDestroyMenu", (const GLUTproc) glutDestroyMenu },
   { "glutGetMenu", (const GLUTproc) glutGetMenu },
   { "glutSetMenu", (const GLUTproc) glutSetMenu },
   { "glutAddMenuEntry", (const GLUTproc) glutAddMenuEntry },
   { "glutAddSubMenu", (const GLUTproc) glutAddSubMenu },
   { "glutChangeToMenuEntry", (const GLUTproc) glutChangeToMenuEntry },
   { "glutChangeToSubMenu", (const GLUTproc) glutChangeToSubMenu },
   { "glutRemoveMenuItem", (const GLUTproc) glutRemoveMenuItem },
   { "glutAttachMenu", (const GLUTproc) glutAttachMenu },
   { "glutDetachMenu", (const GLUTproc) glutDetachMenu },
   { "glutDisplayFunc", (const GLUTproc) glutDisplayFunc },
   { "glutReshapeFunc", (const GLUTproc) glutReshapeFunc },
   { "glutKeyboardFunc", (const GLUTproc) glutKeyboardFunc },
   { "glutMouseFunc", (const GLUTproc) glutMouseFunc },
   { "glutMotionFunc", (const GLUTproc) glutMotionFunc },
   { "glutPassiveMotionFunc", (const GLUTproc) glutPassiveMotionFunc },
   { "glutEntryFunc", (const GLUTproc) glutEntryFunc },
   { "glutVisibilityFunc", (const GLUTproc) glutVisibilityFunc },
   { "glutIdleFunc", (const GLUTproc) glutIdleFunc },
   { "glutTimerFunc", (const GLUTproc) glutTimerFunc },
   { "glutMenuStateFunc", (const GLUTproc) glutMenuStateFunc },
   { "glutSpecialFunc", (const GLUTproc) glutSpecialFunc },
   { "glutSpaceballRotateFunc", (const GLUTproc) glutSpaceballRotateFunc },
   { "glutButtonBoxFunc", (const GLUTproc) glutButtonBoxFunc },
   { "glutDialsFunc", (const GLUTproc) glutDialsFunc },
   { "glutTabletMotionFunc", (const GLUTproc) glutTabletMotionFunc },
   { "glutTabletButtonFunc", (const GLUTproc) glutTabletButtonFunc },
   { "glutMenuStatusFunc", (const GLUTproc) glutMenuStatusFunc },
   { "glutOverlayDisplayFunc", (const GLUTproc) glutOverlayDisplayFunc },
   { "glutSetColor", (const GLUTproc) glutSetColor },
   { "glutGetColor", (const GLUTproc) glutGetColor },
   { "glutCopyColormap", (const GLUTproc) glutCopyColormap },
   { "glutGet", (const GLUTproc) glutGet },
   { "glutDeviceGet", (const GLUTproc) glutDeviceGet },
   { "glutExtensionSupported", (const GLUTproc) glutExtensionSupported },
   { "glutGetModifiers", (const GLUTproc) glutGetModifiers },
   { "glutLayerGet", (const GLUTproc) glutLayerGet },
   { "glutGetProcAddress", (const GLUTproc) glutGetProcAddress },
   { "glutBitmapCharacter", (const GLUTproc) glutBitmapCharacter },
   { "glutBitmapWidth", (const GLUTproc) glutBitmapWidth },
   { "glutStrokeCharacter", (const GLUTproc) glutStrokeCharacter },
   { "glutStrokeWidth", (const GLUTproc) glutStrokeWidth },
   { "glutBitmapLength", (const GLUTproc) glutBitmapLength },
   { "glutStrokeLength", (const GLUTproc) glutStrokeLength },
   { "glutWireSphere", (const GLUTproc) glutWireSphere },
   { "glutSolidSphere", (const GLUTproc) glutSolidSphere },
   { "glutWireCone", (const GLUTproc) glutWireCone },
   { "glutSolidCone", (const GLUTproc) glutSolidCone },
   { "glutWireCube", (const GLUTproc) glutWireCube },
   { "glutSolidCube", (const GLUTproc) glutSolidCube },
   { "glutWireTorus", (const GLUTproc) glutWireTorus },
   { "glutSolidTorus", (const GLUTproc) glutSolidTorus },
   { "glutWireDodecahedron", (const GLUTproc) glutWireDodecahedron },
   { "glutSolidDodecahedron", (const GLUTproc) glutSolidDodecahedron },
   { "glutWireTeapot", (const GLUTproc) glutWireTeapot },
   { "glutSolidTeapot", (const GLUTproc) glutSolidTeapot },
   { "glutWireOctahedron", (const GLUTproc) glutWireOctahedron },
   { "glutSolidOctahedron", (const GLUTproc) glutSolidOctahedron },
   { "glutWireTetrahedron", (const GLUTproc) glutWireTetrahedron },
   { "glutSolidTetrahedron", (const GLUTproc) glutSolidTetrahedron },
   { "glutWireIcosahedron", (const GLUTproc) glutWireIcosahedron },
   { "glutSolidIcosahedron", (const GLUTproc) glutSolidIcosahedron },
   { "glutReportErrors", (const GLUTproc) glutReportErrors },
   { NULL, NULL }
};   

GLUTproc glutGetProcAddress(const char *procName)
{
   /* Try GLUT functions first */
   int i;
   for (i = 0; glut_functions[i].name; i++) {
      if (strcmp(glut_functions[i].name, procName) == 0)
         return glut_functions[i].address;
   }

   /* Try core GL functions */
   return (GLUTproc) glFBDevGetProcAddress(procName);
}
