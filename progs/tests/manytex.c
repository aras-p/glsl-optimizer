/* $Id: manytex.c,v 1.1 2000/08/02 17:57:56 brianp Exp $ */

/*
 * test handling of many texture maps
 * Also tests texture priority and residency.
 *
 * Brian Paul
 * August 2, 2000
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>


static GLint NumTextures = 20;
static GLuint *TextureID = NULL;
static GLboolean *TextureResidency = NULL;
static GLint TexWidth = 128, TexHeight = 128;
static GLfloat Zrot = 0;
static GLboolean Anim = GL_TRUE;
static GLint WinWidth = 500, WinHeight = 400;
static GLboolean MipMap = GL_FALSE;
static GLboolean LinearFilter = GL_FALSE;
static GLboolean RandomSize = GL_FALSE;
static GLint Rows, Columns;
static GLuint LowPriorityCount = 0;



static void Idle( void )
{
   Zrot += 1.0;
   glutPostRedisplay();
}


static void Display( void )
{
   GLfloat spacing = WinWidth / Columns;
   GLfloat size = spacing * 0.4;
   GLint i;

   /* test residency */
   {
      GLboolean b;
      GLint i, resident;
      b = glAreTexturesResident(NumTextures, TextureID, TextureResidency);
      if (b) {
         printf("all resident\n");
      }
      else {
         resident = 0;
         for (i = 0; i < NumTextures; i++) {
            if (TextureResidency[i]) {
               resident++;
            }
         }
         printf("%d of %d texture resident\n", resident, NumTextures);
      }
   }

   /* render the textured quads */
   glClear( GL_COLOR_BUFFER_BIT );
   for (i = 0; i < NumTextures; i++) {
      GLint row = i / Columns;
      GLint col = i % Columns;
      GLfloat x = col * spacing + spacing * 0.5;
      GLfloat y = row * spacing + spacing * 0.5;

      glPushMatrix();
         glTranslatef(x, y, 0.0);
         glRotatef(Zrot, 0, 0, 1);
         glScalef(size, size, 1);

         glBindTexture(GL_TEXTURE_2D, TextureID[i]);
         glBegin(GL_POLYGON);
         glTexCoord2f(0, 0);  glVertex2f(-1, -1);
         glTexCoord2f(1, 0);  glVertex2f( 1, -1);
         glTexCoord2f(1, 1);  glVertex2f( 1,  1);
         glTexCoord2f(0, 1);  glVertex2f(-1,  1);
         glEnd();
      glPopMatrix();
   }

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   WinWidth = width;
   WinHeight = height;
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho(0, width, 0, height, -1, 1);
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
}



