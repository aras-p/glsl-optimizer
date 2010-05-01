/*
 * (C) Copyright IBM Corporation 2006
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file prog_parameter.c
 * 
 * Test various aspects of setting (and getting) low-level program parameters.
 * This is primarilly intended as a test for GL_EXT_gpu_program_parameters,
 * but it turns out that it hits some other functionality along the way.
 * 
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

#ifndef GL_EXT_gpu_program_parameters
typedef void (APIENTRYP PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC)(GLenum,
    GLuint, GLsizei, const GLfloat *);
typedef void (APIENTRYP PFNGLPROGRAMENVPARAMETERS4FVEXTPROC)(GLenum,
    GLuint, GLsizei, const GLfloat *);
#endif

static PFNGLPROGRAMLOCALPARAMETER4FVARBPROC program_local_parameter4fv = NULL;
static PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC get_program_local_parameterfv = NULL;
static PFNGLPROGRAMENVPARAMETER4FVARBPROC program_env_parameter4fv = NULL;
static PFNGLGETPROGRAMENVPARAMETERFVARBPROC get_program_env_parameterfv = NULL;
static PFNGLBINDPROGRAMARBPROC bind_program = NULL;
static PFNGLGETPROGRAMIVARBPROC get_program = NULL;

static PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC program_local_parameters4fv = NULL;
static PFNGLPROGRAMENVPARAMETERS4FVEXTPROC program_env_parameters4fv = NULL;

static int Width = 400;
static int Height = 200;
static const GLfloat Near = 5.0, Far = 25.0;


static void Display( void )
{
}


static void Idle( void )
{
}


static void Visible( int vis )
{
   if ( vis == GLUT_VISIBLE ) {
      glutIdleFunc( Idle );
   }
   else {
      glutIdleFunc( NULL );
   }
}
static void Reshape( int width, int height )
{
   GLfloat ar = (float) width / (float) height;
   Width = width;
   Height = height;
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -ar, ar, -1.0, 1.0, Near, Far );
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static int set_parameter_batch( GLsizei count, GLfloat * param,
				const char * name,
				PFNGLPROGRAMLOCALPARAMETER4FVARBPROC set_parameter,
				PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC set_parameters,
				PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC get_parameter
				)
{
   unsigned i;
   int pass = 1;


   for ( i = 0 ; i < (4 * count) ; i++ ) {
      param[i] = (GLfloat) rand() / (GLfloat) rand();
   }

   /* Try using the "classic" interface.
    */
   printf("Testing glProgram%sParameter4fvARB (count = %u)...\n", name, count);
   for ( i = 0 ; i < count ; i++ ) {
      (*set_parameter)(GL_VERTEX_PROGRAM_ARB, i, & param[i * 4]);
   }

   for ( i = 0 ; i < count ; i++ ) {
      GLfloat temp[4];

      (*get_parameter)(GL_VERTEX_PROGRAM_ARB, i, temp);

      if ( (temp[0] != param[(i * 4) + 0])
	   || (temp[1] != param[(i * 4) + 1])
	   || (temp[2] != param[(i * 4) + 2])
	   || (temp[3] != param[(i * 4) + 3]) ) {
	 printf("Mismatch in glProgram%sParameter4fvARB index %u!\n", name, i);
	 printf("Got { %f, %f, %f, %f }, expected { %f, %f, %f, %f }!\n",
		temp[0],            temp[1],
		temp[2],            temp[3],
		param[(i * 4) + 0], param[(i * 4) + 1],
		param[(i * 4) + 2], param[(i * 4) + 3]);
	 pass = 0;
	 break;
      }
   }


   if ( set_parameters == NULL ) {
      return pass;
   }


   for ( i = 0 ; i < (4 * count) ; i++ ) {
      param[i] = (GLfloat) rand() / (GLfloat) rand();
   }

   printf("Testing glProgram%sParameters4fvEXT (count = %u)...\n", name, count);
   (*set_parameters)(GL_VERTEX_PROGRAM_ARB, 0, count, param);

   for ( i = 0 ; i < count ; i++ ) {
      GLfloat temp[4];

      (*get_parameter)(GL_VERTEX_PROGRAM_ARB, i, temp);

      if ( (temp[0] != param[(i * 4) + 0])
	   || (temp[1] != param[(i * 4) + 1])
	   || (temp[2] != param[(i * 4) + 2])
	   || (temp[3] != param[(i * 4) + 3]) ) {
	 printf("Mismatch in glProgram%sParameters4fvEXT index %u!\n", name, i);
	 printf("Got { %f, %f, %f, %f }, expected { %f, %f, %f, %f }!\n",
		temp[0],            temp[1],
		temp[2],            temp[3],
		param[(i * 4) + 0], param[(i * 4) + 1],
		param[(i * 4) + 2], param[(i * 4) + 3]);
	 pass = 0;
	 break;
      }
   }


   return pass;
}


