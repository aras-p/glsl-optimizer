
/*
 * Paletted texture demo.  Written by Brian Paul.
 * This program is in the public domain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>


static float Rot = 0.0;
static GLboolean Anim = 1;


static void Idle( void )
{
   float t = glutGet(GLUT_ELAPSED_TIME) * 0.001;  /* in seconds */
   Rot = t * 360 / 4;  /* 1 rotation per 4 seconds */
   glutPostRedisplay();
}


static void Display( void )
{
   /* draw background gradient */
   glDisable(GL_TEXTURE_2D);
   glBegin(GL_POLYGON);
   glColor3f(1.0, 0.0, 0.2); glVertex2f(-1.5, -1.0);
   glColor3f(1.0, 0.0, 0.2); glVertex2f( 1.5, -1.0);
   glColor3f(0.0, 0.0, 1.0); glVertex2f( 1.5,  1.0);
   glColor3f(0.0, 0.0, 1.0); glVertex2f(-1.5,  1.0);
   glEnd();

   glPushMatrix();
   glRotatef(Rot, 0, 0, 1);

   glEnable(GL_TEXTURE_2D);
   glBegin(GL_POLYGON);
   glTexCoord2f(0, 1);  glVertex2f(-1, -0.5);
   glTexCoord2f(1, 1);  glVertex2f( 1, -0.5);
   glTexCoord2f(1, 0);  glVertex2f( 1,  0.5);
   glTexCoord2f(0, 0);  glVertex2f(-1,  0.5);
   glEnd();

   glPopMatrix();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho( -1.5, 1.5, -1.0, 1.0, -1.0, 1.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         exit(0);
         break;
      case 's':
         Rot += 0.5;
         break;
      case ' ':
         Anim = !Anim;
         if (Anim)
            glutIdleFunc( Idle );
         else
            glutIdleFunc( NULL );
         break;
   }
   glutPostRedisplay();
}


static void Init( void )
{
#define HEIGHT 8
#define WIDTH 32
   /* 257 = HEIGHT * WIDTH + 1 (for trailing '\0') */
   static char texture[257] = {"\
                                \
    MMM    EEEE   SSS    AAA    \
   M M M  E      S   S  A   A   \
   M M M  EEEE    SS    A   A   \
   M M M  E         SS  AAAAA   \
   M   M  E      S   S  A   A   \
   M   M   EEEE   SSS   A   A   \
                                "
      };
   GLubyte table[256][4];

   if (!glutExtensionSupported("GL_EXT_paletted_texture")) {
      printf("Sorry, GL_EXT_paletted_texture not supported\n");
      exit(0);
   }

   /* load the color table for each texel-index */
   memset(table, 0, 256*4);
   table[' '][0] = 255;
   table[' '][1] = 255;
   table[' '][2] = 255;
   table[' '][3] = 64;
   table['M'][0] = 255;
   table['M'][1] = 0;
   table['M'][2] = 0;
   table['M'][3] = 255;
   table['E'][0] = 0;
   table['E'][1] = 255;
   table['E'][2] = 0;
   table['E'][3] = 255;
   table['S'][0] = 0;
   table['S'][1] = 0;
   table['S'][2] = 255;
   table['S'][3] = 255;
   table['A'][0] = 255;
   table['A'][1] = 255;
   table['A'][2] = 0;
   table['A'][3] = 255;

#ifdef GL_EXT_paletted_texture

#if defined(GL_EXT_shared_texture_palette) && defined(USE_SHARED_PALETTE)
   printf("Using shared palette\n");
   glColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT,    /* target */
                   GL_RGBA,          /* internal format */
                   256,              /* table size */
                   GL_RGBA,          /* table format */
                   GL_UNSIGNED_BYTE, /* table type */
                   table);           /* the color table */
   glEnable(GL_SHARED_TEXTURE_PALETTE_EXT);
#else
   glColorTableEXT(GL_TEXTURE_2D,    /* target */
                   GL_RGBA,          /* internal format */
                   256,              /* table size */
                   GL_RGBA,          /* table format */
                   GL_UNSIGNED_BYTE, /* table type */
                   table);           /* the color table */
#endif

   glTexImage2D(GL_TEXTURE_2D,       /* target */
                0,                   /* level */
                GL_COLOR_INDEX8_EXT, /* internal format */
                WIDTH, HEIGHT,       /* width, height */
                0,                   /* border */
                GL_COLOR_INDEX,      /* texture format */
                GL_UNSIGNED_BYTE,    /* texture type */
                texture);            /* the texture */
#endif

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glEnable(GL_TEXTURE_2D);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#undef HEIGHT
#undef WIDTH
}



/*
 * Color ramp test
 */
static void Init2( void )
{
#define HEIGHT 32
#define WIDTH 256
   static char texture[HEIGHT][WIDTH];
   GLubyte table[256][4];
   int i, j;

   if (!glutExtensionSupported("GL_EXT_paletted_texture")) {
      printf("Sorry, GL_EXT_paletted_texture not supported\n");
      exit(0);
   }

   for (j = 0; j < HEIGHT; j++) {
      for (i = 0; i < WIDTH; i++) {
         texture[j][i] = i;
      }
   }

   for (i = 0; i < 255; i++) {
      table[i][0] = i;
      table[i][1] = 0;
      table[i][2] = 0;
      table[i][3] = 255;
   }

#ifdef GL_EXT_paletted_texture

#if defined(GL_EXT_shared_texture_palette) && defined(USE_SHARED_PALETTE)
   printf("Using shared palette\n");
   glColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT,    /* target */
                   GL_RGBA,          /* internal format */
                   256,              /* table size */
                   GL_RGBA,          /* table format */
                   GL_UNSIGNED_BYTE, /* table type */
                   table);           /* the color table */
   glEnable(GL_SHARED_TEXTURE_PALETTE_EXT);
#else
   glColorTableEXT(GL_TEXTURE_2D,    /* target */
                   GL_RGBA,          /* internal format */
                   256,              /* table size */
                   GL_RGBA,          /* table format */
                   GL_UNSIGNED_BYTE, /* table type */
                   table);           /* the color table */
#endif

   glTexImage2D(GL_TEXTURE_2D,       /* target */
                0,                   /* level */
                GL_COLOR_INDEX8_EXT, /* internal format */
                WIDTH, HEIGHT,       /* width, height */
                0,                   /* border */
                GL_COLOR_INDEX,      /* texture format */
                GL_UNSIGNED_BYTE,    /* texture type */
                texture);            /* teh texture */
#endif

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glEnable(GL_TEXTURE_2D);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 400, 300 );

   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );

   glutCreateWindow(argv[0]);

   Init();
   (void) Init2; /* silence warning */

   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   if (Anim)
      glutIdleFunc( Idle );

   glutMainLoop();
   return 0;
}
