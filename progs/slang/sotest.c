/*
 * GL_ARB_shader_objects & GL_ARB_vertex_shader interface test application.
 * Neither compiler nor executor is being tested here, although some simple shader
 * compilation tests are performed.
 *
 * Perfectly valid behaviour produces output that does not have a line
 * beginning with three stars (***).
 *
 * Author: Michal Krol
 */

#include "framework.h"

enum TEST_TYPE
{
   TT_GETERROR_NOERROR,
   TT_GETERROR_INVALIDVALUE,
   TT_GETERROR_INVALIDOPERATION,
   TT_PARAM1_ZERO,
   TT_PARAM1_NONZERO
};

static enum TEST_TYPE current_test;

static void begintest (enum TEST_TYPE type, const char *name)
{
   current_test = type;
   printf ("\n    BEGIN TEST: %s\n", name);
   while (glGetError () != GL_NO_ERROR)
      ;
}

static void endtest1 (GLuint param1)
{
   const char *msg = NULL;

   switch (current_test)
   {
   case TT_GETERROR_NOERROR:
      if (glGetError () != GL_NO_ERROR)
         msg = "glGetError () does not return GL_NO_ERROR";
      break;
   case TT_GETERROR_INVALIDVALUE:
      if (glGetError () != GL_INVALID_VALUE)
         msg = "glGetError () does not return GL_INVALID_VALUE";
      break;
    case TT_GETERROR_INVALIDOPERATION:
      if (glGetError () != GL_INVALID_OPERATION)
         msg = "glGetError () does not return GL_INVALID_OPERATION";
      break;
   case TT_PARAM1_ZERO:
      if (param1)
         msg = "The parameter is not zero";
      break;
   case TT_PARAM1_NONZERO:
      if (!param1)
         msg = "The parameter is not non-zero";
      break;
   default:
      assert (0);
   }

   if (msg == NULL)
      printf ("    OK\n");
   else
      printf ("*** %s\n", msg);

   while (glGetError () != GL_NO_ERROR)
      ;
}

static void endtest ()
{
   endtest1 (0);
}

static GLhandleARB vert = 0;
static GLhandleARB frag = 0;
static GLhandleARB prog = 0;

static GLhandleARB find_invalid_handle ()
{
   GLhandleARB handle;

   for (handle = 1; handle < 16; handle++)
      if (handle != vert && handle != frag && handle != prog)
         return handle;
   assert (0);
   return 0;
}

static const char *invsynvertsrc =
   "void main () {\n"
   "   gl_Position = gl_ModelViewMatrix ! gl_Vertex;\n"   /* unexpected token */
   "}\n"
;

static const char *invsemvertsrc =
   "void main () {\n"
   "   gl_Position = gl_ModelviewMatrix * gl_Vertex;\n"  /* undeclared identifier */
   "}\n"
;

static const char *uniforms =
   "uniform vec4 CommonUniform;\n"
;

static const char *validvertsrc =
   "uniform vec4 VertexUniform;\n"
   "attribute vec4 FirstAttrib;\n"
   "attribute vec4 SecondAttrib;\n"
   "void main () {\n"
   "   gl_Position = gl_ModelViewMatrix * gl_Vertex + CommonUniform + VertexUniform\n"
   "      + FirstAttrib + SecondAttrib;\n"
   "}\n"
;

static const char *invsynfragsrc =
   "void main () {\n"
   "   gl_FragColor = gl_Color\n"   /* missing ; */
   "}\n"
;

static const char *invsemfragsrc =
   "void main () {\n"
   "   gl_FragColor = gl_FrontColor;\n"   /* gl_FrontColor only in vertex shader */
   "}\n"
;

static const char *validfragsrc =
   "uniform vec4 FragmentUniform;\n"
   "void main () {\n"
   "   gl_FragColor = gl_Color + CommonUniform + FragmentUniform;\n"
   "}\n"
;

