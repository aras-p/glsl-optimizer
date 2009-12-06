/*
 * GL_ARB_vertex_shader test application. Feeds a vertex shader with attributes that
 * that have magic values and check if the values received by the shader are the same.
 *
 * Requires specific support on the GL implementation side. A special function printMESA()
 * must be supported in the language that prints variable's current value of generic type
 * to the appropriate shader's info log, and optionally to the screen.
 *
 * Perfectly valid behaviour produces output that does not have a line
 * beginning with three stars (***).
 *
 * Author: Michal Krol
 */

#include "framework.h"

#define EPSILON 0.0001f

static GLhandleARB vert = 0;
static GLhandleARB prog = 0;

enum SUBMIT_MODE
{
   SM_IM,
   SM_VA,
   SM_IM_DL,
   SM_VA_DL,
   SM_MAX
};

static enum SUBMIT_MODE submit_method = SM_IM;

#define C 0
#define S 1
#define N 2
#define V 3
#define T 4
#define F 5
#define A 6

struct ATTRIB_DATA
{
   const char *name;
   GLuint dispatch;
   GLint index;
   GLint bind;
   GLuint size;
   GLfloat data[4];
};

static struct ATTRIB_DATA attribs[] = {
   { "gl_Color",          C, -1, -1, 4, { 4.2f, 0.56f, -2.1f, 0.29f } },
   { "gl_SecondaryColor", S, -1, -1, 4, { 0.38f, 2.0f, 0.99f, 1.0f } },
   { "gl_Normal",         N, -1, -1, 3, { 54.0f, 77.0f, 1.15f, 0.0f } },
   { "gl_MultiTexCoord0", T, 0,  -1, 4, { 11.1f, 11.2f, 11.3f, 11.4f } },
   { "gl_MultiTexCoord1", T, 1,  -1, 4, { 22.1f, 22.2f, 22.3f, 22.4f } },
   { "gl_MultiTexCoord2", T, 2,  -1, 4, { 33.1f, 33.2f, 33.3f, 33.4f } },
   { "gl_MultiTexCoord3", T, 3,  -1, 4, { 44.1f, 44.2f, 44.3f, 44.4f } },
   { "gl_MultiTexCoord4", T, 4,  -1, 4, { 55.1f, 55.2f, 55.3f, 55.4f } },
   { "gl_MultiTexCoord5", T, 5,  -1, 4, { 66.1f, 66.2f, 66.3f, 66.4f } },
   { "gl_MultiTexCoord6", T, 6,  -1, 4, { 77.1f, 77.2f, 77.3f, 77.4f } },
   { "gl_MultiTexCoord7", T, 7,  -1, 4, { 88.1f, 88.2f, 88.3f, 88.4f } },
   { "gl_FogCoord",       F, -1, -1, 1, { 0.63f, 0.0f, 0.0f, 0.0f } },
   { "Attribute1",        A, 1,  1,  4, { 1.11f, 1.22f, 1.33f, 1.44f } },
   { "Attribute2",        A, 2,  2,  4, { 2.11f, 2.22f, 2.33f, 2.44f } },
   { "Attribute3",        A, 3,  3,  4, { 3.11f, 3.22f, 3.33f, 3.44f } },
   { "Attribute4",        A, 4,  4,  1, { 4.11f, 0.0f, 0.0f, 0.0f } },
   { "Attribute5",        A, 5,  5,  2, { 5.11f, 5.22f, 0.0f, 0.0f } },
   { "Attribute6",        A, 6,  6,  3, { 6.11f, 6.22f, 6.33f, 0.0f } },
   { "Attribute7",        A, 7,  7,  2, { 7.11f, 7.22f, 0.0f, 0.0f } },
   { "Attribute7",        A, 8,  -1, 2, { 8.11f, 8.22f, 0.0f, 0.0f } },
   { "Attribute9",        A, 9,  9,  3, { 9.11f, 9.22f, 9.33f, 0.0f } },
   { "Attribute9",        A, 10, -1, 3, { 10.11f, 10.22f, 10.33f, 0.0f } },
   { "Attribute9",        A, 11, -1, 3, { 11.11f, 11.22f, 11.33f, 0.0f } },
   { "Attribute12",       A, 12, 12, 4, { 12.11f, 12.22f, 12.33f, 12.44f } },
   { "Attribute12",       A, 13, -1, 4, { 13.11f, 13.22f, 13.33f, 13.44f } },
   { "Attribute12",       A, 14, -1, 4, { 14.11f, 14.22f, 14.33f, 14.44f } },
   { "Attribute12",       A, 15, -1, 4, { 15.11f, 15.22f, 15.33f, 15.44f } },
   { "gl_Vertex",         V, 16, -1, 4, { 0.25f, -0.14f, 0.01f, 1.0f } }
};

