#ifndef _FRAMEWORK_H_
#define _FRAMEWORK_H_

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glext.h>

#ifdef WIN32
#define GETPROCADDRESS(x) wglGetProcAddress (x)
#else
#define GETPROCADDRESS(x) glutGetProcAddress (x)
#endif

#define GETPROCADDR(x,T) do { x = (T) (GETPROCADDRESS(#x)); assert (x != NULL); } while (0)

/*
 * GL_ARB_multitexture
 */
#ifndef GL_ARB_multitexture
extern PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;
extern PFNGLMULTITEXCOORD4FVARBPROC glMultiTexCoord4fvARB;
#endif

/*
 * GL_ARB_shader_objects
 */
extern PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
extern PFNGLGETHANDLEARBPROC glGetHandleARB;
extern PFNGLDETACHOBJECTARBPROC glDetachObjectARB;
extern PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
extern PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
extern PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
extern PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
extern PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
extern PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
extern PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
extern PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
extern PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
extern PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
/*static PFNGLUNIFORM4FVARBPROC glUniform4fvARB = NULL;*/

/*
 * GL_ARB_vertex_shader
 */
extern PFNGLVERTEXATTRIB4FVARBPROC glVertexAttrib4fvARB;
extern PFNGLVERTEXATTRIBPOINTERARBPROC glVertexAttribPointerARB;
extern PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB;
extern PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB;
extern PFNGLBINDATTRIBLOCATIONARBPROC glBindAttribLocationARB;
extern PFNGLGETATTRIBLOCATIONARBPROC glGetAttribLocationARB;

/*
 * GL_EXT_fog_coord
 */
extern PFNGLFOGCOORDFVEXTPROC glFogCoordfvEXT;
extern PFNGLFOGCOORDPOINTEREXTPROC glFogCoordPointerEXT;

/*
 * GL_EXT_secondary_color
 */
extern PFNGLSECONDARYCOLOR3FVEXTPROC glSecondaryColor3fvEXT;
extern PFNGLSECONDARYCOLORPOINTEREXTPROC glSecondaryColorPointerEXT;

extern void InitFramework (int *argc, char *argv[]);

extern void InitScene (void);
extern void RenderScene (void);

extern GLboolean CheckObjectStatus (GLhandleARB);

#endif

