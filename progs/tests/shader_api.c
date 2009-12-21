/* Tests to validate fixes to various bugs in src/mesa/shader/shader_api.c
 *
 * Written by Bruce Merry
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>

#ifndef APIENTRY
#define APIENTRY
#endif

static void assert_test(const char *file, int line, int cond, const char *msg)
{
   if (!cond)
      fprintf(stderr, "%s:%d assertion \"%s\" failed\n", file, line, msg);
}

#undef assert
#define assert(x) assert_test(__FILE__, __LINE__, (x), #x)

static void assert_no_error_test(const char *file, int line)
{
   GLenum err;

   err = glGetError();
   if (err != GL_NO_ERROR)
      fprintf(stderr, "%s:%d received error %s\n",
              file, line, gluErrorString(err));
}

#define assert_no_error() assert_no_error_test(__FILE__, __LINE__)

static void assert_error_test(const char *file, int line, GLenum expect)
{
   GLenum err;

   err = glGetError();
   if (err != expect)
      fprintf(stderr, "%s:%d expected %s but received %s\n",
              file, line, gluErrorString(expect), gluErrorString(err));
   while (glGetError()); /* consume any following errors */
}

#define assert_error(err) assert_error_test(__FILE__, __LINE__, (err))

static void check_status(GLuint id, GLenum pname, void (APIENTRY *query)(GLuint, GLenum, GLint *))
{
    GLint status;

    query(id, pname, &status);
    if (!status)
    {
        char info[65536];

        fprintf(stderr, "Compilation/link failure:\n");
        glGetInfoLogARB(id, sizeof(info), NULL, info);
        fprintf(stderr, "%s\n", info);
        exit(1);
    }
}

static void check_compile_status(GLuint id)
{
   check_status(id, GL_COMPILE_STATUS, glGetShaderiv);
}

static void check_link_status(GLuint id)
{
   check_status(id, GL_LINK_STATUS, glGetProgramiv);
}

static GLuint make_shader(GLenum type, const char *src)
{
   GLuint id;

   assert_no_error();
   id = glCreateShader(type);
   glShaderSource(id, 1, &src, NULL);
   glCompileShader(id);
   check_compile_status(id);
   assert_no_error();
   return id;
}

static GLuint make_program(const char *vs_src, const char *fs_src)
{
   GLuint id, vs, fs;

   assert_no_error();
   id = glCreateProgram();
   if (vs_src) {
      vs = make_shader(GL_VERTEX_SHADER, vs_src);
      glAttachShader(id, vs);
      glDeleteShader(vs);
   }
   if (fs_src) {
      fs = make_shader(GL_FRAGMENT_SHADER, fs_src);
      glAttachShader(id, fs);
      glDeleteShader(fs);
   }
   glLinkProgram(id);
   check_link_status(id);
   glUseProgram(id);
   glDeleteProgram(id);
   assert_no_error();
   return id;
}

static void test_uniform_size_type1(const char *glslType, GLenum glType, const char *el)
{
   char buffer[1024];
   GLuint program;
   GLint active, i;
   GLenum type;
   GLint size;

   printf("  Running subtest %s\n", glslType);
   fflush(stdout);
   sprintf(buffer, "#version 120\nuniform %s m[60];\nvoid main() { gl_Position[0] = m[59]%s; }\n",
           glslType, el);

   program = make_program(buffer, NULL);
   glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &active);
   assert_no_error();
   for (i = 0; i < active; i++) {
      size = -1;
      type = 0;
      glGetActiveUniform(program, i, sizeof(buffer), NULL, &size, &type, buffer);
      assert_no_error();
      if (strncmp(buffer, "m", 1) == 0)
         break;
   }
   assert(i < active); /* Otherwise the compiler optimised it out */
   assert(type == glType);
   assert(size == 60);
}