static void im_render ()
{
   GLint i;

   glBegin (GL_POINTS);
   for (i = 0; i < sizeof (attribs) / sizeof (*attribs); i++) {
      struct ATTRIB_DATA *att = &attribs[i];
      switch (att->dispatch)
      {
      case C:
         glColor4fv (att->data);
         break;
      case S:
         glSecondaryColor3fvEXT (att->data);
         break;
      case N:
         glNormal3fv (att->data);
         break;
      case V:
         glVertex4fv (att->data);
         break;
      case T:
         assert (att->index >= 0 && att->index < 8);
         glMultiTexCoord4fvARB (GL_TEXTURE0_ARB + att->index, att->data);
         break;
      case F:
         glFogCoordfvEXT (att->data);
         break;
      case A:
         assert (att->index > 0 && att->index < 16);
         glVertexAttrib4fvARB (att->index, att->data);
         break;
      default:
         assert (0);
      }
   }
   glEnd ();
}

static void va_render ()
{
   GLint i;

   for (i = 0; i < sizeof (attribs) / sizeof (*attribs); i++) {
      struct ATTRIB_DATA *att = &attribs[i];
      switch (att->dispatch)
      {
      case C:
         glColorPointer (4, GL_FLOAT, 0, att->data);
         glEnableClientState (GL_COLOR_ARRAY);
         break;
      case S:
         glSecondaryColorPointerEXT (4, GL_FLOAT, 0, att->data);
         glEnableClientState (GL_SECONDARY_COLOR_ARRAY_EXT);
         break;
      case N:
         glNormalPointer (GL_FLOAT, 0, att->data);
         glEnableClientState (GL_NORMAL_ARRAY);
         break;
      case V:
         glVertexPointer (4, GL_FLOAT, 0, att->data);
         glEnableClientState (GL_VERTEX_ARRAY);
         break;
      case T:
         assert (att->index >= 0 && att->index < 8);
         glClientActiveTextureARB (GL_TEXTURE0_ARB + att->index);
         glTexCoordPointer (4, GL_FLOAT, 0, att->data);
         glEnableClientState (GL_TEXTURE_COORD_ARRAY);
         break;
      case F:
         glFogCoordPointerEXT (GL_FLOAT, 0, att->data);
         glEnableClientState (GL_FOG_COORDINATE_ARRAY_EXT);
         break;
      case A:
         assert (att->index > 0 && att->index < 16);
         glVertexAttribPointerARB (att->index, 4, GL_FLOAT, GL_FALSE, 0, att->data);
         glEnableVertexAttribArrayARB (att->index);
         break;
      default:
         assert (0);
      }
   }

   glDrawArrays (GL_POINTS, 0, 1);

   for (i = 0; i < sizeof (attribs) / sizeof (*attribs); i++) {
      struct ATTRIB_DATA *att = &attribs[i];
      switch (att->dispatch)
      {
      case C:
         glDisableClientState (GL_COLOR_ARRAY);
         break;
      case S:
         glDisableClientState (GL_SECONDARY_COLOR_ARRAY_EXT);
         break;
      case N:
         glDisableClientState (GL_NORMAL_ARRAY);
         break;
      case V:
         glDisableClientState (GL_VERTEX_ARRAY);
         break;
      case T:
         glClientActiveTextureARB (GL_TEXTURE0_ARB + att->index);
         glDisableClientState (GL_TEXTURE_COORD_ARRAY);
         break;
      case F:
         glDisableClientState (GL_FOG_COORDINATE_ARRAY_EXT);
         break;
      case A:
         glDisableVertexAttribArrayARB (att->index);
         break;
      default:
         assert (0);
      }
   }
}

