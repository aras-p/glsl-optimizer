#include "framework.h"

/*
 * GL_ARB_multitexture
 */
#ifndef GL_ARB_multitexture
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;
PFNGLMULTITEXCOORD4FVARBPROC glMultiTexCoord4fvARB;
#endif

/*
 * GL_ARB_shader_objects
 */
PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
PFNGLGETHANDLEARBPROC glGetHandleARB;
PFNGLDETACHOBJECTARBPROC glDetachObjectARB;
PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;

/*
 * GL_ARB_vertex_shader
 */
PFNGLVERTEXATTRIB4FVARBPROC glVertexAttrib4fvARB;
PFNGLVERTEXATTRIBPOINTERARBPROC glVertexAttribPointerARB;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB;
PFNGLBINDATTRIBLOCATIONARBPROC glBindAttribLocationARB;
PFNGLGETATTRIBLOCATIONARBPROC glGetAttribLocationARB;

/*
 * GL_EXT_fog_coord
 */
PFNGLFOGCOORDFVEXTPROC glFogCoordfvEXT;
PFNGLFOGCOORDPOINTEREXTPROC glFogCoordPointerEXT;

/*
 * GL_EXT_secondary_color
 */
PFNGLSECONDARYCOLOR3FVEXTPROC glSecondaryColor3fvEXT;
PFNGLSECONDARYCOLORPOINTEREXTPROC glSecondaryColorPointerEXT;

static void Display (void)
{
   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   RenderScene ();
   glutSwapBuffers ();
}

static void Idle (void)
{
   glutPostRedisplay ();
}

void InitFramework (int *argc, char *argv[])
{
   glutInit (argc, argv);
   glutInitWindowPosition (0, 0);
   glutInitWindowSize (200, 200);
   glutInitDisplayMode (GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   glutCreateWindow (argv[0]);

#ifndef GL_ARB_multitexture
   GETPROCADDR(glClientActiveTextureARB, PFNGLCLIENTACTIVETEXTUREARBPROC);
   GETPROCADDR(glMultiTexCoord4fvARB, PFNGLMULTITEXCOORD4FVARBPROC);
#endif

   GETPROCADDR(glDeleteObjectARB, PFNGLDELETEOBJECTARBPROC);
   GETPROCADDR(glGetHandleARB, PFNGLGETHANDLEARBPROC);
   GETPROCADDR(glDetachObjectARB, PFNGLDETACHOBJECTARBPROC);
   GETPROCADDR(glCreateShaderObjectARB, PFNGLCREATESHADEROBJECTARBPROC);
   GETPROCADDR(glShaderSourceARB, PFNGLSHADERSOURCEARBPROC);
   GETPROCADDR(glCompileShaderARB, PFNGLCOMPILESHADERARBPROC);
   GETPROCADDR(glCreateProgramObjectARB, PFNGLCREATEPROGRAMOBJECTARBPROC);
   GETPROCADDR(glAttachObjectARB, PFNGLATTACHOBJECTARBPROC);
   GETPROCADDR(glLinkProgramARB, PFNGLLINKPROGRAMARBPROC);
   GETPROCADDR(glUseProgramObjectARB, PFNGLUSEPROGRAMOBJECTARBPROC);
   GETPROCADDR(glGetObjectParameterivARB, PFNGLGETOBJECTPARAMETERIVARBPROC);
   GETPROCADDR(glGetInfoLogARB, PFNGLGETINFOLOGARBPROC);
   GETPROCADDR(glGetUniformLocationARB, PFNGLGETUNIFORMLOCATIONARBPROC);

   GETPROCADDR(glVertexAttrib4fvARB, PFNGLVERTEXATTRIB4FVARBPROC);
   GETPROCADDR(glVertexAttribPointerARB, PFNGLVERTEXATTRIBPOINTERARBPROC);
   GETPROCADDR(glEnableVertexAttribArrayARB, PFNGLENABLEVERTEXATTRIBARRAYARBPROC);
   GETPROCADDR(glDisableVertexAttribArrayARB, PFNGLDISABLEVERTEXATTRIBARRAYARBPROC);
   GETPROCADDR(glBindAttribLocationARB, PFNGLBINDATTRIBLOCATIONARBPROC);
   GETPROCADDR(glGetAttribLocationARB, PFNGLGETATTRIBLOCATIONARBPROC);

   GETPROCADDR(glFogCoordfvEXT, PFNGLFOGCOORDFVEXTPROC);
   GETPROCADDR(glFogCoordPointerEXT, PFNGLFOGCOORDPOINTEREXTPROC);

   GETPROCADDR(glSecondaryColor3fvEXT, PFNGLSECONDARYCOLOR3FVEXTPROC);
   GETPROCADDR(glSecondaryColorPointerEXT, PFNGLSECONDARYCOLORPOINTEREXTPROC);

   printf ("VENDOR: %s\n", glGetString (GL_VENDOR));
   printf ("RENDERER: %s\n", glGetString (GL_RENDERER));

   InitScene ();

   glutDisplayFunc (Display);
   glutIdleFunc (Idle);
   glutMainLoop ();
}

GLboolean CheckObjectStatus (GLhandleARB handle)
{
   GLint type, status, length;
   GLcharARB *infolog;

   glGetObjectParameterivARB (handle, GL_OBJECT_TYPE_ARB, &type);
   if (type == GL_SHADER_OBJECT_ARB)
      glGetObjectParameterivARB (handle, GL_OBJECT_COMPILE_STATUS_ARB, &status);
   else if (type == GL_PROGRAM_OBJECT_ARB)
      glGetObjectParameterivARB (handle, GL_OBJECT_LINK_STATUS_ARB, &status);
   else {
      assert (0);
      return GL_FALSE;
   }

   if (status)
      return GL_TRUE;

   printf ("\n%s FAILED. INFO LOG FOLLOWS:\n",
           type == GL_SHADER_OBJECT_ARB ? "SHADER COMPILE" : "PROGRAM LINK");

   glGetObjectParameterivARB (handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
   infolog = (GLcharARB *) (malloc (length));
   if (infolog != NULL) {
      glGetInfoLogARB (handle, length, NULL, infolog);
      printf ("%s", infolog);
      free (infolog);
   }

   printf ("\n");

   return GL_FALSE;
}

