/*
 * GL_ARB_shading_language_100 test application.
 *
 * Tests correctness of emited code. Runs multiple well-formed shaders and checks if
 * they produce valid results.
 *
 * Requires specific support on the GL implementation side. A special function printMESA()
 * must be supported in the language that prints current values of generic type
 * to the appropriate shader's info log, and optionally to the screen.
 *
 * Author: Michal Krol
 */

#include "framework.h"

#define EPSILON 0.0001f

static GLhandleARB vert = 0;
static GLhandleARB prog = 0;

static int get_line (FILE *f, char *line, int size)
{
   if (fgets (line, size, f) == NULL)
      return 0;
   if (line[strlen (line) - 1] == '\n')
      line[strlen (line) - 1] = '\0';
   return 1;
}

struct ATTRIB
{
   char name[32];
   GLfloat value[64][4];
   GLuint count;
};

struct ATTRIBS
{
   struct ATTRIB attrib[32];
   GLuint count;
};

struct SHADER
{
   char code[16000];
   GLfloat output[1000];
   GLuint count;
};

enum SHADER_LOAD_STATE
{
   SLS_NONE,
   SLS_CODE,
   SLS_OUTPUT
};

struct PROGRAM
{
   struct PROGRAM *next;
   char name[256];
   struct ATTRIBS attribs;
   struct SHADER vertex;
};

enum PROGRAM_LOAD_STATE
{
   PLS_NONE,
   PLS_ATTRIB,
   PLS_VERTEX
};

static struct PROGRAM *program = NULL;

static void load_test_file (const char *filename, struct PROGRAM **program)
{
   struct PROGRAM **currprog = program;
   FILE *f;
   char line[256];
   enum PROGRAM_LOAD_STATE pls = PLS_NONE;
   enum SHADER_LOAD_STATE sls = SLS_NONE;

   f = fopen (filename, "r");
   if (f == NULL)
      return;

   while (get_line (f, line, sizeof (line))) {
      if (line[0] == '$') {
         if (strncmp (line + 1, "program", 7) == 0) {
            if (*currprog != NULL)
               currprog = &(**currprog).next;
            *currprog = (struct PROGRAM *) (malloc (sizeof (struct PROGRAM)));
            if (*currprog == NULL)
               break;
            (**currprog).next = NULL;
            strcpy ((**currprog).name, line + 9);
            (**currprog).attribs.count = 0;
            (**currprog).vertex.code[0] = '\0';
            (**currprog).vertex.count = 0;
            pls = PLS_NONE;
         }
         else if (strncmp (line + 1, "attrib", 6) == 0) {
            if (*currprog == NULL)
               break;
            strcpy ((**currprog).attribs.attrib[(**currprog).attribs.count].name, line + 8);
            (**currprog).attribs.attrib[(**currprog).attribs.count].count = 0;
            (**currprog).attribs.count++;
            pls = PLS_ATTRIB;
         }
         else if (strcmp (line + 1, "vertex") == 0) {
            if (*currprog == NULL)
               break;
            pls = PLS_VERTEX;
            sls = SLS_NONE;
         }
         else if (strcmp (line + 1, "code") == 0) {
            if (*currprog == NULL || pls != PLS_VERTEX)
               break;
            sls = SLS_CODE;
         }
         else if (strcmp (line + 1, "output") == 0) {
            if (*currprog == NULL || pls != PLS_VERTEX)
               break;
            sls = SLS_OUTPUT;
         }
      }
      else {
         if ((*currprog == NULL || pls == PLS_NONE || sls == SLS_NONE) && line[0] != '\0')
            break;
         if (*currprog != NULL && pls == PLS_VERTEX) {
            if (sls == SLS_CODE) {
               strcat ((**currprog).vertex.code, line);
               strcat ((**currprog).vertex.code, "\n");
            }
            else if (sls == SLS_OUTPUT && line[0] != '\0') {
               if (strcmp (line, "true") == 0)
                  (**currprog).vertex.output[(**currprog).vertex.count] = 1.0f;
               else if (strcmp (line, "false") == 0)
                  (**currprog).vertex.output[(**currprog).vertex.count] = 0.0f;
               else
                  sscanf (line, "%f", &(**currprog).vertex.output[(**currprog).vertex.count]);
               (**currprog).vertex.count++;
            }
         }
         else if (*currprog != NULL && pls == PLS_ATTRIB && line[0] != '\0') {
            struct ATTRIB *att = &(**currprog).attribs.attrib[(**currprog).attribs.count - 1];
            GLfloat *vec = att->value[att->count];
            sscanf (line, "%f %f %f %f", &vec[0], &vec[1], &vec[2], &vec[3]);
            att->count++;
         }
      }
   }

   fclose (f);
}

void InitScene (void)
{
   prog = glCreateProgramObjectARB ();
   vert = glCreateShaderObjectARB (GL_VERTEX_SHADER_ARB);
   glAttachObjectARB (prog, vert);
   glDeleteObjectARB (vert);
   load_test_file ("cltest.txt", &program);
}

void RenderScene (void)
{
   struct PROGRAM *nextprogram;
   char *code;
   GLint info_length, length;
   char output[65000], *p;
   GLuint i;

   if (program == NULL)
      exit (0);

   code = program->vertex.code;
   glShaderSourceARB (vert, 1, (const GLcharARB **) (&code), NULL);
   glCompileShaderARB (vert);
   CheckObjectStatus (vert);

   for (i = 0; i < program->attribs.count; i++) {
      const char *name = program->attribs.attrib[i].name;
      if (strcmp (name, "gl_Vertex") != 0)
         glBindAttribLocationARB (prog, i, name);
   }

   glLinkProgramARB (prog);
   CheckObjectStatus (prog);
   glUseProgramObjectARB (prog);

   printf ("\n--- %s\n", program->name);

   glGetObjectParameterivARB (vert, GL_OBJECT_INFO_LOG_LENGTH_ARB, &info_length);

   glBegin (GL_POINTS);
   if (program->attribs.count == 0) {
      glVertex2f (0.0f, 0.0f);
   }
   else {
      for (i = 0; i < program->attribs.attrib[0].count; i++) {
         GLuint j;
         for (j = 0; j < program->attribs.count; j++) {
            GLuint n = (j + 1) % program->attribs.count;
            GLfloat *vec = program->attribs.attrib[n].value[i];
            const char *name = program->attribs.attrib[n].name;
            if (strcmp (name, "gl_Vertex") == 0)
               glVertex4fv (vec);
            else
               glVertexAttrib4fvARB (n, vec);
         }
      }
   }
   glEnd ();
   glFlush ();

   glGetInfoLogARB (vert, sizeof (output), &length, output);
   p = output + info_length - 1;
   for (i = 0; i < program->vertex.count; i++) {
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
         break;
      }
      if (fabs (value - program->vertex.output[i]) > EPSILON) {
         printf ("*** Values are different, is %f, should be %f\n", value,
                 program->vertex.output[i]);
      }
      p = strchr (p, '\n');
      if (p != NULL)
         p++;
   }
   if (p && *p != '\0')
      printf ("*** %s\n", "I/O error");

   nextprogram = program->next;
   free (program);
   program = nextprogram;
}

int main (int argc, char *argv[])
{
   InitFramework (&argc, argv);
   return 0;
}