static void test_uniform_size_type(void)
{
   test_uniform_size_type1("float", GL_FLOAT, "");
   test_uniform_size_type1("vec2", GL_FLOAT_VEC2, "[0]");
   test_uniform_size_type1("vec3", GL_FLOAT_VEC3, "[0]");
   test_uniform_size_type1("vec4", GL_FLOAT_VEC4, "[0]");

   test_uniform_size_type1("bool", GL_BOOL, " ? 1.0 : 0.0");
   test_uniform_size_type1("bvec2", GL_BOOL_VEC2, "[0] ? 1.0 : 0.0");
   test_uniform_size_type1("bvec3", GL_BOOL_VEC3, "[0] ? 1.0 : 0.0");
   test_uniform_size_type1("bvec4", GL_BOOL_VEC4, "[0] ? 1.0 : 0.0");

   test_uniform_size_type1("int", GL_INT, "");
   test_uniform_size_type1("ivec2", GL_INT_VEC2, "[0]");
   test_uniform_size_type1("ivec3", GL_INT_VEC3, "[0]");
   test_uniform_size_type1("ivec4", GL_INT_VEC4, "[0]");

   test_uniform_size_type1("mat2", GL_FLOAT_MAT2, "[0][0]");
   test_uniform_size_type1("mat3", GL_FLOAT_MAT3, "[0][0]");
   test_uniform_size_type1("mat4", GL_FLOAT_MAT4, "[0][0]");
   test_uniform_size_type1("mat2x3", GL_FLOAT_MAT2x3, "[0][0]");
   test_uniform_size_type1("mat2x4", GL_FLOAT_MAT2x4, "[0][0]");
   test_uniform_size_type1("mat3x2", GL_FLOAT_MAT3x2, "[0][0]");
   test_uniform_size_type1("mat3x4", GL_FLOAT_MAT3x4, "[0][0]");
   test_uniform_size_type1("mat4x2", GL_FLOAT_MAT4x2, "[0][0]");
   test_uniform_size_type1("mat4x3", GL_FLOAT_MAT4x3, "[0][0]");
}

static void test_attrib_size_type1(const char *glslType, GLenum glType, const char *el)
{
   char buffer[1024];
   GLuint program;
   GLint active, i;
   GLenum type;
   GLint size;

   printf("  Running subtest %s\n", glslType);
   fflush(stdout);
   sprintf(buffer, "#version 120\nattribute %s m;\nvoid main() { gl_Position[0] = m%s; }\n",
           glslType, el);

   program = make_program(buffer, NULL);
   glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &active);
   assert_no_error();
   for (i = 0; i < active; i++) {
      size = -1;
      type = -1;
      glGetActiveAttrib(program, i, sizeof(buffer), NULL, &size, &type, buffer);
      assert_no_error();
      if (strncmp(buffer, "m", 1) == 0)
         break;
   }
   assert(i < active); /* Otherwise the compiler optimised it out */
   assert(type == glType);
   assert(size == 1);
}

static void test_attrib_size_type(void)
{
   test_attrib_size_type1("float", GL_FLOAT, "");
   test_attrib_size_type1("vec2", GL_FLOAT_VEC2, "[0]");
   test_attrib_size_type1("vec3", GL_FLOAT_VEC3, "[0]");
   test_attrib_size_type1("vec4", GL_FLOAT_VEC4, "[0]");

   test_attrib_size_type1("mat2", GL_FLOAT_MAT2, "[0][0]");
   test_attrib_size_type1("mat3", GL_FLOAT_MAT3, "[0][0]");
   test_attrib_size_type1("mat4", GL_FLOAT_MAT4, "[0][0]");
   test_attrib_size_type1("mat2x3", GL_FLOAT_MAT2x3, "[0][0]");
   test_attrib_size_type1("mat2x4", GL_FLOAT_MAT2x4, "[0][0]");
   test_attrib_size_type1("mat3x2", GL_FLOAT_MAT3x2, "[0][0]");
   test_attrib_size_type1("mat3x4", GL_FLOAT_MAT3x4, "[0][0]");
   test_attrib_size_type1("mat4x2", GL_FLOAT_MAT4x2, "[0][0]");
   test_attrib_size_type1("mat4x3", GL_FLOAT_MAT4x3, "[0][0]");
}

static void test_uniform_array_overflow(void)
{
   GLuint program;
   GLint location;
   GLfloat data[128];

   program = make_program("#version 120\nuniform vec2 x[10];\nvoid main() { gl_Position.xy = x[9]; }\n", NULL);
   location = glGetUniformLocation(program, "x");
   assert_no_error();
   glUniform2fv(location, 64, data);
   assert_no_error();
}