static void Init( void )
{
   GLint i;

   if (RandomSize) {
      printf("Creating %d %s random-size textures, ", NumTextures,
             MipMap ? "Mipmapped" : "non-Mipmapped");
   }
   else {
      printf("Creating %d %s %d x %d textures, ", NumTextures,
             MipMap ? "Mipmapped" : "non-Mipmapped",
             TexWidth, TexHeight);
   }

   if (LinearFilter) {
      printf("bilinear filtering\n");
   }
   else {
      printf("nearest filtering\n");
   }


   /* compute number of rows and columns of rects */
   {
      GLfloat area = (GLfloat) (WinWidth * WinHeight) / (GLfloat) NumTextures;
      GLfloat edgeLen = sqrt(area);

      Columns = WinWidth / edgeLen;
      Rows = (NumTextures + Columns - 1) / Columns;
      printf("Rows: %d  Cols: %d\n", Rows, Columns);
   }


   if (!TextureID) {
      TextureID = (GLuint *) malloc(sizeof(GLuint) * NumTextures);
      assert(TextureID);
      glGenTextures(NumTextures, TextureID);
   }

   if (!TextureResidency) {
      TextureResidency = (GLboolean *) malloc(sizeof(GLboolean) * NumTextures);
      assert(TextureResidency);
   }

   for (i = 0; i < NumTextures; i++) {
      GLubyte color[4];
      GLubyte *texImage;
      GLint j, row, col;

      row = i / Columns;
      col = i % Columns;

      glBindTexture(GL_TEXTURE_2D, TextureID[i]);

      if (i < LowPriorityCount)
         glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, 0.5F);

      if (RandomSize) {
         int k = (glutGet(GLUT_ELAPSED_TIME) % 7) + 2;
         TexWidth  = 1 << k;
         TexHeight = 1 << k;
      }

      texImage = (GLubyte*) malloc(4 * TexWidth * TexHeight * sizeof(GLubyte));
      assert(texImage);

      /* determine texture color */
      color[0] = (GLint) (255.0 * ((float) col / (Columns - 1)));
      color[1] = 127;
      color[2] = (GLint) (255.0 * ((float) row / (Rows - 1)));
      color[3] = 255;

      /* fill in solid-colored teximage */
      for (j = 0; j < TexWidth * TexHeight; j++) {
         texImage[j*4+0] = color[0];
         texImage[j*4+1] = color[1];
         texImage[j*4+2] = color[2];
         texImage[j*4+3] = color[3];
     }

      if (MipMap) {
         GLint level = 0;
         GLint w = TexWidth, h = TexHeight;
         while (1) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, texImage);
            if (w == 1 && h == 1)
               break;
            if (w > 1)
               w /= 2;
            if (h > 1)
               h /= 2;
            level++;
            /*printf("%d: %d x %d\n", level, w, h);*/
         }
         if (LinearFilter) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
         }
         else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            GL_NEAREST_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
         }
      }
      else {
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TexWidth, TexHeight, 0,
                      GL_RGBA, GL_UNSIGNED_BYTE, texImage);
         if (LinearFilter) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
         }
         else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
         }
      }

      free(texImage);
   }

   glEnable(GL_TEXTURE_2D);
}


static void Key( unsigned char key, int x, int y )
{
   const GLfloat step = 3.0;
   (void) x;
   (void) y;
   switch (key) {
      case 'a':
         Anim = !Anim;
         if (Anim)
            glutIdleFunc(Idle);
         else
            glutIdleFunc(NULL);
         break;
      case 'z':
         Zrot -= step;
         break;
      case 'Z':
         Zrot += step;
         break;
      case ' ':
         Init();
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


int main( int argc, char *argv[] )
{
   GLint i;

   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( WinWidth, WinHeight );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow(argv[0]);
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   if (Anim)
      glutIdleFunc(Idle);

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-n") == 0) {
         NumTextures = atoi(argv[i+1]);
         if (NumTextures <= 0) {
            printf("Error, bad number of textures\n");
            return 1;
         }
         i++;
      }
      else if (strcmp(argv[i], "-mipmap") == 0) {
         MipMap = GL_TRUE;
      }
      else if (strcmp(argv[i], "-linear") == 0) {
         LinearFilter = GL_TRUE;
      }
      else if (strcmp(argv[i], "-size") == 0) {
         TexWidth = atoi(argv[i+1]);
         TexHeight = atoi(argv[i+2]);
         assert(TexWidth >= 1);
         assert(TexHeight >= 1);
         i += 2;
      }
      else if (strcmp(argv[i], "-randomsize") == 0) {
         RandomSize = GL_TRUE;
      }
      else if (strcmp(argv[i], "-lowpri") == 0) {
         LowPriorityCount = atoi(argv[i+1]);
         i++;
      }
      else {
         printf("Usage:\n");
         printf("  manytex [options]\n");
         printf("Options:\n");
         printf("  -n <number of texture objects>\n");
         printf("  -size <width> <height>  - specify texture size\n");
         printf("  -randomsize  - use random size textures\n");
         printf("  -mipmap      - generate mipmaps\n");
         printf("  -linear      - use linear filtering instead of nearest\n");
         printf("  -lowpri <n>  - Set lower priority on <n> textures\n");
         return 0;
      }
   }

   Init();

   glutMainLoop();

   return 0;
}