void InitScene (void)
{
   GLint params[1];
   const char *tab[2];

   /*
    * GL should silently ignore calls that delete object 0.
    */
   begintest (TT_GETERROR_NOERROR, "glDeleteObject(0)");
   glDeleteObjectARB (0);
   endtest ();

   /*
    * GL generates an error on invalid object handle.
    */
   begintest (TT_GETERROR_INVALIDVALUE, "Pass invalid non-zero object handle");
   glDeleteObjectARB (find_invalid_handle ());
   endtest ();
   glUseProgramObjectARB (find_invalid_handle ());
   endtest ();

   /*
    * Create object. GL should return unique non-zero values.
    */
   begintest (TT_PARAM1_NONZERO, "Create object");
   vert = glCreateShaderObjectARB (GL_VERTEX_SHADER_ARB);
   endtest1 (vert);
   frag = glCreateShaderObjectARB (GL_FRAGMENT_SHADER_ARB);
   endtest1 (frag);
   prog = glCreateProgramObjectARB ();
   endtest1 (prog);
   endtest1 (vert != frag && frag != prog && prog != vert);

   /*
    * Link empty program.
    */
   begintest (TT_PARAM1_NONZERO, "Link empty program");
   glLinkProgramARB (prog);
   endtest1 (CheckObjectStatus (prog));

   /*
    * Use empty program object. Empty program objects are valid.
    */
   begintest (TT_GETERROR_NOERROR, "Use empty program object");
   glUseProgramObjectARB (prog);
   endtest ();

   /*
    * Attach invalid object handles. Program object 0 should not be accepted.
    */
   begintest (TT_GETERROR_INVALIDVALUE, "Attach invalid object handle");
   glAttachObjectARB (0, find_invalid_handle ());
   endtest ();
   glAttachObjectARB (0, frag);
   endtest ();
   glAttachObjectARB (find_invalid_handle (), find_invalid_handle ());
   endtest ();
   glAttachObjectARB (find_invalid_handle (), frag);
   endtest ();
   glAttachObjectARB (prog, find_invalid_handle ());
   endtest ();

   /*
    * Attach valid object handles with wrong semantics.
    */
   begintest (TT_GETERROR_INVALIDOPERATION, "Attach object badly");
   glAttachObjectARB (vert, frag);
   endtest ();
   glAttachObjectARB (vert, prog);
   endtest ();
   glAttachObjectARB (prog, prog);
   endtest ();

   /*
    * Detach non-attached object.
    */
   begintest (TT_GETERROR_INVALIDOPERATION, "Detach non-attached object");
   glDetachObjectARB (prog, vert);
   endtest ();
   glDetachObjectARB (prog, frag);
   endtest ();

   /*
    * Attach shader.
    */
   begintest (TT_GETERROR_NOERROR, "Attach shader to program object");
   glAttachObjectARB (prog, vert);
   endtest ();
   glAttachObjectARB (prog, frag);
   endtest ();

   /*
    * Attach object twice.
    */
   begintest (TT_GETERROR_INVALIDOPERATION, "Attach object twice");
   glAttachObjectARB (prog, vert);
   endtest ();
   glAttachObjectARB (prog, frag);
   endtest ();

   /*
    * Detach attached object.
    */
   begintest (TT_GETERROR_NOERROR, "Detach attached object");
   glDetachObjectARB (prog, vert);
   endtest ();
   glDetachObjectARB (prog, frag);
   endtest ();

   /*
    * Attach shader again.
    */
   begintest (TT_GETERROR_NOERROR, "Attach shader again");
   glAttachObjectARB (prog, vert);
   endtest ();
   glAttachObjectARB (prog, frag);
   endtest ();

   /*
    * Delete attached object.
    */
   begintest (TT_GETERROR_NOERROR, "Delete attached object");
   glDeleteObjectARB (vert);
   endtest ();
   glDeleteObjectARB (frag);
   endtest ();

   /*
    * Query delete status. It should return TRUE. Object handles are still valid
    * as they are referenced by program object container.
    */
   begintest (TT_PARAM1_NONZERO, "Query delete status");
   glGetObjectParameterivARB (vert, GL_OBJECT_DELETE_STATUS_ARB, params);
   endtest1 (params[0]);
   glGetObjectParameterivARB (frag, GL_OBJECT_DELETE_STATUS_ARB, params);
   endtest1 (params[0]);

   /*
    * Delete already deleted attached object. The behaviour is undefined, but we
    * check for no errors. The object still exists, so the handle value is okay.
    * In other words, these calls should be silently ignored by GL.
    */
   begintest (TT_GETERROR_NOERROR, "Delete already deleted attached object");
   glDeleteObjectARB (vert);
   endtest ();
   glDeleteObjectARB (frag);
   endtest ();

   /*
    * Compile shader source with syntax error.
    */
   begintest (TT_PARAM1_ZERO, "Compile shader source with syntax error");
   glShaderSourceARB (vert, 1, &invsynvertsrc, NULL);
   glCompileShaderARB (vert);
   endtest1 (CheckObjectStatus (vert));
   glShaderSourceARB (frag, 1, &invsynfragsrc, NULL);
   glCompileShaderARB (frag);
   endtest1 (CheckObjectStatus (frag));

   /*
    * Compile shader source with semantic error.
    */
   begintest (TT_PARAM1_ZERO, "Compile shader source with semantic error");
   glShaderSourceARB (vert, 1, &invsemvertsrc, NULL);
   glCompileShaderARB (vert);
   endtest1 (CheckObjectStatus (vert));
   glShaderSourceARB (frag, 1, &invsemfragsrc, NULL);
   glCompileShaderARB (frag);
   endtest1 (CheckObjectStatus (frag));

   /*
    * Link ill-formed vertex-fragment program.
    */
   begintest (TT_PARAM1_ZERO, "Link ill-formed vertex-fragment program");
   glLinkProgramARB (prog);
   endtest1 (CheckObjectStatus (prog));

   /*
    * Use badly linked program object.
    */
   begintest (TT_GETERROR_INVALIDOPERATION, "Use badly linked program object");
   glUseProgramObjectARB (prog);
   endtest ();

   /*
    * Compile well-formed shader source. Check if multi-string sources can be handled.
    */
   begintest (TT_PARAM1_NONZERO, "Compile well-formed shader source");
   tab[0] = uniforms;
   tab[1] = validvertsrc;
   glShaderSourceARB (vert, 2, tab, NULL);
   glCompileShaderARB (vert);
   endtest1 (CheckObjectStatus (vert));
   tab[0] = uniforms;
   tab[1] = validfragsrc;
   glShaderSourceARB (frag, 2, tab, NULL);
   glCompileShaderARB (frag);
   endtest1 (CheckObjectStatus (frag));

   /*
    * Link vertex-fragment program.
    */
   begintest (TT_PARAM1_NONZERO, "Link vertex-fragment program");
   glLinkProgramARB (prog);
   endtest1 (CheckObjectStatus (prog));

   /*
    * Use valid linked program object.
    */
   begintest (TT_GETERROR_NOERROR, "Use linked program object");
   glUseProgramObjectARB (prog);
   endtest ();

   /*
    * Get current program.
    */
   begintest (TT_PARAM1_NONZERO, "Get current program");
   endtest1 (glGetHandleARB (GL_PROGRAM_OBJECT_ARB) == prog);

   /*
    * Use 0 program object.
    */
   begintest (TT_GETERROR_NOERROR, "Use 0 program object");
   glUseProgramObjectARB (0);
   endtest ();

   /*
    * Query uniform location. Uniforms with gl_ prefix cannot be queried.
    */
   begintest (TT_PARAM1_NONZERO, "Query uniform location");
   endtest1 (glGetUniformLocationARB (prog, "gl_ModelViewMatrix") == -1);
   endtest1 (glGetUniformLocationARB (prog, "UniformThatDoesNotExist") == -1);
   endtest1 (glGetUniformLocationARB (prog, "") == -1);
   endtest1 (glGetUniformLocationARB (prog, "CommonUniform") != -1);
   endtest1 (glGetUniformLocationARB (prog, "VertexUniform") != -1);
   endtest1 (glGetUniformLocationARB (prog, "FragmentUniform") != -1);

   /*
    * Query attrib location. Attribs with gl_ prefix cannot be queried.
    * When gl_Vertex is used, none of the generic attribs can have index 0.
    */
   begintest (TT_PARAM1_NONZERO, "Query attrib location");
   endtest1 (glGetAttribLocationARB (prog, "gl_Vertex") == -1);
   endtest1 (glGetAttribLocationARB (prog, "AttribThatDoesNotExist") == -1);
   endtest1 (glGetAttribLocationARB (prog, "") == -1);
   endtest1 (glGetAttribLocationARB (prog, "FirstAttrib") > 0);
   endtest1 (glGetAttribLocationARB (prog, "SecondAttrib") > 0);

   /*
    * Bind attrib locations, link and check if locations are correct.
    */
   begintest (TT_PARAM1_NONZERO, "Bind attrib location #1");
   glBindAttribLocationARB (prog, 1, "FirstAttrib");
   glBindAttribLocationARB (prog, 2, "SecondAttrib");
   glLinkProgramARB (prog);
   endtest1 (CheckObjectStatus (prog));
   endtest1 (glGetAttribLocationARB (prog, "FirstAttrib") == 1);
   endtest1 (glGetAttribLocationARB (prog, "SecondAttrib") == 2);

   /*
    * Bind attrib locations in different order. Link and check if locations are correct.
    */
   begintest (TT_PARAM1_NONZERO, "Bind attrib location #2");
   glBindAttribLocationARB (prog, 1, "SecondAttrib");
   glBindAttribLocationARB (prog, 2, "FirstAttrib");
   glLinkProgramARB (prog);
   endtest1 (CheckObjectStatus (prog));
   endtest1 (glGetAttribLocationARB (prog, "SecondAttrib") == 1);
   endtest1 (glGetAttribLocationARB (prog, "FirstAttrib") == 2);

   /*
    * Detach deleted object.
    */
   begintest (TT_GETERROR_NOERROR, "Detach deleted object");
   glDetachObjectARB (prog, vert);
   endtest ();
   glDetachObjectARB (prog, frag);
   endtest ();

   /*
    * Delete deleted detached object.
    */
   begintest (TT_GETERROR_INVALIDVALUE, "Delete deleted detached object");
   glDeleteObjectARB (vert);
   endtest ();
   glDeleteObjectARB (frag);
   endtest ();

   exit (0);
}

void RenderScene (void)
{
   /* never reached */
   assert (0);
}

int main (int argc, char *argv[])
{
   InitFramework (&argc, argv);
   return 0;
}