static void Init( void )
{
   const char * const ver_string = (const char *)
       glGetString( GL_VERSION );
   int pass = 1;
   GLfloat * params;
   GLint max_program_env_parameters;
   GLint max_program_local_parameters;
   int i;


   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION = %s\n\n", ver_string);

   if ( !glutExtensionSupported("GL_ARB_vertex_program") ) {
      printf("Sorry, this program requires GL_ARB_vertex_program\n");
      exit(2);
   }


   program_local_parameter4fv = (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) glutGetProcAddress( "glProgramLocalParameter4fvARB" );
   program_env_parameter4fv = (PFNGLPROGRAMENVPARAMETER4FVARBPROC) glutGetProcAddress( "glProgramEnvParameter4fvARB" );

   get_program_local_parameterfv = (PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC) glutGetProcAddress( "glGetProgramLocalParameterfvARB" );
   get_program_env_parameterfv = (PFNGLGETPROGRAMENVPARAMETERFVARBPROC) glutGetProcAddress( "glGetProgramEnvParameterfvARB" );

   bind_program = (PFNGLBINDPROGRAMARBPROC) glutGetProcAddress( "glBindProgramARB" );
   get_program = (PFNGLGETPROGRAMIVARBPROC) glutGetProcAddress( "glGetProgramivARB" );

   if ( glutExtensionSupported("GL_EXT_gpu_program_parameters") ) {
      printf("GL_EXT_gpu_program_parameters available, testing that path.\n");

      program_local_parameters4fv = (PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC) glutGetProcAddress( "glProgramLocalParameters4fvEXT" );
      program_env_parameters4fv = (PFNGLPROGRAMENVPARAMETERS4FVEXTPROC) glutGetProcAddress( "glProgramEnvParameters4fvEXT" );
   }
   else {
      printf("GL_EXT_gpu_program_parameters not available.\n");

      program_local_parameters4fv = NULL;
      program_env_parameters4fv = NULL;
   }



   /* Since the test sets program local parameters, a program must be bound.
    * Program source, however, is not needed.
    */
   (*bind_program)(GL_VERTEX_PROGRAM_ARB, 1);


   (*get_program)(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB,
		  & max_program_env_parameters);

   params = malloc(max_program_env_parameters * 4 * sizeof(GLfloat));

   for (i = 0; i < max_program_env_parameters * 4; i++) {
      params[i] = 0.0F;
   }

   pass &= set_parameter_batch(max_program_env_parameters, params, "Env",
			       program_env_parameter4fv,
			       program_env_parameters4fv,
			       get_program_env_parameterfv);


   (*get_program)(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB,
		  & max_program_local_parameters);

   if (max_program_local_parameters > max_program_env_parameters) {
      params = realloc(params,
		       max_program_local_parameters * 4 * sizeof(GLfloat));
   }

   pass &= set_parameter_batch(max_program_local_parameters, params, "Local",
			       program_local_parameter4fv,
			       program_local_parameters4fv,
			       get_program_local_parameterfv);

   free(params);

   if (! pass) {
      printf("FAIL!\n");
      exit(1);
   }
   
   printf("PASS!\n");
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( Width, Height );
   glutInitDisplayMode( GLUT_RGB );
   glutCreateWindow( "Program Parameters Test" );
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   glutVisibilityFunc( Visible );

   Init();

   return 0;
}