static void test_uniform_scalar_count(void)
{
   GLuint program;
   GLint location;
   GLfloat data[128];

   program = make_program("#version 110\nuniform vec2 x;\nvoid main() { gl_Position.xy = x; }\n", NULL);
   location = glGetUniformLocation(program, "x");
   assert_no_error();
   glUniform2fv(location, 64, data);
   assert_error(GL_INVALID_OPERATION);
}

static void test_uniform_query_matrix(void)
{
   GLuint program;
   GLfloat data[18];
   GLint i, r, c;
   GLint location;

   program = make_program("#version 110\nuniform mat3 m[2];\nvoid main() { gl_Position.xyz = m[1][2]; }\n", NULL);
   location = glGetUniformLocation(program, "m");
   for (i = 0; i < 9; i++)
      data[i] = i;
   for (i = 9; i < 18; i++)
      data[i] = 321.0;
   glUniformMatrix3fv(location, 1, GL_TRUE, data);

   for (i = 0; i < 18; i++)
      data[i] = 123.0;
   glGetUniformfv(program, location, data);
   for (c = 0; c < 3; c++)
      for (r = 0; r < 3; r++)
         assert(data[c * 3 + r] == r * 3 + c);
   for (i = 9; i < 18; i++)
      assert(data[i] == 123.0);
}

static void test_uniform_neg_location(void)
{
   GLuint program;
   GLfloat data[4];

   program = make_program("#version 110\nvoid main() { gl_Position = vec4(1.0, 1.0, 1.0, 1.0); }\n", NULL);
   assert_no_error();
   glUniform1i(-1, 1);
   assert_no_error();
   glUniform1i(-200, 1);
   assert_error(GL_INVALID_OPERATION);
   glUniformMatrix2fv(-1, 1, GL_FALSE, data);
   assert_no_error();
   glUniformMatrix2fv(-200, 1, GL_FALSE, data);
   assert_error(GL_INVALID_OPERATION);
}

static void test_uniform_bool_conversion(void)
{
    GLuint program;
    GLint location;
    GLint value[16];  /* in case glGetUniformiv goes nuts on the stack */

    assert_no_error();
    program = make_program("uniform bool b;\nvoid main() { gl_Position.x = b ? 1.5 : 0.5; }\n", NULL);
    location = glGetUniformLocation(program, "b");
    assert(location != -1);
    assert_no_error();
    glUniform1i(location, 5);
    assert_no_error();
    glGetUniformiv(program, location, &value[0]);
    assert_no_error();
    assert(value[0] == 1);
}

static void test_uniform_multiple_samplers(void)
{
   GLuint program;
   GLint location;
   GLint values[2] = {0, 1};

   assert_no_error();
   program = make_program(NULL, "uniform sampler2D s[2];\nvoid main() { gl_FragColor = texture2D(s[1], vec2(0.0, 0.0)); }\n");
   location = glGetUniformLocation(program, "s[0]");
   assert(location != -1);
   assert_no_error();
   glUniform1iv(location, 2, values);
   assert_no_error();
}

static void run_test(const char *name, void (*callback)(void))
{
   printf("Running %s\n", name);
   fflush(stdout);
   callback();
}

#define RUN_TEST(name) run_test(#name, (name))

int main(int argc, char **argv)
{
   const char *version;

   glutInit(&argc, argv);
   glutCreateWindow("Mesa bug demo");
   glewInit();

   version = (const char *) glGetString(GL_VERSION);
   if (version[0] == '1') {
      printf("Sorry, this test requires OpenGL 2.x GLSL support\n");
      exit(0);
   }

   RUN_TEST(test_uniform_size_type);
   RUN_TEST(test_attrib_size_type);
   RUN_TEST(test_uniform_array_overflow);
   RUN_TEST(test_uniform_scalar_count);
   RUN_TEST(test_uniform_query_matrix);
   RUN_TEST(test_uniform_neg_location);
   RUN_TEST(test_uniform_bool_conversion);
   /* Leave this one at the end, since it crashes Mesa's shader compiler */
   RUN_TEST(test_uniform_multiple_samplers);
   return 0;
}
