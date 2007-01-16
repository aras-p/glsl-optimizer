/*
 * Copyright (C) 2006 Claudio Ciccani <klan@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"


static const struct {
     const char     *name;
     const GLUTproc  address;
} glut_functions[] = {
   { "glutInit", (const GLUTproc) glutInit },
   { "glutInitDisplayMode", (const GLUTproc) glutInitDisplayMode },
   { "glutInitDisplayString", (const GLUTproc) glutInitDisplayString },
   { "glutInitWindowPosition", (const GLUTproc) glutInitWindowPosition },
   { "glutInitWindowSize", (const GLUTproc) glutInitWindowSize },
   { "glutMainLoop", (const GLUTproc) glutMainLoop },
   { "glutCreateWindow", (const GLUTproc) glutCreateWindow },
   { "glutCreateSubWindow", (const GLUTproc) glutCreateSubWindow },
   { "glutDestroyWindow", (const GLUTproc) glutDestroyWindow },
   { "glutPostRedisplay", (const GLUTproc) glutPostRedisplay },
   { "glutPostWindowRedisplay", (const GLUTproc) glutPostWindowRedisplay },
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
   { "glutPostWindowOverlayRedisplay", (const GLUTproc) glutPostWindowOverlayRedisplay },
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
   { "glutSpaceballMotionFunc", (const GLUTproc) glutSpaceballMotionFunc },
   { "glutSpaceballRotateFunc", (const GLUTproc) glutSpaceballRotateFunc },
   { "glutSpaceballButtonFunc", (const GLUTproc) glutSpaceballButtonFunc },
   { "glutButtonBoxFunc", (const GLUTproc) glutButtonBoxFunc },
   { "glutDialsFunc", (const GLUTproc) glutDialsFunc },
   { "glutTabletMotionFunc", (const GLUTproc) glutTabletMotionFunc },
   { "glutTabletButtonFunc", (const GLUTproc) glutTabletButtonFunc },
   { "glutMenuStatusFunc", (const GLUTproc) glutMenuStatusFunc },
   { "glutOverlayDisplayFunc", (const GLUTproc) glutOverlayDisplayFunc },
   { "glutWindowStatusFunc", (const GLUTproc) glutWindowStatusFunc },
   { "glutKeyboardUpFunc", (const GLUTproc) glutKeyboardUpFunc },
   { "glutSpecialUpFunc", (const GLUTproc) glutSpecialUpFunc },
   { "glutJoystickFunc", (const GLUTproc) glutJoystickFunc },
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
//   { "glutVideoResizeGet", (const GLUTproc) glutVideoResizeGet },
//   { "glutSetupVideoResizing", (const GLUTproc) glutSetupVideoResizing },
//   { "glutStopVideoResizing", (const GLUTproc) glutStopVideoResizing },
//   { "glutVideoResize", (const GLUTproc) glutVideoResize },
//   { "glutVideoPan", (const GLUTproc) glutVideoPan },
   { "glutReportErrors", (const GLUTproc) glutReportErrors },
   { "glutIgnoreKeyRepeat", (const GLUTproc) glutIgnoreKeyRepeat },
   { "glutSetKeyRepeat", (const GLUTproc) glutSetKeyRepeat },
   { "glutForceJoystickFunc", (const GLUTproc) glutForceJoystickFunc },
   { "glutGameModeString", (const GLUTproc) glutGameModeString },
   { "glutEnterGameMode", (const GLUTproc) glutEnterGameMode },
   { "glutLeaveGameMode", (const GLUTproc) glutLeaveGameMode },
   { "glutGameModeGet", (const GLUTproc) glutGameModeGet },
};


GLUTproc GLUTAPIENTRY 
glutGetProcAddress( const char *name )
{
     int i;
     
     for (i = 0; i < sizeof(glut_functions)/sizeof(glut_functions[0]); i++) {
          if (!strcmp( name, glut_functions[i].name ))
               return glut_functions[i].address;
     }

#if DIRECTFBGL_INTERFACE_VERSION >= 1
     if (g_current) {
          void *address = NULL;
          g_current->gl->GetProcAddress( g_current->gl, name, &address );
          return address;
     }
#endif
     return NULL;
}


int GLUTAPIENTRY 
glutExtensionSupported( const char *name )
{
     GLubyte *extensions;
     int      length;
     
     if (!name || !*name)
          return 0;
     
     length = strlen( name );
     extensions = (GLubyte*) glGetString( GL_EXTENSIONS );
     
     while (extensions && *extensions) {
          GLubyte *next;
          
          next = strchr( extensions, ' ' );
          if (next) {
               if (length == (int)(next - extensions)) {
                    if (!strncmp( extensions, name, length ))
                         return 1;
               }
               extensions = next+1;
          }
          else {
               if (!strcmp( extensions, name ))
                    return 1;
               break; 
          }
     } 

     return 0;
}

