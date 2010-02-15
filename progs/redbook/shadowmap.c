/*
 * Copyright (c) 1993-2003, Silicon Graphics, Inc.
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright
 * notice and this permission notice appear in supporting documentation,
 * and that the name of Silicon Graphics, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS" AND
 * WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION, LOSS OF
 * PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF THIRD
 * PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN ADVISED OF
 * THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE POSSESSION, USE
 * OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor clauses
 * in the FAR or the DOD or NASA FAR Supplement.  Unpublished - rights
 * reserved under the copyright laws of the United States.
 *
 * Contractor/manufacturer is:
 *	Silicon Graphics, Inc.
 *	1500 Crittenden Lane
 *	Mountain View, CA  94043
 *	United State of America
 *
 * OpenGL(R) is a registered trademark of Silicon Graphics, Inc.
 */

#define GL_GLEXT_PROTOTYPES

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
/*#include "helpers.h"*/

#define SHADOW_MAP_WIDTH      256
#define SHADOW_MAP_HEIGHT     256

#define PI       3.14159265359

GLdouble    fovy      = 60.0;
GLdouble    nearPlane = 10.0;
GLdouble    farPlane  = 100.0;

GLfloat     angle = 0.0;
GLfloat     torusAngle = 0.0;

GLfloat     lightPos[] = { 25.0, 25.0, 25.0, 1.0 };
GLfloat     lookat[] = { 0.0, 0.0, 0.0 };
GLfloat     up[] = { 0.0, 0.0, 1.0 };

GLboolean showShadow = GL_FALSE;

static void
init( void )
{
    GLfloat  white[] = { 1.0, 1.0, 1.0, 1.0 };

    glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
  		  SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, 0,
		  GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL );

    glLightfv( GL_LIGHT0, GL_POSITION, lightPos );
    glLightfv( GL_LIGHT0, GL_SPECULAR, white );
    glLightfv( GL_LIGHT0, GL_DIFFUSE, white );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
    glTexParameteri( GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
		     GL_COMPARE_R_TO_TEXTURE );

    glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
    glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
    glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
    glTexGeni( GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );

    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );

    glCullFace( GL_BACK );

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_LIGHT0 );
    glEnable( GL_LIGHTING );
    glEnable( GL_TEXTURE_2D );
    glEnable( GL_TEXTURE_GEN_S );
    glEnable( GL_TEXTURE_GEN_T );
    glEnable( GL_TEXTURE_GEN_R );
    glEnable( GL_TEXTURE_GEN_Q );
    glEnable( GL_COLOR_MATERIAL );
    glEnable( GL_CULL_FACE );
}

static void
reshape( int width, int height )
{
    glViewport( 0, 0, width, height );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluPerspective( fovy, (GLdouble) width/height, nearPlane, farPlane );
    glMatrixMode( GL_MODELVIEW );
}

static void
idle( void )
{
    angle += PI / 10000;
    torusAngle += .1;
    glutPostRedisplay();
}

static void
keyboard( unsigned char key, int x, int y )
{
    switch( key ) {
    case 27:  /* Escape */
      exit( 0 );
      break;

    case 't': {
        static GLboolean textureOn = GL_TRUE;
        textureOn = !textureOn;
        if ( textureOn )
	glEnable( GL_TEXTURE_2D );
        else
	glDisable( GL_TEXTURE_2D );
      }
      break;
      
    case 'm': {
        static GLboolean compareMode = GL_TRUE;
        compareMode = !compareMode;
        printf( "Compare mode %s\n", compareMode ? "On" : "Off" );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
		         compareMode ? GL_COMPARE_R_TO_TEXTURE : GL_NONE );
      }
      break;

    case 'f': {
        static GLboolean funcMode = GL_TRUE;
        funcMode = !funcMode;
        printf( "Operator %s\n", funcMode ? "GL_LEQUAL" : "GL_GEQUAL" );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC,
		         funcMode ? GL_LEQUAL : GL_GEQUAL );
      }
      break;

    case 's':
      showShadow = !showShadow;
      break;

    case 'p': {
        static GLboolean  animate = GL_TRUE;
        animate = !animate;
        glutIdleFunc( animate ? idle : NULL );
      }
      break;
    }

    glutPostRedisplay();
}

static void
transposeMatrix( GLfloat m[16] )
{
    GLfloat  tmp;
#define Swap( a, b )    tmp = a; a = b; b = tmp
    Swap( m[1],  m[4]  );
    Swap( m[2],  m[8]  );
    Swap( m[3],  m[12] );
    Swap( m[6],  m[9]  );
    Swap( m[7],  m[13] );
    Swap( m[11], m[14] );
#undef Swap
}