static void dl_start ()
{
   glNewList (GL_COMPILE, 1);
}

static void dl_end ()
{
   glEndList ();
   glCallList (1);
}

static void load_test_file (const char *filename)
{
   FILE *f;
   GLint size;
   char *code;
   GLint i;

   f = fopen (filename, "r");
   if (f == NULL)
      return;

   fseek (f, 0, SEEK_END);
   size = ftell (f);

   if (size == -1) {
      fclose (f);
      return;
   }

   fseek (f, 0, SEEK_SET);

   code = (char *) (malloc (size));
   if (code == NULL) {
      fclose (f);
      return;
   }
   size = fread (code, 1, size, f);
   fclose (f);

   glShaderSourceARB (vert, 1, (const GLcharARB **) (&code), &size);
   glCompileShaderARB (vert);
   if (!CheckObjectStatus (vert))
      exit (0);

   for (i = 0; i < sizeof (attribs) / sizeof (*attribs); i++)
      if (attribs[i].dispatch == A && attribs[i].bind != -1)
         glBindAttribLocationARB (prog, attribs[i].bind, attribs[i].name);
}

void InitScene (void)
{
   prog = glCreateProgramObjectARB ();
   vert = glCreateShaderObjectARB (GL_VERTEX_SHADER_ARB);
   glAttachObjectARB (prog, vert);
   glDeleteObjectARB (vert);
   load_test_file ("vstest.txt");
   glLinkProgramARB (prog);
   if (!CheckObjectStatus (prog))
      exit (0);
   glUseProgramObjectARB (prog);
}

void RenderScene (void)
{
   GLint info_length, length;
   char output[65000], *p;
   GLint i;

   if (submit_method == SM_MAX)
      exit (0);

   /*
    * Get the current size of the info log. Any text output produced by executed
    * shader will be appended to the end of log.
    */
   glGetObjectParameterivARB (vert, GL_OBJECT_INFO_LOG_LENGTH_ARB, &info_length);

   switch (submit_method)
   {
   case SM_IM:
      printf ("\n--- TESTING IMMEDIATE MODE\n");
      im_render ();
      break;
   case SM_VA:
      printf ("\n--- TESTING VERTEX ARRAY MODE\n");
      va_render ();
      break;
   case SM_IM_DL:
      printf ("\n--- TESTING IMMEDIATE + DISPLAY LIST MODE\n");
      dl_start ();
      im_render ();
      dl_end ();
      break;
   case SM_VA_DL:
      printf ("\n--- TESTING VERTEX ARRAY + DISPLAY LIST MODE\n");
      dl_start ();
      va_render ();
      dl_end ();
      break;
   default:
      assert (0);
   }

   glFlush ();

   /*
    * Get the info log and set the pointer to the beginning of the output.
    */
   glGetInfoLogARB (vert, sizeof (output), &length, output);
   p = output + info_length - 1;

   for (i = 0; i < sizeof (attribs) / sizeof (*attribs); i++) {
      GLuint j;
      for (j = 0; j < attribs[i].size; j++) {
         GLfloat value;
         if (p == NULL) {
            printf ("*** %s\n", "I/O error");
            break;
         }
         if (strncmp (p, "true", 4) == 0)
            value = 1.0f;
         else if (strncmp (p, "false", 5) == 0)
            value = 0.0f;
         else if (sscanf (p, "%f", &value) != 1) {
            printf ("*** %s\n", "I/O error");
            p = NULL;
            break;
         }
         if (fabs (value - attribs[i].data[j]) > EPSILON)
            printf ("*** %s, is %f, should be %f\n", "Values are different", value, attribs[i].data[j]);
         p = strchr (p, '\n');
         if (p != NULL)
            p++;
      }
      if (p == NULL)
         break;
   }

   submit_method++;
}

int main (int argc, char *argv[])
{
   InitFramework (&argc, argv);
   return 0;
}