static void
drawObjects( GLboolean shadowRender )
{
    GLboolean textureOn = glIsEnabled( GL_TEXTURE_2D );

    if ( shadowRender )
        glDisable( GL_TEXTURE_2D );

    if ( !shadowRender ) {
        glNormal3f( 0, 0, 1 );
        glColor3f( 1, 1, 1 );
        glRectf( -20.0, -20.0, 20.0, 20.0 );
    }
    
    glPushMatrix();
    glTranslatef( 11, 11, 11 );
    glRotatef( 54.73, -5, 5, 0 );
    glRotatef( torusAngle, 1, 0, 0 );
    glColor3f( 1, 0, 0 );
    glutSolidTorus( 1, 4, 8, 36 );
    glPopMatrix();

    glPushMatrix();
    glTranslatef( 2, 2, 2 );
    glColor3f( 0, 0, 1 );
    glutSolidCube( 4 );
    glPopMatrix();

    glPushMatrix();
    glTranslatef( lightPos[0], lightPos[1], lightPos[2] );
    glColor3f( 1, 1, 1 );
    glutWireSphere( 0.5, 6, 6 );
    glPopMatrix();

    if ( shadowRender && textureOn ) 
        glEnable( GL_TEXTURE_2D );
}

static void
generateShadowMap( void )
{
    GLint    viewport[4];
    GLfloat  lightPos[4];

    glGetLightfv( GL_LIGHT0, GL_POSITION, lightPos );
    glGetIntegerv( GL_VIEWPORT, viewport );

    glViewport( 0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    gluPerspective( 80.0, 1.0, 10.0, 1000.0 );
    glMatrixMode( GL_MODELVIEW );

    glPushMatrix();
    glLoadIdentity();
    gluLookAt( lightPos[0], lightPos[1], lightPos[2],
	       lookat[0], lookat[1], lookat[2],
	       up[0], up[1], up[2] );

    drawObjects( GL_TRUE );

    glPopMatrix();
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW );

    glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0,
		      SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, 0 );

    glViewport( viewport[0], viewport[1], viewport[2], viewport[3] );

    if ( showShadow ) {
      GLfloat depthImage[SHADOW_MAP_WIDTH][SHADOW_MAP_HEIGHT];
      glReadPixels( 0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, 
		    GL_DEPTH_COMPONENT, GL_FLOAT, depthImage );
      glWindowPos2f( viewport[2]/2, 0 );
      glDrawPixels( SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, GL_LUMINANCE, 
		    GL_FLOAT, depthImage );
      glutSwapBuffers();
    }
}

static void
generateTextureMatrix( void )
{
    GLfloat  tmpMatrix[16];

    /*
     *  Set up projective texture matrix.  We use the GL_MODELVIEW matrix
     *    stack and OpenGL matrix commands to make the matrix.
     */
    glPushMatrix();
    glLoadIdentity();
    glTranslatef( 0.5, 0.5, 0.0 );
    glScalef( 0.5, 0.5, 1.0 );
    gluPerspective( 60.0, 1.0, 1.0, 1000.0 );
    gluLookAt( lightPos[0], lightPos[1], lightPos[2],
	       lookat[0], lookat[1], lookat[2],
	       up[0], up[1], up[2] );
    glGetFloatv( GL_MODELVIEW_MATRIX, tmpMatrix );
    glPopMatrix();

    transposeMatrix( tmpMatrix );

    glTexGenfv( GL_S, GL_OBJECT_PLANE, &tmpMatrix[0] );
    glTexGenfv( GL_T, GL_OBJECT_PLANE, &tmpMatrix[4] );
    glTexGenfv( GL_R, GL_OBJECT_PLANE, &tmpMatrix[8] );
    glTexGenfv( GL_Q, GL_OBJECT_PLANE, &tmpMatrix[12] );
}

static void
display( void )
{
    GLfloat  radius = 30;

    generateShadowMap();
    generateTextureMatrix();

    if ( showShadow )
      return;

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    glPushMatrix();
    gluLookAt( radius*cos(angle), radius*sin(angle), 30,
	       lookat[0], lookat[1], lookat[2], 
	       up[0], up[1], up[2] );
    drawObjects( GL_FALSE );
    glPopMatrix();

    glutSwapBuffers();
}

int
main( int argc, char** argv )
{
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE );
    glutInitWindowSize( 512, 512 );
    glutInitWindowPosition( 100, 100 );
    glutCreateWindow( argv[0] );

    init();

    glutDisplayFunc( display );
    glutReshapeFunc( reshape );
    glutKeyboardFunc( keyboard );
    glutIdleFunc( idle );

    glutMainLoop();

    return 0;
}
